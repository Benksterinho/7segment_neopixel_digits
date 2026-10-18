# 07 — Four-digit rollover counter

This network-free sketch starts at `0000`, advances once per elapsed second,
keeps leading zeroes, and wraps from `9999` to `0000`. It uses the physical
four-digit layout `DDDD` with four pixels per segment (112 LEDs). Timing uses
unsigned `millis()` subtraction and accounts for every elapsed second without
drifting from loop latency, so the timer remains valid across wraparound; no
blocking delay is used.

Build with `pio run -e wemos_d1_mini32` or `pio run -e c3_supermini`. The
Wemos D1 Mini32 uses GPIO27 and the ESP32-C3 Super Mini uses GPIO4. Open the
115200-baud serial monitor to follow each count. Use a suitable LED supply and
common ground, and verify that the physical chain contains four digits in the
same order as `DDDD`.
