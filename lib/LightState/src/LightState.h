#pragma once

#include <stddef.h>
#include <stdint.h>

struct LightState {
  bool on = true;
  uint8_t brightness = 50;
  uint8_t red = 255;
  uint8_t green = 0;
  uint8_t blue = 0;

  bool operator==(const LightState& other) const {
    return on == other.on && brightness == other.brightness &&
           red == other.red && green == other.green && blue == other.blue;
  }
  bool operator!=(const LightState& other) const { return !(*this == other); }
};

enum class LightCommandResult { Invalid, Unchanged, Changed };

// Applies a Home Assistant JSON-schema light command such as
// {"state":"ON","brightness":128,"color":{"r":0,"g":255,"b":0}}.
// All fields are optional. Brightness is clamped to 1..255 and color channels
// to 0..255. On an invalid payload, the state is left untouched.
LightCommandResult applyLightCommand(LightState& state, const char* payload,
                                     size_t length);

// Writes the Home Assistant JSON-schema state payload. Returns the payload
// length, or 0 if the buffer is too small.
size_t serializeLightState(const LightState& state, char* buffer,
                           size_t bufferSize);

// Persists the state to NVS. Writes are debounced so that rapid changes
// (e.g. dragging a brightness slider) do not wear out flash.
class LightStore {
 public:
  explicit LightStore(uint32_t saveDelayMs = 2000) : saveDelayMs_(saveDelayMs) {}

  void load(LightState& state);
  void markDirty(uint32_t nowMs);
  void loop(uint32_t nowMs, const LightState& state);

 private:
  void save(const LightState& state);

  uint32_t saveDelayMs_;
  uint32_t dirtySinceMs_ = 0;
  bool dirty_ = false;
};
