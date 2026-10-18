# 08 — Signed decimals

This network-free formatter tour reuses the root
`formatTemperature(float, char (&)[6])` helper. The physical layout is
`DD.DD`: two digits, a decimal point, then two digits (113 LEDs at the shared
four-pixel-per-segment, one-pixel-per-dot defaults). The helper's five
characters are rearranged to place the decimal point at the matching physical
module; the unused fourth digit remains blank.

Every 2.5 seconds the sketch logs and displays a scene: waiting/blank,
`25.74` rounding to `25.7`, `25.75` rounding to `25.8`, `-0.04` normalizing to
`0.0`, `-5.25` rounding to `-5.3`, the `-9.9` supported edge, `100.0` rendered
as overflow dashes, then a blank interval. The helper supports values from
`-9.9` through `99.9` Celsius; values outside that range and non-finite values
produce `" ---"`.

Build with `pio run -e wemos_d1_mini32` or `pio run -e c3_supermini`. The
Wemos D1 Mini32 uses GPIO27 and the ESP32-C3 Super Mini uses GPIO4. Serial
output is 115200 baud. Use an appropriate LED supply and common ground, and
match the physical module order to `DD.DD`.
