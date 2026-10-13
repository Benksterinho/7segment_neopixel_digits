#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
 public:
  WifiManager() = default;

  // Attempts initial connection (up to timeoutMs blocking) so Wi-Fi is ready
  // before subsequent network setup.
  void begin(const char* ssid, const char* password,
             uint32_t timeoutMs = 15000);

  // Checks Wi-Fi connection and reconnects non-blockingly if disconnected.
  void loop();

  bool isConnected() const;

 private:
  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  uint32_t lastReconnectAttemptMs_ = 0;
  static constexpr uint32_t RECONNECT_INTERVAL_MS = 30000;
};
