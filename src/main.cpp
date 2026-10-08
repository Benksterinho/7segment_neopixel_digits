#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <Modular7Segment.h>
#include <WiFi.h>

#include "wifi_config.h"

constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "D";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
  LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");
constexpr uint32_t GLYPH_INTERVAL_MS = 1000;
constexpr uint8_t BRIGHTNESS = 180;

constexpr char OTA_HOSTNAME[] = "neopixel-clock";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 30000;

uint32_t lastWifiReconnectAttempt = 0;

CRGB leds[NUM_LEDS];
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

constexpr char GLYPHS[] = "0123456789ABCDEF ";
constexpr uint8_t GLYPH_COUNT = sizeof(GLYPHS) - 1;

uint8_t currentGlyph = 0;
uint32_t lastGlyphChange = 0;

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

void setup() {
  Serial.begin(115200);

  setupOta();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  if (!display.isValid()) {
    return;
  }

  display.setModule(0, GLYPHS[currentGlyph], CRGB::Red);
  FastLED.show();
}

void loop() {
  ArduinoOTA.handle();
  maintainWifi();

  if (!display.isValid()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastGlyphChange < GLYPH_INTERVAL_MS) {
    return;
  }

  lastGlyphChange = now;
  currentGlyph = (currentGlyph + 1) % GLYPH_COUNT;
  display.setModule(0, GLYPHS[currentGlyph], CRGB::Red);
  FastLED.show();
}