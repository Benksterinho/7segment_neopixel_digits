#pragma once

#include <ArduinoJson.h>
#include <Preferences.h>
#include <PubSubClient.h>

#include "mqtt_module.h"

// Exposes an ON/OFF switch in Home Assistant (e.g. for Auto-Brightness)
// and persists its state to ESP32 Preferences (NVS).
class MqttSwitchModule : public IMqttModule {
 public:
  using StateChangeHandler = void (*)(bool state);

  MqttSwitchModule(const char* entitySuffix, const char* entityName,
                   bool defaultState = true,
                   StateChangeHandler onChange = nullptr);

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  bool getState() const { return state_; }
  void setState(bool state);

 private:
  void load();
  void save();
  void publishDiscovery();
  void publishState();

  const char* entitySuffix_;
  const char* entityName_;
  bool state_;
  StateChangeHandler onChange_ = nullptr;
  PubSubClient* client_ = nullptr;

  char deviceId_[24] = {};
  char uniqueId_[48] = {};
  char stateTopic_[80] = {};
  char commandTopic_[80] = {};
  char discoveryTopic_[96] = {};
  char availabilityTopic_[64] = {};
};
