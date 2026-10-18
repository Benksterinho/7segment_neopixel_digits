#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "mqtt_config.h"
#include "net_config.h"

#include "display/display_mode_controller.h"
#include "display/temperature_format.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/mqtt_select_module.h"
#include "mqtt/mqtt_switch_module.h"
#include "net/ota_manager.h"
#include "net/time_sync.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_ambient_light_sensor.h"
#include "sensors/mqtt_co2_sensor.h"
#include "sensors/mqtt_temperature_sensor.h"

// Hardware Configuration
constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "DD:D.D.";
constexpr uint8_t countDigitModules(const char* layout) {
  return *layout == '\0'
             ? 0
             : (*layout == 'D' ? 1 : 0) + countDigitModules(layout + 1);
}
constexpr uint8_t DISPLAY_DIGIT_COUNT = countDigitModules(LAYOUT);
static_assert(DISPLAY_DIGIT_COUNT > 0, "Layout must contain a digit");

constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint8_t DOT_BRIGHTNESS_PERCENT = 130;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");

constexpr char OTA_HOSTNAME[] = "neopixel-clock";
constexpr uint32_t MODE_SWITCH_INTERVAL_MS = 30000;
constexpr uint32_t DISPLAY_REFRESH_INTERVAL_MS = 1000;

// Display mode options exposed to Home Assistant, indexed by DisplayMode
constexpr const char* DISPLAY_MODE_OPTIONS[DisplayModeController::MODE_COUNT] = {
    "Time", "CO2", "Temperature"};

// Forward Declarations
void render();
void onLightCommand(const LightState& newState, bool brightnessSpecified);
void onAutoBrightnessChange(bool enabled);
void onAutoModeChange(bool enabled);
void onDisplayModeSelect(uint8_t index);
void onCo2Update(uint16_t ppm);
void onTemperatureUpdate(float celsius);
void onAmbientLightUpdate(float lux, uint8_t computedBrightness);

// Hardware and State
Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT,
                        DOT_BRIGHTNESS_PERCENT);
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
MqttSwitchModule autoModeSwitch("auto_mode", "Auto Mode", true,
                                onAutoModeChange);
MqttSelectModule displayModeSelect("display_mode", "Display Mode",
                                   DISPLAY_MODE_OPTIONS,
                                   DisplayModeController::MODE_COUNT, 0,
                                   onDisplayModeSelect);
MqttCo2Sensor co2Sensor(MQTT_CO2_TOPIC, onCo2Update);
MqttTemperatureSensor temperatureSensor(MQTT_TEMPERATURE_TOPIC,
                                       onTemperatureUpdate);
MqttAmbientLightSensor ambientLightSensor(MQTT_AMBIENT_LIGHT_TOPIC,
                                         onAmbientLightUpdate);

// Display State
DisplayModeController modeController(MODE_SWITCH_INTERVAL_MS);
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

  char text[DISPLAY_DIGIT_COUNT + 2] = {};
  uint32_t color = 0;

  const DisplayMode mode = modeController.mode();
  if (mode == DisplayMode::TIME ||
      (mode == DisplayMode::CO2 && !co2Sensor.hasValue()) ||
      (mode == DisplayMode::TEMPERATURE && !temperatureSensor.hasValue())) {
    const bool dotOn =
        (millis() / DISPLAY_REFRESH_INTERVAL_MS) % 2 == 0;
    const char* timeFormat = dotOn ? "%H:%M" : "%H%M";
    if (!timeSync.getFormattedTime(text, sizeof(text), timeFormat)) {
      snprintf(text, sizeof(text), "----");
    }
    color = leds.Color(lightState.red, lightState.green, lightState.blue);
  } else if (mode == DisplayMode::TEMPERATURE) {
    formatTemperature(temperatureSensor.getCelsius() -2, text);
    color = leds.Color(lightState.red, lightState.green, lightState.blue);
  } else if (mode == DisplayMode::CO2) {
    // CO2 Mode
    const uint16_t ppm = co2Sensor.getPpm();
    snprintf(text, sizeof(text), "%4u", ppm > 9999 ? 9999 : ppm);

    if (ppm < 800) {
      color = leds.Color(0, 255, 0);
    } else if (ppm < 1200) {
      color = leds.Color(255, 130, 0);
    } else {
      color = leds.Color(255, 0, 0);
    }
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

void onAutoModeChange(bool enabled) {
  modeController.setAuto(enabled, millis());
  render();
}

void onDisplayModeSelect(uint8_t index) {
  if (index >= DisplayModeController::MODE_COUNT) {
    return;
  }
  autoModeSwitch.setState(false);
  const uint32_t now = millis();
  modeController.setAuto(false, now);
  modeController.selectMode(static_cast<DisplayMode>(index), now);
  render();
}

void onCo2Update(uint16_t ppm) {
  (void)ppm;
  if (modeController.mode() == DisplayMode::CO2) {
    render();
  }
}

void onTemperatureUpdate(float celsius) {
  (void)celsius;
  if (modeController.mode() == DisplayMode::TEMPERATURE) {
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

  modeController.begin(
      autoModeSwitch.getState(),
      static_cast<DisplayMode>(displayModeSelect.getIndex()), millis());

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
  mqttManager.registerModule(&autoModeSwitch);
  mqttManager.registerModule(&displayModeSelect);
  mqttManager.registerModule(&co2Sensor);
  mqttManager.registerModule(&temperatureSensor);
  mqttManager.registerModule(&ambientLightSensor);

  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
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

  // In auto mode, cycle every 30 seconds, skipping sensors without readings.
  if (modeController.loop(now, co2Sensor.hasValue(),
                          temperatureSensor.hasValue())) {
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