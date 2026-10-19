# ESPHome alternative

This directory contains an **independent ESPHome firmware** for the same display.
The existing PlatformIO application, libraries, examples and build environments
are not changed or imported by this build. Only one firmware runs on a board at
a time.

## Supported configurations

| Configuration | Chip | Default LED pin |
| --- | --- | --- |
| [neopixel-clock-esp32.yaml](neopixel-clock-esp32.yaml) | ESP32 / Wemos D1 Mini32 | GPIO27 |
| [neopixel-clock-c3.yaml](neopixel-clock-c3.yaml) | ESP32-C3 / Super Mini | GPIO4 |

Both use ESP-IDF and ESPHome's RMT addressable LED output. The common defaults
match the current Lab firmware: `DD:D.D.`, four pixels per digit segment, one
per dot, **116 LEDs**, GRB WS2812, 100% dot color scale, and a **-2 C**
temperature calibration offset.

This implementation targets **ESPHome 2026.9.1**, pinned in
[requirements-dev.txt](requirements-dev.txt). ESPHome's component interfaces
can change; test both builds before updating that pin.

## Build on an ESPHome server

For the dashboard at `http://<server>:6052`, follow the
[step-by-step server setup and build guide](SERVER_SETUP.md). It covers existing
servers, optional pinned Docker setup, GitHub-backed packages/components, local
secrets, and validating/compiling/downloading without a connected board.
The component already uses ESPHome's external-component format; no separate
package installation is required.

## Install and configure

Run these commands from the repository root on Linux/macOS. Use a Python version
supported by the pinned ESPHome release (development used Python 3.13).

```sh
python3 -m venv esphome/.venv
esphome/.venv/bin/pip install -r esphome/requirements-dev.txt
cp esphome/secrets.yaml.example esphome/secrets.yaml
openssl rand -base64 32
```

Edit the ignored `secrets.yaml`: set Wi-Fi and MQTT credentials and replace the
example API key with the freshly generated private key. The key also protects
encrypted ESPHome OTA. **The committed example key is public test data, not a
usable device credential.**

For this checkout-based workflow, keep the directory structure: package and
external-component paths are checkout-relative. Copying just these device YAMLs
into a dashboard is not sufficient. Use the
[server templates](server/) and [server guide](SERVER_SETUP.md) for Git-based
deployment, or retain the packages/components alongside the local device YAMLs.

The original repository `.venv` remains dedicated to PlatformIO. ESPHome uses
`esphome/.venv` and does not install into or activate the PlatformIO environment.
Credentials, ESPHome build/storage data and its environment are Git-ignored.

### Hardware and sensor settings

Override substitutions in the selected device YAML, for example:

```yaml
substitutions:
  node_name: lab-clock
  led_pin: GPIO27
  temperature_offset: "-2.0"
  timezone: "Europe/Berlin"
  ntp_server: "pool.ntp.org"
  co2_topic: "env-sensor-station/sensor/scd41_co2/state"
  temperature_topic: "env-sensor-station/sensor/bme688_temperature/state"
  ambient_light_topic: "env-sensor-station/sensor/ambient_light/state"
```

Keep the chip variant appropriate to the actual board. The C3 configuration
uses USB Serial/JTAG logging; the original ESP32 uses UART logging.

The renderer supports `D`, `:`, `.`, optional per-module pixel counts such as
`D4`, and up to 32 modules. This clock application requires exactly four digits,
a colon between the second and third digits, and a decimal between the third
and fourth digits. The default extra decimal points may remain present and
unused. If changing densities or layout, also change `num_leds`; schema
validation rejects a mismatched strip length. This build does not auto-detect
the physical chain or support arbitrary-length clock/temperature formatting.

The reusable common configuration is in
[clock-common.yaml](packages/clock-common.yaml); board packages supply chip/logger
settings, and [clock-local.yaml](packages/clock-local.yaml) supplies checkout-local
component loading and secrets. Server templates supply those local settings
themselves, fetching only secret-free common/board packages from Git.
The physical strip is internal, starts OFF, has gamma 1 and zero default
transitions, and must not have competing effects. The component owns rendering
and applies effective brightness once. Transition/flash effects are not a
display feature; mode and content updates render immediately.

## Build without a connected board

With your local secrets configured:

```sh
esphome/.venv/bin/esphome config esphome/neopixel-clock-esp32.yaml
esphome/.venv/bin/esphome compile esphome/neopixel-clock-esp32.yaml
esphome/.venv/bin/esphome config esphome/neopixel-clock-c3.yaml
esphome/.venv/bin/esphome compile esphome/neopixel-clock-c3.yaml
```

These commands validate/compile only; they do not upload or open a serial port.

For repeatable tests and both firmware builds **without real secrets**, use:

```sh
sh esphome/tests/run-host-tests.sh
sh esphome/tests/run-runtime-tests.sh
esphome/.venv/bin/python -m unittest discover -s esphome/tests -p 'test*config.py' -v
sh esphome/tests/compile-firmware.sh
sh esphome/tests/compile-firmware.sh --server
```

The last three commands stage disposable configurations with the public example
credentials; they never read your private secrets. The compile script retains
generated binaries under the ignored `esphome/.esphome/build/` cache. **Do not
flash these dummy-credential builds.** Recompile your normal configuration with
your own secrets before flashing.

Host tests require a C++ compiler with address/undefined-behavior sanitizers.
Core tests cover rendering, bounds, layouts, formatting, sensor validation,
thresholds, mode scheduling and brightness policy. Runtime tests compile the
actual integration against small ESPHome fakes, simulating publication,
OFF behavior, restore sequencing, persistence failures and command handling.
These simulations do not test the real network, flash, RMT driver or hardware.
Schema tests generate C++ for both variants and reject invalid configurations.
Full builds additionally verify the real ESPHome/ESP-IDF interfaces.
Server tests start with only a thin device YAML and dummy secrets, and fetch
packages and components from a temporary Git repository pinned to one commit.
They exercise actual cloning/merging, not local component mounts. The `--server`
build checks both nonempty factory and OTA artifacts. GitHub-backed builds
additionally require publication of the revision; these tests do not push code.

## Home Assistant and MQTT

Add the node using the **ESPHome integration** and its private API key.
It exposes:

- **Display:** power, brightness and RGB color.
- **Auto-Brightness:** ambient-light versus manual brightness.
- **Auto Mode:** automatic page rotation versus a pinned page.
- **Display Mode:** Time, CO2 or Temperature (last manual selection).

MQTT is used only for incoming readings; native control entities do not register
MQTT counterparts, MQTT discovery is disabled, and the internal physical light
is not exposed. The existing numeric sensor topics are subscribed directly,
without needing Home Assistant to relay them.

### Behavior

- Clock: 24-hour `HH:MM`, blinking colon every second, `----` before SNTP sync.
- Auto pages: Time -> CO2 -> Temperature, 30 seconds per page, independently
  skipping readings that have not arrived. Rotation continues while OFF.
- Selecting a page disables Auto Mode, including reselecting the current option.
  Missing manual sensor pages temporarily show Time. Enabling Auto Mode resumes
  from the current page with a fresh interval; disabling pins the current page.
  The select does not follow automatic rotation.
- CO2: rounded integer ppm, display capped at 9999. Green below 800, RGB
  `(255,130,0)` at 800-1199, red from 1200. The saved user color is unchanged.
- Temperature: calibration is applied before nearest-tenth formatting. The
  rightmost three digits show values from -9.9 to 99.9 C; out-of-range values
  show ` ---`. Rounded negative zero becomes zero. Only the decimal before
  the last digit lights; the trailing dot stays off.
- Auto-brightness: logarithmic lux curve with a 500 lux reference, clamped to
  15-255. HA reports effective brightness, but the last manual brightness is
  kept separately and restored when auto brightness is disabled.
- **Approved native-API difference:** changing brightness to a different value
  disables Auto-Brightness. Re-sending the already-reported value does **not**:
  the native light backend sees the resulting values rather than the explicit
  request fields. Color-only changes do not disable auto brightness. Use the
  Auto-Brightness switch to explicitly take manual control in this case.
- OFF remains OFF during sensor updates, mode changes, automatic brightness
  and color/brightness-only updates. Calling HA `light.turn_on` is an explicit
  ON request, including when an automation only intends to change brightness.
- Finite numeric MQTT payloads are validated strictly (maximum 31 bytes).
  Invalid, non-finite, oversized or malformed data is logged and ignored.
  Unlike the original ambient parser, trailing junk and invalid numeric forms
  are not accepted. Last valid readings do not expire when updates stop.

### Persistence and outages

Power, manual brightness, user RGB, both automatic switches and last manual
mode are restored together from an ESPHome-only versioned preference record.
A fresh installation defaults to ON, red, brightness 50, both automatic switches
enabled and manual mode Time. Restored auto mode starts its rotation at Time.

User changes are debounced for two seconds before being handed to ESPHome
preferences; its flash-write interval is another two seconds. Sudden power loss
inside that window may lose the latest change. Ambient brightness updates do
not continuously write preferences. Failed writes are logged and retried;
invalid stored settings fail the component and leave the display dark.

SNTP and display scheduling run locally. API, MQTT and Wi-Fi reboot timeouts
are disabled so losing HA or the broker does not deliberately reboot the clock.
Previously synchronized time continues locally, but a cold boot without SNTP
has no battery-backed time and displays `----`. Sensor values are held in RAM,
not restored after power loss; after a restart they need a new/retained message.

## Flashing and rollback -- only when a board is connected

The following are **manual operations**, not part of validation. Replace the
example port with the explicitly identified target device.

For the original ESP32:

```sh
esphome/.venv/bin/esphome run esphome/neopixel-clock-esp32.yaml --device /dev/ttyUSB0
```

For the C3:

```sh
esphome/.venv/bin/esphome run esphome/neopixel-clock-c3.yaml --device /dev/ttyACM0
```

After a successful USB installation, encrypted ESPHome OTA can be used:

```sh
esphome/.venv/bin/esphome upload esphome/neopixel-clock-esp32.yaml --device neopixel-clock-esp32.local
esphome/.venv/bin/esphome upload esphome/neopixel-clock-c3.yaml --device neopixel-clock-c3.local
```

ESPHome OTA is not ArduinoOTA. Do not assume that cross-flashing firmware or
bootloader/partition changes is safe over OTA; use USB for the first migration
and rollback.

To return to the original firmware, retain your original private header files
and use the unchanged repository-local PlatformIO executable:

```sh
.venv/bin/pio run -e wemos_d1_mini32 -t upload --upload-port /dev/ttyUSB0
# Or, for the ESP32-C3:
.venv/bin/pio run -e c3_supermini -t upload --upload-port /dev/ttyACM0
```

If USB recovery requires a bootloader mode, follow the actual board's BOOT/RESET
procedure. No automatic erase, partition migration or credential transfer is
performed.

Existing PlatformIO NVS values and HA MQTT entity IDs do not migrate into
ESPHome automatically. Update automations to the new native entities. Old
retained MQTT discovery entries can remain visible; remove only the specific
old device's retained discovery records deliberately after confirming the
replacement, or restore them by returning to the original firmware. Never
blanket-delete broker topics.

## Hardware acceptance -- pending

No boards were connected during development. Complete these checks separately
on **both** board variants before treating this firmware as hardware-verified:

1. Confirm board, pin, power, common ground, physical layout and LED count.
2. Verify startup blanking, all digit segments, colon/decimal placement,
   trailing-dot blanking, RGB order and minimum/maximum brightness.
3. Check time sync/timezone, blinking, all pages, thresholds, offset and rounding.
4. Exercise every HA control, reselect a mode, and test same/different brightness
   values while auto brightness is enabled.
5. Verify that sensor updates and mode changes cannot illuminate an OFF display.
6. Restart after settings settle; verify restored settings and missing-sensor
   fallback. Test the short unsaved-change window separately.
7. Disconnect HA, the broker and Wi-Fi independently; verify local rendering
   continues and reconnects resume data without unintended turn-on/restarts.
8. Verify encrypted OTA, interrupted-update recovery and USB rollback.

## Maintenance boundary

The ESPHome renderer/policy deliberately duplicate the corresponding standalone
logic to keep [the existing library](../lib/Modular7Segment/) and firmware
untouched. The duplication is scoped, tested and documented rather than hidden
behind an Adafruit compatibility shim. Future sharing/refactoring should be a
separate change with explicit permission to modify PlatformIO code.
