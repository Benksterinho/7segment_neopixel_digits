# My Lab Display

This repository contains the firmware for the modular NeoPixel 7-segment
display currently running in my **Lab**. It is a real, working project that
brings together a physical LED display, Home Assistant, and sensor data from
the Lab network. The examples and library are separated out so you can also
use the display in your own build.

## The display in my Lab

The current firmware is configured for a WS2812B / NeoPixel chain with this
module layout, in data order:

```text
DD:D.D.
```

That is four digit modules, one colon, and two decimal points. With four
pixels per digit segment and one pixel per dot, this configuration requires
**116 LEDs**. The layout and pixel counts describe the physical hardware; they
must match the modules and wiring in the chain. They are not inferred from the
text being displayed.

The main firmware runs on an ESP32 and combines:

- **Time:** local time synchronized over Wi-Fi using NTP.
- **Environmental readings:** CO2, temperature, and ambient-light values
  received over MQTT from the Lab sensor setup (including SCD41 and BME688
  data).
- **Automatic display modes:** rotates between time, CO2, and temperature,
  skipping sensor modes until a valid reading is available.
- **Home Assistant controls:** MQTT discovery exposes the display as a light
  with on/off, brightness, and RGB controls, plus switches/selects for
  automatic brightness and display mode.
- **Brightness automation:** ambient-light readings can set brightness
  automatically; a manual brightness adjustment returns control to the user.
- **OTA updates:** firmware can be uploaded over Wi-Fi after initial USB
  setup.

The root application is configured for the Wemos D1 Mini32 on GPIO 27 by
default. A separate ESP32-C3 Super Mini PlatformIO environment uses GPIO 4.
These are project defaults, not universal pin assignments.

## Use it in your own build

Start with the [runnable examples guide](examples/README.md). It introduces
the display in small, independent steps, from layout and colors to networking
and Home Assistant. Keep the repository checkout when building an example:
the lessons reuse the library and shared project files under `lib/` and `src/`.

For a display connected to different hardware, choose a layout that describes
your modules in DIN-to-DOUT order, set the pixel counts and data pin for your
setup, and size the NeoPixel buffer for that layout. For example, `D` is one
digit, while `DD:DD` is four digits with a colon; their required LED counts
are different. Confirm that the configured layout, total LED count, and
physical chain all agree before troubleshooting display output.

Useful starting points:

- [Numbers and layouts](examples/01_numbers_and_layouts/): choose a physical
  layout, calculate its LED count, and render text.
- [Colors and brightness](examples/02_colors/): set colors, per-module output,
  and brightness.
- [Clock, NTP, and OTA](examples/03_clock_ntp_ota/): add Wi-Fi time and
  wireless uploads.
- [MQTT discovery and light control](examples/04_mqtt_discovery/): connect a
  display to Home Assistant.
- [The complete examples guide](examples/README.md): prerequisites, board
  setup, all twelve lessons, and build commands.
- [Modular7Segment library reference](lib/Modular7Segment/README.md): layout
  syntax, inline pixel counts, supported characters, and API.
- [Hardware checklist](docs/hw_checklist.md): verify the physical layout,
  chain order, data path, and power.
- [MQTT / Home Assistant guide](docs/MQTT.md): configuration, topics, and
  integration behavior.

## Build and configure the Lab firmware

Install PlatformIO Core or the PlatformIO VS Code extension. Before building
the root application, create local configuration files from the example
templates:

```sh
cp include/net_config.example.h include/net_config.h
cp include/mqtt_config.example.h include/mqtt_config.h
```

Edit `include/net_config.h` with your Wi-Fi and time-zone settings, and
`include/mqtt_config.h` with your broker credentials and sensor topics. These
local configuration files are ignored by Git; do not commit real credentials.

Build or upload to the default Wemos D1 Mini32 environment:

```sh
pio run -e wemos_d1_mini32
pio run -e wemos_d1_mini32 -t upload
pio device monitor -b 115200
```

For the ESP32-C3 Super Mini, use `-e c3_supermini` instead.

OTA requires an
initial USB flash:

1. Connect the board over USB and upload with `wemos_d1_mini32` or
   `c3_supermini`, as appropriate. Configure Wi-Fi so the board joins your
   network.
2. Find the board's IP address on your network (for example, from the serial
   monitor or your router's DHCP client list).
3. Set that IP as `upload_port` in the matching OTA environment (`ota` for
   Wemos, `c3_supermini_ota` for C3) in [platformio.ini](platformio.ini).
4. Upload subsequent firmware updates over Wi-Fi:

   ```sh
   pio run -e ota -t upload
   # or, for ESP32-C3:
   pio run -e c3_supermini_ota -t upload
   ```

## Project structure

```text
examples/                 Runnable lessons for building your own display
include/                  Example and local network/MQTT configuration
lib/                      Modular7Segment and Home Assistant light-state code
src/                      Lab application, display modes, MQTT, network, sensors
test/                     Unit tests and standalone LED hardware checks
docs/                     Hardware checklist and MQTT reference
```

The standalone hardware tests are intentionally simpler than the Lab
application. They bypass layout rendering and use a fixed LED buffer; adjust
their pin and LED count to match your hardware before running them. See the
[hardware checklist](docs/hw_checklist.md) for details.
