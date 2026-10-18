# Runnable examples

Twelve small PlatformIO projects introduce the display library and then the
existing networking/MQTT modules. Open an individual lesson folder in VS Code
with PlatformIO, or build it from the repository root with `pio run -d ...`.
Keep the full repository checkout: lessons reference the original code under
`lib/` and selected files under `src/`; copying a lesson folder alone is
not sufficient.

## Learning path

| Lesson | Learn | Network needed |
|---|---|---|
| [01 - Numbers and layouts](01_numbers_and_layouts/) | Single/multiple digits, separators, formatting, LED counts | No |
| [02 - Colors](02_colors/) | RGB, per-module colors, brightness, scheduled updates | No |
| [03 - Clock, NTP, OTA](03_clock_ntp_ota/) | `DD:DD` time, Wi-Fi recovery, timezone, wireless uploads | Wi-Fi and NTP |
| [04 - MQTT Part 1](04_mqtt_discovery/) | Discovery, availability, fixed-number HA light controls | Wi-Fi, MQTT; HA for discovery |
| [05 - MQTT Part 2](05_mqtt_temperature/) | Subscribe to temperature, signed decimals, invalid readings | Same as Part 1 |
| [06 - Segment and wiring walk](06_segment_wiring_walk/) | Identify module/segment wiring with scheduled low-brightness diagnostics | No |
| [07 - Counter](07_counter/) | Nonblocking one-second counter, zero padding, rollover | No |
| [08 - Signed decimals](08_signed_decimals/) | Rounding, negative zero, waiting and out-of-range output | No |
| [09 - Mixed-size chains](09_mixed_size_chains/) | Inline per-module pixel counts and physical chain sizing | No |
| [10 - MQTT CO2 thresholds](10_mqtt_co2_thresholds/) | CO2 parsing, four-digit display and illustrative color bands | Wi-Fi, MQTT |
| [11 - Ambient-light brightness](11_ambient_light_brightness/) | MQTT lux mapping, automatic brightness and manual override | Wi-Fi, MQTT |
| [12 - Multiple display modes](12_multiple_display_modes/) | Clock/CO2/temperature modes, automatic cycling and manual selection | Wi-Fi, NTP, MQTT; HA controls |

MQTT lessons intentionally use a fixed number and then a sensor, not the clock,
so NTP/OTA do not distract from learning MQTT. They do not persist light state.
Lessons 10–12 build on the same MQTT components for CO2, ambient brightness,
and multiple display modes.
The full [root application](../README.md) combines more features.

## Prerequisites and hardware

- PlatformIO Core (`pio`) or the PlatformIO VS Code extension.
  If using the repository's existing virtual environment, run
  `.venv/bin/pio` from the root instead of `pio`.
- A supported ESP32 board, USB data cable, and WS2812B/NeoPixel display modules.
- A suitable external LED supply, shared ground between supply/ESP32/LEDs,
  and the correct data-chain direction. Never power a large chain from an
  ESP32 GPIO. Start at low brightness and follow the
  [hardware checklist](../HARDWARE_CHECKLIST.md).
- The physical modules must match the selected layout and pixel counts.
  Choose a suitable logic-level interface for your LED supply/hardware.

| PlatformIO environment | Board | Default data pin |
|---|---|---|
| `wemos_d1_mini32` | Wemos D1 Mini32 | GPIO27 |
| `c3_supermini` | ESP32-C3 Super Mini (`esp32-c3-devkitm-1`) | GPIO4 |

Default geometry is **4 pixels per segment** and **1 per dot**. Adjust each
lesson's constants to match your modules. Change `LED_DATA_PIN` in the build
settings when wiring a different GPIO; retain the C3 USB flags.
On C3, hold BOOT, press/release RESET, then release BOOT if automatic USB
download mode fails.

## Layout versus displayed text

The **layout string describes physical modules in data-chain order**:

- `D`: digit, seven segments (28 LEDs at the defaults).
- `:`: two-dot colon (2 LEDs).
- `.`: decimal point (1 LED).

For example `DD:DD` needs 114 LEDs at these defaults. Text `"12:34"` lights
the colon; `"1234"` leaves it off and still fills the four digit modules.
`setModule` indexes physical modules, including separators, not just digits.
The display library writes the NeoPixel pixel buffer; call `leds.show()` to
transmit it. Read the [library API](../lib/Modular7Segment/README.md) for
inline per-module counts, supported characters, and punctuation consumption.

## Build and run

For example, from the repository root:

```sh
pio run -d examples/01_numbers_and_layouts -e wemos_d1_mini32
pio run -d examples/01_numbers_and_layouts -e wemos_d1_mini32 -t upload
pio device monitor -d examples/01_numbers_and_layouts -b 115200
```

Use `-e c3_supermini` for the C3. Inside a lesson folder, omit `-d ...`.
Omitting `-e` builds both board environments, but does not upload either.
Shared [board settings](platformio-common.ini) and explicit source filters
prevent the full application's `main.cpp` from being built into a lesson.
Only the MQTT lessons declare PubSubClient and ArduinoJson dependencies.

Lessons 03-05 and 10-12 each have their own `include/config.example.h`. Copy that
template to `include/config.h` in the same lesson, then edit your local
values as described in its README. These local files and per-project build
outputs are ignored by Git. Never put real credentials in a committed template.
Lessons 01-02 and 06-09 need no Wi-Fi, MQTT, or root application configuration
files.

Start with USB flashing. Only the clock lesson defines optional OTA upload
environments (`ota` and `c3_supermini_ota`); see its README for addresses and
trusted-network limitations. Running a build is not evidence that physical
LEDs, OTA, MQTT, or Home Assistant have been tested.

## Troubleshooting

- Incorrect modules or shifted output: compare the physical data order,
  pixels-per-segment/dot, and layout. The layout is not a formatting mask.
- Dark strip: check supply/ground/data direction and selected data GPIO first.
  Inspect serial logs for an invalid display or network connection.
- Unexpected configuration/build: use `-d` for the lesson, not the root project.
- Network example fails to compile with missing `config.h`: copy that lesson's
  template; network-free examples should never require it.
- MQTT lessons keep the existing MAC-derived `neopixel-clock` device identity.
  Running different lessons on the same board reuses its HA light identity.

## Lesson implementation notes

The additional lesson rationale and implementation choices are summarized in
[more_examples.md](../more_examples.md). CO2 color thresholds in lesson 10 are
illustrative display choices, not safety guarantees. Lessons 06–09 are
standalone and network-free; lessons 10–12 use local credentials and existing
MQTT components.
