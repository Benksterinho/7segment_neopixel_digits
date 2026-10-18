#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "config.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "net/wifi_manager.h"
#include "sensors/mqtt_co2_sensor.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

#if CO2_AMBER_THRESHOLD_PPM >= CO2_RED_THRESHOLD_PPM
#error "CO2_AMBER_THRESHOLD_PPM must be lower than CO2_RED_THRESHOLD_PPM"
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
void onCo2Update(uint16_t ppm);
MqttCo2Sensor co2Sensor(MQTT_CO2_TOPIC, onCo2Update);
bool mqttModulesRegistered = false;

uint32_t colorForPpm(uint16_t ppm) {
  if (ppm < CO2_AMBER_THRESHOLD_PPM) {
    return leds.Color(0, 255, 0);
  }
  if (ppm < CO2_RED_THRESHOLD_PPM) {
    return leds.Color(255, 130, 0);
  }
  return leds.Color(255, 0, 0);
}

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

  if (!co2Sensor.hasValue()) {
    display.printString("----", leds.Color(lightState.red, lightState.green,
                                             lightState.blue));
    leds.show();
    return;
  }

  const uint16_t ppm = co2Sensor.getPpm();
  if (ppm > 9999) {
    display.printString("----", leds.Color(255, 0, 0));
    leds.show();
    Serial.printf("CO2 reading %u ppm exceeds the four-digit display range\n",
                  ppm);
    return;
  }

  char text[5] = {};
  snprintf(text, sizeof(text), "%4u", static_cast<unsigned>(ppm));
  display.printString(text, colorForPpm(ppm));
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  (void)brightnessSpecified;
  renderDisplay();
}

void onCo2Update(uint16_t ppm) {
  (void)ppm;
  renderDisplay();
}

MqttLightModule lightModule(lightState, onLightCommand);

}  // namespace

void setup() {
  Serial.begin(115200);

  // Sensor updates only redraw the current light state; they never turn it on.
  lightState.on = false;
  leds.begin();
  leds.setBrightness(lightState.brightness);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout; output will remain blank");
  }

  const bool lightRegistered = mqttManager.registerModule(&lightModule);
  const bool sensorRegistered = mqttManager.registerModule(&co2Sensor);
  if (!lightRegistered || !sensorRegistered) {
    if (!lightRegistered) {
      Serial.println("ERROR: MQTT light module registration failed; MQTT will not start");
    }
    if (!sensorRegistered) {
      Serial.println("ERROR: MQTT CO2 sensor registration failed; MQTT will not start");
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
