#include "mqtt_temperature_sensor.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

MqttTemperatureSensor::MqttTemperatureSensor(const char* topic,
                                             UpdateCallback onUpdate)
    : topic_(topic), onUpdate_(onUpdate) {}

void MqttTemperatureSensor::onMqttConnected(PubSubClient& client,
                                            const char* deviceId) {
  (void)deviceId;
  if (topic_ != nullptr && topic_[0] != '\0') {
    Serial.printf("MqttTemperatureSensor: subscribing to %s\n", topic_);
    if (!client.subscribe(topic_)) {
      Serial.println("MqttTemperatureSensor: subscription failed");
    }
  }
}

bool MqttTemperatureSensor::handleMessage(const char* topic,
                                           const uint8_t* payload,
                                           unsigned int length) {
  if (topic_ == nullptr || topic == nullptr || strcmp(topic, topic_) != 0) {
    return false;
  }

  float value = 0.0f;
  if (!parsePayload(reinterpret_cast<const char*>(payload), length, value)) {
    Serial.println("MqttTemperatureSensor: failed to parse payload");
    return true;
  }

  celsius_ = value;
  hasValue_ = true;
  lastUpdateMs_ = millis();
  Serial.printf("MqttTemperatureSensor: received temperature = %.2f C\n",
                celsius_);
  if (onUpdate_ != nullptr) {
    onUpdate_(celsius_);
  }
  return true;
}

bool MqttTemperatureSensor::parsePayload(const char* payload, size_t length,
                                          float& outCelsius) {
  char buffer[32] = {};
  if (payload == nullptr || length == 0 || length >= sizeof(buffer) ||
      memchr(payload, '\0', length) != nullptr) {
    return false;
  }
  memcpy(buffer, payload, length);

  errno = 0;
  char* end = nullptr;
  const float value = strtof(buffer, &end);
  if (end == buffer || errno == ERANGE || !isfinite(value)) {
    return false;
  }
  while (isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  if (*end != '\0') {
    return false;
  }

  outCelsius = value;
  return true;
}
