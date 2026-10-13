#include "time_sync.h"
#include <WiFi.h>

void TimeSync::begin(const char* timezone, const char* ntpServer) {
  configTzTime(timezone, ntpServer);
  configured_ = true;
  Serial.printf("TimeSync: SNTP configured for %s (tz: %s)\n", ntpServer,
                timezone);
}

bool TimeSync::isTimeSet() const {
  if (!configured_) {
    return false;
  }
  time_t now = 0;
  time(&now);
  struct tm timeinfo = {};
  localtime_r(&now, &timeinfo);
  // Default un-synced clock starts around 1970
  return timeinfo.tm_year > (2020 - 1900);
}

bool TimeSync::getFormattedTime(char* outBuffer, size_t bufSize,
                               const char* format) const {
  if (outBuffer == nullptr || bufSize == 0) {
    return false;
  }
  if (!isTimeSet()) {
    return false;
  }
  time_t now = 0;
  time(&now);
  struct tm timeinfo = {};
  localtime_r(&now, &timeinfo);
  return strftime(outBuffer, bufSize, format, &timeinfo) > 0;
}
