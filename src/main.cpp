#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "mqtt_config.h"
#include "net_config.h"

#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/mqtt_switch_module.h"
#include "net/ota_manager.h"
#include "net/time_sync.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_ambient_light_sensor.h"
#include "sensors/mqtt_co2_sensor.h"

// Hardware Configuration
constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "DDDD";
constexpr uint8_t countDigitModules(const char* layout) {
  return *layout == '\0'
             ? 0
             : (*layout == 'D' ? 1 : 0) + countDigitModules(layout + 1);
}
constexpr uint8_t DISPLAY_DIGIT_COUNT = countDigitModules(LAYOUT);
static_assert(DISPLAY_DIGIT_COUNT > 0, "Layout must contain a digit");

constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");

constexpr char OTA_HOSTNAME[] = "neopixel-clock";
constexpr uint32_t MODE_SWITCH_INTERVAL_MS = 30000;
constexpr uint32_t DISPLAY_REFRESH_INTERVAL_MS = 1000;

// Display Mode
enum class DisplayMode {
  TIME,
  CO2,
};

// Forward Declarations
void render();
void onLightCommand(const LightState& newState, bool brightnessSpecified);
void onAutoBrightnessChange(bool enabled);
void onCo2Update(uint16_t ppm);
void onAmbientLightUpdate(float lux, uint8_t computedBrightness);

// Hardware and State
Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
LightState lightState;
LightStore lightStore;

// Network & Time Services
WifiManager wifiManager;
OtaManager otaManager;
TimeSync timeSync;

// MQTT Services & Modules
MqttManager mqttManager;
MqttLightModule lightModule(lightState, onLightCommand);
MqttSwitchModule autoBrightnessSwitch("auto_brightness", "Auto-Brightness", true,
                                      onAutoBrightnessChange);
MqttCo2Sensor co2Sensor(MQTT_CO2_TOPIC, onCo2Update);
MqttAmbientLightSensor ambientLightSensor(MQTT_AMBIENT_LIGHT_TOPIC,
                                         onAmbientLightUpdate);

// Display State
DisplayMode currentMode = DisplayMode::TIME;
uint32_t lastModeSwitchMs = 0;
uint32_t lastDisplayRefreshMs = 0;

uint8_t resolveEffectiveBrightness() {
  if (autoBrightnessSwitch.getState() && ambientLightSensor.hasValue()) {
    return ambientLightSensor.getComputedBrightness();
  }
  return lightState.brightness;
}

void render() {
  if (!lightState.on || !display.isValid()) {
    leds.clear();
    leds.show();
    return;
  }

  const uint8_t effectiveBrightness = resolveEffectiveBrightness();
  leds.setBrightness(effectiveBrightness);

  char text[DISPLAY_DIGIT_COUNT + 1] = {};
  uint32_t color = 0;

  if (currentMode == DisplayMode::TIME || !co2Sensor.hasValue()) {
    if (!timeSync.getFormattedTime(text, sizeof(text), "%H%M")) {
      snprintf(text, sizeof(text), "----");
    }
    color = leds.Color(lightState.red, lightState.green, lightState.blue);
  } else {
    // CO2 Mode
    const uint16_t ppm = co2Sensor.getPpm();
    snprintf(text, sizeof(text), "%4u", ppm > 9999 ? 9999 : ppm);

    uint8_t r = 0, g = 0, b = 0;
    MqttCo2Sensor::getColorRgb(ppm, r, g, b);
    color = leds.Color(r, g, b);
  }

  display.printString(text, color);
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  if (brightnessSpecified && autoBrightnessSwitch.getState()) {
    Serial.println(
        "Manual brightness command received: turning off Auto-Brightness");
    autoBrightnessSwitch.setState(false);
  }
  render();
  lightStore.markDirty(millis());
}

void onAutoBrightnessChange(bool enabled) {
  (void)enabled;
  render();
  lightModule.publishState();
}

void onCo2Update(uint16_t ppm) {
  (void)ppm;
  if (currentMode == DisplayMode::CO2) {
    render();
  }
}

void onAmbientLightUpdate(float lux, uint8_t computedBrightness) {
  (void)lux;
  if (autoBrightnessSwitch.getState() && lightState.on) {
    render();
    static uint8_t lastReportedBrightness = 0;
    if (computedBrightness != lastReportedBrightness) {
      lastReportedBrightness = computedBrightness;
      lightModule.publishState();
    }
  }
}

void setup() {
  Serial.begin(115200);

  lightStore.load(lightState);

  // Initialize and clear LED strip immediately to prevent power-on flash
  leds.begin();
  leds.setBrightness(resolveEffectiveBrightness());
  leds.clear();
  leds.show();
  delay(100);

  Serial.printf("NUM_LEDS: %u\n", NUM_LEDS);
  Serial.printf("Display valid: %s\n", display.isValid() ? "yes" : "no");

  render();

  // Connect Wi-Fi
  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);

  // Setup OTA and SNTP once Wi-Fi is ready
  if (wifiManager.isConnected()) {
    otaManager.begin(OTA_HOSTNAME);
    timeSync.begin(NTP_TIMEZONE, NTP_SERVER);
  }

  // Configure MQTT and register feature modules
  lightModule.setCommandHandler(onLightCommand);
  lightModule.setBrightnessProvider(resolveEffectiveBrightness);
  mqttManager.registerModule(&lightModule);
  mqttManager.registerModule(&autoBrightnessSwitch);
  mqttManager.registerModule(&co2Sensor);
  mqttManager.registerModule(&ambientLightSensor);

  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);

  lastModeSwitchMs = millis();
}

void loop() {
  wifiManager.loop();
  otaManager.loop();
  mqttManager.loop();

  const uint32_t now = millis();
  lightStore.loop(now, lightState);

  if (!display.isValid()) {
    return;
  }

  // Switch display mode every 30 seconds if CO2 reading is available
  if (now - lastModeSwitchMs >= MODE_SWITCH_INTERVAL_MS) {
    lastModeSwitchMs = now;
    if (co2Sensor.hasValue()) {
      currentMode = (currentMode == DisplayMode::TIME) ? DisplayMode::CO2
                                                       : DisplayMode::TIME;
    } else {
      currentMode = DisplayMode::TIME;
    }
    render();
  }

  // Periodic refresh (at least once per second for time clock ticking)
  if (now - lastDisplayRefreshMs >= DISPLAY_REFRESH_INTERVAL_MS) {
    lastDisplayRefreshMs = now;
    if (lightState.on) {
      render();
    }
  }
}