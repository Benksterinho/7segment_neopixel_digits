#include "LightState.h"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <strings.h>

namespace {

constexpr char PREFS_NAMESPACE[] = "light";
constexpr char KEY_ON[] = "on";
constexpr char KEY_BRIGHTNESS[] = "bri";
constexpr char KEY_RGB[] = "rgb";

uint8_t clampToByte(long value, long minimum) {
  if (value < minimum) {
    return static_cast<uint8_t>(minimum);
  }
  if (value > 255) {
    return 255;
  }
  return static_cast<uint8_t>(value);
}

bool readNumber(JsonVariantConst value, long& out) {
  if (value.is<long>()) {
    out = value.as<long>();
    return true;
  }
  if (value.is<float>()) {
    out = static_cast<long>(value.as<float>() + 0.5f);
    return true;
  }
  return false;
}

}  // namespace

LightCommandResult applyLightCommand(LightState& state, const char* payload,
                                     size_t length) {
  if (payload == nullptr) {
    return LightCommandResult::Invalid;
  }

  JsonDocument doc;
  if (deserializeJson(doc, payload, length) ||
      !doc.is<JsonObjectConst>()) {
    return LightCommandResult::Invalid;
  }

  LightState next = state;

  JsonVariantConst stateValue = doc["state"];
  if (!stateValue.isNull()) {
    const char* text = stateValue.as<const char*>();
    if (text == nullptr) {
      return LightCommandResult::Invalid;
    }
    if (strcasecmp(text, "ON") == 0) {
      next.on = true;
    } else if (strcasecmp(text, "OFF") == 0) {
      next.on = false;
    } else {
      return LightCommandResult::Invalid;
    }
  }

  JsonVariantConst brightnessValue = doc["brightness"];
  if (!brightnessValue.isNull()) {
    long brightness = 0;
    if (!readNumber(brightnessValue, brightness)) {
      return LightCommandResult::Invalid;
    }
    // Brightness 0 would make "on" indistinguishable from "off".
    next.brightness = clampToByte(brightness, 1);
  }

  JsonVariantConst colorValue = doc["color"];
  if (!colorValue.isNull()) {
    if (!colorValue.is<JsonObjectConst>()) {
      return LightCommandResult::Invalid;
    }
    long channels[3] = {next.red, next.green, next.blue};
    const char* keys[3] = {"r", "g", "b"};
    for (uint8_t index = 0; index < 3; ++index) {
      JsonVariantConst channel = colorValue[keys[index]];
      if (!channel.isNull() && !readNumber(channel, channels[index])) {
        return LightCommandResult::Invalid;
      }
    }
    next.red = clampToByte(channels[0], 0);
    next.green = clampToByte(channels[1], 0);
    next.blue = clampToByte(channels[2], 0);
  }

  if (next == state) {
    return LightCommandResult::Unchanged;
  }
  state = next;
  return LightCommandResult::Changed;
}

size_t serializeLightState(const LightState& state, char* buffer,
                           size_t bufferSize) {
  JsonDocument doc;
  doc["state"] = state.on ? "ON" : "OFF";
  doc["brightness"] = state.brightness;
  doc["color_mode"] = "rgb";
  JsonObject color = doc["color"].to<JsonObject>();
  color["r"] = state.red;
  color["g"] = state.green;
  color["b"] = state.blue;

  if (buffer == nullptr || measureJson(doc) + 1 > bufferSize) {
    return 0;
  }
  return serializeJson(doc, buffer, bufferSize);
}

void LightStore::load(LightState& state) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, true)) {
    return;  // Namespace does not exist yet: keep defaults.
  }
  state.on = prefs.getBool(KEY_ON, state.on);
  state.brightness = prefs.getUChar(KEY_BRIGHTNESS, state.brightness);
  if (state.brightness == 0) {
    state.brightness = 1;
  }
  const uint32_t defaultRgb = (static_cast<uint32_t>(state.red) << 16) |
                              (static_cast<uint32_t>(state.green) << 8) |
                              state.blue;
  const uint32_t rgb = prefs.getUInt(KEY_RGB, defaultRgb);
  state.red = static_cast<uint8_t>(rgb >> 16);
  state.green = static_cast<uint8_t>(rgb >> 8);
  state.blue = static_cast<uint8_t>(rgb);
  prefs.end();
}

void LightStore::markDirty(uint32_t nowMs) {
  dirty_ = true;
  dirtySinceMs_ = nowMs;
}

void LightStore::loop(uint32_t nowMs, const LightState& state) {
  if (!dirty_ || nowMs - dirtySinceMs_ < saveDelayMs_) {
    return;
  }
  dirty_ = false;
  save(state);
}

void LightStore::save(const LightState& state) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false)) {
    return;
  }
  prefs.putBool(KEY_ON, state.on);
  prefs.putUChar(KEY_BRIGHTNESS, state.brightness);
  prefs.putUInt(KEY_RGB, (static_cast<uint32_t>(state.red) << 16) |
                             (static_cast<uint32_t>(state.green) << 8) |
                             state.blue);
  prefs.end();
}
