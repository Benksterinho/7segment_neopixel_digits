#include "display_policy.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace esphome {
namespace modular_7segment {

bool parse_numeric_payload(const std::string& payload, double& value) {
  if (payload.empty() || payload.size() > 31 ||
      payload.find('\0') != std::string::npos) {
    return false;
  }

  const char* begin = payload.c_str();
  const char* const end = begin + payload.size();
  const char* cursor = begin;
  while (cursor < end && isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }

  if (cursor < end && (*cursor == '+' || *cursor == '-')) {
    ++cursor;
  }

  bool has_digits = false;
  while (cursor < end && *cursor >= '0' && *cursor <= '9') {
    has_digits = true;
    ++cursor;
  }
  if (cursor < end && *cursor == '.') {
    ++cursor;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
      has_digits = true;
      ++cursor;
    }
  }
  if (!has_digits) {
    return false;
  }

  if (cursor < end && (*cursor == 'e' || *cursor == 'E')) {
    ++cursor;
    if (cursor < end && (*cursor == '+' || *cursor == '-')) {
      ++cursor;
    }
    const char* exponent_start = cursor;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
      ++cursor;
    }
    if (cursor == exponent_start) {
      return false;
    }
  }

  while (cursor < end && isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  if (cursor != end) {
    return false;
  }

  errno = 0;
  char* parsed_end = nullptr;
  const double parsed_value = strtod(begin, &parsed_end);
  while (parsed_end < end &&
         isspace(static_cast<unsigned char>(*parsed_end))) {
    ++parsed_end;
  }
  if (parsed_end != end || errno == ERANGE || !isfinite(parsed_value)) {
    return false;
  }

  value = parsed_value;
  return true;
}

void DisplayPolicy::begin(uint32_t now) {
  if (settings.manual_mode >= kModeCount) {
    settings.manual_mode = kTimeMode;
  }
  mode_ = settings.auto_mode ? kTimeMode : settings.manual_mode;
  last_switch_ms_ = now;
}

bool DisplayPolicy::set_auto_mode(bool enabled, uint32_t now) {
  if (settings.auto_mode == enabled) {
    return false;
  }
  if (enabled) {
    last_switch_ms_ = now;
  } else {
    settings.manual_mode = mode_;
  }
  settings.auto_mode = enabled;
  return true;
}

bool DisplayPolicy::select_mode(uint8_t selected_mode, uint32_t now) {
  if (selected_mode >= kModeCount) {
    return false;
  }
  const bool changed = mode_ != selected_mode || settings.auto_mode ||
                       settings.manual_mode != selected_mode;
  settings.auto_mode = false;
  settings.manual_mode = selected_mode;
  mode_ = selected_mode;
  last_switch_ms_ = now;
  return changed;
}

bool DisplayPolicy::tick(uint32_t now) {
  if (!settings.auto_mode ||
      static_cast<uint32_t>(now - last_switch_ms_) < switch_interval_ms_) {
    return false;
  }
  last_switch_ms_ = now;

  const uint8_t previous = mode_;
  do {
    mode_ = static_cast<uint8_t>((mode_ + 1) % kModeCount);
  } while ((mode_ == kCo2Mode && !has_co2_) ||
           (mode_ == kTemperatureMode && !has_temperature_));
  return mode_ != previous;
}

bool DisplayPolicy::accept_co2(double ppm) {
  if (!isfinite(ppm) || ppm < 0.0 ||
      ppm >= static_cast<double>(UINT16_MAX) + 0.5) {
    return false;
  }
  co2_ppm_ = static_cast<uint16_t>(ppm + 0.5);
  has_co2_ = true;
  return true;
}

bool DisplayPolicy::accept_temperature(double celsius) {
  if (!isfinite(celsius)) {
    return false;
  }
  temperature_celsius_ = celsius;
  has_temperature_ = true;
  return true;
}

bool DisplayPolicy::accept_lux(double lux_value) {
  if (!isfinite(lux_value) || lux_value < 0.0) {
    return false;
  }
  lux_ = lux_value;
  has_lux_ = true;
  return true;
}

bool DisplayPolicy::set_temperature_offset(double offset) {
  if (!isfinite(offset)) {
    return false;
  }
  temperature_offset_ = offset;
  return true;
}

uint8_t DisplayPolicy::effective_brightness() const {
  if (!settings.on) {
    return 0;
  }
  if (settings.auto_brightness && has_lux_) {
    return lux_to_brightness(lux_);
  }
  return settings.manual_brightness;
}

bool DisplayPolicy::light_command(bool on, Rgb color,
                                  uint8_t reported_brightness,
                                  uint8_t incoming_brightness) {
  bool changed = false;
  if (settings.on != on) {
    settings.on = on;
    changed = true;
  }
  if (settings.color != color) {
    settings.color = color;
    changed = true;
  }
  if (incoming_brightness != reported_brightness) {
    if (settings.auto_brightness) {
      settings.auto_brightness = false;
      changed = true;
    }
    if (settings.manual_brightness != incoming_brightness) {
      settings.manual_brightness = incoming_brightness;
      changed = true;
    }
  }
  return changed;
}

void DisplayPolicy::frame(const char* time_text, char (&text)[6],
                          Rgb& color) const {
  color = settings.color;
  if (mode_ == kCo2Mode && has_co2_) {
    const uint16_t display_ppm = co2_ppm_ > 9999 ? 9999 : co2_ppm_;
    snprintf(text, sizeof(text), "%4u", display_ppm);
    if (co2_ppm_ < 800) {
      color = {0, 255, 0};
    } else if (co2_ppm_ < 1200) {
      color = {255, 130, 0};
    } else {
      color = {255, 0, 0};
    }
    return;
  }
  if (mode_ == kTemperatureMode && has_temperature_) {
    format_temperature(temperature_celsius_ + temperature_offset_, text);
    return;
  }
  copy_time_or_placeholder(time_text, text);
}

uint8_t DisplayPolicy::lux_to_brightness(double lux_value) {
  constexpr double kMaxLuxReference = 500.0;
  constexpr uint8_t kMinimumBrightness = 15;
  constexpr uint8_t kMaximumBrightness = 255;
  if (lux_value <= 0.0) {
    return kMinimumBrightness;
  }
  const double clamped_lux =
      lux_value > kMaxLuxReference ? kMaxLuxReference : lux_value;
  const double factor =
      log10(1.0 + clamped_lux) / log10(1.0 + kMaxLuxReference);
  const double brightness =
      kMinimumBrightness +
      factor * (kMaximumBrightness - kMinimumBrightness);
  return static_cast<uint8_t>(brightness + 0.5);
}

void DisplayPolicy::format_temperature(double celsius, char (&text)[6]) {
  if (!isfinite(celsius) || celsius < -9.9 || celsius > 99.9) {
    snprintf(text, sizeof(text), " ---");
    return;
  }

  const int tenths = static_cast<int>(round(celsius * 10.0));
  const unsigned int magnitude =
      static_cast<unsigned int>(tenths < 0 ? -tenths : tenths);
  if (tenths < 0) {
    text[0] = ' ';
    text[1] = '-';
    text[2] = static_cast<char>('0' + magnitude / 10);
  } else {
    text[0] = ' ';
    text[1] = magnitude < 100
                  ? ' '
                  : static_cast<char>('0' + magnitude / 100);
    text[2] = static_cast<char>('0' + (magnitude / 10) % 10);
  }
  text[3] = '.';
  text[4] = static_cast<char>('0' + magnitude % 10);
  text[5] = '\0';
}

void DisplayPolicy::copy_time_or_placeholder(const char* time_text,
                                             char (&text)[6]) const {
  if (time_text == nullptr || time_text[0] == '\0') {
    snprintf(text, sizeof(text), "----");
    return;
  }
  snprintf(text, sizeof(text), "%s", time_text);
}

}  // namespace modular_7segment
}  // namespace esphome
