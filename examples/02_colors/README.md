# 02 — Colors

This sketch displays `12:34` on a fixed `DD:DD` chain and cycles through
whole-display RGB color, separate colors for digits and the colon, and the same
per-module colors at a lower global brightness. Scene changes are scheduled
with `millis()`; there are no long blocking delays.

## Before you build

Build from this lesson directory inside the repository checkout. The project
uses the original `lib/Modular7Segment` implementation and selects only this
lesson's `src/main.cpp`. The shared example configuration supplies the Arduino
platform and NeoPixel dependency. It defaults to four pixels per segment and
one pixel per dot; GPIO27 is used on Wemos D1 Mini32 and GPIO4 on ESP32-C3
Super Mini.

This lesson expects exactly two digits, a colon, and two more digits, wired in
that order. Check the data pin and the board pinout, use an appropriate
power supply with common ground, and start at the example's deliberately low
brightness.

## RGB, module colors, and brightness

`leds.Color(red, green, blue)` creates the RGB color values used by
`printString()` and `setModule()`. The first scene uses one RGB color across the
whole text. In the next scene, `setModule()` assigns independent colors to the
physical modules:

| Index | Physical module | Rendered character |
| ---: | --- | --- |
| 0 | first digit | `1` |
| 1 | second digit | `2` |
| 2 | colon | `:` |
| 3 | third digit | `3` |
| 4 | fourth digit | `4` |

Module indices follow layout/data-chain order. They are not segment numbers.
For a different physical layout, update the indices and text to match it.

Global strip brightness (`setBrightness`) scales all emitted RGB light.
`Modular7Segment` also scales the RGB channels of colon and decimal-point colors
using its `dotBrightnessPercent` constructor argument; it does not affect
digit segments. This example passes `100` for unchanged dot RGB values. The
library default is `110`, which brightens dots by 10 percent where possible,
capping each channel at full intensity. Neither setting changes the RGB color
chosen by `leds.Color()`.

Adafruit NeoPixel brightness affects buffered pixel data. The sketch sets
global brightness before clearing and rendering each scene, then calls
`show()`. When changing brightness in another sketch, clear and render the
scene again from its original color values before showing it; do not assume
changing brightness alone reconstructs the original buffer.

## Build, upload, and monitor

From this lesson directory (or from the repository root with
`-d examples/02_colors`):

```sh
pio run -e wemos_d1_mini32
pio run -e wemos_d1_mini32 -t upload
pio device monitor -b 115200
```

Replace `wemos_d1_mini32` with `c3_supermini` for the ESP32-C3. Uploading
requires a connected board on the correct port.

## Troubleshooting

- **No light or unexpected colors:** verify power, common ground, data direction,
  and the selected board's configured pin.
- **Colors appear on the wrong digit:** check physical module order and the
  index table above.
- **Dots are brighter or dimmer than expected:** review the constructor's
  `dotBrightnessPercent` separately from `setBrightness`.
- **Serial monitor is unreadable:** select the board's serial port at 115200
  baud.

Reference: [Modular7Segment API](../../lib/Modular7Segment/README.md).
Previous: [01 — Numbers and layouts](../01_numbers_and_layouts/README.md).
