#include "mqtt_light.h"

#include <ArduinoJson.h>
#include <string.h>

namespace {

constexpr char TOPIC_PREFIX[] = "neopixel-clock";
constexpr char HA_STATUS_TOPIC[] = "homeassistant/status";
constexpr char PAYLOAD_ONLINE[] = "online";
constexpr char PAYLOAD_OFFLINE[] = "offline";
constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;
constexpr uint16_t MQTT_BUFFER_SIZE = 1024;
// Bounds the blocking TCP connect so an unreachable broker cannot freeze the
// display for long. Arduino-ESP32's WiFiClient::setTimeout takes seconds.
constexpr uint32_t TCP_CONNECT_TIMEOUT_S = 2;

MqttLight* instance = nullptr;

}  // namespace

MqttLight::MqttLight() : client_(wifiClient_) { instance = this; }

void MqttLight::begin(const char* host, uint16_t port, const char* user,
                      const char* password, const LightState& state,
                      CommandHandler onCommand) {
  host_ = host;
  port_ = port;
  user_ = user;
  password_ = password;
  state_ = &state;
  onCommand_ = onCommand;

  uint8_t mac[6] = {};
  WiFi.macAddress(mac);
  snprintf(deviceId_, sizeof(deviceId_), "%s-%02x%02x%02x", TOPIC_PREFIX,
           mac[3], mac[4], mac[5]);
  snprintf(availabilityTopic_, sizeof(availabilityTopic_), "%s/%s/status",
           TOPIC_PREFIX, deviceId_);
  snprintf(stateTopic_, sizeof(stateTopic_), "%s/%s/state", TOPIC_PREFIX,
           deviceId_);
  snprintf(commandTopic_, sizeof(commandTopic_), "%s/%s/set", TOPIC_PREFIX,
           deviceId_);
  snprintf(discoveryTopic_, sizeof(discoveryTopic_),
           "homeassistant/light/%s/config", deviceId_);

  wifiClient_.setTimeout(TCP_CONNECT_TIMEOUT_S);
  client_.setServer(host_, port_);
  client_.setBufferSize(MQTT_BUFFER_SIZE);
  client_.setCallback(&MqttLight::onMessage);

  Serial.printf("MQTT device id: %s\n", deviceId_);
}

void MqttLight::loop() {
  if (host_ == nullptr || WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (client_.connected()) {
    client_.loop();
    return;
  }

  const uint32_t now = millis();
  if (attemptedConnect_ && now - lastConnectAttemptMs_ < RECONNECT_INTERVAL_MS) {
    return;
  }
  attemptedConnect_ = true;
  lastConnectAttemptMs_ = now;
  connect();
}

bool MqttLight::connect() {
  Serial.printf("MQTT connecting to %s:%u\n", host_, port_);
  const bool hasCredentials = user_ != nullptr && user_[0] != '\0';
  const bool connected = client_.connect(
      deviceId_, hasCredentials ? user_ : nullptr,
      hasCredentials ? password_ : nullptr, availabilityTopic_, 0, true,
      PAYLOAD_OFFLINE);
  if (!connected) {
    Serial.printf("MQTT connect failed, rc=%d\n", client_.state());
    return false;
  }

  Serial.println("MQTT connected");
  client_.subscribe(commandTopic_);
  client_.subscribe(HA_STATUS_TOPIC);
  publishDiscovery();
  client_.publish(availabilityTopic_, PAYLOAD_ONLINE, true);
  publishState();
  return true;
}

void MqttLight::publishDiscovery() {
  JsonDocument doc;
  doc["name"] = nullptr;  // Entity takes the device name.
  doc["unique_id"] = deviceId_;
  doc["schema"] = "json";
  doc["command_topic"] = commandTopic_;
  doc["state_topic"] = stateTopic_;
  doc["availability_topic"] = availabilityTopic_;
  doc["payload_available"] = PAYLOAD_ONLINE;
  doc["payload_not_available"] = PAYLOAD_OFFLINE;
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
    Serial.println("MQTT discovery payload too large");
    return;
  }
  serializeJson(doc, payload, sizeof(payload));
  client_.publish(discoveryTopic_, payload, true);
}

void MqttLight::publishState() {
  if (state_ == nullptr || !client_.connected()) {
    return;
  }
  char payload[160];
  const size_t length = serializeLightState(*state_, payload, sizeof(payload));
  if (length == 0) {
    return;
  }
  client_.publish(stateTopic_, payload, true);
}

void MqttLight::onMessage(char* topic, uint8_t* payload, unsigned int length) {
  if (instance != nullptr) {
    instance->handleMessage(topic, payload, length);
  }
}

void MqttLight::handleMessage(const char* topic, const uint8_t* payload,
                              unsigned int length) {
  if (strcmp(topic, HA_STATUS_TOPIC) == 0) {
    // Home Assistant restarted: announce the entity and current state again.
    if (length == strlen(PAYLOAD_ONLINE) &&
        memcmp(payload, PAYLOAD_ONLINE, length) == 0) {
      publishDiscovery();
      publishState();
    }
    return;
  }
  if (strcmp(topic, commandTopic_) == 0 && onCommand_ != nullptr) {
    onCommand_(reinterpret_cast<const char*>(payload), length);
  }
}
