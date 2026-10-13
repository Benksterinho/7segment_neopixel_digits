#include "mqtt_co2_sensor.h"
#include <stdlib.h>
#include <string.h>

MqttCo2Sensor::MqttCo2Sensor(const char* topic, UpdateCallback onUpdate)
    : topic_(topic), onUpdate_(onUpdate) {}

void MqttCo2Sensor::onMqttConnected(PubSubClient& client, const char* deviceId) {
  (void)deviceId;
  if (topic_ != nullptr && topic_[0] != '\0') {
    Serial.printf("MqttCo2Sensor: subscribing to %s\n", topic_);
    client.subscribe(topic_);
  }
}

bool MqttCo2Sensor::handleMessage(const char* topic, const uint8_t* payload,
                                  unsigned int length) {
  if (topic_ == nullptr || strcmp(topic, topic_) != 0) {
    return false;
  }

  uint16_t val = 0;
  if (parsePayload(reinterpret_cast<const char*>(payload), length, val)) {
    ppm_ = val;
    hasValue_ = true;
    lastUpdateMs_ = millis();
    Serial.printf("MqttCo2Sensor: received CO2 = %u ppm\n", ppm_);
    if (onUpdate_ != nullptr) {
      onUpdate_(ppm_);
    }
  } else {
    Serial.println("MqttCo2Sensor: failed to parse payload");
  }
  return true;
}

bool MqttCo2Sensor::parsePayload(const char* payload, size_t length,
                                 uint16_t& outPpm) {
  if (payload == nullptr || length == 0) {
    return false;
  }

  char buf[32] = {};
  const size_t copyLen = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
  memcpy(buf, payload, copyLen);
  buf[copyLen] = '\0';

  char* end = nullptr;
  const float val = strtof(buf, &end);
  if (end == buf || val < 0.0f) {
    return false;
  }

  outPpm = static_cast<uint16_t>(val + 0.5f);
  return true;
}
