#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>

#include "config.h"
#include "mqtt/mqtt_light_module.h"
#include "mqtt/mqtt_manager.h"
#include "net/wifi_manager.h"

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

constexpr char DISPLAY_TEXT[] = "1234";

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
LightState lightState;
WifiManager wifiManager;
MqttManager mqttManager;

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
  display.printString(DISPLAY_TEXT, color);
  leds.show();
}

void onLightCommand(const LightState& newState, bool brightnessSpecified) {
  (void)newState;
  (void)brightnessSpecified;
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

  if (!mqttManager.registerModule(&lightModule)) {
    Serial.println("Failed to register the MQTT light module");
  }

  renderDisplay();
  wifiManager.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
}

void loop() {
  wifiManager.loop();
  mqttManager.loop();
}
