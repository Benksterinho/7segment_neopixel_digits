# Modular7Segment Improvement and ESPHome Integration Plan

> Migration note: This plan predates the move from FastLED to Adafruit
> NeoPixel. Its FastLED-specific examples and API references describe the
> previous implementation; see `lib/Modular7Segment/README.md` for the current
> interface.

## Objective

Evolve `Modular7Segment` into a robust, reusable seven-segment rendering library that works well in normal Arduino projects and can be cleanly integrated into ESPHome and Home Assistant based systems.

The intended reference application is a separate ESP32 display node that receives entities such as time, CO2 concentration, temperature, humidity, light level, and room presence through ESPHome/Home Assistant, then selectively renders them on chained 18 cm NeoPixel seven-segment modules.

The core library must remain independent of ESPHome and Home Assistant. ESPHome-specific behavior belongs in a thin adapter or external component around the renderer.

---

## Core Architecture to Preserve

The current design has strong embedded properties that must remain intact:

1. The caller owns the FastLED `CRGB` array.
2. The templated constructor infers the array size.
3. `requiredLeds()` can be evaluated at compile time.
4. The layout string describes modules in physical data-chain order.
5. Module offsets and types are calculated once during construction.
6. Rendering modifies the LED buffer but does not transmit it.
7. The library does not register a FastLED controller.
8. The library does not call `FastLED.show()`.
9. The library does not allocate memory dynamically.
10. Invalid layouts and undersized buffers fail safely through `isValid()`.

Do not replace these properties with internal GPIO ownership, hidden transmission, Arduino `String` dependencies, dynamic allocation, or Home Assistant specific logic.

---

# Part 1: Core Library Improvements

## Priority 1: Add Practical Character Support

Extend `characterMask()` with symbols useful for clocks, sensor values, diagnostics, and status messages.

### Minimum additions

- `-` maps to the middle segment.
- `_` maps to the bottom segment.

### Recommended additions

Add readable seven-segment approximations where sensible:

- `H` and `h`
- `L` and `l`
- `P` and `p`
- `U` and `u`
- `O` and `o`
- `r`

Unsupported characters must continue to render as off.

### Acceptance criteria

- Existing mappings remain unchanged.
- Minus can be used for signed numbers and unavailable-value indications.
- Unsupported characters clear the target digit safely.
- The complete supported character set is documented.

---

## Priority 2: Document `printString()` Parsing

Document the exact parser behavior:

- Digit modules consume one input character each.
- Colon and decimal modules consume their matching punctuation only when present at the current input position.
- Omitted punctuation leaves the punctuation module off without consuming input.
- Missing digit characters render as blank.
- Unsupported characters render the corresponding digit as off.
- Remaining input after the final configured module is ignored.
- A null text pointer is treated as an empty string.

For layout `"DD:DD"`:

```cpp
display.printString("12:34", CRGB::Green);     // Explicit colon
display.printString("1234", CRGB::Green);      // Colon omitted
display.printString("12", CRGB::Green);        // Remaining digits blank
display.printString("12:34:56", CRGB::Green);  // Extra input ignored
```

---

## Priority 3: Add Unit Tests

Tests must cover:

### LED calculation

- `"D"` with 3 pixels per segment and 1 per dot returns 21.
- `"DD:DD"` with those densities returns 86.
- Mixed layouts calculate correctly.
- Null or empty layouts return 0.
- Zero density values return 0.
- Unsupported layout characters return 0.
- Boundaries at 31, 32, and 33 modules are explicit.

### Constructor validation

- Exactly sized buffer.
- Larger buffer.
- Undersized buffer.
- Invalid layout.
- Empty layout.
- Zero density.
- Maximum module count.

### Rendering

- Every supported number and letter.
- Lowercase mappings.
- Space and unsupported characters.
- Minus and underscore.
- Previously lit segments are cleared before redrawing.
- Only the selected module is modified.

### Punctuation

- Colon activates both configured dot ranges.
- Decimal point activates one configured range.
- Nonmatching characters clear punctuation modules.

### Parser behavior

- Explicit punctuation.
- Omitted punctuation.
- Short, empty, and null input.
- Extra trailing input.
- Mixed module layouts.

### Buffer integrity

Use sentinel pixels where practical to verify that no public operation writes outside the configured display range.

---

## Priority 4: Prevent LED Count Overflow

Both compile-time and runtime LED-count calculations must explicitly protect against `uint16_t` overflow.

Before adding a module size, check whether the result would exceed `UINT16_MAX`. Do not rely on unsigned wraparound followed by a final comparison.

### Acceptance criteria

- Overflowing compile-time configurations return 0.
- Overflowing runtime configurations set `valid_` to false.
- Normal configurations remain unchanged.
- Overflow attempts are covered by tests.

---

## Priority 5: Align the 32-Module Boundary

Define one consistent rule:

> Layouts may contain up to and including 32 physical modules. A 33rd module is invalid.

Review the recursive `requiredLeds()` termination condition carefully. A valid layout ending immediately after module 32 must not be rejected.

### Acceptance criteria

- 32 modules work at compile time and runtime.
- 33 modules are rejected in both paths.
- README wording matches implementation.

---

## Priority 6: Harden Internal Helpers

Add a defensive guard to `clearModule()` or make its unchecked precondition explicit through naming and documentation.

Preferred behavior:

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

---

## Priority 7: Add Direct Segment Control

Add low-level control for diagnostics, wiring tests, and animations:

```cpp
void setSegment(uint8_t moduleIndex, Segment segment, CRGB color);
void clearSegment(uint8_t moduleIndex, Segment segment);
```

Optional:

```cpp
void setDigitMask(uint8_t moduleIndex, uint8_t mask, CRGB color);
```

Invalid indexes, punctuation modules, and invalid segment values must be safe no-ops.

---

## Priority 8: Add Non-Allocating Numeric Rendering

Add helpers that do not require Arduino `String` or heap allocation:

```cpp
bool printUnsigned(uint32_t value, CRGB color, bool leadingZeros = false);
bool printSigned(int32_t value, CRGB color, bool leadingZeros = false);
```

### Required behavior

- Values are right-aligned.
- Leading positions are blank unless `leadingZeros` is true.
- Negative values use the minus character.
- Return `false` when the value cannot fit.
- Do not silently truncate numeric values.
- Punctuation modules remain off unless controlled separately.
- Zero renders correctly.

These APIs are especially important for ESPHome lambdas because callers can render numeric sensor states without constructing temporary `String` objects.

---

## Priority 9: Add Direct Punctuation Control

Add module-index-based punctuation control:

```cpp
void setPunctuation(uint8_t moduleIndex, bool enabled, CRGB color);
```

This should work for both colon and decimal-point modules and safely do nothing for digit modules.

Do not add automatic blinking or timing to the core library.

---

## Priority 10: Keep Brightness Policy Outside the Core

Do not add retained per-module brightness state to the core renderer.

Preferred approaches:

- The caller supplies an already scaled `CRGB` value.
- A future overload may scale a local color copy before writing it.
- ESPHome or the adapter owns global brightness and transitions.

Do not introduce hidden frame state merely to support brightness.

---

# Part 2: ESPHome Integration

## Integration Principle

Implement ESPHome support as a thin adapter or external component. The core `Modular7Segment` class must compile and remain useful without ESPHome headers.

Recommended layering:

```text
Home Assistant entities and automations
                |
                v
ESPHome display-node component
  - entity subscriptions
  - time and presence state
  - display-mode scheduling
  - color and brightness policy
  - FastLED transmission
                |
                v
Modular7Segment renderer
  - layout parsing
  - character masks
  - numeric rendering
  - buffer modification only
```

---

## Priority 11: Provide an ESPHome External Component Wrapper

Create an optional ESPHome component around the core class. Keep ESPHome-specific files separate from the generic library.

Suggested structure:

```text
components/
  modular_7segment/
    __init__.py
    display.py or light.py
    modular_7segment_component.h
    modular_7segment_component.cpp
    Modular7Segment.h
    Modular7Segment.cpp
```

The exact ESPHome component type should be selected after checking the intended integration model. The wrapper must own lifecycle and configuration, while the core class remains the renderer.

### Wrapper responsibilities

- Initialize the LED output through the selected ESPHome-compatible mechanism.
- Construct `Modular7Segment` with the owned LED array.
- Validate the layout during setup.
- Expose a method for rendering text or numbers.
- Call the relevant LED refresh operation after a completed frame.
- Track display mode and dirty state.
- Turn the display off without disabling the sensor station.
- Avoid blocking delays.
- Publish useful diagnostics when setup is invalid.

### Core library responsibilities

- Calculate layout size.
- Map characters to segments.
- Render modules into a `CRGB` array.
- Clear logical modules.
- Never subscribe to Home Assistant entities.
- Never make presence or color decisions.
- Never own automation timing.

---

## Priority 12: Provide a Lambda-Friendly Integration Path

Allow the first implementation to work from ordinary ESPHome lambdas even if the full external component is introduced later.

The integration should make calls conceptually similar to:

```cpp
id(segment_display).show_co2(
    static_cast<uint32_t>(id(lab_co2).state));
```

or, at a lower level:

```cpp
id(segment_display).renderer().printUnsigned(
    static_cast<uint32_t>(id(lab_co2).state),
    CRGB::Green);
id(segment_display).show();
```

The wrapper should provide the cleaner high-level API. Do not require users to access private offsets or character masks from YAML lambdas.

---

## Priority 13: Import Home Assistant Entities

The reference configuration should consume entities for:

- CO2 concentration.
- Temperature.
- Humidity.
- Light level, if used for adaptive brightness.
- Presence or occupancy.
- Optional Home Assistant mode selection.

Entity IDs must be examples or substitutions, not hard-coded into the C++ library.

### State validation

Before rendering a numeric entity:

- Check that the state is available.
- Check that the value is finite.
- Define behavior for unavailable, unknown, or invalid states.
- Prefer a blank display or a short error indicator such as `----`.
- Do not cast NaN directly to an integer.
- Retain the previous value only if this behavior is explicitly configured.

---

## Priority 14: Add Time Display Support in the Wrapper

Time belongs in the ESPHome wrapper, not in `Modular7Segment`.

For a `"DD:DD"` layout, the wrapper should format local time as:

```text
HH:MM
```

### Requirements

- Use ESPHome's configured time source.
- Do not add RTC or timezone logic to the core library.
- Provide configurable 12-hour or 24-hour formatting if implemented.
- Make leading-zero behavior explicit.
- Allow the colon to blink without blocking the main loop.
- Skip time rendering until the time source is valid.

---

## Priority 15: Add Display Modes

The ESPHome wrapper should support a small, explicit mode model.

Suggested modes:

```cpp
enum class DisplayMode : uint8_t {
  OFF,
  AUTO,
  TIME,
  CO2,
  TEMPERATURE,
  HUMIDITY,
  CUSTOM
};
```

### Behavior

- `OFF`: clear the logical display and transmit once.
- `TIME`: show local time.
- `CO2`: show rounded ppm.
- `TEMPERATURE`: show a fixed-point value if layout permits.
- `HUMIDITY`: show a fixed-point or rounded value.
- `CUSTOM`: show externally supplied text.
- `AUTO`: rotate through configured pages.

Keep application-specific mode decisions out of the core renderer.

---

## Priority 16: Implement Non-Blocking Page Rotation

For automatic mode, support configurable pages and durations without `delay()`.

Reference sequence:

1. Time for 10 seconds.
2. CO2 for 5 seconds.
3. Temperature for 5 seconds.
4. Humidity for 5 seconds.
5. Repeat.

These values are reference defaults only and must be configurable.

### Requirements

- Use ESPHome scheduling, intervals, or component update timing.
- Do not block the main loop.
- Do not transmit unchanged frames unnecessarily.
- A direct mode selection must override automatic rotation.
- Returning to `AUTO` resumes rotation predictably.

---

## Priority 17: Presence-Aware Power Behavior

Use the lab presence entity to control visible outputs.

Suggested policy:

```text
Presence detected
  -> mark lab active
  -> enable display
  -> allow normal page rotation
  -> allow lighting automations

No presence for configured timeout
  -> clear and refresh seven-segment display
  -> disable or dim decorative lighting through Home Assistant
  -> leave sensor reporting active
```

### Important separation

- The display node may react locally to an imported presence entity.
- Home Assistant should coordinate other devices such as Shelly-controlled lighting.
- The sensor station should continue measuring and reporting while the display is off.
- Presence must not power-cycle the ESP32 or disable network connectivity.

### Configuration options

- Presence-off delay.
- Immediate wake on presence.
- Optional night-time behavior.
- Optional minimum display brightness while occupied.
- Optional manual override.

### Acceptance criteria

- Presence wake is responsive.
- Short presence dropouts do not cause flicker.
- Display-off clears the LEDs and sends the cleared frame once.
- Sensor entity updates continue while the display is off.
- Manual mode or override behavior is documented.

---

## Priority 18: CO2 Color Policy

Implement color selection in the ESPHome wrapper rather than in the renderer.

Provide configurable thresholds. Suggested initial defaults for the project may be expressed as configuration, not compiled policy:

```yaml
co2_colors:
  good_below: 700
  warning_below: 1000
  high_below: 1400
```

Suggested color progression:

- Good: green.
- Elevated: yellow.
- High: orange.
- Very high: red.

### Requirements

- Thresholds must be configurable.
- The core library receives only the selected `CRGB` color.
- Invalid CO2 readings must not accidentally render as a valid green zero.
- Optional hysteresis should prevent rapid color toggling near thresholds.
- Do not flash high-CO2 values by default. If alert flashing is added, make it optional and non-blocking.

---

## Priority 19: Adaptive Brightness

Use the light-level entity or a local light sensor to choose display brightness in the ESPHome wrapper.

### Requirements

- Brightness policy remains outside the renderer.
- Use configurable minimum and maximum brightness.
- Apply smoothing or stepped ranges to avoid visible brightness jitter.
- Presence-off overrides adaptive brightness and results in a cleared display.
- Provide a manual brightness override.
- Avoid rewriting segment colors destructively when only global output brightness changes, if the selected LED integration supports independent master brightness.

---

## Priority 20: Expose Home Assistant Controls

Expose suitable controls and diagnostics from the display-node ESPHome configuration.

Recommended entities:

- Select: display mode.
- Switch: display enabled or manual blanking override.
- Number: brightness override.
- Number: page duration, if useful.
- Binary sensor: display node online or active state.
- Text sensor: current display page or status.
- Button: test all segments.
- Button: cycle display pages.

Avoid exposing every internal implementation detail as an entity. Prefer a compact control surface that is useful in the Home Assistant dashboard and automations.

---

## Priority 21: Add Diagnostics and Safe Startup

At startup:

1. Initialize the LED output.
2. Clear the full configured logical display.
3. Validate the display layout.
4. Log required LED count, module count, and validity.
5. Do not render imported entity values until they are valid.
6. Optionally run a short, configurable segment self-test.
7. Enter the configured default mode.

### Fault behavior

- Invalid layout: keep output off and log an error.
- Missing Home Assistant entity: keep the relevant page blank and continue operation.
- Invalid time: skip the time page or render a defined placeholder.
- Lost Home Assistant connection: define whether the display continues showing local time, blanks imported values, or retains the last valid value.
- Recovery must not require a device restart.

---

## Priority 22: Avoid Excessive LED Refreshes

Add dirty-frame behavior in the wrapper.

Only transmit when:

- Display content changed.
- Display color changed.
- Brightness changed.
- Colon blink state changed.
- Display was enabled or disabled.
- A segment test is active.

Do not call the refresh operation continuously when the frame is unchanged.

---

## Priority 23: Add ESPHome Examples

Provide at least these example configurations:

### Example A: Basic CO2 display

- Imports one Home Assistant CO2 entity.
- Renders rounded ppm.
- Selects color from configurable thresholds.
- Handles unavailable state.

### Example B: Clock and environmental rotation

- Uses layout `"DD:DD"` or a documented alternative.
- Rotates through time, CO2, temperature, and humidity.
- Uses non-blocking scheduling.

### Example C: Presence-aware lab display

- Imports lab presence.
- Wakes immediately on presence.
- Clears after a configurable absence delay.
- Continues network operation while visually off.

### Example D: Diagnostics

- Segment test button.
- Mode selector.
- Current-page text sensor.
- Brightness override.

All YAML examples must use placeholder entity IDs that are easy to replace.

---

# Part 3: Home Assistant Coordination

## Priority 24: Keep Room Orchestration in Home Assistant

Home Assistant should coordinate behaviors spanning multiple physical nodes, including:

- Shelly-controlled lab lights.
- Seven-segment display visibility.
- Optional OLED backlight.
- Occupancy-driven lab mode.
- Time-of-day conditions.
- Manual overrides.

The ESPHome display node should implement reliable local rendering, while Home Assistant owns cross-device orchestration.

---

## Priority 25: Define a Lab Active State

Use a derived lab-active state or equivalent automation logic rather than scattering raw presence checks across many automations.

Conceptual behavior:

```text
lab_active = presence detected
             OR manual override
             OR temporary keep-awake timer
```

When lab activity ends, apply an absence delay before turning off lights and displays. The exact helper or automation type belongs to Home Assistant configuration and is not part of the C++ core.

### Acceptance criteria

- One stable state drives room-wide occupancy behavior.
- Individual devices can still be overridden.
- Presence sensor noise does not cause rapid device toggling.
- Sensor reporting remains independent of room-active state.

---

## Priority 26: Design Manual Overrides

Provide predictable manual control for cases where automation should temporarily yield.

Recommended concepts:

- Keep display on.
- Force display off.
- Keep lab active for a configurable period.
- Automatic behavior restored after restart or timeout, as configured.
- Physical light switches remain functional after Shelly upcycling.

Do not build Shelly relay control into `Modular7Segment` or the display-node C++ component.

---

# Part 4: Packaging and Code Quality

## Priority 27: Arduino Library Metadata

Add or verify:

- `library.properties`
- `keywords.txt` if desired
- `examples/` directory
- Basic single-digit example
- Four-digit clock-layout example
- Segment wiring-test example
- Mixed digit and punctuation example

---

## Priority 28: ESPHome Packaging

Document both supported consumption models:

1. Copy or include the generic C++ library in an ESPHome project.
2. Use the optional ESPHome external component wrapper.

Keep versioning clear so the wrapper and core renderer remain compatible.

---

## Priority 29: Formatting and Compatibility

- Apply consistent C++ formatting.
- Preserve compatibility with the selected Arduino and ESPHome toolchains.
- Keep formatting-only changes separate where practical.
- Avoid STL-heavy abstractions unless compatibility and memory impact are understood.
- Do not introduce exceptions or RTTI dependencies.

---

# Suggested Implementation Order

1. Add tests that capture current behavior.
2. Fix and test the exact 32-module boundary.
3. Add overflow protection.
4. Add minus and underscore support.
5. Add non-allocating numeric rendering.
6. Document parser and truncation behavior.
7. Add direct segment and punctuation control.
8. Add Arduino examples and metadata.
9. Build a minimal ESPHome lambda-based proof of concept.
10. Build the thin ESPHome wrapper or external component.
11. Import CO2 and presence entities.
12. Add time mode and non-blocking page rotation.
13. Add configurable CO2 colors and adaptive brightness.
14. Add Home Assistant controls and diagnostics.
15. Add lab-active orchestration examples.
16. Validate on the real chained 18 cm NeoPixel hardware.

---

# Non-Goals

Do not include the following in the core library:

- Home Assistant entity IDs.
- ESPHome subscriptions.
- Presence policy.
- CO2 thresholds.
- Timezone or RTC logic.
- Shelly relay control.
- Dynamic memory allocation.
- Arduino `String` dependencies.
- Internal `FastLED.addLeds()` calls.
- Internal `FastLED.show()` calls.
- Blocking delays.
- Automatic animation scheduling.
- Hidden brightness state.
- Arbitrary segment remapping unless required by physical hardware.

---

# Definition of Done

The project is ready for ESPHome/Home Assistant use when:

- The generic renderer remains reusable outside ESPHome.
- Compile-time and runtime layout validation agree.
- The 32-module boundary is correct.
- LED-count overflow cannot create a wrapped valid configuration.
- Numeric values render without heap allocation.
- Invalid sensor states are handled safely.
- Time, CO2, temperature, humidity, and custom pages can be selected.
- Page rotation is non-blocking.
- Presence can wake the display and delayed absence can blank it.
- Sensor reporting continues while the visual display is off.
- CO2 color thresholds are configurable outside the renderer.
- Brightness policy is configurable outside the renderer.
- Home Assistant can select display mode and trigger a segment test.
- The wrapper avoids unnecessary LED refreshes.
- No operation writes outside the configured LED range.
- The library never registers a FastLED controller or calls `FastLED.show()`.
- Arduino and ESPHome examples are included.

---

# Planner Guidance

Implement the work in small, reviewable commits. Do not combine core boundary fixes, API extensions, ESPHome integration, Home Assistant examples, and formatting into one large change.

Recommended deliverables:

1. Core tests and correctness fixes.
2. Numeric and low-level rendering APIs.
3. Basic Arduino examples.
4. ESPHome proof of concept.
5. ESPHome wrapper with entity-state validation.
6. Presence-aware page controller.
7. Home Assistant examples and documentation.

The architectural rule for every change is:

> `Modular7Segment` renders pixels. ESPHome operates the display node. Home Assistant orchestrates the lab.
