#include "mqtt_ambient_light_sensor.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

MqttAmbientLightSensor::MqttAmbientLightSensor(const char* topic,
                                               UpdateCallback onUpdate)
    : topic_(topic), onUpdate_(onUpdate) {}

void MqttAmbientLightSensor::onMqttConnected(PubSubClient& client,
                                             const char* deviceId) {
  (void)deviceId;
  if (topic_ != nullptr && topic_[0] != '\0') {
    Serial.printf("MqttAmbientLightSensor: subscribing to %s\n", topic_);
    client.subscribe(topic_);
  }
}

bool MqttAmbientLightSensor::handleMessage(const char* topic,
                                          const uint8_t* payload,
                                          unsigned int length) {
  if (topic_ == nullptr || strcmp(topic, topic_) != 0) {
    return false;
  }

  float val = 0.0f;
  if (parsePayload(reinterpret_cast<const char*>(payload), length, val)) {
    lux_ = val;
    computedBrightness_ = luxToBrightness(lux_);
    hasValue_ = true;
    lastUpdateMs_ = millis();
    Serial.printf(
        "MqttAmbientLightSensor: received lux = %.1f -> computed brightness "
        "= %u\n",
        lux_, computedBrightness_);
    if (onUpdate_ != nullptr) {
      onUpdate_(lux_, computedBrightness_);
    }
  } else {
    Serial.println("MqttAmbientLightSensor: failed to parse payload");
  }
  return true;
}

bool MqttAmbientLightSensor::parsePayload(const char* payload, size_t length,
                                         float& outLux) {
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

  outLux = val;
  return true;
}

uint8_t MqttAmbientLightSensor::luxToBrightness(float lux,
                                               uint8_t minBrightness,
                                               uint8_t maxBrightness) {
  // Indoor light levels typically range from ~0-1 lux (pitch dark / night)
  // to ~500 lux (bright indoor lighting) or >1000 lux (near window / sunlight).
  //
  // Logarithmic human eye response formula:
  // normalized = log10(1 + lux) / log10(1 + maxLuxRef)
  constexpr float MAX_LUX_REF = 500.0f;
  if (lux <= 0.0f) {
    return minBrightness;
  }

  float clampedLux = lux > MAX_LUX_REF ? MAX_LUX_REF : lux;
  float factor = log10f(1.0f + clampedLux) / log10f(1.0f + MAX_LUX_REF);

  float brightness =
      minBrightness + factor * (maxBrightness - minBrightness);

  if (brightness < minBrightness) {
    return minBrightness;
  }
  if (brightness > maxBrightness) {
    return maxBrightness;
  }
  return static_cast<uint8_t>(brightness + 0.5f);
}
