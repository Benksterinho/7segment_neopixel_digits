#include "components/modular_7segment/display_policy.h"
#include "components/modular_7segment/segment_renderer.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <string>
#include <vector>

using esphome::modular_7segment::DisplayPolicy;
using esphome::modular_7segment::Rgb;
using esphome::modular_7segment::SegmentRenderer;
using esphome::modular_7segment::parse_numeric_payload;

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

void test_character_masks() {
  const char characters[] = "0123456789ABCDEFabcdef-? ";
  const uint8_t masks[] = {
      kZero,
      kUpperRight | kLowerRight,
      kTop | kUpperRight | kMiddle | kLowerLeft | kBottom,
      kTop | kUpperRight | kMiddle | kLowerRight | kBottom,
      kUpperLeft | kMiddle | kUpperRight | kLowerRight,
      kTop | kUpperLeft | kMiddle | kLowerRight | kBottom,
      kTop | kUpperLeft | kMiddle | kLowerLeft | kLowerRight | kBottom,
      kTop | kUpperRight | kLowerRight,
      kZero | kMiddle,
      kTop | kUpperLeft | kMiddle | kUpperRight | kLowerRight | kBottom,
      kTop | kUpperLeft | kMiddle | kUpperRight | kLowerRight | kLowerLeft,
      kUpperLeft | kMiddle | kLowerLeft | kLowerRight | kBottom,
      kTop | kUpperLeft | kLowerLeft | kBottom,
      kUpperRight | kMiddle | kLowerLeft | kLowerRight | kBottom,
      kTop | kUpperLeft | kMiddle | kLowerLeft | kBottom,
      kTop | kUpperLeft | kMiddle | kLowerLeft,
      kTop | kUpperLeft | kMiddle | kUpperRight | kLowerRight | kLowerLeft,
      kUpperLeft | kMiddle | kLowerLeft | kLowerRight | kBottom,
      kTop | kUpperLeft | kLowerLeft | kBottom,
      kUpperRight | kMiddle | kLowerLeft | kLowerRight | kBottom,
      kTop | kUpperLeft | kMiddle | kLowerLeft | kBottom,
      kTop | kUpperLeft | kMiddle | kLowerLeft,
      kMiddle,
      0,
      0,
  };

  SegmentRenderer renderer("D", 1, 1, 100);
  assert(renderer.valid());
  std::vector<Rgb> frame;
  const Rgb lit(20, 40, 60);
  for (size_t index = 0; index < sizeof(characters) - 1; ++index) {
    assert(renderer.render(&characters[index], lit, 255, frame));
    assert(frame.size() == 7);
    for (size_t segment = 0; segment < frame.size(); ++segment) {
      assert(frame[segment] ==
             ((masks[index] & (1U << segment)) != 0 ? lit : Rgb()));
    }
  }
}

void test_renderer_layout_and_scaling() {
  SegmentRenderer renderer("D2:3.4D", 2, 1, 150);
  assert(renderer.valid());
  assert(renderer.count() == 4);
  assert(renderer.digit_count() == 2);
  assert(renderer.led_count() == 38);

  std::vector<Rgb> frame;
  const Rgb color(100, 40, 20);
  assert(renderer.render("1:.8", color, 128, frame));
  assert(frame.size() == 38);
  const Rgb dimmed_digit(50, 20, 10);
  const Rgb dimmed_dot(75, 30, 15);
  assert(frame[0] == Rgb());
  assert(frame[10] == dimmed_digit);
  assert(frame[14] == dimmed_dot);
  assert(frame[19] == dimmed_dot);
  assert(frame[20] == dimmed_dot);
  assert(frame[23] == dimmed_dot);
  assert(frame[24] == dimmed_digit);

  assert(renderer.render("1x.8", color, 255, frame));
  assert(frame[14] == Rgb());
  assert(frame[19] == Rgb());
  assert(frame[20] == Rgb());

  assert(renderer.render(nullptr, color, 255, frame));
  for (const Rgb& pixel : frame) {
    assert(pixel == Rgb());
  }

  SegmentRenderer clamped_dot("D.", 1, 2, 255);
  assert(clamped_dot.render("1.", Rgb(255, 100, 1), 255, frame));
  assert(frame.size() == 9);
  assert(frame[7] == Rgb(255, 255, 3));
  assert(frame[8] == Rgb(255, 255, 3));

  SegmentRenderer current_layout("DD:D.D.", 4, 1, 100);
  assert(current_layout.valid());
  assert(current_layout.led_count() == 116);
  assert(current_layout.digit_count() == 4);
  assert(current_layout.render("12:3.4.", Rgb(1, 2, 3), 255, frame));
  assert(frame.size() == 116);
}

void test_renderer_boundaries() {
  SegmentRenderer renderer;
  assert(!renderer.configure(nullptr, 1, 1));
  assert(!renderer.valid());
  assert(renderer.count() == 0);
  assert(renderer.led_count() == 0);
  assert(renderer.error()[0] != '\0');
  assert(!renderer.configure("", 1, 1));
  assert(!renderer.configure("D", 0, 1));
  assert(!renderer.configure("D", 1, 0));
  assert(!renderer.configure("X", 1, 1));
  assert(!renderer.configure("D0", 1, 1));
  assert(!renderer.configure("D256", 1, 1));
  assert(!renderer.configure("D0001", 1, 1));

  assert(renderer.configure("D1", 1, 1));
  assert(renderer.count() == 1);
  assert(renderer.led_count() == 7);
  assert(renderer.configure("D255", 1, 1));
  assert(renderer.count() == 1);
  assert(renderer.led_count() == 1785);

  std::string thirty_two_digits(32, 'D');
  assert(renderer.configure(thirty_two_digits.c_str(), 1, 1));
  assert(renderer.count() == 32);
  assert(renderer.led_count() == 224);
  assert(!renderer.configure((thirty_two_digits + "D").c_str(), 1, 1));
  assert(!renderer.valid());
  assert(renderer.count() == 0);
  assert(renderer.led_count() == 0);

  assert(renderer.configure(thirty_two_digits.c_str(), 255, 1));
  assert(renderer.led_count() == 57120);
  std::vector<Rgb> frame;
  assert(renderer.render(nullptr, Rgb(1, 2, 3), 255, frame));
  assert(frame.size() == 57120);
}

void test_policy_startup_and_modes() {
  DisplayPolicy policy;
  policy.settings.manual_mode = DisplayPolicy::kTemperatureMode;
  policy.begin(100);
  assert(policy.mode() == DisplayPolicy::kTimeMode);
  assert(policy.settings.auto_mode);

  assert(policy.tick(30100) == false);
  assert(policy.mode() == DisplayPolicy::kTimeMode);
  assert(policy.accept_co2(500));
  assert(policy.tick(60100));
  assert(policy.mode() == DisplayPolicy::kCo2Mode);
  assert(!policy.tick(90099));
  assert(policy.tick(90100));
  assert(policy.mode() == DisplayPolicy::kTimeMode);
  assert(policy.accept_temperature(20));
  assert(policy.tick(120100));
  assert(policy.mode() == DisplayPolicy::kCo2Mode);
  assert(policy.tick(150100));
  assert(policy.mode() == DisplayPolicy::kTemperatureMode);

  assert(policy.set_auto_mode(false, 150200));
  assert(policy.settings.manual_mode == DisplayPolicy::kTemperatureMode);
  assert(!policy.tick(200000));
  assert(policy.select_mode(DisplayPolicy::kCo2Mode, 200100));
  assert(!policy.settings.auto_mode);
  assert(policy.settings.manual_mode == DisplayPolicy::kCo2Mode);
  assert(!policy.select_mode(DisplayPolicy::kModeCount, 200200));

  policy.settings.auto_mode = false;
  policy.settings.manual_mode = DisplayPolicy::kCo2Mode;
  policy.begin(300000);
  assert(policy.mode() == DisplayPolicy::kCo2Mode);

  DisplayPolicy wrapping;
  wrapping.begin(UINT32_MAX - 10000U);
  assert(!wrapping.tick(19998U));
  assert(!wrapping.tick(19999U));
  assert(wrapping.accept_temperature(20));
  assert(wrapping.tick(49999U));
  assert(wrapping.mode() == DisplayPolicy::kTemperatureMode);
}

void test_sensor_validation_and_frames() {
  DisplayPolicy policy;
  policy.begin(0);
  assert(!policy.has_co2());
  assert(policy.accept_co2(799.49));
  assert(policy.co2_ppm() == 799);
  assert(policy.accept_co2(799.5));
  assert(policy.co2_ppm() == 800);
  assert(policy.accept_co2(1199.49));
  assert(policy.co2_ppm() == 1199);
  assert(policy.accept_co2(1199.5));
  assert(policy.co2_ppm() == 1200);

  assert(!policy.accept_co2(-0.1));
  assert(!policy.accept_co2(NAN));
  assert(!policy.accept_co2(INFINITY));
  assert(!policy.accept_co2(65535.5));
  assert(policy.co2_ppm() == 1200);
  assert(policy.accept_co2(65535.49));
  assert(policy.co2_ppm() == UINT16_MAX);

  char text[6] = {};
  Rgb color;
  policy.frame("12:34", text, color);
  assert(std::string(text) == "12:34");
  assert(color == policy.settings.color);

  assert(policy.select_mode(DisplayPolicy::kCo2Mode, 0));
  policy.accept_co2(799);
  policy.frame(nullptr, text, color);
  assert(std::string(text) == " 799");
  assert(color == Rgb(0, 255, 0));
  policy.accept_co2(800);
  policy.frame(nullptr, text, color);
  assert(color == Rgb(255, 130, 0));
  policy.accept_co2(1199);
  policy.frame(nullptr, text, color);
  assert(color == Rgb(255, 130, 0));
  policy.accept_co2(1200);
  policy.frame(nullptr, text, color);
  assert(color == Rgb(255, 0, 0));
  policy.accept_co2(10000);
  policy.frame(nullptr, text, color);
  assert(std::string(text) == "9999");

  assert(policy.select_mode(DisplayPolicy::kTemperatureMode, 0));
  assert(!policy.has_temperature());
  policy.frame("08:05", text, color);
  assert(std::string(text) == "08:05");
  assert(policy.accept_temperature(22.26));
  policy.frame("08:05", text, color);
  assert(std::string(text) == " 20.3");
  assert(color == policy.settings.color);
  policy.accept_temperature(-1.96);
  policy.frame("08:05", text, color);
  assert(std::string(text) == " -4.0");
  assert(policy.set_temperature_offset(0));
  policy.accept_temperature(-0.04);
  policy.frame("08:05", text, color);
  assert(std::string(text) == "  0.0");
  policy.accept_temperature(200);
  policy.frame("08:05", text, color);
  assert(std::string(text) == " ---");
  assert(!policy.accept_temperature(NAN));
  assert(policy.temperature_celsius() == 200);

  assert(policy.accept_lux(0));
  assert(policy.effective_brightness() == 15);
  assert(policy.accept_lux(500));
  assert(policy.effective_brightness() == 255);
  assert(!policy.accept_lux(-1));
  assert(!policy.accept_lux(INFINITY));
  assert(policy.lux() == 500);
}

void test_policy_brightness_and_off_invariance() {
  DisplayPolicy policy;
  policy.begin(0);
  assert(policy.effective_brightness() == 50);
  assert(policy.accept_lux(0));
  assert(policy.effective_brightness() == 15);

  const Rgb changed_color(9, 8, 7);
  assert(policy.light_command(true, changed_color, 15, 15));
  assert(policy.settings.auto_brightness);
  assert(policy.settings.manual_brightness == 50);
  assert(policy.settings.color == changed_color);
  assert(policy.effective_brightness() == 15);

  assert(policy.light_command(true, changed_color, 15, 77));
  assert(!policy.settings.auto_brightness);
  assert(policy.settings.manual_brightness == 77);
  assert(policy.effective_brightness() == 77);

  policy.settings.auto_brightness = true;
  assert(policy.accept_lux(500));
  assert(policy.effective_brightness() == 255);
  assert(policy.settings.manual_brightness == 77);
  policy.settings.auto_brightness = false;
  assert(policy.effective_brightness() == 77);

  assert(policy.set_auto_mode(false, 1000));
  assert(policy.select_mode(DisplayPolicy::kCo2Mode, 1100));
  assert(policy.accept_co2(900));
  assert(policy.light_command(false, changed_color, 77, 77));
  assert(!policy.settings.on);
  assert(policy.effective_brightness() == 0);
  assert(policy.mode() == DisplayPolicy::kCo2Mode);
  assert(!policy.settings.auto_mode);
  assert(policy.settings.manual_brightness == 77);
  assert(policy.settings.color == changed_color);
  assert(!policy.tick(100000));

  policy.accept_lux(0);
  assert(!policy.settings.on);
  assert(policy.effective_brightness() == 0);
}

void test_numeric_payload_parser() {
  double value = 987.0;
  assert(parse_numeric_payload(" \t+1.25e2\n", value));
  assert(value == 125.0);
  assert(parse_numeric_payload("-.5", value));
  assert(value == -0.5);
  assert(parse_numeric_payload("1.", value));
  assert(value == 1.0);
  assert(parse_numeric_payload("-0", value));
  assert(value == 0.0 && signbit(value));
  assert(parse_numeric_payload("1.0 \t", value));
  assert(value == 1.0);
  assert(parse_numeric_payload("1e+2", value));
  assert(value == 100.0);
  assert(parse_numeric_payload(std::string(31, '1'), value));

  const char embedded_nul[] = {'1', '\0', '2'};
  const std::string with_nul(embedded_nul, sizeof(embedded_nul));
  const char* const invalid_payloads[] = {
      "",          " ",          "  + 1",      ".",       "+.",
      "1e",        "1e+",        "1e-",        "1e2e3",   "1x",
      "0x1p2",     "nan",        "inf",        "infinity", "1e9999",
      "1e-9999",   "1,5",        "1\nx"};
  for (const char* payload : invalid_payloads) {
    value = 987.0;
    assert(!parse_numeric_payload(payload, value));
    assert(value == 987.0);
  }
  assert(!parse_numeric_payload(with_nul, value));
  assert(!parse_numeric_payload(std::string(32, '1'), value));
}

}  // namespace

int main() {
  test_character_masks();
  test_renderer_layout_and_scaling();
  test_renderer_boundaries();
  test_policy_startup_and_modes();
  test_sensor_validation_and_frames();
  test_policy_brightness_and_off_invariance();
  test_numeric_payload_parser();
  puts("host core tests passed");
  return 0;
}
