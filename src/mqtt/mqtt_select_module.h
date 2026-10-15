#pragma once

#include <ArduinoJson.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt_module.h"

// Exposes a Home Assistant MQTT select entity with a fixed list of options
// and persists the selected index to ESP32 Preferences (NVS).
class MqttSelectModule : public IMqttModule {
 public:
  using SelectHandler = void (*)(uint8_t index);

  MqttSelectModule(const char* entitySuffix, const char* entityName,
                   const char* const* options, uint8_t optionCount,
                   uint8_t defaultIndex = 0, SelectHandler onSelect = nullptr);

  void onMqttConnected(PubSubClient& client, const char* deviceId) override;
  bool handleMessage(const char* topic, const uint8_t* payload,
                     unsigned int length) override;

  uint8_t getIndex() const { return index_; }
  const char* getOption() const { return options_[index_]; }

  // Updates, persists and publishes the selection without invoking the handler.
  void setIndex(uint8_t index);

  // Case-insensitive match of payload against options.
  static bool parseOption(const char* payload, size_t length,
                          const char* const* options, uint8_t optionCount,
                          uint8_t& outIndex);

 private:
  void load();
  void save();
  void publishDiscovery();
  void publishState();

  const char* entitySuffix_;
  const char* entityName_;
  const char* const* options_;
  uint8_t optionCount_;
  uint8_t index_;
  SelectHandler onSelect_ = nullptr;
  PubSubClient* client_ = nullptr;

  char deviceId_[24] = {};
  char uniqueId_[48] = {};
  char stateTopic_[80] = {};
  char commandTopic_[80] = {};
  char discoveryTopic_[96] = {};
  char availabilityTopic_[64] = {};
};
