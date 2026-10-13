#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt/mqtt_module.h"

// Subscribes to the SCD41 CO2 sensor state topic,
// parses ppm values, tracks freshness, and maps to air-quality indicator colors.
class MqttCo2Sensor : public IMqttModule {
 public:
  using UpdateCallback = void (*)(uint16_t ppm);

  explicit MqttCo2Sensor(const char* topic, UpdateCallback onUpdate = nullptr);

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  bool hasValue() const { return hasValue_; }
  uint16_t getPpm() const { return ppm_; }
  uint32_t lastUpdateMs() const { return lastUpdateMs_; }

  // Parses CO2 payload (integer or floating point string, e.g. "650" or "650.0")
  static bool parsePayload(const char* payload, size_t length, uint16_t& outPpm);

  // Evaluates air-quality color:
  // Green (< 800), Yellow (800..1199), Red (>= 1200)
  static void getColorRgb(uint16_t ppm, uint8_t& r, uint8_t& g, uint8_t& b);

 private:
  const char* topic_ = nullptr;
  UpdateCallback onUpdate_ = nullptr;
  uint16_t ppm_ = 0;
  bool hasValue_ = false;
  uint32_t lastUpdateMs_ = 0;
};
