# 06 — Segment and wiring walk

This low-brightness, scheduled diagnostic lights each digit segment, each of
the colon's two pixels, and the decimal-point pixel in turn. Serial output
names the item being illuminated. It uses the physical layout `D:.`: one
four-pixel-per-segment digit, a two-pixel colon, and a one-pixel decimal point
(31 LEDs total).

The public `Modular7Segment` API can set whole digit characters and whole dot
modules, but it has no single-segment or single-colon-dot method. This lesson
uses the library's public `Segment` ordering (`BOTTOM`, `LOWER_LEFT`, `MIDDLE`,
`UPPER_LEFT`, `TOP`, `UPPER_RIGHT`, `LOWER_RIGHT`) to address the four LEDs in
one segment directly. It also addresses the two colon pixels individually;
their actual upper/lower physical order depends on how the colon module is
assembled. The decimal point is the final pixel. These offsets assume the
module order shown in `D:.` and standard library module sizes.

Build with `pio run -e wemos_d1_mini32` or `pio run -e c3_supermini`. The
Wemos D1 Mini32 uses GPIO27 and the ESP32-C3 Super Mini uses GPIO4. The
brightness is intentionally limited to 12/255. Use an appropriate LED supply
and common ground; confirm the physical chain order before connecting power.
The sketch reports at 115200 baud and advances without blocking delays.
