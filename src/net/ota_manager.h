#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

class OtaManager {
 public:
  OtaManager() = default;

  void begin(const char* hostname);
  void loop();

 private:
  bool initialized_ = false;
};
