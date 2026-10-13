#include <Arduino.h>
#include <unity.h>

#include "../../src/sensors/mqtt_ambient_light_sensor.cpp"
#include "../../src/sensors/mqtt_co2_sensor.cpp"

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

void test_co2_color_thresholds() {
  uint8_t r = 0, g = 0, b = 0;

  // Green: < 800
  MqttCo2Sensor::getColorRgb(450, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(0, r);
  TEST_ASSERT_EQUAL_UINT8(255, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);

  MqttCo2Sensor::getColorRgb(799, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(0, r);
  TEST_ASSERT_EQUAL_UINT8(255, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);

  // Yellow: 800 <= ppm < 1200
  MqttCo2Sensor::getColorRgb(800, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(255, r);
  TEST_ASSERT_EQUAL_UINT8(180, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);

  MqttCo2Sensor::getColorRgb(1199, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(255, r);
  TEST_ASSERT_EQUAL_UINT8(180, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);

  // Red: >= 1200
  MqttCo2Sensor::getColorRgb(1200, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(255, r);
  TEST_ASSERT_EQUAL_UINT8(0, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);

  MqttCo2Sensor::getColorRgb(2500, r, g, b);
  TEST_ASSERT_EQUAL_UINT8(255, r);
  TEST_ASSERT_EQUAL_UINT8(0, g);
  TEST_ASSERT_EQUAL_UINT8(0, b);
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
  RUN_TEST(test_co2_color_thresholds);
  RUN_TEST(test_ambient_light_parsing);
  RUN_TEST(test_lux_to_brightness_scaling);
  UNITY_END();
}

void loop() {}
