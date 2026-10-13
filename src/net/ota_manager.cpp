#include "ota_manager.h"
#include <WiFi.h>

void OtaManager::begin(const char* hostname) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA: WiFi not connected, skipping OTA begin");
    return;
  }

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() { Serial.println("OTA start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA end"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error %u\n", error);
  });
  ArduinoOTA.begin();
  initialized_ = true;
  Serial.printf("OTA initialized with hostname: %s\n", hostname);
}

void OtaManager::loop() {
  if (!initialized_) {
    if (WiFi.status() == WL_CONNECTED) {
      // Lazy init if Wi-Fi came up later
      ArduinoOTA.begin();
      initialized_ = true;
    }
    return;
  }
  ArduinoOTA.handle();
}
