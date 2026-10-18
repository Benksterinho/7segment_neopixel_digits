# 12 - Multiple display modes

This independent PlatformIO lesson combines `DisplayModeController`, `TimeSync`,
the MQTT CO2 and temperature subscribers, and the MQTT light, switch, and select
modules. It demonstrates scheduled mode changes without a 30-second blocking
delay. Home Assistant discovers an RGB light, **Auto Mode** switch, and
**Display Mode** select (`Time`, `CO2`, `Temperature`).

## Configure, build, and upload

From the repository root:

```sh
cp examples/12_multiple_display_modes/include/config.example.h \
   examples/12_multiple_display_modes/include/config.h
```

Edit the local `include/config.h`: Wi-Fi, broker credentials, sensor topics,
NTP server, and POSIX timezone. `UTC0` is the template default; for example,
`CET-1CEST,M3.5.0,M10.5.0/3` selects Central European time with DST. For an
unauthenticated broker, set both MQTT credentials to empty strings.
The private local configuration is already ignored by Git; never commit it.

```sh
pio run -d examples/12_multiple_display_modes -e wemos_d1_mini32
pio run -d examples/12_multiple_display_modes -e c3_supermini
pio run -d examples/12_multiple_display_modes -e wemos_d1_mini32 -t upload
pio run -d examples/12_multiple_display_modes -e c3_supermini -t upload
pio device monitor -d examples/12_multiple_display_modes
```

From this directory, omit `-d examples/12_multiple_display_modes`. Serial runs
at 115200 baud. Wemos D1 Mini32 data is GPIO27; ESP32-C3 Super Mini data is
GPIO4. Use an appropriate LED supply and common ground.

## Physical layout and output

The physical chain is `DD:D.D`: four digits, a two-dot colon after digit two,
and a decimal point before digit four. Four LEDs per segment and one per dot
require **115 LEDs**. Text punctuation only lights corresponding physical
modules; it does not create LEDs. The full application also has a trailing dot,
which this lesson does not need.

| Mode | Output while the light is ON |
| --- | --- |
| Time | Local 24-hour `HH:MM`, colon toggling each second; `----` before SNTP sync |
| CO2 | Right-aligned 0-9999 ppm; green below 800, amber 800-1199, red from 1200; red `----` above 9999 |
| Temperature | Unadjusted Celsius, rounded to one decimal in the rightmost three digits (` 25.7`, ` -5.2`); ` ---` outside -9.9 to 99.9 or for non-finite values |

The CO2 color boundaries are illustrative display choices, **not safety limits
or health guidance**. Time and temperature use the light's chosen RGB color.
The temperature formatter normalizes rounded negative zero. Unused dots are
cleared by each complete render.

## Scheduling, availability, and controls

- The light always boots **OFF**. Turning it ON reveals the current mode;
  turning it OFF clears all LEDs. Sensor callbacks, clock refreshes, and mode
  changes never turn an OFF display back on.
- On fresh NVS, Auto Mode starts ON and the controller starts with Time. Every
  30 seconds it advances Time -> CO2 -> Temperature -> Time, independently
  skipping sensor modes that have not received a reading.
- Selecting any mode, including the currently selected one, immediately
  disables Auto Mode and pins that choice. Turning Auto Mode ON resumes from
  the current mode with a new 30-second interval, not an immediate jump.
- A manually selected sensor without a reading shows Time instead (or `----`
  if time is not set). The selected mode stays pinned: the first sensor update
  replaces the fallback immediately, without changing the selection.
- The select represents the **last manual choice**, not the currently cycling
  mode. Automatic cycling does not rewrite the select or write NVS every cycle.
- The shared switch and select persist their settings in NVS (`auto_mode` and
  `display_mode`, also used by the full application). With Auto Mode OFF,
  reboot restores the manual choice; with it ON, reboot starts at Time.
- Sensor availability means "a reading has arrived since boot", not freshness.
  No expiry or MQTT sensor-availability topics are used. Stopped publishers or
  network loss leave the latest reading available. Reconnect resubscribes and
  can receive retained readings. A rejected payload leaves prior data intact.
- SNTP is configured once Wi-Fi first becomes connected, even if connection
  failed at boot. Until synchronization, the clock shows `----`; after sync,
  the local clock continues during disconnection.

Unsigned `millis()` subtraction schedules cycling and one-second redraws across
rollover. Wi-Fi and MQTT are serviced on every loop iteration; there is no
lesson-level `delay()`, polling wait, or mode-duration loop. Wi-Fi initialization
uses a zero timeout. The reused MQTT client still performs synchronous connect
attempts subject to its network timeouts; this is cooperative scheduling, not
a fully asynchronous network stack.

## MQTT walkthrough

Read `MqttManager device id:` on serial and replace `DEVICE_ID` with the entire
printed ID (such as `neopixel-clock-a1b2c3`). Topics use
`neopixel-clock/DEVICE_ID`, intentionally repeating `neopixel-clock`.
Replace broker/user/password placeholders, or omit `-u` and `-P` when unused.

```sh
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/set' -m '{"state":"ON","brightness":50}'
# With no sensor readings, pinning CO2 shows the time fallback.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/display_mode/set' -m 'CO2'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/scd41_co2/state' -m '850'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'env-sensor-station/sensor/bme688_temperature/state' -m '25.7'
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/display_mode/set' -m 'Temperature'
# Resume cycling; Time, CO2, and Temperature are now all eligible.
mosquitto_pub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/auto_mode/set' -m 'ON'
mosquitto_sub -h YOUR_MQTT_BROKER -u YOUR_MQTT_USER -P YOUR_MQTT_PASSWORD \
  -t 'neopixel-clock/DEVICE_ID/state' \
  -t 'neopixel-clock/DEVICE_ID/auto_mode/state' \
  -t 'neopixel-clock/DEVICE_ID/display_mode/state'
```

Publish plain finite numeric values without units. The shared CO2 parser rejects
malformed, non-finite, overlong, and out-of-range payloads and rounds accepted
values to `uint16_t` ppm. The shared temperature parser also rejects incomplete,
non-finite, embedded-NUL, and overlong payloads; finite values outside the
display's temperature range produce ` ---`. This lesson adds no separate Home
Assistant sensor entities.

## What remains in the full application

This lesson intentionally does **not** reproduce the entire root firmware.
The full application adds these integrated features beyond this lesson:

- Ambient-lux subscription and logarithmic Auto-Brightness switch, including
  manual-brightness override, switch restoration, and effective-brightness
  reporting to Home Assistant (demonstrated separately in
  [lesson 11](../11_ambient_light_brightness/README.md)).
- `LightStore` NVS load and debounced persistence of power, manual brightness,
  and RGB color. Here those reset each boot; only auto-mode and mode-selection
  persistence are inherited from the reused controls.
- ArduinoOTA initialization/handling and OTA upload environments. This lesson
  supports USB uploads only.
- The root application's installation-specific temperature correction
  (currently subtracting 2 degrees Celsius) and extra trailing physical dot.
  This lesson displays unadjusted sensor Celsius, so it does not silently
  introduce that calibration into a reusable example.

These are exclusive to the full application **relative to this lesson**, not
necessarily to all other examples. Time, sensor modes, MQTT discovery,
availability, reconnect handling, RGB light control, auto cycling, and manual
selection are already present here. Root source and shared documentation are
not modified by this lesson.

## Verification

On hardware, check time waiting/synchronization, independent sensor skipping,
manual fallback replaced by the first reading, select disabling auto (even for
the same option), restored cycling after 30 seconds, OFF remaining blank,
overflow output, NVS restoration, and MQTT responsiveness between switches.
Build success alone does not verify physical LEDs, NTP/network services,
MQTT broker behavior, or Home Assistant discovery.
