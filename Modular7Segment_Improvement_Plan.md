# Modular7Segment Improvement Plan

## Objective

Improve the `Modular7Segment` library without changing its core architecture or its current ownership model.

The library should continue to:

- Render into a caller-owned FastLED `CRGB` array.
- Avoid dynamic memory allocation.
- Avoid registering a FastLED controller.
- Avoid calling `FastLED.show()`.
- Support physical modules arranged in data-chain order through a layout string.
- Remain suitable for Arduino and ESP32 projects.
- Preserve the existing API wherever practical.

## Current Architecture to Preserve

The current design has several strong properties that should remain unchanged:

1. The caller owns the LED array.
2. The array size is inferred by the templated constructor.
3. `requiredLeds()` can be evaluated at compile time.
4. The layout string describes the physical chain order.
5. Module offsets and module types are calculated once during construction.
6. Rendering methods modify the buffer but do not transmit it.
7. No heap allocation is used.
8. Invalid layouts and undersized buffers fail safely through `isValid()`.

Do not replace these properties with dynamic allocation, internal FastLED registration, hidden calls to `FastLED.show()`, or String-based APIs.

---

## Priority 1: Add Practical Character Support

### Requirement

Extend `characterMask()` with useful seven-segment symbols.

### Minimum character set

Add support for:

- `-` as the middle segment.
- `_` as the bottom segment.

### Recommended additional characters

Add readable seven-segment approximations where they are unambiguous:

- `H` and `h`
- `L` and `l`
- `P` and `p`
- `U` and `u`
- `O` and `o`
- `r`

Unsupported characters must continue to render as off.

### Acceptance criteria

- Existing mappings for `0` through `9`, `A` through `F`, lowercase `a` through `f`, and space remain unchanged.
- `'-'` lights only the middle segment.
- `'_'` lights only the bottom segment.
- Unsupported characters clear the target digit and do not access memory outside the module.
- The README documents the complete supported character set.

---

## Priority 2: Document String Parsing and Truncation

### Requirement

Clarify the exact behavior of `printString()`.

The documentation should explicitly state:

- Digit modules consume one input character.
- Colon and decimal-point modules consume their matching punctuation only when it is present at the current input position.
- A colon or decimal-point module remains off when its punctuation is omitted.
- Missing digit characters are rendered as blank.
- Input remaining after all configured modules have been processed is ignored.
- Unsupported characters render the corresponding digit as off.
- A null text pointer is treated as an empty string.

### Examples to add

For layout `"DD:DD"`:

```cpp
display.printString("12:34", CRGB::Green);  // Explicit colon
display.printString("1234", CRGB::Green);   // Colon omitted
display.printString("12", CRGB::Green);     // Remaining digits blank
display.printString("12:34:56", CRGB::Green); // Extra input ignored
```

### Acceptance criteria

- The README explains all parsing behaviors listed above.
- Existing parser behavior is preserved unless a separate API is introduced intentionally.
- No heap-allocated or temporary Arduino `String` object is required.

---

## Priority 3: Add Unit Tests

### Requirement

Add host-side or embedded-compatible tests for layout validation, LED calculation, character rendering, punctuation handling, and buffer boundaries.

### Required test groups

#### `requiredLeds()`

Test at least:

- `"D"`, 3 pixels per segment, 1 pixel per dot returns 21.
- `"DD:DD"`, 3 pixels per segment, 1 pixel per dot returns 86.
- `"DD.DD"` returns the expected LED count.
- Empty layout returns 0.
- Null layout returns 0.
- Zero pixels per segment returns 0.
- Zero pixels per dot returns 0.
- Unsupported layout characters return 0.
- Exactly 32 valid modules are handled consistently with the documented limit.
- More than 32 modules returns 0.

#### Constructor validation

Test at least:

- Valid layout with an exactly sized buffer.
- Valid layout with a larger buffer.
- Buffer smaller than the required LED count.
- Empty layout.
- Invalid layout character.
- Zero density values.
- Maximum module count boundary.

#### Digit rendering

Test:

- Every supported numeral.
- Every supported hexadecimal letter.
- Lowercase equivalents.
- Space.
- Unsupported characters.
- Newly added symbols.
- Only LEDs belonging to the selected module are modified.
- Previously lit segments are cleared when a new character uses fewer segments.

#### Punctuation modules

Test:

- `:` activates both dot ranges in a colon module.
- Any character other than `:` clears a colon module.
- `.` activates its decimal-point range.
- Any character other than `.` clears a decimal-point module.

#### `printString()`

Test:

- Text including punctuation.
- Text omitting punctuation.
- Short input.
- Empty input.
- Null input.
- Extra trailing input.
- Mixed layouts containing digits, colons, and decimal points.

#### Buffer integrity

Use sentinel LEDs before or after the display's logical range where the test framework permits it. Verify that rendering and clearing never modify LEDs outside the configured layout range.

### Acceptance criteria

- Tests cover all public methods.
- Boundary cases for 0, 1, 31, 32, and 33 modules are included where applicable.
- Character masks are verified by expected segment states rather than only by LED counts.
- Existing behavior is captured before larger refactoring work begins.

---

## Priority 4: Strengthen Integer Overflow Handling

### Problem

LED counts and offsets use `uint16_t`. Expressions are accumulated while parsing the layout. Current practical layouts are small, but invalid configurations should not be able to wrap an intermediate LED count and appear valid.

### Requirement

Make overflow handling explicit in both:

- The compile-time `requiredLeds()` helper.
- Runtime constructor parsing.

### Recommended approach

Before adding a module size, check whether the addition would exceed `UINT16_MAX`.

Do not rely on unsigned wraparound followed by the final buffer-size comparison.

### Acceptance criteria

- Calculations that exceed `UINT16_MAX` produce an invalid result.
- Compile-time `requiredLeds()` returns 0 for an overflowing configuration.
- Runtime construction sets `valid_` to false for an overflowing configuration.
- Normal supported configurations produce unchanged results.
- Tests cover overflow attempts using large density values and multi-module layouts.

---

## Priority 5: Clarify the 32-Module Boundary

### Problem

The implementation and compile-time helper must agree on whether exactly 32 modules are valid.

### Requirement

Define and enforce one clear rule:

> Layouts may contain up to and including 32 physical modules.

The 33rd module must make the layout invalid.

Review the recursive `requiredLeds()` termination condition carefully so that a valid layout containing exactly 32 modules is accepted when the string ends immediately after the 32nd module.

### Acceptance criteria

- A layout containing exactly 32 valid module characters is accepted.
- A layout containing 33 module characters is rejected.
- Compile-time and runtime validation return consistent results.
- README wording matches the implemented behavior.
- Tests explicitly protect this boundary.

---

## Priority 6: Defensive Internal Helper Behavior

### Requirement

Keep private helper methods safe when practical, without adding significant runtime overhead.

Review `clearModule()` and either:

1. Add a defensive module/buffer guard inside it, or
2. Preserve it as an unchecked private helper but rename or document it so its precondition is explicit.

Preferred implementation:

```cpp
void Modular7Segment::clearModule(uint8_t moduleIndex) {
  if (!moduleFitsBuffer(moduleIndex)) {
    return;
  }

  const uint16_t moduleOffset = moduleOffsets_[moduleIndex];
  const uint16_t count = moduleSize(moduleTypes_[moduleIndex]);
  for (uint16_t pixel = 0; pixel < count; ++pixel) {
    leds_[moduleOffset + pixel] = CRGB::Black;
  }
}
```

### Acceptance criteria

- Invalid indexes cannot cause out-of-bounds access through any helper call path.
- Public behavior remains unchanged.
- No dynamic memory is introduced.

---

## Priority 7: Add Direct Segment Control

### Goal

Support diagnostics, wiring tests, and animation without forcing callers to reproduce internal offset calculations.

### Proposed API

Add methods similar to:

```cpp
void setSegment(uint8_t moduleIndex, Segment segment, CRGB color);
void clearSegment(uint8_t moduleIndex, Segment segment);
```

These methods should operate only on digit modules. Calling them for a punctuation module, invalid module index, or invalid segment must safely do nothing.

Do not call `FastLED.show()`.

### Optional API

If useful, add:

```cpp
void setDigitMask(uint8_t moduleIndex, uint8_t mask, CRGB color);
```

Only the lower seven bits should be considered.

### Acceptance criteria

- Segment control uses the existing physical wiring order.
- Segment updates affect only the requested contiguous segment range.
- Invalid calls are safe no-ops.
- The README includes a wiring-test example.
- Tests verify every segment index.

---

## Priority 8: Add Non-Allocating Number Helpers

### Goal

Make common clock, counter, and sensor-display use cases easier without using Arduino `String` or heap allocation.

### Proposed APIs

Consider:

```cpp
bool printUnsigned(uint32_t value, CRGB color, bool leadingZeros = false);
bool printSigned(int32_t value, CRGB color, bool leadingZeros = false);
```

Alternative names are acceptable if they match the existing style.

### Behavior to define

- Number helpers render only into digit modules.
- Punctuation modules remain off unless controlled separately.
- Values are right-aligned by default.
- Unused leading digit modules are blank unless `leadingZeros` is true.
- Signed negative values use the new `-` character.
- Define whether overflow results in blank output, truncation, or a false return value.
- Prefer returning `false` when the value cannot fit.
- Do not allocate memory dynamically.

### Acceptance criteria

- Zero renders correctly.
- Positive and negative values render correctly.
- Leading-zero behavior is tested.
- Values that do not fit are handled consistently and documented.
- Existing `printString()` remains available.

---

## Priority 9: Improve Punctuation Control

### Goal

Make clock and measurement displays convenient while preserving generic module access.

### Proposed API

Consider adding:

```cpp
void setPunctuation(uint8_t moduleIndex, bool enabled, CRGB color);
```

This should work for colon and decimal-point modules and safely do nothing for digit modules.

A specialized `setColon()` API is not recommended unless module lookup semantics are clearly defined. Module-index-based control remains more consistent with the current architecture and supports multiple colons or dots.

### Acceptance criteria

- Colon and decimal modules can be enabled or disabled without constructing text.
- Multiple punctuation modules can be controlled independently.
- No automatic timing or calls to `FastLED.show()` are added.
- Colon blinking remains the responsibility of the sketch.

---

## Priority 10: Keep Brightness Outside the Library

### Decision

Do not add per-module brightness as a primary feature yet.

FastLED already provides global brightness during transmission. Per-module brightness would require color scaling and raises questions about destructive versus non-destructive buffer updates.

If per-module intensity is later required, prefer one of these explicit approaches:

- The caller supplies an already scaled `CRGB` color.
- Add a rendering overload accepting a scale value and apply `nscale8_video()` to a local color copy before storing it.

Do not store hidden brightness state per module unless a broader retained-mode rendering architecture is intentionally introduced.

### Acceptance criteria

- Current color behavior remains direct and predictable.
- Any future scaling API clearly documents whether the input color is modified.
- Rendering remains stateless unless a separate architectural change is approved.

---

## Priority 11: Consider a Data-Driven Character Table Only When Needed

### Decision

The current `switch` in `characterMask()` is readable and efficient for the present character set. Do not refactor it merely for stylistic reasons.

Move to a lookup table only if the supported character set grows enough that the switch becomes difficult to maintain.

If a table is introduced:

- Keep it in flash/static storage.
- Avoid dynamic containers.
- Keep uppercase and lowercase handling explicit or normalize ASCII input safely.
- Preserve unsupported-character behavior.
- Confirm compatibility with the targeted Arduino toolchains.

### Acceptance criteria

- The implementation remains easy to audit against physical segment masks.
- No RAM-heavy character map is introduced.
- Flash and RAM impact is documented if the representation changes.

---

## Priority 12: Add Library Metadata and Examples

### Requirement

Prepare the project to behave like a reusable Arduino library.

Add or verify:

- `library.properties`
- `keywords.txt` if desired
- An `examples/` directory
- A basic single-digit example
- A four-digit clock-layout example
- A segment wiring-test example
- A mixed display example using digits and punctuation

### Example expectations

Each example should:

- Own and register the FastLED buffer in the sketch.
- Check `display.isValid()`.
- Call `FastLED.show()` only from the sketch.
- Use conservative default brightness for the large printed hardware.
- State the expected layout and LED count.

### Acceptance criteria

- Examples compile for at least one supported ESP32 target.
- The class remains independently testable without requiring a physical display.
- README setup instructions match the examples.

---

## Priority 13: Formatting and Code Quality Cleanup

### Requirement

Run the project through the selected C++ formatter and keep a consistent style.

Review specifically:

- Indentation of the private recursive `requiredLeds()` overload.
- Long mask definitions.
- Brace and line-break consistency.
- Naming consistency between `DECIMAL_POINT`, layout `.`, and README terminology.

### Constraints

- Do not perform broad architecture changes as part of formatting.
- Keep formatting-only changes in a separate commit where practical.
- Preserve compatibility with the project's Arduino C++ standard.

---

## Suggested Implementation Order

1. Add tests capturing current behavior.
2. Fix and test the exact 32-module boundary.
3. Add overflow protection.
4. Add `-` and `_` character support.
5. Update README parsing and truncation documentation.
6. Harden or document `clearModule()` preconditions.
7. Add direct segment-control APIs and tests.
8. Add punctuation-control API and examples.
9. Add non-allocating number helpers if required by the first real application.
10. Add Arduino library metadata and format the codebase.

---

## Non-Goals

Do not include the following in this improvement pass:

- Dynamic memory allocation.
- Ownership of the LED array.
- Internal `FastLED.addLeds()` calls.
- Internal `FastLED.show()` calls.
- Automatic animation scheduling.
- Automatic colon blinking.
- Font rendering beyond practical seven-segment approximations.
- Physical LED reversal or arbitrary per-segment remapping unless real hardware demonstrates a need.
- Arduino `String` dependencies in the core API.
- A retained scene graph or hidden frame buffer.

---

## Definition of Done

The improvement work is complete when:

- Existing public behavior remains compatible unless explicitly documented otherwise.
- The exact 32-module limit behaves consistently at compile time and runtime.
- LED count overflow cannot produce a valid wrapped configuration.
- Practical symbols such as minus are supported.
- String parsing and ignored trailing input are documented.
- Tests cover calculations, validation, rendering, punctuation, parsing, and boundaries.
- No rendering operation writes outside the configured display range.
- No dynamic allocation is introduced.
- The library never registers a FastLED controller or calls `FastLED.show()`.
- Examples demonstrate caller-owned buffer management and explicit transmission.

## Planner Guidance

Implement the work in small, reviewable commits. Avoid combining API extensions, boundary fixes, tests, and formatting in one large change. Treat tests for current behavior and boundary conditions as the first deliverable, then make behavioral changes against that safety net.
