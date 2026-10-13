#pragma once

#include <Arduino.h>
#include <time.h>

class TimeSync {
 public:
  TimeSync() = default;

  // Configures SNTP with POSIX timezone string and NTP server.
  // Requires active Wi-Fi.
  void begin(const char* timezone, const char* ntpServer = "pool.ntp.org");

  // Returns true if local time has been set and year is plausible (> 2020).
  bool isTimeSet() const;

  // Formats time into outBuffer using strftime (e.g. "%H%M" for 4-digit display).
  // Returns true on success, false if time is not yet synchronized.
  bool getFormattedTime(char* outBuffer, size_t bufSize,
                        const char* format = "%H%M") const;

 private:
  bool configured_ = false;
};
