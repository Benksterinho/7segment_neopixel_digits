#pragma once

#include <LightState.h>
#include <PubSubClient.h>
#include <WiFi.h>

// Exposes the display as a Home Assistant MQTT light (JSON schema) with
// on/off, brightness and RGB color, plus an online/offline availability topic.
class MqttLight {
 public:
  // Called with the raw payload of every command received on the set topic.
  using CommandHandler = void (*)(const char* payload, size_t length);

  MqttLight();

  // `state` must outlive this object; it is republished on every reconnect.
  void begin(const char* host, uint16_t port, const char* user,
             const char* password, const LightState& state,
             CommandHandler onCommand);
  void loop();
  void publishState();

  const char* deviceId() const { return deviceId_; }

 private:
  static void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length);
  bool connect();
  void publishDiscovery();

  WiFiClient wifiClient_;
  PubSubClient client_;
  const char* host_ = nullptr;
  uint16_t port_ = 1883;
  const char* user_ = nullptr;
  const char* password_ = nullptr;
  const LightState* state_ = nullptr;
  CommandHandler onCommand_ = nullptr;
  uint32_t lastConnectAttemptMs_ = 0;
  bool attemptedConnect_ = false;

  char deviceId_[24] = {};
  char availabilityTopic_[64] = {};
  char stateTopic_[64] = {};
  char commandTopic_[64] = {};
  char discoveryTopic_[80] = {};
};
