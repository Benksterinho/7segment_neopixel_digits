# 03 - Wi-Fi clock, NTP, and OTA

Show local 24-hour time on a **DD:DD** chain. This lesson adds Wi-Fi,
network time (SNTP), and wireless firmware updates, but no MQTT.
It reuses the [network helpers](../../src/net/) and
[display library](../../lib/Modular7Segment/).

## Hardware and configuration

Use four digits and a two-dot colon in data-chain order. Defaults are four
pixels per segment and one per dot: **114 LEDs**. See the
[examples guide](../README.md) for wiring, power, and board settings.

From the repository root:

```sh
cp examples/03_clock_ntp_ota/include/config.example.h examples/03_clock_ntp_ota/include/config.h
```

Edit the ignored local configuration with your Wi-Fi credentials, NTP server,
POSIX timezone, and a hostname unique on your LAN. The supplied timezone is
Central European time with daylight saving; use `UTC0` for UTC. POSIX offset
signs differ from typical UTC offsets: consult your timezone's POSIX rule.
No root application credential files are needed.

## Build, flash, and monitor

Run from the repository root with PlatformIO installed:

```sh
pio run -d examples/03_clock_ntp_ota -e wemos_d1_mini32
pio run -d examples/03_clock_ntp_ota -e wemos_d1_mini32 -t upload
pio device monitor -d examples/03_clock_ntp_ota -b 115200
```

Replace the environment with `c3_supermini` for ESP32-C3 Super Mini.
Inside this example directory, omit `-d examples/03_clock_ntp_ota`.
Both USB environments build if `-e` is omitted.

The display starts with `----` (colon off), then shows e.g. `12:34`, with
the colon alternating on and off each second. Serial output reports Wi-Fi
connection, OTA hostname, SNTP setup, and first time synchronization.

## How it works

- `WifiManager::begin` tries the initial connection for up to 15 seconds.
  Subsequent `loop` calls retry a disconnected network periodically.
- `serviceNetwork` waits for an actual connection before calling
  `OtaManager::begin` and `TimeSync::begin`, even if the first Wi-Fi attempt
  timed out. OTA is configured before its loop is serviced.
- `TimeSync` uses `configTzTime`; `getFormattedTime` returns false until
  the system clock is plausible. The explicit `----` placeholder distinguishes
  waiting from midnight.
- `%H:%M` lights the colon; `%H%M` leaves it off without shifting digits.
- A 100 ms `millis()` schedule renders without long delays, so the loop can
  handle OTA. After synchronization, the system clock keeps ticking through
  a temporary network loss. A cold boot without network stays at `----`.
- SNTP continues periodic synchronization; Wi-Fi recovery does not reset time.

## OTA after the first USB flash

The board must already be running this example and reachable on the same
trusted network. Note its IP from serial output. Change `upload_port` in this
example's [PlatformIO configuration](platformio.ini), or pass it explicitly:

```sh
pio run -d examples/03_clock_ntp_ota -e ota -t upload --upload-port 192.168.1.50
# For ESP32-C3:
pio run -d examples/03_clock_ntp_ota -e c3_supermini_ota -t upload --upload-port 192.168.1.51
```

The committed `192.0.2.x` addresses are documentation placeholders, not your
device's address. Use the OTA environment matching the board; never flash a
different board's binary. Reserve its IP in your router if desired.

**The existing OTA helper has no authentication configured.** Use this lesson
only on a trusted LAN; do not expose OTA ports to the Internet. This is not
a production authentication/TLS tutorial.

## Checks and troubleshooting

- `----` persists: check Wi-Fi credentials, Internet/NTP reachability (UDP 123),
  and SNTP logs. Wrong local hours generally indicate a timezone rule issue.
- No serial output on C3: use `c3_supermini` for native USB, and reselect the
  serial port after flashing. If needed, hold BOOT, press/release RESET, then
  release BOOT before retrying USB upload.
- First start with the access point unavailable, then restore it: the clock
  should recover, report OTA initialization with your hostname, and sync time.
- Disconnect Wi-Fi after synchronization: the time should keep advancing.
  Reconnect before attempting OTA.
- OTA timeout: verify IP, matching board environment, firewall/routing, and
  that this example is running. USB upload remains the recovery path.
- These network, physical LED, and actual upload checks require hardware;
  a successful compile alone does not verify them.
