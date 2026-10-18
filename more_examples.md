# Additional example lessons

## Context

The repository has twelve independent PlatformIO lessons under
[examples/](examples/README.md):

1. Numbers and layouts.
2. Colors and brightness.
3. A `DD:DD` clock with Wi-Fi, NTP, and OTA.
4. MQTT Part 1: Home Assistant discovery and fixed-number light controls.
5. MQTT Part 2: subscribe to a plain numeric temperature topic.
6. Segment and wiring walk.
7. A nonblocking four-digit counter.
8. Signed decimal formatting without MQTT.
9. Mixed-size module chains.
10. MQTT CO2 readings with threshold colors.
11. MQTT ambient-light auto-brightness.
12. Automatic and manual clock/sensor display modes.

Lessons 06–12 are implemented independently. See each lesson README for its
build commands, hardware setup, configuration, expected output, and behavior.

## Implemented lessons 06–12

### 06 - Segment and wiring walk

Illuminate modules and segments one scheduled step at a time at low brightness.

- Verify data-chain direction, physical module order, and pixel counts.
- Cover digits, colon dots, and decimal points.
- Print the active module/segment to serial so readers can compare wiring
  with what lights up.
- Use scheduled steps rather than long delays.
- The lesson README explains the selected display API and the relationship
  between physical pixel order and segment labels.

This lesson is intended for hardware troubleshooting before debugging
application code.

### 07 - Counter

Teach time-based updates without networking using a four-digit counter.

- The counter advances once per elapsed second using rollover-safe `millis()`
  subtraction and displays leading zeroes.
- It wraps from `9999` to `0000`; this lesson does not add a button.

### 08 - Signed decimal values without MQTT

Focus on numeric formatting separately from network delivery.

- Show positive and negative readings, rounding, and negative-zero
  normalization using the existing temperature formatter.
- Demonstrate blank/waiting and out-of-range output with a scheduled set of
  values, not a sensor.
- Use a physical layout whose decimal point matches the formatted text.

### 09 - Mixed-size module chains

Demonstrate inline per-module pixel counts.

- Use the layout `D4D4:3D2D2`, including a custom colon pixel count.
- Explain fallback counts, physical wiring, and compile-time LED sizing.
- Show that displayed text is independent of module pixel counts and that
  physical modules must match the configured chain.

This lesson gives the mixed geometry a dedicated hardware walkthrough beyond
the introductory layout reference in lesson 01.

### 10 - MQTT CO2 thresholds

Extend the MQTT subscription pattern to CO2 readings.

- Show configurable green/amber/red bands using illustrative defaults of
  800/1200 ppm.
- Use a four-digit display and explicitly define waiting and overflow output.
- Explain accepted/rejected payloads and test them with `mosquitto_pub`.
- Keep incoming sensor updates from turning an OFF display back on.
- Reject malformed, non-finite, oversized, and out-of-range payloads
  explicitly.
- Treat thresholds as illustrative display choices, not safety guarantees.

### 11 - Ambient-light auto-brightness

Reuse the existing MQTT lux subscriber.

- Explain the logarithmic brightness mapping and minimum/maximum output.
- Demonstrate manual brightness override and how the MQTT switch restores
  automatic mode.
- Use manual brightness before the first valid reading; retain the latest
  computed brightness if updates stop.
- Re-render after brightness changes, rather than repeatedly scaling stored
  NeoPixel pixel data.
- Keep the lesson focused on this feature instead of copying the full app.

### 12 - Multiple display modes

Combine clock and sensor lessons as a bridge to the full application.

- Reuse the existing display mode controller.
- Alternate time and available sensor readings without blocking networking.
- Use MQTT auto-mode and mode-select controls for manual selection; selecting
  a mode disables cycling until automatic mode is restored.
- Define unavailable-sensor and waiting behavior.
- Keep Home Assistant state and display behavior consistent.
- Explain which other features remain exclusive to the full application.

## Implementation conventions to preserve

- Keep each lesson independently buildable with its own README and
  PlatformIO configuration, following the existing examples.
- Reuse original library/helper implementations instead of copying them.
- Support Wemos D1 Mini32 (GPIO27) and ESP32-C3 Super Mini (GPIO4).
- Keep default geometry at four pixels per segment and one pixel per dot,
  except where a lesson intentionally demonstrates different geometry.
- Keep credentials in ignored local configuration files; commit templates only.
- Document hardware, layout, commands, expected output, and failure behavior.
- Preserve the root application's behavior and avoid unrelated refactoring.
- Build for both boards and run relevant tests where possible. Distinguish
  compilation from executed assertions and actual hardware/network checks.

## Build and verification

Follow the shared [examples guide](examples/README.md) to build lessons for
both supported boards. A successful build does not verify physical LED wiring,
MQTT delivery, Home Assistant behavior, NTP, or OTA; perform and report those
checks separately on the relevant hardware and services.
