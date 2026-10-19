#include "modular_7segment.h"

#include <algorithm>
#include <cmath>
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::modular_7segment {
namespace {
constexpr const char *TAG = "modular_7segment";

uint8_t channel(float value) {
  return static_cast<uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}
}  // namespace

light::LightTraits Modular7Segment::get_traits() {
  light::LightTraits traits;
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void Modular7Segment::setup_state(light::LightState *state) {
  this->control_state_ = state;
  state->add_remote_values_listener(this);
}

void Modular7Segment::set_layout(const std::string &layout, uint8_t per_segment,
                                 uint8_t per_dot, uint8_t dot_percent) {
  this->renderer_.configure(layout.c_str(), per_segment, per_dot, dot_percent);
}

void Modular7Segment::setup() {
  this->strip_ = static_cast<light::AddressableLight *>(this->strip_state_->get_output());
  if (this->strip_->is_failed()) {
    ESP_LOGE(TAG, "Physical LED output failed to initialize");
    this->mark_failed();
    return;
  }
  this->strip_->set_effect_active(true);
  this->strip_state_->make_call().set_state(true).set_brightness(1.0f)
      .set_transition_length(0).set_save(false).perform();
  this->strip_->all() = Color::BLACK;
  this->strip_->schedule_show();
  if (!this->renderer_.valid() || this->renderer_.digit_count() != 4 ||
      this->renderer_.led_count() != this->strip_->size()) {
    ESP_LOGE(TAG, "Invalid layout or physical LED count mismatch");
    this->mark_failed();
    return;
  }

  this->preference_ = global_preferences->make_preference<SavedSettings>(
      fnv1_hash("modular_7segment.v1." + std::string(this->control_state_->get_name().c_str())));
  SavedSettings saved{};
  if (this->preference_.load(&saved)) {
    if (!this->restore_settings_(saved)) {
      ESP_LOGE(TAG, "Stored display settings are invalid");
      this->mark_failed();
      return;
    }
  } else {
    ESP_LOGD(TAG, "No saved ESPHome display settings; using initial defaults");
  }

  this->frame_.resize(this->renderer_.led_count());
  this->policy_.begin(millis());
  this->ready_ = true;
  this->publish_controls_();
  this->render_();
}

void Modular7Segment::dump_config() {
  ESP_LOGCONFIG(TAG, "Modular seven-segment display: %zu LEDs; isolated ESPHome renderer",
                this->renderer_.led_count());
}

void Modular7Segment::on_light_remote_values_update() {
  if (!this->ready_ || this->publishing_)
    return;
  const auto &values = this->control_state_->remote_values;
  if (!std::isfinite(values.get_brightness()) ||
      !std::isfinite(values.get_color_brightness()) ||
      !std::isfinite(values.get_red()) || !std::isfinite(values.get_green()) ||
      !std::isfinite(values.get_blue())) {
    ESP_LOGW(TAG, "Ignoring non-finite light control values");
    this->publish_controls_();
    return;
  }
  const float color_brightness = values.get_color_brightness();
  const Rgb color{channel(values.get_red() * color_brightness),
                  channel(values.get_green() * color_brightness),
                  channel(values.get_blue() * color_brightness)};
  if (this->policy_.light_command(values.is_on(), color, this->reported_brightness_,
                                 channel(values.get_brightness()))) {
    this->settings_changed_();
  }
  this->publish_controls_();
  this->dirty_ = true;
}

void Modular7Segment::publish_controls_() {
  const auto &settings = this->policy_.settings;
  this->publishing_ = true;
  this->auto_brightness_->publish_state(settings.auto_brightness);
  this->auto_mode_->publish_state(settings.auto_mode);
  this->mode_select_->publish_state(static_cast<size_t>(settings.manual_mode));
  this->reported_brightness_ = this->policy_.effective_brightness();
  auto &values = this->control_state_->remote_values;
  values.set_state(settings.on);
  values.set_color_mode(light::ColorMode::RGB);
  values.set_brightness(this->reported_brightness_ / 255.0f);
  values.set_color_brightness(1.0f);
  values.set_red(settings.color.red / 255.0f);
  values.set_green(settings.color.green / 255.0f);
  values.set_blue(settings.color.blue / 255.0f);
  this->control_state_->current_values = values;
  this->control_state_->publish_state();
  this->publishing_ = false;
}

void Modular7Segment::set_auto_brightness(bool enabled) {
  if (!this->ready_)
    return;
  if (this->policy_.settings.auto_brightness != enabled) {
    this->policy_.settings.auto_brightness = enabled;
    this->settings_changed_();
  }
  this->publish_controls_();
  this->dirty_ = true;
}

void Modular7Segment::set_auto_mode(bool enabled) {
  if (!this->ready_)
    return;
  const bool changed = this->policy_.settings.auto_mode != enabled;
  this->policy_.set_auto_mode(enabled, millis());
  if (changed)
    this->settings_changed_();
  this->publish_controls_();
  this->dirty_ = true;
}

void Modular7Segment::select_mode(size_t index) {
  if (!this->ready_)
    return;
  if (index >= 3) {
    ESP_LOGW(TAG, "Invalid display mode index: %u", static_cast<unsigned>(index));
    return;
  }
  const auto before = this->encode_settings_();
  this->policy_.select_mode(static_cast<uint8_t>(index), millis());
  if (before != this->encode_settings_())
    this->settings_changed_();
  this->publish_controls_();
  this->dirty_ = true;
}

void Modular7Segment::receive_co2(const std::string &payload) {
  double value;
  if (!parse_numeric_payload(payload, value) || !this->policy_.accept_co2(value)) {
    ESP_LOGW(TAG, "Ignoring invalid CO2 payload");
    return;
  }
  this->dirty_ = true;
}

void Modular7Segment::receive_temperature(const std::string &payload) {
  double value;
  if (!parse_numeric_payload(payload, value) || !this->policy_.accept_temperature(value)) {
    ESP_LOGW(TAG, "Ignoring invalid temperature payload");
    return;
  }
  this->dirty_ = true;
}

void Modular7Segment::receive_lux(const std::string &payload) {
  double value;
  if (!parse_numeric_payload(payload, value) || !this->policy_.accept_lux(value)) {
    ESP_LOGW(TAG, "Ignoring invalid ambient-light payload");
    return;
  }
  if (this->ready_ && this->reported_brightness_ != this->policy_.effective_brightness())
    this->publish_controls_();
  this->dirty_ = true;
}

void Modular7Segment::loop() {
  if (!this->ready_)
    return;
  const uint32_t now = millis();
  if (this->policy_.tick(now))
    this->dirty_ = true;
  if (now - this->last_refresh_ >= 1000) {
    this->last_refresh_ = now;
    this->dirty_ = true;
  }
  if (this->dirty_)
    this->render_();
  if (this->save_pending_ && now - this->last_change_ >= 2000)
    this->save_();
}

void Modular7Segment::render_() {
  char time_text[6] = "----";
  auto now = this->clock_->now();
  if (now.is_valid())
    now.strftime(time_text, sizeof(time_text), (millis() / 1000) % 2 == 0 ? "%H:%M" : "%H%M");
  char text[6]{};
  Rgb color{};
  this->policy_.frame(time_text, text, color);
  this->renderer_.render(text, color,
                        this->policy_.settings.on ? this->policy_.effective_brightness() : 0,
                        this->frame_);
  for (size_t i = 0; i < this->frame_.size(); ++i) {
    const auto &pixel = this->frame_[i];
    (*this->strip_)[i] = Color(pixel.red, pixel.green, pixel.blue);
  }
  this->strip_->schedule_show();
  this->dirty_ = false;
}

Modular7Segment::SavedSettings Modular7Segment::encode_settings_() const {
  const auto &s = this->policy_.settings;
  return {1, static_cast<uint8_t>(s.on), s.manual_brightness, s.color.red,
          s.color.green, s.color.blue, static_cast<uint8_t>(s.auto_brightness),
          static_cast<uint8_t>(s.auto_mode), s.manual_mode, 0};
}

bool Modular7Segment::restore_settings_(const SavedSettings &saved) {
  if (saved[0] != 1 || saved[1] > 1 || saved[2] == 0 ||
      saved[6] > 1 || saved[7] > 1 || saved[8] >= 3 || saved[9] != 0)
    return false;
  auto &s = this->policy_.settings;
  s.on = saved[1];
  s.manual_brightness = saved[2];
  s.color = {saved[3], saved[4], saved[5]};
  s.auto_brightness = saved[6];
  s.auto_mode = saved[7];
  s.manual_mode = saved[8];
  return true;
}

void Modular7Segment::settings_changed_() {
  this->save_pending_ = true;
  this->last_change_ = millis();
}

void Modular7Segment::save_() {
  const auto saved = this->encode_settings_();
  if (!this->preference_.save(&saved)) {
    ESP_LOGE(TAG, "Failed to save display settings");
    this->last_change_ = millis();
    return;
  }
  this->save_pending_ = false;
}

void Modular7Segment::on_shutdown() {
  if (this->save_pending_) {
    this->save_();
    if (!global_preferences->sync())
      ESP_LOGE(TAG, "Failed to flush display settings before shutdown");
  }
}

}  // namespace esphome::modular_7segment
