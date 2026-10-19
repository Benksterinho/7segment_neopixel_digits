#pragma once

#include <array>
#include <string>
#include <vector>

#include "display_policy.h"
#include "segment_renderer.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::modular_7segment {

class Modular7Segment : public Component,
                        public light::LightOutput,
                        public light::LightRemoteValuesListener {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override { this->dirty_ = true; }
  void on_light_remote_values_update() override;

  void set_strip_id(light::AddressableLightState *strip) { this->strip_state_ = strip; }
  void set_time_id(time::RealTimeClock *clock) { this->clock_ = clock; }
  void set_auto_brightness_id(switch_::Switch *value) { this->auto_brightness_ = value; }
  void set_auto_mode_id(switch_::Switch *value) { this->auto_mode_ = value; }
  void set_mode_select_id(select::Select *value) { this->mode_select_ = value; }
  void set_layout(const std::string &layout, uint8_t per_segment, uint8_t per_dot, uint8_t dot_percent);
  void set_temperature_offset(float offset) { this->policy_.set_temperature_offset(offset); }

  void set_auto_brightness(bool enabled);
  void set_auto_mode(bool enabled);
  void select_mode(size_t index);
  void receive_co2(const std::string &payload);
  void receive_temperature(const std::string &payload);
  void receive_lux(const std::string &payload);

 protected:
  using SavedSettings = std::array<uint8_t, 10>;
  void render_();
  void publish_controls_();
  void settings_changed_();
  void save_();
  SavedSettings encode_settings_() const;
  bool restore_settings_(const SavedSettings &saved);

  SegmentRenderer renderer_{nullptr, 4, 1, 100};
  DisplayPolicy policy_;
  std::vector<Rgb> frame_;
  light::LightState *control_state_{nullptr};
  light::AddressableLightState *strip_state_{nullptr};
  light::AddressableLight *strip_{nullptr};
  time::RealTimeClock *clock_{nullptr};
  switch_::Switch *auto_brightness_{nullptr};
  switch_::Switch *auto_mode_{nullptr};
  select::Select *mode_select_{nullptr};
  ESPPreferenceObject preference_;
  uint32_t last_refresh_{0};
  uint32_t last_change_{0};
  uint8_t reported_brightness_{50};
  bool ready_{false};
  bool publishing_{false};
  bool dirty_{true};
  bool save_pending_{false};
};

class ClockSwitch : public switch_::Switch {
 public:
  ClockSwitch(Modular7Segment *parent, bool brightness)
      : parent_(parent), brightness_(brightness) {}

 protected:
  void write_state(bool state) override {
    if (this->brightness_)
      this->parent_->set_auto_brightness(state);
    else
      this->parent_->set_auto_mode(state);
  }
  Modular7Segment *parent_;
  bool brightness_;
};

class ClockSelect : public select::Select {
 public:
  explicit ClockSelect(Modular7Segment *parent) : parent_(parent) {}

 protected:
  void control(size_t index) override { this->parent_->select_mode(index); }
  Modular7Segment *parent_;
};

}  // namespace esphome::modular_7segment
