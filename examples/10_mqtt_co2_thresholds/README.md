# 10 — MQTT CO₂ thresholds

This standalone lesson subscribes to one MQTT topic carrying a plain numeric
CO₂ reading in ppm. It uses a four-digit `DDDD` display and changes the digit
color at configurable thresholds: green below 800 ppm, amber from 800 through
1199 ppm, and red at or above 1200 ppm. These example thresholds are
illustrative only; **they are not safety limits, exposure guidance, or a
replacement for a certified CO₂ monitor or applicable health guidance.**

## Configure

From the repository root, copy the configuration template and set your Wi-Fi
and MQTT connection details. The default topic is
`env-sensor-station/sensor/scd41_co2/state`.

```sh
cp examples/10_mqtt_co2_thresholds/include/config.example.h \
   examples/10_mqtt_co2_thresholds/include/config.h
```

`CO2_AMBER_THRESHOLD_PPM` and `CO2_RED_THRESHOLD_PPM` set the lower and upper
color boundaries. The build rejects configurations where the amber boundary
is not lower than the red boundary. Keep `include/config.h` private; it is
ignored by Git.

## Build and upload

From the repository root:

```sh
pio run -d examples/10_mqtt_co2_thresholds -e wemos_d1_mini32
pio run -d examples/10_mqtt_co2_thresholds -e c3_supermini
pio run -d examples/10_mqtt_co2_thresholds -e wemos_d1_mini32 -t upload
pio run -d examples/10_mqtt_co2_thresholds -e c3_supermini -t upload
pio device monitor -d examples/10_mqtt_co2_thresholds
```

Or run those commands without `-d examples/10_mqtt_co2_thresholds` from this
lesson directory. The Wemos D1 Mini32 uses GPIO27 for data; the ESP32-C3 Super
Mini uses GPIO4. Defaults are four pixels per segment and one pixel per dot.
Use an appropriate LED supply and common ground.

## Publish readings

With the default topic, replace the broker/user/password placeholders with
your own connection settings. These shell commands contain no credentials:

```sh
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '650'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '850.4'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '1200'
```

Valid integer and fractional decimal readings are rounded to the nearest ppm.
The parser accepts only complete finite decimal numbers in the 0–65535 range;
empty, malformed, embedded-NUL, non-finite, out-of-range, and overlong
payloads are rejected without replacing the last valid reading. For example,
these payloads are invalid:

```sh
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '850 ppm'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '65536'
```

Before the first valid reading the display shows `----` while on. Readings up
to 9999 ppm are right-aligned in the four digit positions; larger accepted
`uint16_t` readings show red `----` rather than a clipped or misleading value.
Payloads above 65535 ppm are rejected by the sensor parser.

The light starts OFF on every boot. CO₂ readings received while it is OFF update
the retained value but do not turn the light on; a Home Assistant MQTT light
ON command reveals the latest reading. Turning the light OFF clears the LEDs.
The sensor subscribes again after MQTT reconnects, including delivery of any
broker-retained value. This lesson does not create a separate Home Assistant
CO₂ sensor entity.

## Troubleshooting

Monitor at 115200 baud for Wi-Fi/MQTT connection and sensor parse messages.
Check the topic exactly, use plain numbers without units, and verify broker
connectivity. A rejected message leaves the prior valid reading unchanged.
