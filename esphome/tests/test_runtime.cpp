#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "modular_7segment.h"

using namespace esphome;
using namespace esphome::modular_7segment;

class TestDisplay : public Modular7Segment {
 public:
  const DisplayPolicy::Settings &settings() const { return this->policy_.settings; }
  bool pending_save() const { return this->save_pending_; }
};

struct Fixture {
  light::AddressableLight strip{116};
  light::AddressableLightState physical{&strip};
  TestDisplay display;
  light::LightState control{&display};
  ClockSwitch brightness{&display, true};
  ClockSwitch mode{&display, false};
  ClockSelect select{&display};
  time::RealTimeClock clock;

  Fixture() {
    display.set_strip_id(&physical);
    display.set_time_id(&clock);
    display.set_auto_brightness_id(&brightness);
    display.set_auto_mode_id(&mode);
    display.set_mode_select_id(&select);
    display.set_layout("DD:D.D.", 4, 1, 100);
    display.setup_state(&control);
  }
  void setup() { display.setup(); assert(!display.is_failed()); }
  bool dark() const {
    for (auto pixel : strip.pixels)
      if (pixel.red || pixel.green || pixel.blue)
        return false;
    return true;
  }
  void command_brightness(uint8_t value) {
    control.remote_values.set_brightness(value / 255.0f);
    control.publish_state();
    display.loop();
  }
};

void reset() {
  test_millis = 0;
  test_errors = 0;
  test_saves = 0;
  test_save_failure = false;
  test_preferences.clear();
}

void test_automatic_feedback_and_commands() {
  reset();
  Fixture f;
  f.setup();
  assert(f.control.remote_values.is_on());
  assert(f.brightness.state && f.mode.state);
  assert(!f.dark());
  f.display.receive_lux("500");
  assert(f.display.settings().auto_brightness);
  assert(f.display.settings().manual_brightness == 50);
  assert(f.control.remote_values.get_brightness() == 1.0f);
  assert(!f.display.pending_save());
  f.command_brightness(255);
  assert(f.brightness.state);  // Approved native API same-value exception.
  f.control.remote_values.set_green(1.0f);
  f.control.publish_state();
  assert(f.brightness.state);
  f.command_brightness(70);
  assert(!f.brightness.state);
  assert(f.display.settings().manual_brightness == 70);
  f.brightness.control(true);
  assert(f.control.remote_values.get_brightness() == 1.0f);
  f.brightness.control(false);
  assert(std::abs(f.control.remote_values.get_brightness() - 70 / 255.0f) < 0.0001f);
  assert(test_saves == 0);
}

void test_off_and_modes() {
  reset();
  Fixture f;
  f.setup();
  f.control.remote_values.set_state(false);
  f.control.publish_state();
  f.display.loop();
  assert(f.dark());
  f.display.receive_lux("500");
  f.display.receive_co2("1200");
  f.display.receive_temperature("25.7");
  test_millis = 30000;
  f.display.loop();
  assert(f.dark() && !f.control.remote_values.is_on());
  assert(f.select.index == 0);
  f.select.test_control(0);
  assert(!f.mode.state);  // Selecting the existing option still disables auto.
  f.command_brightness(80);
  f.control.remote_values.set_blue(1.0f);
  f.control.publish_state();
  f.display.loop();
  assert(f.dark());
  f.select.test_control(1);
  f.control.remote_values.set_state(true);
  f.control.publish_state();
  f.display.loop();
  assert(!f.dark());
  for (const auto pixel : f.strip.pixels)
    assert(pixel.green == 0 && pixel.blue == 0);  // CO2 stays red, not user RGB.
  f.select.test_control(0);
  f.display.loop();
  bool blue = false;
  for (const auto pixel : f.strip.pixels)
    blue |= pixel.blue > 0;
  assert(blue);
}

void test_restore_and_write_failures() {
  reset();
  {
    Fixture f;
    f.setup();
    f.command_brightness(90);
    f.select.test_control(2);
    f.control.remote_values.set_state(false);
    f.control.publish_state();
    test_millis = 1999;
    f.display.loop();
    assert(test_saves == 0);
    test_millis = 2000;
    f.display.loop();
    assert(test_saves == 1);
  }
  Fixture restored;
  // Boot callbacks cannot overwrite combined restored settings.
  restored.brightness.control(true);
  restored.mode.control(true);
  restored.select.test_control(0);
  restored.setup();
  assert(restored.dark());
  assert(!restored.brightness.state && !restored.mode.state);
  assert(restored.select.index == 2);
  assert(restored.display.settings().manual_brightness == 90);
  restored.display.receive_lux("500");
  test_millis += 10000;
  restored.display.loop();
  assert(test_saves == 1);
  restored.command_brightness(100);
  test_save_failure = true;
  test_millis += 2000;
  restored.display.loop();
  assert(restored.display.pending_save());
  assert(test_errors > 0);
  test_save_failure = false;
  test_millis += 2000;
  restored.display.loop();
  assert(!restored.display.pending_save());
}

void test_faults_remain_dark() {
  reset();
  Fixture f;
  f.display.set_layout("DDD", 4, 1, 100);
  f.display.setup();
  assert(f.display.is_failed() && f.dark());
  reset();
  {
    Fixture source;
    source.setup();
    source.command_brightness(60);
    source.display.on_shutdown();
  }
  assert(!test_preferences.empty());
  test_preferences.begin()->second[0] = 99;
  Fixture corrupt;
  corrupt.display.setup();
  assert(corrupt.display.is_failed() && corrupt.dark());
  reset();
  Fixture input;
  input.setup();
  input.display.receive_lux("500junk");
  input.display.receive_co2("nan");
  input.display.receive_temperature(std::string("2\0", 2));
  assert(test_errors == 3);
  assert(input.display.settings().manual_brightness == 50);
  input.control.remote_values.set_brightness(std::numeric_limits<float>::quiet_NaN());
  input.control.publish_state();
  assert(test_errors == 4);
  assert(std::isfinite(input.control.remote_values.get_brightness()));
  assert(!input.display.pending_save());
}

void test_local_clock_and_separators() {
  reset();
  Fixture f;
  f.setup();
  assert(f.strip.pixels[56].red == 0 && f.strip.pixels[57].red == 0);
  f.clock.valid = true;
  test_millis = 2000;
  f.display.loop();
  assert(f.strip.pixels[56].red > 0 && f.strip.pixels[57].red > 0);
  assert(f.strip.pixels[86].red == 0 && f.strip.pixels[115].red == 0);
  test_millis = 3000;
  f.display.loop();
  assert(f.strip.pixels[56].red == 0 && f.strip.pixels[57].red == 0);
  f.display.receive_temperature("25.7");
  f.select.test_control(2);
  f.display.loop();
  for (size_t i = 0; i < 28; ++i)
    assert(f.strip.pixels[i].red == 0);
  assert(f.strip.pixels[56].red == 0 && f.strip.pixels[57].red == 0);
  assert(f.strip.pixels[86].red > 0 && f.strip.pixels[115].red == 0);
}

int main() {
  test_automatic_feedback_and_commands();
  test_off_and_modes();
  test_restore_and_write_failures();
  test_faults_remain_dark();
  test_local_clock_and_separators();
  std::cout << "Runtime integration simulations passed\n";
}
