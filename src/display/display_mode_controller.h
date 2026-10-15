#pragma once

#include <stdint.h>

enum class DisplayMode : uint8_t {
  TIME = 0,
  CO2 = 1,
  TEMPERATURE = 2,
};

// Decides which display mode is active. In auto mode it cycles through the
// modes on a fixed interval; in manual mode it stays on the selected mode.
class DisplayModeController {
 public:
  static constexpr uint8_t MODE_COUNT = 3;

  explicit DisplayModeController(uint32_t switchIntervalMs)
      : switchIntervalMs_(switchIntervalMs) {}

  void begin(bool autoEnabled, DisplayMode manualMode, uint32_t now);

  // Enabling resumes cycling from the current mode; disabling pins it.
  void setAuto(bool enabled, uint32_t now);
  // Pins the given mode. Callers are responsible for disabling auto.
  void selectMode(DisplayMode mode, uint32_t now);

  // Returns true when the active mode changed.
  bool loop(uint32_t now, bool co2Available, bool temperatureAvailable);

  bool isAuto() const { return autoEnabled_; }
  DisplayMode mode() const { return mode_; }

 private:
  uint32_t switchIntervalMs_;
  bool autoEnabled_ = true;
  DisplayMode mode_ = DisplayMode::TIME;
  uint32_t lastSwitchMs_ = 0;
};
