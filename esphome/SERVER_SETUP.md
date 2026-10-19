# Build the clock on an ESPHome server (port 6052)

This guide uses a standalone ESPHome Device Builder dashboard at
`http://<server>:6052`. The server downloads this project's configuration and
external component from GitHub, compiles the firmware, and lets your browser
download it. **No connected or online board is needed to validate or compile.**

The component is already packaged as an ESPHome **external component**. The
**YAML packages** provide the shared clock configuration and board settings.
Neither needs publishing to PyPI or installing manually into the server's Python
environment. You do not need a repository checkout mounted on the server.

## 1. Check the server

Use **ESPHome 2026.9.1**, the version tested by this project. Do not assume an
older server can load this component, or that `latest` will remain compatible.
The official image for this release includes Device Builder **1.14.9**.
Other dashboard releases can have different menu names.

For an existing server:

1. Open `http://<server>:6052` and sign in if required.
2. Check the ESPHome version in the server installation/container, not just the
   separate dashboard's version. With Docker, the image should be
   `ghcr.io/esphome/esphome:2026.9.1`; the server operator can also run
   `docker compose exec esphome esphome version` from its Compose directory.
3. Identify the persistent configuration directory. Inside the official Docker
   image this is `/config`; on the host it is the directory mounted there.
   A standalone Python server uses the directory passed to Device Builder.
4. Ensure the server can write that directory and its caches, resolve DNS, and
   download from GitHub and the ESPHome/ESP-IDF build dependency hosts. Allow
   sufficient disk space and RAM for ESP32 builds; compile one device at a time
   on resource-limited servers.

Do not expose port 6052 directly to the internet. Use dashboard authentication,
a trusted LAN/VPN, and HTTPS through a correctly configured reverse proxy for
untrusted networks. Firmware and downloaded configuration can contain secrets.

If your server already works, skip to step 2.

### Optional: create a new Docker server on Linux

These are instructions for the server operator; Docker/Compose must already be
installed. Do not replace an existing server or overwrite its configuration.

1. Create a dedicated deployment directory outside the repository, then copy
   [compose.yaml](server/compose.yaml) and
   [dashboard.env.example](server/dashboard.env.example) into it.
2. In that deployment directory, run:

   ```sh
   mkdir -p config
   cp dashboard.env.example dashboard.env
   chmod 600 dashboard.env
   ```

3. Edit `dashboard.env`, replacing the password placeholder with a strong,
   private dashboard password. Keep both variables:

   ```dotenv
   ESPHOME_USERNAME=admin
   ESPHOME_PASSWORD=REPLACE_WITH_A_PRIVATE_PASSWORD
   ```

   These are dashboard login credentials, not Wi-Fi, MQTT or device API keys.
   Device Builder uses `ESPHOME_USERNAME` / `ESPHOME_PASSWORD`, **not** the
   legacy `USERNAME` / `PASSWORD` names.

4. Start and check the server:

   ```sh
   docker compose up -d
   docker compose ps
   docker compose logs --tail=50 esphome
   ```

5. Open `http://<server-ip>:6052`. Confirm that it responds and requires the
   configured login before adding private secrets.

The template uses Linux host networking; do not add `ports:` alongside
`network_mode: host`. This permits mDNS for future device access. For a bridge
network deployment, remove host networking and publish `6052:6052`; `.local`
discovery/resolution may not work, so future device access can require IP
addresses. Host networking availability differs on Docker Desktop.

The host's `config/` directory maps to `/config` and persists device YAML,
secrets and build/Git caches. Back it up securely. Set ownership/permissions
for the actual container user rather than making it world-writable.
No privileged mode or USB device mapping is needed to build/download firmware.

## 2. Choose the board and a published project revision

| Actual hardware | Complete server-local template | Default LED pin |
| --- | --- | --- |
| ESP32 / Wemos D1 Mini32 | [neopixel-clock-esp32.yaml.example](server/neopixel-clock-esp32.yaml.example) | GPIO27 |
| ESP32-C3 / Super Mini | [neopixel-clock-c3.yaml.example](server/neopixel-clock-c3.yaml.example) | GPIO4 |

Use the matching template, not the checkout-local YAML from the parent directory.
The server templates contain all local connectivity and secret lookups; remote
packages contain no secrets or local component paths.

**Publication prerequisite:** the revision must exist on
[this GitHub repository](https://github.com/Benksterinho/7segment_neopixel_digits)
and contain all of these files:

```text
esphome/packages/clock-common.yaml
esphome/packages/board-esp32.yaml
esphome/packages/board-c3.yaml
esphome/components/modular_7segment/
```

New server-packaging changes cannot be fetched until someone commits and pushes
them to GitHub. These examples deliberately use
`REPLACE_WITH_PUBLISHED_COMMIT_SHA`, not a fictitious release or an older commit
missing the packages. The placeholder is not a working revision.

Choose the full commit SHA from the tested published commit, or an immutable
release tag. The template uses **one** `project_revision` substitution for both
the package and component refs. Keep them matched. `refresh: never` prevents
background updates; a different revision gets its own cache entry.

## 3. Add local secrets

Open **Secrets** in the dashboard, or edit `secrets.yaml` in the server's
configuration directory. Preserve existing entries used by other devices.
Add missing values or choose distinct key names and update the selected device
template if existing shared values are not appropriate:

```yaml
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
mqtt_host: "192.168.1.10"
mqtt_port: 1883
mqtt_user: "YOUR_MQTT_USER"
mqtt_password: "YOUR_MQTT_PASSWORD"
api_encryption_key: "PASTE_A_NEW_PRIVATE_BASE64_KEY_HERE"
```

Generate a private 32-byte base64 API/OTA key on a trusted machine:

```sh
openssl rand -base64 32
```

Paste the resulting value into `api_encryption_key`. Do not use the public key
from [secrets.yaml.example](secrets.yaml.example) on a real device. The same
private key is used for native API and encrypted ESPHome OTA in these examples.
Keep `mqtt_port` numeric and quote passwords.

Secrets stay local to the server, not in the GitHub packages. Do not commit
them or share validation output with secrets revealed. Restrict access to the
configuration directory, backups and downloaded firmware.

## 4. Create the device configuration

1. Copy the complete template for your board from step 2.
2. On the dashboard choose **Create device**, then **Empty Configuration**.
   Create a unique filename such as `lab-clock.yaml`. Open its YAML editor,
   replace its entire contents with the template, and save.
   Alternatively, copy the template directly to the server's configuration
   directory as `lab-clock.yaml`, then refresh the dashboard.
3. Replace `project_revision` with the published revision selected in step 2.
4. Set a unique `node_name`, for example `lab-clock`, and the appropriate LED
   pin and timezone.
5. If needed, extend the existing `substitutions:` block with your settings:

   ```yaml
   substitutions:
     node_name: lab-clock
     project_revision: "REPLACE_WITH_PUBLISHED_COMMIT_SHA"
     led_pin: GPIO27
     timezone: "Europe/Berlin"
     layout: "DD:D.D."
     num_leds: "116"
     pixels_per_segment: "4"
     pixels_per_dot: "1"
     temperature_offset: "-2.0"
     co2_topic: "env-sensor-station/sensor/scd41_co2/state"
     temperature_topic: "env-sensor-station/sensor/bme688_temperature/state"
     ambient_light_topic: "env-sensor-station/sensor/ambient_light/state"
   ```

   This is a substitutions example, **not a replacement for the complete
   template**. Set the real revision here too; for the C3 use GPIO4 unless your
   hardware wiring specifies another appropriate pin.

Do not append a template to wizard-generated YAML: duplicate `esp32`, `api`,
`wifi` or `light` blocks can conflict. Do not copy `clock-local.yaml` to the
server or add `type: local` / `path: components` to this Git-backed configuration.

The clock requires exactly four digit modules, a colon between digits two and
three, and a decimal between digits three and four. Match layout, LED count and
pixel densities to the physical chain. Defaults describe **116 LEDs**, not an
auto-detected strip.

Your server configuration directory needs only:

```text
config/
  lab-clock.yaml
  secrets.yaml
  .esphome/          # generated Git/build cache; keep it persistent
```

The first validation fetches both the YAML packages and external component
automatically. There is no Python/C++ source copying or server-side pip install
for this project.

## 5. Validate

1. Save the YAML.
2. On the device card/menu choose **Validate**.
3. Wait for Git downloads and configuration validation.
4. Confirm **Configuration is valid!** before compiling.

A missing secret, invalid revision, unsupported ESPHome version or mismatched
LED count must be fixed, not bypassed. Leave secrets hidden in validation output.
An **Offline** or **Unknown** device badge is expected without a flashed board;
it is not a validation or build failure.

## 6. Compile and download, without flashing

For the Device Builder shipped with ESPHome 2026.9.1:

1. Choose **Install** on the device.
2. Select **Download firmware binary**. This explicitly compiles and downloads;
   do **not** choose wireless/network installation or either USB install option.
   Older dashboard versions may call this **Manual download**.
3. Follow the build log. The first build downloads the SDK/toolchain and can
   take substantially longer than subsequent builds.
4. Wait for a successful compile/completed job, then choose the download image
   when prompted:
   - **Factory image** (`firmware.factory.bin`): full image for a future first
     USB installation with a suitable flashing tool.
   - **OTA update** (`firmware.ota.bin`): application update for a device already
     running compatible ESPHome firmware.
5. Save the desired binary. Depending on the dashboard, the downloaded filename
   can include the node name. Do not select ELF debug symbols as flash firmware.

**Stop here when no board is connected.** A successful compile plus a nonempty
downloaded binary is the build-server acceptance check. The board does not need
to resolve over mDNS, join Wi-Fi or connect to MQTT to compile.

The first migration from the standalone PlatformIO firmware remains a separate
USB installation. ESPHome OTA is not ArduinoOTA, and its update image is not a
substitute for the factory image on a blank board. Future OTA requires the
correct reachable device and private key. This guide does not perform flashing,
partition migration, recovery or hardware acceptance; see
[the firmware guide](README.md#flashing-and-rollback----only-when-a-board-is-connected).

### Optional command-line equivalent on the server

For the supplied Docker deployment, with `lab-clock.yaml` in `config/`:

```sh
docker compose exec esphome esphome config /config/lab-clock.yaml
docker compose exec esphome esphome compile /config/lab-clock.yaml
```

These commands do not upload. The dashboard remains the easiest way to choose
and download the resulting factory/OTA artifact.

## 7. Update deliberately

1. Back up device YAML and local secrets privately.
2. Choose a newly tested, published project commit/tag.
3. Change only `project_revision` to update both packages and component together.
4. Validate and compile again; download the new artifact.
5. Handle physical installation separately when the board is available.

Do not move a tag already in use. Do not change the ESPHome image to `latest`
as part of a routine project revision update. An ESPHome version upgrade needs
both board builds and compatibility testing.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Git ref not found or shared package missing | Replace the revision placeholder; confirm the commit and all required files are on GitHub, not just a local Git server. |
| Cannot clone/download | Check server DNS, internet/TLS access and GitHub access. A public repo needs no GitHub token; private access requires separate authentication configuration. |
| Local component folder missing | Use the server template, not a checkout YAML; its source must be `type: git` with `path: esphome/components`. |
| Secret not defined | Put `secrets.yaml` beside the device YAML in the server configuration directory; verify exact key names. Remote packages must not perform secret lookups. |
| Invalid API or OTA key | Generate a new 32-byte base64 key and quote it; do not use placeholder text. |
| Duplicate YAML keys or conflicting components | Replace generated wizard YAML with the complete template rather than appending it. |
| LED count/layout validation error | Make `layout`, pixel densities and `num_leds` agree; retain the required colon/decimal positions. |
| Schema/C++ interface error | Confirm ESPHome 2026.9.1 and matching component/package refs. Do not suppress validation errors. |
| Unexpected old source | Use a new immutable revision instead of moving a cached tag. Both refs use the same substitution; `refresh: never` intentionally avoids polling. |
| Permission denied or repeated downloads | Check ownership and persistence of `/config` and its `.esphome` directory; do not delete secrets to clear a cache. |
| Compile process killed or disk full | Check server RAM, free disk and build logs; avoid concurrent builds and retry after addressing the actual resource issue. |
| Offline/Unknown device | Expected without a running board; use download-only build, not OTA. |
| Cannot open dashboard | Check process/container logs, host firewall and port 6052; confirm authentication and reverse-proxy WebSocket forwarding. |

## Verification scope

The local and Git-backed templates are tested with ESPHome 2026.9.1 using
disposable example secrets and real Git clones of a temporary repository.
Both ESP32 targets compile and produce nonempty factory and OTA artifacts.
Those dummy-credential binaries are **not deployable device firmware**.

The pinned Docker entrypoint, dashboard credentials, menu labels and artifact
choices were checked against upstream release sources (Device Builder 1.14.9,
frontend 0.1.329). Docker execution and interactive dashboard clicks were not
tested in the development environment because Docker/the dashboard were not
installed. Final GitHub fetch verification requires publication of these changes.
Hardware, real-network operation, flashing and OTA recovery remain unverified.

References: [external components](https://esphome.io/components/external_components/),
[packages](https://esphome.io/components/packages/),
[Docker installation](https://esphome.io/install/docker/),
[pinned Docker setup](https://github.com/esphome/esphome/blob/2026.9.1/docker/Dockerfile),
[dashboard authentication](https://github.com/esphome/device-builder/blob/1.14.9/README.md#username--password-authentication),
[pinned download UI](https://github.com/esphome/device-builder-frontend/blob/0.1.329/src/components/install-method-dialog-rows.ts).
