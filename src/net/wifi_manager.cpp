#include "wifi_manager.h"

void WifiManager::begin(const char* ssid, const char* password,
                        uint32_t timeoutMs) {
  ssid_ = ssid;
  password_ = password;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_, password_);

  Serial.print("WiFi connecting");
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect failed or timed out (will retry in loop)");
  }
}

void WifiManager::loop() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastReconnectAttemptMs_ < RECONNECT_INTERVAL_MS) {
    return;
  }

  lastReconnectAttemptMs_ = now;
  Serial.println("WiFi disconnected, reconnecting...");
  WiFi.disconnect();
  WiFi.begin(ssid_, password_);
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}
