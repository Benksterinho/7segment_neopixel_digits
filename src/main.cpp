#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <LightState.h>
#include <Modular7Segment.h>
#include <WiFi.h>

#include "mqtt_light.h"
#include "wifi_config.h"

constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "DDD";
constexpr uint8_t countDigitModules(const char* layout) {
  return *layout == '\0'
             ? 0
             : (*layout == 'D' ? 1 : 0) + countDigitModules(layout + 1);
}
constexpr uint8_t DISPLAY_DIGIT_COUNT = countDigitModules(LAYOUT);
static_assert(DISPLAY_DIGIT_COUNT > 0, "Layout must contain a digit");
// Fallbacks only: every module in LAYOUT has an inline pixel count, but the
// library requires non-zero fallback values.
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
  LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");
constexpr uint32_t COUNT_INTERVAL_MS = 1000;

constexpr char OTA_HOSTNAME[] = "neopixel-clock";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 30000;

uint32_t lastWifiReconnectAttempt = 0;

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

constexpr uint16_t COUNTER_MAX = 999;

uint16_t counter = 0;
uint32_t lastCountChange = 0;

// Defaults (on, brightness 50 ~30% of 255, red) apply until NVS has a state.
LightState lightState;
LightStore lightStore;
MqttLight mqttLight;

void setupOta() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("WiFi connecting");
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connect failed, OTA disabled (clock still runs)");
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() { Serial.println("OTA start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA end"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error %u\n", error);
  });
  ArduinoOTA.begin();

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  const uint32_t now = millis();
  if (now - lastWifiReconnectAttempt < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }
  lastWifiReconnectAttempt = now;
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void render() {
  if (!lightState.on || !display.isValid()) {
    leds.clear();
    leds.show();
    return;
  }

  char text[DISPLAY_DIGIT_COUNT + 1] = {};
  uint16_t value = counter;
  for (uint8_t index = DISPLAY_DIGIT_COUNT; index > 0; --index) {
    text[index - 1] = static_cast<char>('0' + value % 10);
    value /= 10;
  }
  display.printString(
      text, leds.Color(lightState.red, lightState.green, lightState.blue));
  leds.show();
}

void onLightCommand(const char* payload, size_t length) {
  const LightCommandResult result =
      applyLightCommand(lightState, payload, length);
  if (result == LightCommandResult::Invalid) {
    Serial.println("MQTT: ignoring invalid light command");
  } else if (result == LightCommandResult::Changed) {
    // Adafruit NeoPixel scales stored pixel data on brightness changes, so
    // always redraw from the source color afterwards.
    leds.setBrightness(lightState.brightness);
    render();
    lightStore.markDirty(millis());
  }
  // Always confirm so Home Assistant's optimistic UI matches the device.
  mqttLight.publishState();
}

void setup() {
  Serial.begin(115200);

  lightStore.load(lightState);

  // Initialize and clear the strip before anything else so the powered LEDs
  // never sit uninitialized (which can show up as power-up all-white).
  leds.begin();
  leds.setBrightness(lightState.brightness);
  leds.clear();
  leds.show();
  delay(100);

  Serial.printf("NUM_LEDS: %u\n", NUM_LEDS);
  Serial.printf("Display required LEDs: %u\n", display.requiredLeds());
  Serial.printf("Display modules: %u\n", display.moduleCount());
  Serial.printf("Display valid: %s\n", display.isValid() ? "yes" : "no");

  render();  // Show zero (or stay dark if off) while Wi-Fi connects below.

  // Connect Wi-Fi/OTA last: the up-to-15 s blocking wait no longer leaves
  // the strip without initialization.
  setupOta();
  mqttLight.begin(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, lightState,
                  onLightCommand);
}

void loop() {
  ArduinoOTA.handle();
  maintainWifi();
  mqttLight.loop();

  const uint32_t now = millis();
  lightStore.loop(now, lightState);

  if (!display.isValid()) {
    return;
  }

  if (now - lastCountChange < COUNT_INTERVAL_MS) {
    return;
  }

  lastCountChange = now;
  counter = (counter + 1) % (COUNTER_MAX + 1);
  if (lightState.on) {
    render();
  }
}