# 11 - Ambient-light brightness

This independent PlatformIO lesson shows a fixed `1234` while MQTT lux readings
control its brightness. It reuses `MqttAmbientLightSensor`, `MqttLightModule`,
`MqttSwitchModule`, and the Wi-Fi/MQTT managers; it does not copy their logic.
Home Assistant discovers an RGB light and an **Auto-Brightness** switch.

## Configure, build, and upload

From the repository root:

```sh
cp examples/11_ambient_light_brightness/include/config.example.h \
   examples/11_ambient_light_brightness/include/config.h
```

Edit the local `include/config.h` with your Wi-Fi, broker credentials, and lux
topic. Set `MQTT_USER` and `MQTT_PASSWORD` to empty strings for an unauthenticated
broker. This local file is already ignored by the repository; never commit it.
Only placeholder credentials are supplied in the template.

```sh
pio run -d examples/11_ambient_light_brightness -e wemos_d1_mini32
pio run -d examples/11_ambient_light_brightness -e c3_supermini
pio run -d examples/11_ambient_light_brightness -e wemos_d1_mini32 -t upload
pio run -d examples/11_ambient_light_brightness -e c3_supermini -t upload
pio device monitor -d examples/11_ambient_light_brightness
```

From this directory, omit `-d examples/11_ambient_light_brightness`. The Wemos
D1 Mini32 uses GPIO27; the ESP32-C3 Super Mini uses GPIO4. Serial is 115200 baud.
The `DDDD` layout has four digits, four LEDs per segment, and 112 LEDs in total.
The one-LED dot default is unused here. Wire the actual chain in layout order,
use an appropriate LED supply, and connect a common ground.

## Brightness rules

- The light starts **OFF** at every boot. Turn it ON using the discovered light
  or the MQTT command below. Lux updates never turn it on.
- With no reading yet, the manual `LightState` brightness (initially 50/255)
  applies, even if Auto-Brightness is ON.
- Auto-Brightness defaults ON on fresh NVS. Once a reading arrives, the shared
  sensor maps it logarithmically:
  `15 + log10(1 + min(lux, 500)) / log10(501) * 240`, rounded to the nearest
  integer. Zero lux gives 15; 500 lux and above give 255.
- Any accepted explicit brightness command, even one equal to the previous
  manual value or sent while OFF, disables Auto-Brightness. Power-only and
  color-only commands do not disable it.
- Switching Auto-Brightness OFF restores the remembered manual brightness.
  Switching it ON immediately uses the latest computed value, or manual
  brightness if no reading has arrived yet.
- Readings continue updating the cached value while auto is OFF or the display
  is OFF. There is no freshness timeout: stopped updates or network loss retain
  the latest computed value until another reading or reboot.
- Each brightness change redraws `1234` from its original RGB color after
  `setBrightness()`. It does not repeatedly scale old pixel data, which would
  lose precision. The light's retained MQTT state reports **effective**
  brightness, including automatic changes while OFF.

Only the switch setting is persisted through the shared switch module. Power,
RGB, and manual brightness reset on reboot; this lesson does not use
`LightStore`. The switch uses the same `auto_brightness` NVS key as the full
application, so flashing between them on the same board can reuse that setting.

## MQTT walkthrough

Find `MqttManager device id:` on serial. Replace `DEVICE_ID` below with the
whole printed ID (for example `neopixel-clock-a1b2c3`). The topic prefix is
`neopixel-clock/DEVICE_ID`; the repeated `neopixel-clock` is intentional.
Use your own broker settings; omit `-u` and `-P` if authentication is disabled.

```sh
# Turn on before the first reading: manual brightness is used.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/set' -m '{"state":"ON"}'
# Restore automatic control, including after a previous manual override.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/auto_brightness/set' -m 'ON'
# Repeat with 0, 10, 100, and 500 lux to observe the logarithmic curve.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/ambient_light/state' -m '100'
# Manual override disables the switch and takes effect immediately.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/set' -m '{"brightness":40}'
# Observe the effective brightness and switch state.
mosquitto_sub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/state' \
  -t 'neopixel-clock/DEVICE_ID/auto_brightness/state'
```

Send plain finite nonnegative numeric lux without units. The reused ambient
parser is permissive (it can accept numeric prefixes and truncate long input);
this lesson does not add stricter validation. Do not rely on it to reject every
malformed payload. Empty, nonnumeric, and negative inputs are logged as parse
failures and keep the last reading. No separate Home Assistant lux sensor is
created. Reconnection resubscribes, including any retained sensor reading.

The loop has no lesson-level waits. Initial Wi-Fi connection is started with
zero wait, then retried by the shared manager. MQTT connection attempts still
use the shared synchronous client and its network timeouts.

## Scope and verification

This is a brightness lesson, not the full clock: it omits time synchronization,
mode cycling/selection, CO2/temperature rendering, ArduinoOTA, and debounced
light-state persistence. See [lesson 12](../12_multiple_display_modes/README.md)
for modes and its explicit list of full-application-only integration features.

For a hardware check, verify manual-before-first-reading, lux 0/500 endpoints,
manual override, switch restoration, OFF staying blank during lux updates,
retained brightness after updates stop, and low-to-high redraw without fading
artifacts. A successful build alone does not verify LEDs, broker connectivity,
Home Assistant discovery, or those physical behaviors.
