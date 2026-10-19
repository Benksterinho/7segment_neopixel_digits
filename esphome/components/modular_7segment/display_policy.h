#pragma once

#include <stdint.h>

#include <string>

#include "segment_renderer.h"

namespace esphome {
namespace modular_7segment {

bool parse_numeric_payload(const std::string& payload, double& value);

class DisplayPolicy {
 public:
  static constexpr uint8_t kTimeMode = 0;
  static constexpr uint8_t kCo2Mode = 1;
  static constexpr uint8_t kTemperatureMode = 2;
  static constexpr uint8_t kModeCount = 3;
  static constexpr uint32_t kDefaultSwitchIntervalMs = 30000;

  struct Settings {
    bool on = true;
    uint8_t manual_brightness = 50;
    Rgb color{255, 0, 0};
    bool auto_brightness = true;
    bool auto_mode = true;
    uint8_t manual_mode = kTimeMode;
  };

  explicit DisplayPolicy(
      uint32_t switch_interval_ms = kDefaultSwitchIntervalMs)
      : switch_interval_ms_(switch_interval_ms) {}

  Settings settings;

  void begin(uint32_t now);
  bool set_auto_mode(bool enabled, uint32_t now);
  bool select_mode(uint8_t mode, uint32_t now);
  bool tick(uint32_t now);

  bool accept_co2(double ppm);
  bool accept_temperature(double celsius);
  bool accept_lux(double lux);
  bool set_temperature_offset(double offset);

  uint8_t effective_brightness() const;
  bool light_command(bool on, Rgb color, uint8_t reported_brightness,
                     uint8_t incoming_brightness);
  void frame(const char* time_text, char (&text)[6], Rgb& color) const;

  uint8_t mode() const { return mode_; }
  bool has_co2() const { return has_co2_; }
  bool has_temperature() const { return has_temperature_; }
  bool has_lux() const { return has_lux_; }
  uint16_t co2_ppm() const { return co2_ppm_; }
  double temperature_celsius() const { return temperature_celsius_; }
  double lux() const { return lux_; }
  double temperature_offset() const { return temperature_offset_; }

 private:
  uint32_t switch_interval_ms_;
  uint32_t last_switch_ms_ = 0;
  uint8_t mode_ = kTimeMode;
  uint16_t co2_ppm_ = 0;
  double temperature_celsius_ = 0;
  double lux_ = 0;
  double temperature_offset_ = -2.0;
  bool has_co2_ = false;
  bool has_temperature_ = false;
  bool has_lux_ = false;

  static uint8_t lux_to_brightness(double lux);
  static void format_temperature(double celsius, char (&text)[6]);
  void copy_time_or_placeholder(const char* time_text, char (&text)[6]) const;
};

}  // namespace modular_7segment
}  // namespace esphome
