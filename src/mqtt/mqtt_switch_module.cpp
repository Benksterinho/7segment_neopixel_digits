#include "mqtt_switch_module.h"
#include <string.h>

namespace {
constexpr char TOPIC_PREFIX[] = "neopixel-clock";
constexpr char PREFS_NAMESPACE[] = "switch";
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;
}  // namespace

MqttSwitchModule::MqttSwitchModule(const char* entitySuffix,
                                   const char* entityName, bool defaultState,
                                   StateChangeHandler onChange)
    : entitySuffix_(entitySuffix),
      entityName_(entityName),
      state_(defaultState),
      onChange_(onChange) {
  load();
}

void MqttSwitchModule::load() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, true)) {
    state_ = prefs.getBool(entitySuffix_, state_);
    prefs.end();
  }
}

void MqttSwitchModule::save() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, false)) {
    prefs.putBool(entitySuffix_, state_);
    prefs.end();
  }
}

void MqttSwitchModule::setState(bool state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  save();
  publishState();
  if (onChange_ != nullptr) {
    onChange_(state_);
  }
}

void MqttSwitchModule::onMqttConnected(PubSubClient& client,
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
           "homeassistant/switch/%s/%s/config", deviceId_, entitySuffix_);

  client_->subscribe(commandTopic_);
  publishDiscovery();
  publishState();
}

bool MqttSwitchModule::handleMessage(const char* topic, const uint8_t* payload,
                                     unsigned int length) {
  if (strcmp(topic, commandTopic_) != 0) {
    return false;
  }

  char buf[8] = {};
  const size_t copyLen = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
  memcpy(buf, payload, copyLen);
  buf[copyLen] = '\0';

  if (strcasecmp(buf, "ON") == 0) {
    setState(true);
  } else if (strcasecmp(buf, "OFF") == 0) {
    setState(false);
  }
  return true;
}

void MqttSwitchModule::publishDiscovery() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }

  JsonDocument doc;
  doc["name"] = entityName_;
  doc["unique_id"] = uniqueId_;
  doc["command_topic"] = commandTopic_;
  doc["state_topic"] = stateTopic_;
  doc["availability_topic"] = availabilityTopic_;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["state_on"] = "ON";
  doc["state_off"] = "OFF";

  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"].to<JsonArray>().add(deviceId_);
  device["name"] = "NeoPixel Clock";

  char payload[MQTT_BUFFER_SIZE - 128];
  if (measureJson(doc) + 1 > sizeof(payload)) {
    Serial.println("MqttSwitchModule: discovery payload too large");
    return;
  }
  serializeJson(doc, payload, sizeof(payload));
  client_->publish(discoveryTopic_, payload, true);
}

void MqttSwitchModule::publishState() {
  if (client_ == nullptr || !client_->connected()) {
    return;
  }
  client_->publish(stateTopic_, state_ ? "ON" : "OFF", true);
}
