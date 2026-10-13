#pragma once

#include <PubSubClient.h>
#include <stddef.h>
#include <stdint.h>

// Interface for modules that interact with MQTT.
// Modules specify topics they care about and respond to messages.
class IMqttModule {
 public:
  virtual ~IMqttModule() = default;

  // Called when MQTT broker connection is established (or re-established).
  // The module should subscribe to its topics and publish discovery/states if needed.
  virtual void onMqttConnected(PubSubClient& client, const char* deviceId) = 0;

  // Called when an incoming MQTT message arrives.
  // Returns true if the message was handled by this module.
  virtual bool handleMessage(const char* topic, const uint8_t* payload,
                             unsigned int length) = 0;
};
