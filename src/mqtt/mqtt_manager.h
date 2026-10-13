#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt_module.h"

// Manages the single shared MQTT connection, reconnection backoff,
// device availability status, and routes messages to registered IMqttModule instances.
class MqttManager {
 public:
  static constexpr uint8_t MAX_MODULES = 8;

  MqttManager();

  void begin(const char* host, uint16_t port, const char* user,
             const char* password);

  bool registerModule(IMqttModule* module);

  void loop();

  bool isConnected();
  const char* deviceId() const { return deviceId_; }
  PubSubClient& client() { return client_; }

 private:
  static void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length);
  bool connect();

  WiFiClient wifiClient_;
  PubSubClient client_;
  const char* host_ = nullptr;
  uint16_t port_ = 1883;
  const char* user_ = nullptr;
  const char* password_ = nullptr;

  IMqttModule* modules_[MAX_MODULES] = {};
  uint8_t moduleCount_ = 0;

  uint32_t lastConnectAttemptMs_ = 0;
  bool attemptedConnect_ = false;

  char deviceId_[24] = {};
  char availabilityTopic_[64] = {};

  static constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;
  static constexpr uint16_t MQTT_BUFFER_SIZE = 1024;
  static constexpr uint32_t TCP_CONNECT_TIMEOUT_S = 2;
};
