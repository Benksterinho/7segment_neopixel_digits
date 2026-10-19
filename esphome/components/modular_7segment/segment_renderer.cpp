#include "segment_renderer.h"

#include <ctype.h>

#include <algorithm>
#include <limits>

namespace esphome {
namespace modular_7segment {
namespace {

constexpr uint8_t kBottom = 1U << 0;
constexpr uint8_t kLowerLeft = 1U << 1;
constexpr uint8_t kMiddle = 1U << 2;
constexpr uint8_t kUpperLeft = 1U << 3;
constexpr uint8_t kTop = 1U << 4;
constexpr uint8_t kUpperRight = 1U << 5;
constexpr uint8_t kLowerRight = 1U << 6;
constexpr uint8_t kZero = kTop | kUpperRight | kLowerRight | kBottom |
                          kLowerLeft | kUpperLeft;

bool parse_pixel_count(const char*& cursor, uint8_t fallback, uint8_t& count) {
  const char* first_digit = cursor;
  unsigned int value = 0;
  while (*cursor >= '0' && *cursor <= '9') {
    if (value <= 255) {
      value = value * 10U + static_cast<unsigned int>(*cursor - '0');
    }
    ++cursor;
  }

  if (cursor == first_digit) {
    count = fallback;
    return fallback != 0;
  }
  if (cursor - first_digit > 3 || value == 0 || value > 255) {
    return false;
  }
  count = static_cast<uint8_t>(value);
  return true;
}

}  // namespace

SegmentRenderer::SegmentRenderer(const char* layout, uint8_t pixels_per_segment,
                                 uint8_t pixels_per_dot, uint8_t dot_percent) {
  configure(layout, pixels_per_segment, pixels_per_dot, dot_percent);
}

bool SegmentRenderer::configure(const char* layout, uint8_t pixels_per_segment,
                                uint8_t pixels_per_dot, uint8_t dot_percent) {
  modules_ = {};
  module_count_ = 0;
  led_count_ = 0;
  digit_count_ = 0;
  dot_percent_ = dot_percent;
  valid_ = false;
  error_ = "invalid layout";

  const auto fail = [this](const char* message) -> bool {
    modules_ = {};
    module_count_ = 0;
    led_count_ = 0;
    digit_count_ = 0;
    error_ = message;
    return false;
  };
  if (layout == nullptr || pixels_per_segment == 0 || pixels_per_dot == 0) {
    return fail("layout and fallback pixel counts must be valid");
  }
  if (*layout == '\0') {
    return fail("layout must contain at least one module");
  }

  const char* cursor = layout;
  while (*cursor != '\0') {
    if (module_count_ == kMaxModules) {
      return fail("layout exceeds maximum module count");
    }

    Module module;
    uint8_t fallback = 0;
    size_t elements = 0;
    switch (*cursor) {
      case 'D':
        module.type = ModuleType::kDigit;
        fallback = pixels_per_segment;
        elements = 7;
        ++digit_count_;
        break;
      case ':':
        module.type = ModuleType::kColon;
        fallback = pixels_per_dot;
        elements = 2;
        break;
      case '.':
        module.type = ModuleType::kDecimal;
        fallback = pixels_per_dot;
        elements = 1;
        break;
      default:
        return fail("layout contains an unsupported module");
    }

    ++cursor;
    if (!parse_pixel_count(cursor, fallback, module.pixels_per_element)) {
      return fail("inline pixel count must be 1-255 using at most 3 digits");
    }

    const size_t module_leds =
        elements * static_cast<size_t>(module.pixels_per_element);
    if (module_leds > std::numeric_limits<size_t>::max() - led_count_) {
      return fail("layout LED count overflows size_t");
    }
    module.offset = led_count_;
    led_count_ += module_leds;
    modules_[module_count_++] = module;
  }

  valid_ = module_count_ > 0;
  error_ = valid_ ? "" : "layout must contain at least one module";
  return valid_;
}

bool SegmentRenderer::render(const char* text, Rgb color, uint8_t brightness,
                             std::vector<Rgb>& frame) const {
  if (!valid_) {
    frame.clear();
    return false;
  }

  frame.assign(led_count_, Rgb{});
  const char* input = text == nullptr ? "" : text;
  const Rgb digit_color = scale(color, brightness, 255);
  const Rgb dot_color =
      scale(scale(color, dot_percent_, 100), brightness, 255);

  for (size_t index = 0; index < module_count_; ++index) {
    const Module& module = modules_[index];
    if (module.type != ModuleType::kDigit) {
      const char punctuation =
          module.type == ModuleType::kColon ? ':' : '.';
      if (*input == punctuation) {
        const size_t module_leds =
            (module.type == ModuleType::kColon ? 2U : 1U) *
            module.pixels_per_element;
        std::fill(frame.begin() + module.offset,
                  frame.begin() + module.offset + module_leds, dot_color);
        ++input;
      }
      continue;
    }

    const char character = *input == '\0' ? ' ' : *input++;
    const uint8_t mask = character_mask(character);
    for (uint8_t segment = 0; segment < 7; ++segment) {
      if ((mask & (1U << segment)) == 0) {
        continue;
      }
      const size_t first_pixel =
          module.offset + segment * module.pixels_per_element;
      std::fill(frame.begin() + first_pixel,
                frame.begin() + first_pixel + module.pixels_per_element,
                digit_color);
    }
  }
  return true;
}

uint8_t SegmentRenderer::character_mask(char character) {
  switch (character) {
    case '0': return kZero;
    case '1': return kUpperRight | kLowerRight;
    case '2': return kTop | kUpperRight | kMiddle | kLowerLeft | kBottom;
    case '3': return kTop | kUpperRight | kMiddle | kLowerRight | kBottom;
    case '4': return kUpperLeft | kMiddle | kUpperRight | kLowerRight;
    case '5': return kTop | kUpperLeft | kMiddle | kLowerRight | kBottom;
    case '6': return kTop | kUpperLeft | kMiddle | kLowerLeft | kLowerRight |
                     kBottom;
    case '7': return kTop | kUpperRight | kLowerRight;
    case '8': return kZero | kMiddle;
    case '9': return kTop | kUpperLeft | kMiddle | kUpperRight | kLowerRight |
                     kBottom;
    case 'A':
    case 'a':
      return kTop | kUpperLeft | kMiddle | kUpperRight | kLowerRight |
             kLowerLeft;
    case 'B':
    case 'b':
      return kUpperLeft | kMiddle | kLowerLeft | kLowerRight | kBottom;
    case 'C':
    case 'c':
      return kTop | kUpperLeft | kLowerLeft | kBottom;
    case 'D':
    case 'd':
      return kUpperRight | kMiddle | kLowerLeft | kLowerRight | kBottom;
    case 'E':
    case 'e':
      return kTop | kUpperLeft | kMiddle | kLowerLeft | kBottom;
    case 'F':
    case 'f':
      return kTop | kUpperLeft | kMiddle | kLowerLeft;
    case '-': return kMiddle;
    default: return 0;
  }
}

Rgb SegmentRenderer::scale(Rgb color, uint16_t numerator,
                           uint16_t denominator) {
  const auto channel = [numerator, denominator](uint8_t value) -> uint8_t {
    const uint32_t scaled =
        (static_cast<uint32_t>(value) * numerator + denominator / 2) /
        denominator;
    return static_cast<uint8_t>(std::min<uint32_t>(scaled, 255));
  };
  return {channel(color.red), channel(color.green), channel(color.blue)};
}

}  // namespace modular_7segment
}  // namespace esphome
