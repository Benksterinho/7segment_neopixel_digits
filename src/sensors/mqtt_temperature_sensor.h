#pragma once

#include <Arduino.h>
#include <stddef.h>

#include "mqtt/mqtt_module.h"

class MqttTemperatureSensor : public IMqttModule {
 public:
  using UpdateCallback = void (*)(float celsius);

  explicit MqttTemperatureSensor(const char* topic,
                                 UpdateCallback onUpdate = nullptr);

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  bool hasValue() const { return hasValue_; }
  float getCelsius() const { return celsius_; }
  uint32_t lastUpdateMs() const { return lastUpdateMs_; }

  static bool parsePayload(const char* payload, size_t length, float& outCelsius);

 private:
  const char* topic_;
  UpdateCallback onUpdate_;
  float celsius_ = 0.0f;
  bool hasValue_ = false;
  uint32_t lastUpdateMs_ = 0;
};
