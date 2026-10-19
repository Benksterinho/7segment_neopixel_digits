#pragma once

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <vector>

namespace esphome {
namespace modular_7segment {

struct Rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  constexpr Rgb() : red(0), green(0), blue(0) {}
  constexpr Rgb(uint8_t red_value, uint8_t green_value, uint8_t blue_value)
      : red(red_value), green(green_value), blue(blue_value) {}

  bool operator==(const Rgb& other) const {
    return red == other.red && green == other.green && blue == other.blue;
  }

  bool operator!=(const Rgb& other) const { return !(*this == other); }
};

class SegmentRenderer {
 public:
  static constexpr uint8_t kMaxModules = 32;

  SegmentRenderer() = default;
  SegmentRenderer(const char* layout, uint8_t pixels_per_segment,
                  uint8_t pixels_per_dot, uint8_t dot_percent = 110);

  bool configure(const char* layout, uint8_t pixels_per_segment,
                 uint8_t pixels_per_dot, uint8_t dot_percent = 110);

  bool valid() const { return valid_; }
  size_t count() const { return module_count_; }
  size_t led_count() const { return led_count_; }
  size_t digit_count() const { return digit_count_; }
  const char* error() const { return error_; }

  bool render(const char* text, Rgb color, uint8_t brightness,
              std::vector<Rgb>& frame) const;

 private:
  enum class ModuleType : uint8_t { kDigit, kColon, kDecimal };

  struct Module {
    ModuleType type = ModuleType::kDigit;
    size_t offset = 0;
    uint8_t pixels_per_element = 0;
  };

  std::array<Module, kMaxModules> modules_{};
  size_t module_count_ = 0;
  size_t led_count_ = 0;
  size_t digit_count_ = 0;
  uint8_t dot_percent_ = 100;
  bool valid_ = false;
  const char* error_ = "not configured";

  static uint8_t character_mask(char character);
  static Rgb scale(Rgb color, uint16_t numerator, uint16_t denominator);
};

}  // namespace modular_7segment
}  // namespace esphome
