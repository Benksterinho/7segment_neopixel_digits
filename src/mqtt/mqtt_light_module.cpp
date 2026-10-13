#include "mqtt_light_module.h"
#include <string.h>

namespace {
constexpr char TOPIC_PREFIX[] = "neopixel-clock";
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;
}  // namespace

MqttLightModule::MqttLightModule(LightState& state, CommandHandler onCommand)
    : state_(state), onCommand_(onCommand) {}

void MqttLightModule::onMqttConnected(PubSubClient& client,
                                      const char* deviceId) {
  client_ = &client;
  strncpy(deviceId_, deviceId, sizeof(deviceId_) - 1);

  snprintf(availabilityTopic_, sizeof(availabilityTopic_), "%s/%s/status",
           TOPIC_PREFIX, deviceId_);
  snprintf(stateTopic_, sizeof(stateTopic_), "%s/%s/state", TOPIC_PREFIX,
           deviceId_);
  snprintf(commandTopic_, sizeof(commandTopic_), "%s/%s/set", TOPIC_PREFIX,
           deviceId_);
  snprintf(discoveryTopic_, sizeof(discoveryTopic_),
           "homeassistant/light/%s/config", deviceId_);

  client_->subscribe(commandTopic_);
  publishDiscovery();
  publishState();
}

bool MqttLightModule::handleMessage(const char* topic, const uint8_t* payload,
                                    unsigned int length) {
  if (strcmp(topic, commandTopic_) != 0) {
    return false;
  }

  bool brightnessSpecified = false;
  JsonDocument doc;
  if (!deserializeJson(doc, payload, length) && doc.is<JsonObjectConst>()) {
    brightnessSpecified = !doc["brightness"].isNull();
  }

  const LightCommandResult result =
      applyLightCommand(state_, reinterpret_cast<const char*>(payload), length);
  if (result == LightCommandResult::Invalid) {
    Serial.println("MqttLightModule: ignoring invalid light command");
  } else if (result == LightCommandResult::Changed || brightnessSpecified) {
    if (onCommand_ != nullptr) {
      onCommand_(state_, brightnessSpecified);
    }
  }

  publishState();
  return true;
}

void MqttLightModule::publishDiscovery() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }

  JsonDocument doc;
  doc["name"] = nullptr;  // Takes device name
  doc["unique_id"] = deviceId_;
  doc["schema"] = "json";
  doc["command_topic"] = commandTopic_;
  doc["state_topic"] = stateTopic_;
  doc["availability_topic"] = availabilityTopic_;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["brightness"] = true;
  doc["brightness_scale"] = 255;
  doc["supported_color_modes"].to<JsonArray>().add("rgb");

  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"].to<JsonArray>().add(deviceId_);
  device["name"] = "NeoPixel Clock";
  device["manufacturer"] = "DIY";
  device["model"] = "ESP32 7-segment display";

  char payload[MQTT_BUFFER_SIZE - 128];
  if (measureJson(doc) + 1 > sizeof(payload)) {
    Serial.println("MqttLightModule: discovery payload too large");
    return;
  }
  serializeJson(doc, payload, sizeof(payload));
  client_->publish(discoveryTopic_, payload, true);
}

void MqttLightModule::publishState() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }
  LightState reported = state_;
  if (brightnessProvider_ != nullptr) {
    reported.brightness = brightnessProvider_();
  }
  char payload[160];
  const size_t length = serializeLightState(reported, payload, sizeof(payload));
  if (length == 0) {
    return;
  }
  client_->publish(stateTopic_, payload, true);
}
