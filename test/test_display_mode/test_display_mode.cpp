#include <Arduino.h>
#include <unity.h>

#include "../../src/display/display_mode_controller.cpp"
#include "../../src/mqtt/mqtt_select_module.cpp"

namespace {
constexpr uint32_t INTERVAL_MS = 30000;
const char* const OPTIONS[] = {"Time", "CO2", "Temperature"};
}  // namespace

void test_auto_cycles_when_co2_available() {
  DisplayModeController c(INTERVAL_MS);
  c.begin(true, DisplayMode::CO2, 0);
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);

  TEST_ASSERT_FALSE(c.loop(INTERVAL_MS - 1, true, false));
  TEST_ASSERT_TRUE(c.loop(INTERVAL_MS, true, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::CO2);
  TEST_ASSERT_TRUE(c.loop(2 * INTERVAL_MS, true, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
}

void test_auto_stays_on_time_without_co2() {
  DisplayModeController c(INTERVAL_MS);
  c.begin(true, DisplayMode::TIME, 0);
  TEST_ASSERT_FALSE(c.loop(INTERVAL_MS, false, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
}

void test_manual_mode_is_pinned() {
  DisplayModeController c(INTERVAL_MS);
  c.begin(false, DisplayMode::CO2, 0);
  TEST_ASSERT_FALSE(c.isAuto());
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::CO2);
  TEST_ASSERT_FALSE(c.loop(5 * INTERVAL_MS, true, true));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::CO2);
}

void test_select_disables_cycling_and_auto_resumes() {
  DisplayModeController c(INTERVAL_MS);
  c.begin(true, DisplayMode::TIME, 0);
  c.setAuto(false, 1000);
  c.selectMode(DisplayMode::CO2, 1000);
  TEST_ASSERT_FALSE(c.loop(10 * INTERVAL_MS, true, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::CO2);

  const uint32_t resumeAt = 20 * INTERVAL_MS;
  c.setAuto(true, resumeAt);
  TEST_ASSERT_FALSE(c.loop(resumeAt + INTERVAL_MS - 1, true, false));
  TEST_ASSERT_TRUE(c.loop(resumeAt + INTERVAL_MS, true, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
}

void test_select_option_parsing() {
  uint8_t index = 99;
  TEST_ASSERT_TRUE(MqttSelectModule::parseOption("Time", 4, OPTIONS, 3, index));
  TEST_ASSERT_EQUAL_UINT8(0, index);
  TEST_ASSERT_TRUE(MqttSelectModule::parseOption("co2", 3, OPTIONS, 3, index));
  TEST_ASSERT_EQUAL_UINT8(1, index);
  TEST_ASSERT_TRUE(
      MqttSelectModule::parseOption("temperature", 11, OPTIONS, 3, index));
  TEST_ASSERT_EQUAL_UINT8(2, index);
  TEST_ASSERT_FALSE(MqttSelectModule::parseOption("Date", 4, OPTIONS, 3, index));
  TEST_ASSERT_FALSE(MqttSelectModule::parseOption("Time", 3, OPTIONS, 3, index));
  TEST_ASSERT_FALSE(MqttSelectModule::parseOption(nullptr, 0, OPTIONS, 3, index));
}

void test_three_mode_cycle_and_sensor_availability() {
  for (uint8_t mask = 0; mask < 4; ++mask) {
    DisplayModeController c(INTERVAL_MS);
    c.begin(true, DisplayMode::TIME, 0);
    const bool co2 = (mask & 1) != 0;
    const bool temperature = (mask & 2) != 0;
    uint32_t now = INTERVAL_MS;
    if (co2) {
      TEST_ASSERT_TRUE(c.loop(now, co2, temperature));
      TEST_ASSERT_TRUE(c.mode() == DisplayMode::CO2);
      now += INTERVAL_MS;
    }
    if (temperature) {
      TEST_ASSERT_TRUE(c.loop(now, co2, temperature));
      TEST_ASSERT_TRUE(c.mode() == DisplayMode::TEMPERATURE);
      now += INTERVAL_MS;
    }
    TEST_ASSERT_EQUAL(mask != 0, c.loop(now, co2, temperature));
    TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
  }
}

void test_temperature_manual_resume_and_rollover() {
  static_assert(static_cast<uint8_t>(DisplayMode::TIME) == 0, "Time index");
  static_assert(static_cast<uint8_t>(DisplayMode::CO2) == 1, "CO2 index");
  static_assert(static_cast<uint8_t>(DisplayMode::TEMPERATURE) == 2,
                "Temperature index");
  DisplayModeController c(INTERVAL_MS);
  c.begin(false, DisplayMode::TEMPERATURE, 0);
  TEST_ASSERT_FALSE(c.loop(INTERVAL_MS, false, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TEMPERATURE);
  c.selectMode(DisplayMode::TIME, INTERVAL_MS);
  c.selectMode(DisplayMode::TEMPERATURE, INTERVAL_MS);
  TEST_ASSERT_FALSE(c.loop(2 * INTERVAL_MS, true, true));
  const uint32_t resumeAt = UINT32_MAX - 1000;
  c.setAuto(true, resumeAt);
  TEST_ASSERT_FALSE(c.loop(resumeAt + INTERVAL_MS - 1, true, true));
  TEST_ASSERT_TRUE(c.loop(resumeAt + INTERVAL_MS, true, true));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
}

void test_availability_changes_during_cycle() {
  DisplayModeController c(INTERVAL_MS);
  c.begin(true, DisplayMode::TIME, 0);
  TEST_ASSERT_TRUE(c.loop(INTERVAL_MS, true, true));
  TEST_ASSERT_TRUE(c.loop(2 * INTERVAL_MS, false, true));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TEMPERATURE);
  TEST_ASSERT_TRUE(c.loop(3 * INTERVAL_MS, false, false));
  TEST_ASSERT_TRUE(c.mode() == DisplayMode::TIME);
}

void setup() {
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_auto_cycles_when_co2_available);
  RUN_TEST(test_auto_stays_on_time_without_co2);
  RUN_TEST(test_manual_mode_is_pinned);
  RUN_TEST(test_select_disables_cycling_and_auto_resumes);
  RUN_TEST(test_select_option_parsing);
  RUN_TEST(test_three_mode_cycle_and_sensor_availability);
  RUN_TEST(test_temperature_manual_resume_and_rollover);
  RUN_TEST(test_availability_changes_during_cycle);
  UNITY_END();
}

void loop() {}
