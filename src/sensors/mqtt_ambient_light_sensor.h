#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt/mqtt_module.h"

// Subscribes to the ambient light sensor state topic,
// parses lux values, and computes display brightness via logarithmic curve.
class MqttAmbientLightSensor : public IMqttModule {
 public:
  using UpdateCallback = void (*)(float lux, uint8_t computedBrightness);

  explicit MqttAmbientLightSensor(const char* topic,
                                  UpdateCallback onUpdate = nullptr);

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  bool hasValue() const { return hasValue_; }
  float getLux() const { return lux_; }
  uint8_t getComputedBrightness() const { return computedBrightness_; }
  uint32_t lastUpdateMs() const { return lastUpdateMs_; }

  static bool parsePayload(const char* payload, size_t length, float& outLux);

  // Maps ambient lux to brightness in range [minBrightness, maxBrightness]
  // using logarithmic perception scaling.
  static uint8_t luxToBrightness(float lux, uint8_t minBrightness = 15,
                                 uint8_t maxBrightness = 255);

 private:
  const char* topic_ = nullptr;
  UpdateCallback onUpdate_ = nullptr;
  float lux_ = 0.0f;
  uint8_t computedBrightness_ = 50;
  bool hasValue_ = false;
  uint32_t lastUpdateMs_ = 0;
};
