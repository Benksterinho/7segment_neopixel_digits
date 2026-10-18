# 05 — MQTT temperature subscription

This lesson reuses the MQTT light from Part 1 and subscribes to one configurable
topic containing plain Celsius values. The physical display layout is `DD.DD`.
The existing `formatTemperature` helper formats tenths, while this example
aligns its integer field with that layout's physical decimal point. It shows
`----` (decimal point off) until it receives a valid reading, rather than
showing a placeholder temperature value. Turning the light OFF clears the
display. No second Home Assistant temperature entity is created.

## Configure

From the repository root, copy the example configuration and set your network
and broker details. `MQTT_TEMPERATURE_TOPIC` can be changed to any topic that
publishes plain numeric Celsius values.

```sh
cp examples/05_mqtt_temperature/include/config.example.h \
   examples/05_mqtt_temperature/include/config.h
```

The default topic is `examples/7segment/temperature/state`. Do not commit your
local `include/config.h`.

## Build and upload

From the repository root:

```sh
pio run -d examples/05_mqtt_temperature -e wemos_d1_mini32
pio run -d examples/05_mqtt_temperature -e c3_supermini
pio run -d examples/05_mqtt_temperature -e wemos_d1_mini32 -t upload
pio run -d examples/05_mqtt_temperature -e c3_supermini -t upload
pio device monitor -d examples/05_mqtt_temperature
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
pixel per dot. Use an appropriate LED supply and common ground.

## Send temperature readings

With the default topic, publish sample payloads using your broker credentials:

```sh
mosquitto_sub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'examples/7segment/temperature/state' -v
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'examples/7segment/temperature/state' -m '25.7'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'examples/7segment/temperature/state' -m '5.2'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'examples/7segment/temperature/state' -m '-5.2'
```

The corresponding display readings are `25.7`, `5.2`, and `-5.2`. The helper
rounds to tenths, normalizes negative zero, and supports -9.9 through 99.9 °C;
out-of-range numeric values render its overflow indication rather than being
clipped. Malformed, empty, non-finite, embedded-NUL, and oversized payloads are
rejected by the existing sensor helper. Invalid messages are logged and leave
the last valid display reading unchanged. Before the first valid reading, the
display shows four dashes (`----`) with the physical decimal point off.

The sensor subscribes again after every MQTT reconnection. If the broker has a
retained reading for this topic, it is delivered upon subscription. Readings do
not expire automatically. The light remains OFF when a reading arrives while
OFF; switching it ON later shows the latest valid reading. The light discovery,
state, and availability topics and Home Assistant reconnect behavior are the
same as in [Part 1](../04_mqtt_discovery/README.md).

## Troubleshooting

Monitor at 115200 baud for Wi-Fi/MQTT connection status and sensor parse
messages. The initial Wi-Fi attempt can block for up to 15 seconds, with
retries serviced from `loop()`. Check that the topic matches exactly, that
payloads are plain numbers without units, and that the broker is reachable.
Home Assistant MQTT discovery must be enabled to use the light control; raw
temperature subscription does not require creating an HA entity.
