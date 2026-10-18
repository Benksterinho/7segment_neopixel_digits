# 09 — Mixed-size chains

This lesson demonstrates the physical layout `D4D4:3D2D2`, rendered as
`12:34`. The inline counts mean four pixels per segment for the first two
digits, three pixels per colon dot, and two pixels per segment for the last two
digits. The compile-time total is
`28 + 28 + 6 + 14 + 14 = 90` LEDs.

`Modular7Segment::requiredLeds()` computes the strip length from the layout
string at compile time, and a `static_assert` checks the expected total. The
constructor's fallback settings remain four pixels per segment and one pixel
per dot; they apply only to modules without an inline count. Here every module
has an explicit count. Inline counts describe the physical modules in
data-chain order, not the number of characters or punctuation in the text:
`12:34` does not determine the pixel total.

Build with `pio run -e wemos_d1_mini32` or `pio run -e c3_supermini`. The
Wemos D1 Mini32 uses GPIO27 and the ESP32-C3 Super Mini uses GPIO4. Use the
115200-baud serial monitor to see the layout and compile-time count. Build the
physical mixed-size chain in the exact order represented by the layout, and
use an appropriate LED supply and common ground.
