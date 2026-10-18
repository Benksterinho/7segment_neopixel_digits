#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "config.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "mqtt/mqtt_switch_module.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_ambient_light_sensor.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "DDDD";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
LightState lightState;
WifiManager wifiManager;
MqttManager mqttManager;
bool mqttModulesRegistered = false;

void onLightCommand(const LightState& newState, bool brightnessSpecified);
void onAutoBrightnessChange(bool enabled);
void onAmbientLightUpdate(float lux, uint8_t computedBrightness);

MqttLightModule lightModule(lightState, onLightCommand);
MqttSwitchModule autoBrightnessSwitch("auto_brightness", "Auto-Brightness",
                                      true, onAutoBrightnessChange);
MqttAmbientLightSensor ambientLightSensor(MQTT_AMBIENT_LIGHT_TOPIC,
                                         onAmbientLightUpdate);

uint8_t resolveEffectiveBrightness() {
  if (autoBrightnessSwitch.getState() && ambientLightSensor.hasValue()) {
    return ambientLightSensor.getComputedBrightness();
  }
  return lightState.brightness;
}

void renderDisplay() {
  if (!lightState.on || !display.isValid()) {
    leds.clear();
    leds.show();
    return;
  }

  leds.setBrightness(resolveEffectiveBrightness());
  // Recreate the pixels after setBrightness; rescaling stored pixels loses data.
  display.printString("1234",
                      leds.Color(lightState.red, lightState.green,
                                 lightState.blue));
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  if (brightnessSpecified) {
    autoBrightnessSwitch.setState(false);
  }
  renderDisplay();
}

void onAutoBrightnessChange(bool enabled) {
  (void)enabled;
  renderDisplay();
  lightModule.publishState();
}

void onAmbientLightUpdate(float lux, uint8_t computedBrightness) {
  (void)lux;
  (void)computedBrightness;
  if (autoBrightnessSwitch.getState()) {
    renderDisplay();
    lightModule.publishState();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  lightState.on = false;
  leds.begin();
  leds.setBrightness(resolveEffectiveBrightness());
  leds.clear();
  leds.show();
  if (!display.isValid()) {
    Serial.println("Invalid display layout; output will remain blank");
  }

  lightModule.setBrightnessProvider(resolveEffectiveBrightness);
  IMqttModule* modules[] = {
      &lightModule, &autoBrightnessSwitch, &ambientLightSensor};
  for (IMqttModule* module : modules) {
    if (!mqttManager.registerModule(module)) {
      Serial.println("ERROR: MQTT module registration failed; MQTT will not start");
      return;
    }
  }
  mqttModulesRegistered = true;

  Serial.println("Ambient brightness: turn the MQTT light ON to show 1234");
  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD, 0);
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
}

void loop() {
  if (!mqttModulesRegistered) {
    return;
  }
  wifiManager.loop();
  mqttManager.loop();
}
