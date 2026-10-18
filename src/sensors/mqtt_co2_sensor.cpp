#include "mqtt_co2_sensor.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {

bool isDecimalNumber(const char* payload, size_t length) {
  const char* current = payload;
  const char* const end = payload + length;

  while (current < end && isspace(static_cast<unsigned char>(*current))) {
    ++current;
  }
  if (current < end && (*current == '+' || *current == '-')) {
    ++current;
  }

  bool hasDigits = false;
  while (current < end && *current >= '0' && *current <= '9') {
    hasDigits = true;
    ++current;
  }
  if (current < end && *current == '.') {
    ++current;
    while (current < end && *current >= '0' && *current <= '9') {
      hasDigits = true;
      ++current;
    }
  }
  if (!hasDigits) {
    return false;
  }

  if (current < end && (*current == 'e' || *current == 'E')) {
    ++current;
    if (current < end && (*current == '+' || *current == '-')) {
      ++current;
    }
    const char* exponentStart = current;
    while (current < end && *current >= '0' && *current <= '9') {
      ++current;
    }
    if (current == exponentStart) {
      return false;
    }
  }

  while (current < end && isspace(static_cast<unsigned char>(*current))) {
    ++current;
  }
  return current == end;
}

}  // namespace

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
  if (topic_ == nullptr || topic == nullptr || strcmp(topic, topic_) != 0) {
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
  constexpr size_t MAX_PAYLOAD_LENGTH = 31;
  if (payload == nullptr || length == 0 || length > MAX_PAYLOAD_LENGTH ||
      memchr(payload, '\0', length) != nullptr ||
      !isDecimalNumber(payload, length)) {
    return false;
  }

  char buffer[MAX_PAYLOAD_LENGTH + 1] = {};
  memcpy(buffer, payload, length);

  errno = 0;
  char* end = nullptr;
  const float value = strtof(buffer, &end);
  while (isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  constexpr float MAX_ROUNDED_PPM = static_cast<float>(UINT16_MAX) + 0.5f;
  if (end == buffer || *end != '\0' || errno == ERANGE || !isfinite(value) ||
      value < 0.0f || value >= MAX_ROUNDED_PPM) {
    return false;
  }

  outPpm = static_cast<uint16_t>(value + 0.5f);
  return true;
}
