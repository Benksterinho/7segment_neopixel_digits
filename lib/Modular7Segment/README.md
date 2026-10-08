# Modular7Segment

An Adafruit NeoPixel-based 7-segment display library for modular WS2812B digit
chains. The caller owns an `Adafruit_NeoPixel` strip; the library writes pixel
colors into its buffer and leaves transmission to the caller.

## Layout string

The layout string describes the physical modules in data-chain order:

| Character | Module         | Default size          |
|-----------|----------------|-----------------------|
| `D`       | 7-segment digit | 7 x `pixelsPerSegment` |
| `:`       | colon (2 dots)  | 2 x `pixelsPerDot`     |
| `.`       | decimal point   | 1 x `pixelsPerDot`     |

### Inline per-module pixel counts

Each module character may be followed by a decimal number that overrides the
pixel count for that module only:

- `D[n]` — digit with `n` pixels per segment
- `:[n]` — colon with `n` pixels per dot
- `.[n]` — decimal point with `n` pixels per dot

A bare `D`, `:`, or `.` falls back to the `pixelsPerSegment` / `pixelsPerDot`
defaults passed to the constructor, so existing layouts keep working
unchanged.

Rules:

- Within one digit, all 7 segments share the same pixel count.
- `n` must be in the range 1–255 and written with at most 3 digits.
- An invalid count (`0`, above 255, or more than 3 digits) makes
  `requiredLeds()` return 0 at compile time and `isValid()` return false at
  runtime.

### Examples

```cpp
// Classic clock: 4 digits, 4 px/segment, colon with 1 px/dot.
constexpr char LAYOUT[] = "DD:DD";
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(LAYOUT, 4, 1);

// Mixed sizes: hour digits 4 px/segment, minute digits 2 px/segment.
constexpr char MIXED_LAYOUT[] = "D4D4:D2D2";
constexpr uint16_t MIXED_LEDS = Modular7Segment::requiredLeds(MIXED_LAYOUT, 4, 1);
// = 28 + 28 + 2 + 14 + 14 = 86

constexpr uint8_t DATA_PIN = 27;
Adafruit_NeoPixel leds(MIXED_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
leds.begin();
Modular7Segment display(leds, MIXED_LAYOUT, 4, 1);

if (display.isValid()) {
  display.printString("12:34", leds.Color(0, 255, 0));
  leds.show();
}
```

`requiredLeds()` is a compile-time constant expression, so the strip size can
be selected statically. Guard invalid layouts at build time:

```cpp
static_assert(NUM_LEDS > 0, "Invalid display layout");
```

## API

- `Modular7Segment(Adafruit_NeoPixel& leds, const char* layout, uint8_t pixelsPerSegment, uint8_t pixelsPerDot)`
- `static constexpr uint16_t requiredLeds(const char* layout, uint8_t pixelsPerSegment, uint8_t pixelsPerDot)`
- `void setModule(uint8_t moduleIndex, char character, uint32_t color)`
- `void printString(const char* text, uint32_t color)`
- `void clear()`
- `uint8_t moduleCount() const`
- `uint16_t requiredLeds() const`
- `bool isValid() const`

## Supported characters

Digits `0`–`9`, hexadecimal letters `A`–`F` (case-insensitive), and space
(blank). Unsupported characters render the digit as off. Colon and decimal
point modules light up only for their matching punctuation character.

## printString() behavior

- Digit modules consume one input character.
- Colon and decimal-point modules consume their matching punctuation only
  when it is present at the current input position; otherwise they stay off.
- Missing digit characters render blank; extra trailing input is ignored.
- A null text pointer is treated as an empty string.

## Design constraints

- Layout tables are fixed-size (`MAX_MODULES = 32`); the library does not
  allocate memory.
- The caller constructs and initializes the `Adafruit_NeoPixel` strip. That
  library owns its pixel buffer.
- Rendering methods update the strip buffer but never call `show()`.
- Set brightness during strip initialization; Adafruit NeoPixel applies
  brightness to stored pixel data.
