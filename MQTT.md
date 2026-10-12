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

- Off blanks the strip; the counter keeps running in the background.
- Brightness is clamped to 1–255, color channels to 0–255. Invalid commands are
  ignored, and the current state is republished so Home Assistant stays in sync.
- On/off, brightness and color are stored in NVS (written ~2 s after the last
  change) and restored on boot.
- The display and OTA keep working without a broker; reconnects are retried
  every 5 s.

## Tests

```sh
.venv/bin/pio test -e wemos_d1_mini32 -f test_light_command
```
