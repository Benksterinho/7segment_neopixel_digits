# MQTT / Home Assistant integration

The firmware exposes the display as a Home Assistant **MQTT light** (JSON
schema) with on/off, brightness and RGB color. It is discovered automatically
via MQTT Discovery.

## Configuration

Add the broker settings to your local `include/wifi_config.h` (see
`include/wifi_config.example.h`):

```cpp
constexpr char MQTT_HOST[] = "192.168.178.10";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USER[] = "your-mqtt-user";      // "" for anonymous
constexpr char MQTT_PASSWORD[] = "your-mqtt-password";
```

## Topics

`<id>` is `neopixel-clock-` followed by the last three MAC bytes, e.g.
`neopixel-clock-a1b2c3`. It is printed on the serial console at boot.

| Topic | Direction | Retained | Payload |
|-------|-----------|----------|---------|
| `neopixel-clock/<id>/status` | device → broker | yes | `online` / `offline` (offline via Last Will) |
| `neopixel-clock/<id>/state` | device → broker | yes | state JSON (below) |
| `neopixel-clock/<id>/set` | broker → device | no | command JSON (below) |
| `homeassistant/light/<id>/config` | device → broker | yes | discovery config |
| `neopixel-clock/<id>/auto_mode/state` | device → broker | yes | `ON` / `OFF` |
| `neopixel-clock/<id>/auto_mode/set` | broker → device | no | `ON` / `OFF` |
| `homeassistant/switch/<id>/auto_mode/config` | device → broker | yes | discovery config |
| `neopixel-clock/<id>/display_mode/state` | device → broker | yes | `Time` / `CO2` / `Temperature` |
| `neopixel-clock/<id>/display_mode/set` | broker → device | no | `Time` / `CO2` / `Temperature` (case-insensitive) |
| `homeassistant/select/<id>/display_mode/config` | device → broker | yes | discovery config |
| `env-sensor-station/sensor/bme688_temperature/state` | broker → device | publisher-dependent | numeric Celsius, e.g. `25.7` |
| `homeassistant/status` | broker → device | – | `online` triggers a re-announce |

State payload:

```json
{"state":"ON","brightness":50,"color_mode":"rgb","color":{"r":255,"g":0,"b":0}}
```

Commands use the same shape; every field is optional:

```sh
mosquitto_pub -h <broker> -u <user> -P <pass> -t neopixel-clock/<id>/set -m '{"state":"OFF"}'
mosquitto_pub ... -m '{"state":"ON","brightness":128}'
mosquitto_pub ... -m '{"color":{"r":0,"g":255,"b":0}}'
```

## Behavior

- Off blanks the strip in all modes; mode cycling continues in
  the background. Sensor updates and brightness/color-only commands do not
  turn the light back on.
- Brightness is clamped to 1–255, color channels to 0–255. Invalid commands are
  ignored, and the current state is republished so Home Assistant stays in sync.
- On/off, brightness and color are stored in NVS (written ~2 s after the last
  change) and restored on boot.
- The display and OTA keep working without a broker; reconnects are retried
  every 5 s.

## Display mode

- `Auto Mode` switch ON: the display cycles Time → CO2 → Temperature → Time
  every 30 s, independently skipping sensors without a valid reading.
- Publishing an option to the `Display Mode` select shows that mode right away
  and turns `Auto Mode` OFF. Re-sending the current option also disables auto.
- Turning `Auto Mode` ON resumes cycling from the current mode; turning it OFF
  pins the mode currently shown.
- The select state is the last manual choice and is not updated while cycling.
- Selecting `CO2` or `Temperature` before that sensor's first valid reading
  shows the time until one arrives.
- Temperature uses the light's RGB color on the rightmost three digits of the
  `DD.D.D.` layout: ` 25.7`, `  5.2`, or ` -5.2`. Only the decimal before the
  last digit lights; the first digit and other dots stay off. The trailing
  physical dot also stays off in Time and CO2 modes.
- Celsius readings from -9.9 to 99.9 are rounded to the nearest tenth, with
  rounded negative zero shown as zero. Outside this range, the display shows
  ` ---` instead of clipping or using the leftmost digit.
- Configure `MQTT_TEMPERATURE_TOPIC` in `include/mqtt_config.h` using the
  example template. Empty, malformed, oversized, or non-finite temperature
  payloads are logged and ignored, retaining the last valid reading. Like the
  existing sensors, readings do not expire merely because updates stop.
- Unknown options are ignored and the current state is republished.

```sh
mosquitto_pub ... -t neopixel-clock/<id>/display_mode/set -m 'CO2'
mosquitto_pub ... -t neopixel-clock/<id>/display_mode/set -m 'Temperature'
mosquitto_pub ... -t neopixel-clock/<id>/auto_mode/set -m 'ON'
```

## Tests

```sh
.venv/bin/pio test -e wemos_d1_mini32 -f test_light_command
.venv/bin/pio test -e wemos_d1_mini32 -f test_display_mode
.venv/bin/pio test -e wemos_d1_mini32 -f test_sensor_parsing -f test_temperature_display
```

Without a connected ESP32, compile firmware and focused test suites only:

```sh
.venv/bin/pio run -e wemos_d1_mini32
.venv/bin/pio test -e wemos_d1_mini32 -f test_display_mode -f test_sensor_parsing -f test_temperature_display --without-uploading --without-testing
```

Build-only validation does not execute assertions or verify physical LEDs.

## Troubleshooting an unexpected turn-on

Monitor the command and state topics together while reproducing the problem
(commands should **not** be retained):

```sh
mosquitto_sub -h <broker> -u <user> -P <pass> -v \
  -t 'neopixel-clock/<id>/set' \
  -t 'neopixel-clock/<id>/state' \
  -t 'neopixel-clock/<id>/status'
```

- An `ON` message on the command topic identifies an incoming MQTT command,
  not a display-mode change. Check Home Assistant automations and other
  publishers of the command topic. Brightness/color automations should check
  the light's current state before calling `light.turn_on`, since that service
  also turns an OFF light on. A retained `ON` command can replay after a
  reconnect or Home Assistant re-announcement.
- Use the serial console at 115200 baud to check for a restart. State
  persistence is debounced by about two seconds, so a restart immediately
  after an OFF command may restore the previous state.
- If MQTT state stays OFF but LEDs illuminate, investigate the deployed
  firmware and LED output separately.

Timing alone (including a roughly 60-second recurrence) does not identify the
cause. Capture the command/state topics and serial logs across the recurrence.
