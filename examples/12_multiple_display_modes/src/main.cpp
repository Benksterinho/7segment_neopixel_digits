#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "config.h"
#include "display/display_mode_controller.h"
#include "display/temperature_format.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/mqtt_select_module.h"
#include "mqtt/mqtt_switch_module.h"
#include "net/time_sync.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_co2_sensor.h"
#include "sensors/mqtt_temperature_sensor.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "DD:D.D";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");
constexpr uint32_t MODE_SWITCH_INTERVAL_MS = 30000;
constexpr uint32_t DISPLAY_REFRESH_INTERVAL_MS = 1000;
constexpr const char* DISPLAY_MODE_OPTIONS[DisplayModeController::MODE_COUNT] = {
    "Time", "CO2", "Temperature"};

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
LightState lightState;
WifiManager wifiManager;
TimeSync timeSync;
MqttManager mqttManager;
DisplayModeController modeController(MODE_SWITCH_INTERVAL_MS);
bool mqttModulesRegistered = false;
bool timeSyncStarted = false;
uint32_t lastDisplayRefreshMs = 0;

void onLightCommand(const LightState& newState, bool brightnessSpecified);
void onAutoModeChange(bool enabled);
void onDisplayModeSelect(uint8_t index);
void onCo2Update(uint16_t ppm);
void onTemperatureUpdate(float celsius);

MqttLightModule lightModule(lightState, onLightCommand);
MqttSwitchModule autoModeSwitch("auto_mode", "Auto Mode", true,
                                onAutoModeChange);
MqttSelectModule displayModeSelect("display_mode", "Display Mode",
                                   DISPLAY_MODE_OPTIONS,
                                   DisplayModeController::MODE_COUNT, 0,
                                   onDisplayModeSelect);
MqttCo2Sensor co2Sensor(MQTT_CO2_TOPIC, onCo2Update);
MqttTemperatureSensor temperatureSensor(MQTT_TEMPERATURE_TOPIC,
                                        onTemperatureUpdate);

void renderDisplay() {
  if (!lightState.on || !display.isValid()) {
    leds.clear();
    leds.show();
    return;
  }

  leds.setBrightness(lightState.brightness);
  char text[6] = {};
  uint32_t color = leds.Color(lightState.red, lightState.green, lightState.blue);
  const DisplayMode mode = modeController.mode();
  if (mode == DisplayMode::CO2 && co2Sensor.hasValue()) {
    const uint16_t ppm = co2Sensor.getPpm();
    if (ppm > 9999) {
      snprintf(text, sizeof(text), "----");
    } else {
      snprintf(text, sizeof(text), "%4u", static_cast<unsigned>(ppm));
    }
    color = ppm < 800 ? leds.Color(0, 255, 0)
                     : (ppm < 1200 ? leds.Color(255, 130, 0)
                                   : leds.Color(255, 0, 0));
  } else if (mode == DisplayMode::TEMPERATURE && temperatureSensor.hasValue()) {
    formatTemperature(temperatureSensor.getCelsius(), text);
  } else {
    // A manually selected sensor without a reading falls back to the clock.
    const bool colonOn = (millis() / DISPLAY_REFRESH_INTERVAL_MS) % 2 == 0;
    if (!timeSync.getFormattedTime(text, sizeof(text),
                                   colonOn ? "%H:%M" : "%H%M")) {
      snprintf(text, sizeof(text), "----");
    }
  }
  display.printString(text, color);
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  (void)brightnessSpecified;
  renderDisplay();
}

void onAutoModeChange(bool enabled) {
  modeController.setAuto(enabled, millis());
  renderDisplay();
}

void onDisplayModeSelect(uint8_t index) {
  if (index >= DisplayModeController::MODE_COUNT) {
    Serial.println("ERROR: display mode index is out of range");
    return;
  }
  autoModeSwitch.setState(false);
  const uint32_t now = millis();
  modeController.setAuto(false, now);
  modeController.selectMode(static_cast<DisplayMode>(index), now);
  renderDisplay();
}

void onCo2Update(uint16_t ppm) {
  (void)ppm;
  if (modeController.mode() == DisplayMode::CO2) {
    renderDisplay();
  }
}

void onTemperatureUpdate(float celsius) {
  (void)celsius;
  if (modeController.mode() == DisplayMode::TEMPERATURE) {
    renderDisplay();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  lightState.on = false;
  leds.begin();
  leds.setBrightness(lightState.brightness);
  leds.clear();
  leds.show();
  if (!display.isValid()) {
    Serial.println("Invalid display layout; output will remain blank");
  }

  modeController.begin(
      autoModeSwitch.getState(),
      static_cast<DisplayMode>(displayModeSelect.getIndex()), millis());
  IMqttModule* modules[] = {&lightModule, &autoModeSwitch, &displayModeSelect,
                           &co2Sensor, &temperatureSensor};
  for (IMqttModule* module : modules) {
    if (!mqttManager.registerModule(module)) {
      Serial.println("ERROR: MQTT module registration failed; MQTT will not start");
      return;
    }
  }
  mqttModulesRegistered = true;

  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD, 0);
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
}

void loop() {
  if (!mqttModulesRegistered) {
    return;
  }
  wifiManager.loop();
  if (wifiManager.isConnected() && !timeSyncStarted) {
    timeSync.begin(NTP_TIMEZONE, NTP_SERVER);
    timeSyncStarted = true;
  }
  mqttManager.loop();

  const uint32_t now = millis();
  const bool modeChanged = modeController.loop(
      now, co2Sensor.hasValue(), temperatureSensor.hasValue());
  if (modeChanged || now - lastDisplayRefreshMs >= DISPLAY_REFRESH_INTERVAL_MS) {
    lastDisplayRefreshMs = now;
    renderDisplay();
  }
}
