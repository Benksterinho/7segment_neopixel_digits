#include <Arduino.h>
#include <LightState.h>
#include <string.h>
#include <unity.h>

// Validates the MQTT/Home Assistant light command parsing. No LEDs needed.
//
// Run with: pio test -e wemos_d1_mini32 -f test_light_command

namespace {

LightCommandResult apply(LightState& state, const char* payload) {
  return applyLightCommand(state, payload, strlen(payload));
}

void assertState(const LightState& state, bool on, uint8_t brightness,
                 uint8_t red, uint8_t green, uint8_t blue) {
  TEST_ASSERT_EQUAL(on, state.on);
  TEST_ASSERT_EQUAL_UINT8(brightness, state.brightness);
  TEST_ASSERT_EQUAL_UINT8(red, state.red);
  TEST_ASSERT_EQUAL_UINT8(green, state.green);
  TEST_ASSERT_EQUAL_UINT8(blue, state.blue);
}

}  // namespace

void test_turn_off_keeps_brightness_and_color() {
  LightState state;
  TEST_ASSERT_EQUAL(LightCommandResult::Changed,
                    apply(state, R"({"state":"OFF"})"));
  assertState(state, false, 50, 255, 0, 0);
}

void test_partial_brightness_update() {
  LightState state;
  TEST_ASSERT_EQUAL(LightCommandResult::Changed,
                    apply(state, R"({"state":"ON","brightness":128})"));
  assertState(state, true, 128, 255, 0, 0);
}

void test_brightness_is_clamped() {
  LightState state;
  apply(state, R"({"brightness":0})");
  TEST_ASSERT_EQUAL_UINT8(1, state.brightness);
  apply(state, R"({"brightness":999})");
  TEST_ASSERT_EQUAL_UINT8(255, state.brightness);
}

void test_color_update_and_clamp() {
  LightState state;
  TEST_ASSERT_EQUAL(
      LightCommandResult::Changed,
      apply(state, R"({"state":"ON","color":{"r":-5,"g":300,"b":12}})"));
  assertState(state, true, 50, 0, 255, 12);
}

void test_partial_color_keeps_other_channels() {
  LightState state;
  apply(state, R"({"color":{"g":10}})");
  assertState(state, true, 50, 255, 10, 0);
}

void test_updates_without_on_command_keep_light_off() {
  LightState state;
  apply(state, R"({"state":"OFF"})");
  TEST_ASSERT_EQUAL(LightCommandResult::Changed,
                    apply(state, R"({"brightness":128})"));
  assertState(state, false, 128, 255, 0, 0);
  TEST_ASSERT_EQUAL(LightCommandResult::Changed,
                    apply(state, R"({"color":{"g":10}})"));
  assertState(state, false, 128, 255, 10, 0);
  TEST_ASSERT_EQUAL(LightCommandResult::Unchanged,
                    apply(state, R"({"state":"OFF"})"));
  TEST_ASSERT_FALSE(state.on);
  TEST_ASSERT_EQUAL(LightCommandResult::Changed,
                    apply(state, R"({"state":"ON"})"));
  assertState(state, true, 128, 255, 10, 0);
}

void test_same_state_is_unchanged() {
  LightState state;
  TEST_ASSERT_EQUAL(LightCommandResult::Unchanged,
                    apply(state, R"({"state":"ON","brightness":50})"));
}

void test_invalid_payloads_leave_state_untouched() {
  const char* payloads[] = {
      "not json",
      R"(["ON"])",
      R"({"state":"MAYBE"})",
      R"({"state":1})",
      R"({"brightness":"high"})",
      R"({"color":"red"})",
      R"({"state":"OFF","color":{"r":"x"}})",
  };
  for (const char* payload : payloads) {
    LightState state;
    TEST_ASSERT_EQUAL_MESSAGE(LightCommandResult::Invalid,
                              apply(state, payload), payload);
    assertState(state, true, 50, 255, 0, 0);
  }
  LightState state;
  TEST_ASSERT_EQUAL(LightCommandResult::Invalid,
                    applyLightCommand(state, nullptr, 0));
}

void test_serialize_state_round_trips() {
  LightState original;
  original.on = false;
  original.brightness = 77;
  original.red = 1;
  original.green = 2;
  original.blue = 3;

  char buffer[160];
  const size_t length = serializeLightState(original, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN(0, length);
  TEST_ASSERT_NOT_NULL(strstr(buffer, R"("color_mode":"rgb")"));

  LightState parsed;
  apply(parsed, buffer);
  TEST_ASSERT_TRUE(parsed == original);

  char tiny[8];
  TEST_ASSERT_EQUAL(0, serializeLightState(original, tiny, sizeof(tiny)));
}

void setup() {
  delay(2000);  // Give the serial monitor time to attach.
  UNITY_BEGIN();
  RUN_TEST(test_turn_off_keeps_brightness_and_color);
  RUN_TEST(test_partial_brightness_update);
  RUN_TEST(test_brightness_is_clamped);
  RUN_TEST(test_color_update_and_clamp);
  RUN_TEST(test_partial_color_keeps_other_channels);
  RUN_TEST(test_updates_without_on_command_keep_light_off);
  RUN_TEST(test_same_state_is_unchanged);
  RUN_TEST(test_invalid_payloads_leave_state_untouched);
  RUN_TEST(test_serialize_state_round_trips);
  UNITY_END();
}

void loop() {}
