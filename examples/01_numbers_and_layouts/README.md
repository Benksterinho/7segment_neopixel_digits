# 01 — Numbers and layouts

This first sketch renders a short text on a fixed physical chain of seven-segment
digits, colons, and decimal points. It starts with one digit (`D`) and `7`.
Choose another layout and matching text at build time to try a two-digit number,
a clock, or a decimal value.

## Before you build

Use this project from a checkout of the repository: its PlatformIO project
references the original `lib/` directory and selects only this lesson's
`src/main.cpp`. The shared example configuration supplies the ESP32 Arduino
platform and Adafruit NeoPixel dependency. Defaults are four pixels per
segment, one pixel per dot, and GPIO27 on the Wemos D1 Mini32; the shared C3
environment selects GPIO4.

Connect a compatible WS2812B chain to the selected data pin and ground, and
provide an appropriate power supply for the strip. Do not power a long or
high-brightness strip from the board's 3.3 V pin. Verify the board's pinout and
common ground before connecting data.

## Layout and text are different

The layout describes the physical modules in data-chain order; it is not a
format string that changes when the text changes. For example:

| Build-time layout | Example text | Physical modules |
| --- | --- | --- |
| `D` | `7` | one digit |
| `DD` | `42` | two digits |
| `DD:DD` | `12:34` | two digits, colon, two digits |
| `DD.D` | `12.3` | two digits, decimal point, one digit |

The last column matters when selecting the number of LEDs. Changing `D` to
`DD:DD` changes the compiled strip size, but it cannot change the actual modules
or their wiring. The library blanks missing digit characters and ignores
trailing input, so use text appropriate for the selected layout.

The sketch's `EXAMPLE_TEXT` can be changed independently of `EXAMPLE_LAYOUT`.
For example, to build `DD` displaying `42` on a Wemos board, pass preprocessor
tokens through the project's build-flags environment variable:

```sh
EXAMPLE_BUILD_FLAGS='-DEXAMPLE_LAYOUT=DD -DEXAMPLE_TEXT=42' \
  pio run -e wemos_d1_mini32
```

The project reads `EXAMPLE_BUILD_FLAGS` from the shell environment through
PlatformIO's `${sysenv...}` option expansion. The values are unquoted
preprocessor tokens; the sketch stringizes them into C++ text. Use a matching
layout/text pair from the table. The command works with `-e c3_supermini` for
the ESP32-C3 too. For the clock, set the value to
`-DEXAMPLE_LAYOUT=DD:DD -DEXAMPLE_TEXT=12:34`; for the decimal layout, use
`-DEXAMPLE_LAYOUT=DD.D -DEXAMPLE_TEXT=12.3`.

As an optional extension, a module can have an inline pixel count:
`D4D4:D2D2` describes two four-pixel-per-segment digits, the default-size colon,
and two two-pixel-per-segment digits (86 LEDs with one pixel per dot).
It still renders `12:34`; the numbers describe physical LED counts, not text.
See the [library layout reference](../../lib/Modular7Segment/README.md) for
the syntax and limits. Only select this layout if it matches your wiring.

## What the sketch demonstrates

- `Modular7Segment::requiredLeds()` calculates the strip length at compile time;
  a `static_assert` rejects a layout that needs no LEDs.
- Runtime `isValid()` checking reports a bad display setup and leaves the strip
  blank rather than trying to render into a mismatched buffer.
- A bounded `snprintf` conversion formats the number `7` as `07` for the serial
  monitor. This is a separate text-formatting example: select the display text
  above to match your physical layout.
- `printString()` renders text, while `clear()` and `printString("")` blank the
  pixel buffer. The sketch alternates between the configured text and blank
  output without blocking delays.
- Rendering changes the NeoPixel buffer; `leds.show()` is still required to
  send it to the LEDs.

The dot brightness uses the library default (110 percent, capped at full RGB
intensity). The whole strip is initialized at a modest brightness.

## Build, upload, and monitor

Run these commands from the lesson directory (or add
`-d examples/01_numbers_and_layouts` when running from the repository root):

```sh
pio run -e wemos_d1_mini32
pio run -e wemos_d1_mini32 -t upload
pio device monitor -b 115200
```

Use `c3_supermini` instead of `wemos_d1_mini32` for the ESP32-C3. Check the
selected board environment and data pin before uploading; upload commands
require a connected board.

## Troubleshooting

- **No light:** confirm the board, configured data pin, LED direction, power,
  and common ground.
- **Wrong characters or module order:** check the physical chain against the
  selected layout and make sure the build-time text matches it.
- **Serial output is missing:** select the correct serial port and use 115200
  baud.
- **Unexpected brightness:** the example intentionally uses low global
  brightness; digit brightness and dot RGB scaling are separate settings.

Next: [02 — Colors](../02_colors/README.md).
