#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "config.h"
#include "display/temperature_format.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_temperature_sensor.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "DD.DD";
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
void onTemperatureUpdate(float celsius);
MqttTemperatureSensor temperatureSensor(MQTT_TEMPERATURE_TOPIC,
                                        onTemperatureUpdate);
bool mqttModulesRegistered = false;

void renderDisplay() {
  if (!display.isValid()) {
    Serial.println("Display layout is invalid; keeping the LEDs off");
    leds.clear();
    leds.show();
    return;
  }

  leds.setBrightness(lightState.brightness);
  if (!lightState.on) {
    leds.clear();
    leds.show();
    return;
  }

  const uint32_t color =
      leds.Color(lightState.red, lightState.green, lightState.blue);
  if (!temperatureSensor.hasValue()) {
    display.printString("----", color);
    leds.show();
    return;
  }

  char formatted[6] = {};
  formatTemperature(temperatureSensor.getCelsius(), formatted);

  // Move the formatter's two-character integer field to the two digits before
  // this layout's decimal point; leave its fourth digit blank.
  char displayText[6] = {};
  if (formatted[4] == '\0') {
    snprintf(displayText, sizeof(displayText), "%s", formatted);
  } else {
    displayText[0] = formatted[1];
    displayText[1] = formatted[2];
    displayText[2] = '.';
    displayText[3] = formatted[4];
    displayText[4] = ' ';
  }

  display.printString(displayText, color);
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  (void)brightnessSpecified;
  renderDisplay();
}

void onTemperatureUpdate(float celsius) {
  (void)celsius;
  renderDisplay();
}

MqttLightModule lightModule(lightState, onLightCommand);

}  // namespace

void setup() {
  Serial.begin(115200);

  leds.begin();
  leds.setBrightness(lightState.brightness);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout; output will remain blank");
  }

  Serial.println("Waiting for first valid temperature reading");

  const bool lightRegistered = mqttManager.registerModule(&lightModule);
  const bool sensorRegistered = mqttManager.registerModule(&temperatureSensor);
  if (!lightRegistered || !sensorRegistered) {
    if (!lightRegistered) {
      Serial.println("ERROR: MQTT light module registration failed; MQTT will not start");
    }
    if (!sensorRegistered) {
      Serial.println("ERROR: MQTT temperature sensor registration failed; MQTT will not start");
    }
    return;
  }
  mqttModulesRegistered = true;

  renderDisplay();
  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
}

void loop() {
  if (!mqttModulesRegistered) {
    return;
  }
  wifiManager.loop();
  mqttManager.loop();
}
