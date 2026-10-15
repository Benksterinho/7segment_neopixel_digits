#include <Arduino.h>
#include <unity.h>

#include "../../src/sensors/mqtt_ambient_light_sensor.cpp"
#include "../../src/sensors/mqtt_co2_sensor.cpp"
#include "../../src/sensors/mqtt_temperature_sensor.cpp"

namespace {
unsigned int temperatureUpdates = 0;
void onTemperature(float value) {
  (void)value;
  ++temperatureUpdates;
}
}  // namespace

void test_temperature_payload_parsing() {
  const char* valid[] = {"25.7", "-5.2", "0", " +12.34 \r\n", "100.0"};
  const float expected[] = {25.7f, -5.2f, 0.0f, 12.34f, 100.0f};
  for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
    float value = 0.0f;
    TEST_ASSERT_TRUE(MqttTemperatureSensor::parsePayload(
        valid[i], strlen(valid[i]), value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, expected[i], value);
  }
}

void test_temperature_rejects_invalid_payloads() {
  const char* invalid[] = {"", " ", "unknown", "unavailable", "NaN", "inf",
                           "-inf", "25.7C", "25 7", "1e999", "1e-999",
                           "12345678901234567890123456789012"};
  float value = 42.0f;
  for (const char* payload : invalid) {
    TEST_ASSERT_FALSE(MqttTemperatureSensor::parsePayload(
        payload, strlen(payload), value));
    TEST_ASSERT_EQUAL_FLOAT(42.0f, value);
  }
  TEST_ASSERT_FALSE(MqttTemperatureSensor::parsePayload(nullptr, 1, value));
  const char embeddedNull[] = {'2', '5', '\0', '7'};
  TEST_ASSERT_FALSE(MqttTemperatureSensor::parsePayload(
      embeddedNull, sizeof(embeddedNull), value));
}

void test_temperature_retains_last_valid_reading() {
  MqttTemperatureSensor sensor("temperature", onTemperature);
  temperatureUpdates = 0;
  TEST_ASSERT_FALSE(sensor.hasValue());
  const uint8_t valid[] = {'2', '5', '.', '7'};
  const uint8_t invalid[] = {'n', 'a', 'n'};
  TEST_ASSERT_FALSE(sensor.handleMessage("different", valid, sizeof(valid)));
  TEST_ASSERT_TRUE(sensor.handleMessage("temperature", invalid, sizeof(invalid)));
  TEST_ASSERT_FALSE(sensor.hasValue());
  TEST_ASSERT_TRUE(sensor.handleMessage("temperature", valid, sizeof(valid)));
  TEST_ASSERT_TRUE(sensor.hasValue());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.7f, sensor.getCelsius());
  const uint32_t updatedAt = sensor.lastUpdateMs();
  TEST_ASSERT_TRUE(sensor.handleMessage("temperature", invalid, sizeof(invalid)));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.7f, sensor.getCelsius());
  TEST_ASSERT_EQUAL_UINT32(updatedAt, sensor.lastUpdateMs());
  TEST_ASSERT_EQUAL_UINT(1, temperatureUpdates);
}

void test_co2_payload_parsing_integer() {
  uint16_t ppm = 0;
  const char payload[] = "650";
  TEST_ASSERT_TRUE(MqttCo2Sensor::parsePayload(payload, strlen(payload), ppm));
  TEST_ASSERT_EQUAL_UINT16(650, ppm);
}

void test_co2_payload_parsing_float() {
  uint16_t ppm = 0;
  const char payload[] = "799.6";
  TEST_ASSERT_TRUE(MqttCo2Sensor::parsePayload(payload, strlen(payload), ppm));
  TEST_ASSERT_EQUAL_UINT16(800, ppm);
}

void test_co2_payload_parsing_invalid() {
  uint16_t ppm = 0;
  const char payload[] = "invalid_number";
  TEST_ASSERT_FALSE(MqttCo2Sensor::parsePayload(payload, strlen(payload), ppm));
  TEST_ASSERT_FALSE(MqttCo2Sensor::parsePayload(nullptr, 0, ppm));
}

void test_ambient_light_parsing() {
  float lux = 0.0f;
  const char payload[] = "123.45";
  TEST_ASSERT_TRUE(
      MqttAmbientLightSensor::parsePayload(payload, strlen(payload), lux));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 123.45f, lux);

  TEST_ASSERT_FALSE(
      MqttAmbientLightSensor::parsePayload("abc", 3, lux));
  TEST_ASSERT_FALSE(
      MqttAmbientLightSensor::parsePayload(nullptr, 0, lux));
}

void test_lux_to_brightness_scaling() {
  // 0 lux -> minBrightness (15)
  TEST_ASSERT_EQUAL_UINT8(15, MqttAmbientLightSensor::luxToBrightness(0.0f));
  TEST_ASSERT_EQUAL_UINT8(15, MqttAmbientLightSensor::luxToBrightness(-10.0f));

  // Max lux reference (500 lux) -> 255
  TEST_ASSERT_EQUAL_UINT8(255, MqttAmbientLightSensor::luxToBrightness(500.0f));
  TEST_ASSERT_EQUAL_UINT8(255, MqttAmbientLightSensor::luxToBrightness(1000.0f));

  // Mid levels: monotonic increase between 15 and 255
  uint8_t dim = MqttAmbientLightSensor::luxToBrightness(5.0f);
  uint8_t room = MqttAmbientLightSensor::luxToBrightness(50.0f);
  uint8_t bright = MqttAmbientLightSensor::luxToBrightness(200.0f);

  TEST_ASSERT_GREATER_THAN(15, dim);
  TEST_ASSERT_GREATER_THAN(dim, room);
  TEST_ASSERT_GREATER_THAN(room, bright);
  TEST_ASSERT_LESS_OR_EQUAL(255, bright);
}

void setup() {
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_co2_payload_parsing_integer);
  RUN_TEST(test_co2_payload_parsing_float);
  RUN_TEST(test_co2_payload_parsing_invalid);
  RUN_TEST(test_ambient_light_parsing);
  RUN_TEST(test_lux_to_brightness_scaling);
  RUN_TEST(test_temperature_payload_parsing);
  RUN_TEST(test_temperature_rejects_invalid_payloads);
  RUN_TEST(test_temperature_retains_last_valid_reading);
  UNITY_END();
}

void loop() {}
