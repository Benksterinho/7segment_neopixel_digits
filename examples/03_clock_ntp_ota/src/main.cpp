#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#include "config.h"
#include "net/ota_manager.h"
#include "net/time_sync.h"
#include "net/wifi_manager.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

constexpr char LAYOUT[] = "DD:DD";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT,
                        100);
WifiManager wifi;
TimeSync clockTime;
OtaManager ota;

bool servicesStarted = false;
bool wasConnected = false;
bool wasTimeSet = false;
uint32_t lastRenderMs = 0;

void render() {
  if (!display.isValid()) {
    return;
  }
  char text[6] = "----";
  const char* format = (millis() / 1000) % 2 == 0 ? "%H:%M" : "%H%M";
  if (!clockTime.getFormattedTime(text, sizeof(text), format)) {
    snprintf(text, sizeof(text), "----");
  }
  display.printString(text, leds.Color(0, 180, 255));
  leds.show();
}

void serviceNetwork() {
  const bool connected = wifi.isConnected();
  if (connected != wasConnected) {
    Serial.println(connected ? "Clock: Wi-Fi available" : "Clock: Wi-Fi lost");
    wasConnected = connected;
  }
  if (connected && !servicesStarted) {
    // Configure before loop(): its lazy OTA initialization lacks our hostname.
    ota.begin(OTA_HOSTNAME);
    clockTime.begin(NTP_TIMEZONE, NTP_SERVER);
    servicesStarted = true;
  }
  if (servicesStarted) {
    ota.loop();
  }
}

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(40);
  leds.clear();
  leds.show();
  if (!display.isValid()) {
    Serial.println("Clock: invalid display layout or LED buffer; staying blank");
  }
  render();
  wifi.begin(WIFI_SSID, WIFI_PASSWORD);
  serviceNetwork();
}

void loop() {
  wifi.loop();
  serviceNetwork();

  const bool timeSet = clockTime.isTimeSet();
  if (timeSet != wasTimeSet) {
    Serial.println(timeSet ? "Clock: time synchronized" : "Clock: time not set");
    wasTimeSet = timeSet;
  }

  const uint32_t now = millis();
  if (now - lastRenderMs >= 100) {
    lastRenderMs = now;
    render();
  }
}
