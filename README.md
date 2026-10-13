# NeoPixel 7-Segment Display Clock & Environmental Monitor

An ESP32-based modular firmware for driving daisy-chained WS2812B / NeoPixel 7-segment displays.

The system alternates between showing the synchronized **current local time** and **SCD41 CO2 readings**, with dynamic **auto-brightness** based on room ambient lux, full **Home Assistant MQTT integration**, and **ArduinoOTA** support.

---

## Features

- **Decoupled & Modular Architecture**:
  - **Hardware / Display**: `Modular7Segment` runs completely standalone without any network or MQTT requirements.
  - **Network (`src/net/`)**: Self-contained `WifiManager`, `OtaManager`, and `TimeSync` (SNTP).
  - **MQTT Core (`src/mqtt/`)**: `MqttManager` handles connection lifecycle, reconnect backoff, and routes messages to registered `IMqttModule` implementations.
  - **Sensors (`src/sensors/`)**: Plug-in MQTT sensors such as `MqttCo2Sensor` and `MqttAmbientLightSensor`.
- **Dual Display Modes (30-second cycle)**:
  - **Time Mode (30s)**: Formats current local time (24h `HH.MM`) using the configured Home Assistant light color, blinking the decimal point every second. Shows `----` if time is not yet synced via NTP.
  - **CO2 Mode (30s)**: Displays SCD41 CO2 concentration (right-aligned ppm, e.g. ` 750`). Color-coded by air quality thresholds:
    - **Green**: `< 800 ppm` (Good)
    - **Yellow**: `800 .. 1199 ppm` (Fair)
    - **Red**: `≥ 1200 ppm` (Poor / Ventilate)
  - Falls back gracefully to Time Mode if no CO2 reading has been received yet.
- **Ambient Light & Auto-Brightness**:
  - Subscribes to ambient lux readings from MQTT (`env-sensor-station/sensor/ambient_light/state`).
  - Home Assistant MQTT switch entity (`Auto-Brightness`) to toggle between automatic and manual brightness control.
  - When Auto-Brightness is **ON**:
    - The display dynamically scales its brightness via a logarithmic perception curve clamped between `15` (~6%) and `255`.
    - The actual lux-calculated brightness is published back to Home Assistant, keeping the brightness slider synchronized in real time.
  - When you **manually adjust the brightness slider** in Home Assistant:
    - Auto-Brightness automatically turns **OFF** and your manual setting takes effect immediately.
    - Toggling the Auto-Brightness switch back ON restores automatic ambient control.
  - Auto-brightness toggle and manual brightness are persisted in ESP32 flash (NVS).
- **Home Assistant MQTT Light Entity**:
  - Native JSON schema MQTT light with automatic discovery.
  - Power control (`ON`/`OFF`), brightness slider (`1..255`), and RGB color picker.
  - Turning the light `OFF` blanks the display completely across all modes; turning it back `ON` resumes the active display.
  - State and brightness debounced and persisted to NVS flash (`LightStore`).
- **ArduinoOTA**:
  - Over-the-air firmware updates over Wi-Fi.

---

## Directory Structure

```
.
├── include/
│   ├── net_config.example.h       # Template for Wi-Fi & NTP configuration
│   ├── mqtt_config.example.h      # Template for MQTT broker & sensor topics
│   ├── net_config.h               # Git-ignored private Wi-Fi & NTP credentials
│   └── mqtt_config.h              # Git-ignored private MQTT credentials & topics
├── lib/
│   ├── LightState/                # Home Assistant light state parser & NVS store
│   └── Modular7Segment/           # Standalone NeoPixel 7-segment layout & rendering
├── src/
│   ├── net/
│   │   ├── wifi_manager.h/.cpp    # Non-blocking Wi-Fi management & reconnects
│   │   ├── ota_manager.h/.cpp     # ArduinoOTA integration
│   │   └── time_sync.h/.cpp       # SNTP (configTzTime) and time formatting
│   ├── mqtt/
│   │   ├── mqtt_module.h          # IMqttModule lifecycle interface
│   │   ├── mqtt_manager.h/.cpp    # Central PubSubClient & message dispatcher
│   │   ├── mqtt_light_module.h/.cpp  # HA MQTT Light entity
│   │   └── mqtt_switch_module.h/.cpp # HA MQTT Switch entity (Auto-Brightness)
│   ├── sensors/
│   │   ├── mqtt_co2_sensor.h/.cpp # SCD41 CO2 subscriber & color mapper
│   │   └── mqtt_ambient_light_sensor.h/.cpp # Lux subscriber & log brightness
│   └── main.cpp                   # Firmware orchestration & display loop
├── test/                          # Unit tests (parsing, thresholds, layouts)
└── platformio.ini
```

---

## Configuration

Credentials and topics are cleanly separated by semantic concern:

1. Copy the example templates:
   ```bash
   cp include/net_config.example.h include/net_config.h
   cp include/mqtt_config.example.h include/mqtt_config.h
   ```

2. Configure `include/net_config.h`:
   ```cpp
   constexpr char WIFI_SSID[] = "your-wifi-ssid";
   constexpr char WIFI_PASSWORD[] = "your-wifi-password";

   constexpr char NTP_SERVER[] = "pool.ntp.org";
   // POSIX timezone string (e.g. Central Europe CET/CEST with daylight saving time):
   constexpr char NTP_TIMEZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";
   ```

3. Configure `include/mqtt_config.h`:
   ```cpp
   constexpr char MQTT_HOST[] = "192.168.1.100";
   constexpr uint16_t MQTT_PORT = 1883;
   constexpr char MQTT_USER[] = "sensor_user";
   constexpr char MQTT_PASSWORD[] = "secret";

   constexpr char MQTT_CO2_TOPIC[] = "env-sensor-station/sensor/scd41_co2/state";
   constexpr char MQTT_AMBIENT_LIGHT_TOPIC[] = "env-sensor-station/sensor/ambient_light/state";
   ```

---

## Usage Examples

### Example 1: Minimal Standalone Display (No Wi-Fi / No MQTT)

To use the 7-segment display library in an isolated Arduino sketch without any networking:

```cpp
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "DDDD"; // 4 digits
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

void setup() {
  leds.begin();
  leds.setBrightness(50);
  leds.clear();
  leds.show();

  if (display.isValid()) {
    // Print text in red:
    display.printString("1234", leds.Color(255, 0, 0));
    leds.show();
  }
}

void loop() {
  // Update display as needed
}
```

### Example 2: Adding a Custom MQTT Sensor Module

Adding a new sensor (e.g., temperature) to the shared `MqttManager` only requires implementing `IMqttModule`:

```cpp
#include "mqtt/mqtt_module.h"

class MqttTempSensor : public IMqttModule {
 public:
  explicit MqttTempSensor(const char* topic) : topic_(topic) {}

  void onMqttConnected(PubSubClient& client, const char* deviceId) override {
    client.subscribe(topic_);
  }

  bool handleMessage(const char* topic, const uint8_t* payload, unsigned int length) override {
    if (strcmp(topic, topic_) != 0) {
      return false;
    }
    // Parse temperature and store value
    char buf[16] = {};
    size_t copyLen = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
    memcpy(buf, payload, copyLen);
    temperature_ = atof(buf);
    return true;
  }

  float getTemperature() const { return temperature_; }

 private:
  const char* topic_;
  float temperature_ = 0.0f;
};
```

Register it in `setup()`:
```cpp
MqttTempSensor tempSensor("env-sensor-station/sensor/temperature/state");

void setup() {
  // ...
  mqttManager.registerModule(&tempSensor);
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
}
```

---

## Building and Flashing

Build the project using PlatformIO:

```bash
# Compile for serial upload (Wemos D1 Mini ESP32)
pio run -e wemos_d1_mini32

# Upload via USB
pio run -e wemos_d1_mini32 -t upload

# Upload via OTA over Wi-Fi
pio run -e ota -t upload
```

## Running Tests

PlatformIO unit tests run natively or on-target:

```bash
pio test -e wemos_d1_mini32
```
