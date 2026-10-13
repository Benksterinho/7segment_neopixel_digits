#include "mqtt_manager.h"
#include <string.h>

namespace {

constexpr char TOPIC_PREFIX[] = "neopixel-clock";
constexpr char HA_STATUS_TOPIC[] = "homeassistant/status";
constexpr char PAYLOAD_ONLINE[] = "online";
constexpr char PAYLOAD_OFFLINE[] = "offline";

MqttManager* instance = nullptr;

}  // namespace

MqttManager::MqttManager() : client_(wifiClient_) { instance = this; }

void MqttManager::begin(const char* host, uint16_t port, const char* user,
                        const char* password) {
  host_ = host;
  port_ = port;
  user_ = user;
  password_ = password;

  uint8_t mac[6] = {};
  WiFi.macAddress(mac);
  snprintf(deviceId_, sizeof(deviceId_), "%s-%02x%02x%02x", TOPIC_PREFIX,
           mac[3], mac[4], mac[5]);
  snprintf(availabilityTopic_, sizeof(availabilityTopic_), "%s/%s/status",
           TOPIC_PREFIX, deviceId_);

  wifiClient_.setTimeout(TCP_CONNECT_TIMEOUT_S);
  client_.setServer(host_, port_);
  client_.setBufferSize(MQTT_BUFFER_SIZE);
  client_.setCallback(&MqttManager::onMessage);

  Serial.printf("MqttManager device id: %s\n", deviceId_);
}

bool MqttManager::registerModule(IMqttModule* module) {
  if (module == nullptr || moduleCount_ >= MAX_MODULES) {
    return false;
  }
  modules_[moduleCount_++] = module;
  // If already connected, notify the module immediately
  if (client_.connected()) {
    module->onMqttConnected(client_, deviceId_);
  }
  return true;
}

void MqttManager::loop() {
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

bool MqttManager::isConnected() {
  return client_.connected();
}

bool MqttManager::connect() {
  Serial.printf("MqttManager: connecting to %s:%u\n", host_, port_);
  const bool hasCredentials = user_ != nullptr && user_[0] != '\0';
  const bool connected = client_.connect(
      deviceId_, hasCredentials ? user_ : nullptr,
      hasCredentials ? password_ : nullptr, availabilityTopic_, 0, true,
      PAYLOAD_OFFLINE);
  if (!connected) {
    Serial.printf("MqttManager: connect failed, rc=%d\n", client_.state());
    return false;
  }

  Serial.println("MqttManager: connected");
  client_.subscribe(HA_STATUS_TOPIC);
  client_.publish(availabilityTopic_, PAYLOAD_ONLINE, true);

  for (uint8_t i = 0; i < moduleCount_; ++i) {
    if (modules_[i] != nullptr) {
      modules_[i]->onMqttConnected(client_, deviceId_);
    }
  }

  return true;
}

void MqttManager::onMessage(char* topic, uint8_t* payload, unsigned int length) {
  if (instance != nullptr) {
    instance->handleMessage(topic, payload, length);
  }
}

void MqttManager::handleMessage(const char* topic, const uint8_t* payload,
                               unsigned int length) {
  // If Home Assistant restarts, re-announce modules
  if (strcmp(topic, HA_STATUS_TOPIC) == 0) {
    if (length == strlen(PAYLOAD_ONLINE) &&
        memcmp(payload, PAYLOAD_ONLINE, length) == 0) {
      Serial.println("MqttManager: Home Assistant online, redispatching onMqttConnected");
      client_.publish(availabilityTopic_, PAYLOAD_ONLINE, true);
      for (uint8_t i = 0; i < moduleCount_; ++i) {
        if (modules_[i] != nullptr) {
          modules_[i]->onMqttConnected(client_, deviceId_);
        }
      }
    }
    return;
  }

  for (uint8_t i = 0; i < moduleCount_; ++i) {
    if (modules_[i] != nullptr) {
      if (modules_[i]->handleMessage(topic, payload, length)) {
        break;
      }
    }
  }
}
