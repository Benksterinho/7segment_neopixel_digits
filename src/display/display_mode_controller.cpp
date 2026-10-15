#include "display_mode_controller.h"

void DisplayModeController::begin(bool autoEnabled, DisplayMode manualMode,
                                  uint32_t now) {
  autoEnabled_ = autoEnabled;
  mode_ = autoEnabled ? DisplayMode::TIME : manualMode;
  lastSwitchMs_ = now;
}

void DisplayModeController::setAuto(bool enabled, uint32_t now) {
  if (enabled && !autoEnabled_) {
    lastSwitchMs_ = now;
  }
  autoEnabled_ = enabled;
}

void DisplayModeController::selectMode(DisplayMode mode, uint32_t now) {
  mode_ = mode;
  lastSwitchMs_ = now;
}

bool DisplayModeController::loop(uint32_t now, bool co2Available,
                                 bool temperatureAvailable) {
  if (!autoEnabled_ || now - lastSwitchMs_ < switchIntervalMs_) {
    return false;
  }
  lastSwitchMs_ = now;

  const DisplayMode previous = mode_;
  do {
    mode_ = static_cast<DisplayMode>(
        (static_cast<uint8_t>(mode_) + 1) % MODE_COUNT);
  } while ((mode_ == DisplayMode::CO2 && !co2Available) ||
           (mode_ == DisplayMode::TEMPERATURE && !temperatureAvailable));
  return mode_ != previous;
}
