#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace esphome {
inline uint32_t test_millis = 0;
inline unsigned test_errors = 0;
inline uint32_t millis() { return test_millis; }
inline uint32_t fnv1_hash(const std::string &value) {
  uint32_t result = 2166136261u;
  for (unsigned char byte : value)
    result = (result * 16777619u) ^ byte;
  return result;
}
namespace setup_priority {
constexpr float LATE = -100;
}
class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual void loop() {}
  virtual void dump_config() {}
  virtual void on_shutdown() {}
  virtual float get_setup_priority() const { return 0; }
  bool is_failed() const { return failed_; }
  void mark_failed() { failed_ = true; }
 private:
  bool failed_{false};
};
struct Color {
  uint8_t red{0}, green{0}, blue{0};
  Color() = default;
  Color(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
  static const Color BLACK;
};
inline const Color Color::BLACK{};
inline std::map<uint32_t, std::vector<uint8_t>> test_preferences;
inline unsigned test_saves = 0;
inline bool test_save_failure = false;
class ESPPreferenceObject {
 public:
  explicit ESPPreferenceObject(uint32_t key = 0) : key_(key) {}
  template<typename T> bool load(T *value) {
    const auto it = test_preferences.find(key_);
    if (it == test_preferences.end() || it->second.size() != sizeof(T))
      return false;
    std::memcpy(value, it->second.data(), sizeof(T));
    return true;
  }
  template<typename T> bool save(const T *value) {
    ++test_saves;
    if (test_save_failure)
      return false;
    const auto *bytes = reinterpret_cast<const uint8_t *>(value);
    test_preferences[key_] = std::vector<uint8_t>(bytes, bytes + sizeof(T));
    return true;
  }
 private:
  uint32_t key_;
};
struct Preferences {
  template<typename T> ESPPreferenceObject make_preference(uint32_t key) {
    return ESPPreferenceObject(key);
  }
  bool sync() { return !test_save_failure; }
};
inline Preferences test_manager;
inline Preferences *global_preferences = &test_manager;
namespace light {
enum class ColorMode { RGB };
class LightTraits {
 public:
  void set_supported_color_modes(std::initializer_list<ColorMode>) {}
};
class LightColorValues {
 public:
  bool is_on() const { return on_; }
  void set_state(bool value) { on_ = value; }
  void set_color_mode(ColorMode) {}
  float get_brightness() const { return brightness_; }
  float get_color_brightness() const { return color_brightness_; }
  float get_red() const { return red_; }
  float get_green() const { return green_; }
  float get_blue() const { return blue_; }
  void set_brightness(float value) { brightness_ = value; }
  void set_color_brightness(float value) { color_brightness_ = value; }
  void set_red(float value) { red_ = value; }
  void set_green(float value) { green_ = value; }
  void set_blue(float value) { blue_ = value; }
 private:
  bool on_{false};
  float brightness_{1}, color_brightness_{1}, red_{1}, green_{1}, blue_{1};
};
class LightState;
class LightOutput {
 public:
  virtual ~LightOutput() = default;
  virtual LightTraits get_traits() { return {}; }
  virtual void setup_state(LightState *) {}
  virtual void write_state(LightState *) {}
};
class LightRemoteValuesListener {
 public:
  virtual ~LightRemoteValuesListener() = default;
  virtual void on_light_remote_values_update() = 0;
};
class LightCall {
 public:
  explicit LightCall(LightState *state) : state_(state) {}
  LightCall &set_state(bool value);
  LightCall &set_brightness(float value);
  LightCall &set_transition_length(uint32_t) { return *this; }
  LightCall &set_save(bool) { return *this; }
  void perform();
 private:
  LightState *state_;
};
class LightState {
 public:
  explicit LightState(LightOutput *output) : output_(output) {}
  LightColorValues remote_values, current_values;
  unsigned publications{0};
  LightOutput *get_output() const { return output_; }
  const std::string &get_name() const { return name_; }
  LightCall make_call() { return LightCall(this); }
  void add_remote_values_listener(LightRemoteValuesListener *listener) {
    listeners_.push_back(listener);
  }
  void publish_state() {
    ++publications;
    for (auto *listener : listeners_)
      listener->on_light_remote_values_update();
  }
 private:
  LightOutput *output_;
  std::string name_{"Display"};
  std::vector<LightRemoteValuesListener *> listeners_;
};
inline LightCall &LightCall::set_state(bool value) {
  state_->remote_values.set_state(value);
  return *this;
}
inline LightCall &LightCall::set_brightness(float value) {
  state_->remote_values.set_brightness(value);
  return *this;
}
inline void LightCall::perform() {
  state_->current_values = state_->remote_values;
  state_->publish_state();
}
class AddressableLightState : public LightState {
 public:
  using LightState::LightState;
};
class AddressableLight : public LightOutput, public Component {
 public:
  explicit AddressableLight(size_t size) : pixels(size) {}
  std::vector<Color> pixels;
  unsigned shows{0};
  size_t size() const { return pixels.size(); }
  Color &operator[](size_t index) { return pixels.at(index); }
  struct All {
    AddressableLight *output;
    void operator=(Color color) { std::fill(output->pixels.begin(), output->pixels.end(), color); }
  };
  All all() { return All{this}; }
  void schedule_show() { ++shows; }
  void set_effect_active(bool active) { effect_active_ = active; }
 private:
  bool effect_active_{false};
};
}  // namespace light
namespace switch_ {
class Switch {
 public:
  bool state{false};
  void publish_state(bool value) { state = value; }
  void control(bool value) { write_state(value); }
 protected:
  virtual void write_state(bool value) = 0;
};
}
namespace select {
class Select {
 public:
  size_t index{0};
  void publish_state(size_t value) { index = value; }
  void test_control(size_t value) { control(value); }
 protected:
  virtual void control(size_t value) = 0;
};
}
struct ESPTime {
  bool valid{false};
  bool is_valid() const { return valid; }
  size_t strftime(char *buffer, size_t size, const char *format) {
    return std::snprintf(buffer, size, "%s", std::strcmp(format, "%H:%M") == 0 ? "12:34" : "1234");
  }
};
namespace time {
class RealTimeClock {
 public:
  bool valid{false};
  ESPTime now() { return ESPTime{valid}; }
};
}
}  // namespace esphome

#define ESP_LOGE(...) (++::esphome::test_errors)
#define ESP_LOGW(...) (++::esphome::test_errors)
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGCONFIG(...) ((void) 0)
