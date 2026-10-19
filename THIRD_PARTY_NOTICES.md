# Third-party dependencies and licensing

The project's original code and documentation are provided under the
[MIT License](LICENSE). Third-party material remains under its respective
license; the project's MIT license does not replace those licenses.

## Declared libraries

The root [PlatformIO configuration](platformio.ini) and
[example configurations](examples/README.md) download external libraries
rather than vendor their source into this repository.

| Library | Declared version range | License checked at baseline version | Upstream license |
|---|---|---|---|
| Adafruit NeoPixel | `^1.12.3` | GNU Lesser General Public License v3 (LGPLv3) | [1.12.3 COPYING](https://github.com/adafruit/Adafruit_NeoPixel/blob/1.12.3/COPYING) |
| PubSubClient | `^2.8` | MIT; copyright (c) 2008-2020 Nicholas O'Leary | [v2.8 LICENSE.txt](https://github.com/knolleary/pubsubclient/blob/v2.8/LICENSE.txt) |
| ArduinoJson | `^7.0.0` | MIT; copyright (c) 2014-2024 Benoit BLANCHON | [v7.0.0 LICENSE.txt](https://github.com/bblanchon/ArduinoJson/blob/v7.0.0/LICENSE.txt) |

Version ranges permit newer releases. This table records the baseline versions
reviewed, not necessarily the versions resolved for a particular build.
Preserve the applicable copyright and full license texts when redistributing
these libraries or substantial portions of them. Upstream links alone are not
a substitute for including required notices with a distribution.

## Framework and transitive components

The project also uses the Arduino framework through PlatformIO's `espressif32`
platform. Arduino-ESP32, ESP-IDF, bundled libraries, and other components can
have separate licenses and notices. The table above is not an exhaustive
inventory of a compiled firmware image.

Before publishing a firmware build, record the resolved platform, framework,
and library versions and inspect the license and notice files in those exact
packages. PlatformIO normally places project libraries under `.pio/libdeps/`
and framework packages under its configured package storage directory.

## Distributing compiled firmware

Licensing the original application under MIT does not remove the obligations
of libraries linked into the firmware.

In particular, Adafruit NeoPixel's LGPLv3 license incorporates GPLv3 terms
with additional permissions. For a statically linked combined work, compliance
can require library source, application object files or another permitted
relinking mechanism, license texts and notices, and installation information
where applicable. Review the applicable license terms for the actual release
and distribution method before publishing firmware binaries.

## Source provenance

This project was developed with AI assistance. That does not change the
licenses of third-party material or establish that every generated passage is
original. Preserve existing notices and check the provenance and license of
any copied or adapted code, examples, or documentation before including them.
