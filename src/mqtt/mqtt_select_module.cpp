#include "mqtt_select_module.h"
#include <string.h>

namespace {
constexpr char TOPIC_PREFIX[] = "neopixel-clock";
constexpr char PREFS_NAMESPACE[] = "select";
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;
constexpr size_t MAX_OPTION_LENGTH = 32;
}  // namespace

MqttSelectModule::MqttSelectModule(const char* entitySuffix,
                                   const char* entityName,
                                   const char* const* options,
                                   uint8_t optionCount, uint8_t defaultIndex,
                                   SelectHandler onSelect)
    : entitySuffix_(entitySuffix),
      entityName_(entityName),
      options_(options),
      optionCount_(optionCount),
      index_(defaultIndex < optionCount ? defaultIndex : 0),
      onSelect_(onSelect) {
  load();
}

bool MqttSelectModule::parseOption(const char* payload, size_t length,
                                   const char* const* options,
                                   uint8_t optionCount, uint8_t& outIndex) {
  if (payload == nullptr || options == nullptr || length == 0 ||
      length > MAX_OPTION_LENGTH) {
    return false;
  }

  char buf[MAX_OPTION_LENGTH + 1] = {};
  memcpy(buf, payload, length);
  buf[length] = '\0';

  for (uint8_t i = 0; i < optionCount; ++i) {
    if (strcasecmp(buf, options[i]) == 0) {
      outIndex = i;
      return true;
    }
  }
  return false;
}

void MqttSelectModule::load() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, true)) {
    const uint8_t stored = prefs.getUChar(entitySuffix_, index_);
    if (stored < optionCount_) {
      index_ = stored;
    }
    prefs.end();
  }
}

void MqttSelectModule::save() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, false)) {
    prefs.putUChar(entitySuffix_, index_);
    prefs.end();
  }
}

void MqttSelectModule::setIndex(uint8_t index) {
  if (index >= optionCount_) {
    return;
  }
  if (index != index_) {
    index_ = index;
    save();
  }
  publishState();
}

void MqttSelectModule::onMqttConnected(PubSubClient& client,
                                       const char* deviceId) {
  client_ = &client;
  strncpy(deviceId_, deviceId, sizeof(deviceId_) - 1);

  snprintf(uniqueId_, sizeof(uniqueId_), "%s_%s", deviceId_, entitySuffix_);
  snprintf(availabilityTopic_, sizeof(availabilityTopic_), "%s/%s/status",
           TOPIC_PREFIX, deviceId_);
  snprintf(stateTopic_, sizeof(stateTopic_), "%s/%s/%s/state", TOPIC_PREFIX,
           deviceId_, entitySuffix_);
  snprintf(commandTopic_, sizeof(commandTopic_), "%s/%s/%s/set", TOPIC_PREFIX,
           deviceId_, entitySuffix_);
  snprintf(discoveryTopic_, sizeof(discoveryTopic_),
           "homeassistant/select/%s/%s/config", deviceId_, entitySuffix_);

  client_->subscribe(commandTopic_);
  publishDiscovery();
  publishState();
}

bool MqttSelectModule::handleMessage(const char* topic, const uint8_t* payload,
                                     unsigned int length) {
  if (strcmp(topic, commandTopic_) != 0) {
    return false;
  }

  uint8_t index = 0;
  if (!parseOption(reinterpret_cast<const char*>(payload), length, options_,
                   optionCount_, index)) {
    Serial.println("MqttSelectModule: ignoring unknown option");
    publishState();
    return true;
  }

  setIndex(index);
  if (onSelect_ != nullptr) {
    onSelect_(index_);
  }
  return true;
}

void MqttSelectModule::publishDiscovery() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }

  JsonDocument doc;
  doc["name"] = entityName_;
  doc["unique_id"] = uniqueId_;
  doc["command_topic"] = commandTopic_;
  doc["state_topic"] = stateTopic_;
  doc["availability_topic"] = availabilityTopic_;
  JsonArray options = doc["options"].to<JsonArray>();
  for (uint8_t i = 0; i < optionCount_; ++i) {
    options.add(options_[i]);
  }

  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"].to<JsonArray>().add(deviceId_);
  device["name"] = "NeoPixel Clock";

  char payload[MQTT_BUFFER_SIZE - 128];
  if (measureJson(doc) + 1 > sizeof(payload)) {
    Serial.println("MqttSelectModule: discovery payload too large");
    return;
  }
  serializeJson(doc, payload, sizeof(payload));
  client_->publish(discoveryTopic_, payload, true);
}

void MqttSelectModule::publishState() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }
  client_->publish(stateTopic_, options_[index_], true);
}
