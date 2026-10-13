#pragma once

#include <ArduinoJson.h>
#include <LightState.h>
#include <PubSubClient.h>

#include "mqtt_module.h"

// Exposes the display as a Home Assistant MQTT light entity with
// ON/OFF, brightness, and RGB color.
class MqttLightModule : public IMqttModule {
 public:
  using CommandHandler = void (*)(const LightState& newState,
                                  bool brightnessSpecified);
  using BrightnessProvider = uint8_t (*)();

  explicit MqttLightModule(LightState& state, CommandHandler onCommand = nullptr);

  void setCommandHandler(CommandHandler onCommand) { onCommand_ = onCommand; }
  void setBrightnessProvider(BrightnessProvider provider) {
    brightnessProvider_ = provider;
  }

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  void publishState();

 private:
  void publishDiscovery();

  LightState& state_;
  CommandHandler onCommand_ = nullptr;
  BrightnessProvider brightnessProvider_ = nullptr;
  PubSubClient* client_ = nullptr;

  char deviceId_[24] = {};
  char stateTopic_[64] = {};
  char commandTopic_[64] = {};
  char discoveryTopic_[80] = {};
  char availabilityTopic_[64] = {};
};
