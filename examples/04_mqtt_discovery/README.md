# 04 — MQTT discovery and light control

This project displays the fixed number `1234` as a Home Assistant MQTT light.
It introduces Wi-Fi and broker configuration, MQTT availability, retained
discovery, and Home Assistant ON/OFF, brightness, and RGB commands. It reuses
the repository's existing display, Wi-Fi, MQTT manager, and light module; it
does not add clock/NTP, OTA, or NVS persistence.

## Configure

From the repository root, copy the example configuration and fill it in:

```sh
cp examples/04_mqtt_discovery/include/config.example.h \
   examples/04_mqtt_discovery/include/config.h
```

Set the Wi-Fi credentials, broker hostname/IP, and (if required) MQTT username
and password in `include/config.h`. Do not commit this local file.

## Build and upload

From the repository root:

```sh
pio run -d examples/04_mqtt_discovery -e wemos_d1_mini32
pio run -d examples/04_mqtt_discovery -e c3_supermini
pio run -d examples/04_mqtt_discovery -e wemos_d1_mini32 -t upload
pio run -d examples/04_mqtt_discovery -e c3_supermini -t upload
pio device monitor -d examples/04_mqtt_discovery
```

Or from this lesson directory:

```sh
pio run -e wemos_d1_mini32
pio run -e c3_supermini
pio run -e wemos_d1_mini32 -t upload
pio run -e c3_supermini -t upload
pio device monitor
```

The first upload is over USB. The Wemos D1 Mini32 uses GPIO27 for data; the
ESP32-C3 Super Mini uses GPIO4 and its USB build flags are supplied by the
shared example configuration. Defaults are four pixels per segment and one
pixel per dot. Verify your display wiring and use an appropriately sized,
externally powered LED supply with a common ground.

## MQTT and Home Assistant

Home Assistant MQTT discovery must be enabled and connected to the same broker.
The existing manager uses the device's MAC address to form an ID such as
`neopixel-clock-a1b2c3`; it retains the existing `neopixel-clock` topic prefix
and reports the device name **NeoPixel Clock**. As a result, Part 2 updates the
same discovered light when flashed to the same board.

For device ID `neopixel-clock-a1b2c3`, the relevant topics are:

| Topic | Purpose | Retained |
| --- | --- | --- |
| `homeassistant/light/neopixel-clock-a1b2c3/config` | HA discovery configuration | Yes |
| `neopixel-clock/neopixel-clock-a1b2c3/state` | Current light state | Yes |
| `neopixel-clock/neopixel-clock-a1b2c3/status` | Availability (`online` / `offline`) | Yes |
| `neopixel-clock/neopixel-clock-a1b2c3/set` | JSON light commands | No |

Replace the sample device ID and broker flags below with your actual values:

```sh
mosquitto_sub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/neopixel-clock-a1b2c3/#' -v
mosquitto_sub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'homeassistant/light/neopixel-clock-a1b2c3/config' -v
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/neopixel-clock-a1b2c3/set' \
  -m '{"state":"ON","brightness":128,"color":{"r":0,"g":255,"b":32}}'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/neopixel-clock-a1b2c3/set' -m '{"state":"OFF"}'
```

The ON command shows `1234` at the requested brightness and RGB color; OFF
clears the LEDs. Discovery, state, and availability are retained so a broker or
HA restart can recover the last published values. Commands are not retained.
The manager sets a retained Last Will of `offline`, publishes retained `online`
after connecting, retries a lost broker connection, and re-announces discovery
and state when it receives Home Assistant's `online` birth message. MQTT and
HA behavior should be verified against your own broker and installation.

The Wi-Fi helper can block for up to 15 seconds during its initial connection
attempt; subsequent attempts are serviced from `loop()`. Check the serial
monitor at 115200 baud for Wi-Fi/MQTT status and confirm broker reachability,
credentials, and HA discovery configuration if the entity does not appear.
