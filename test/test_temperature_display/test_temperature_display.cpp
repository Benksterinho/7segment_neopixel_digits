#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>
#include <math.h>
#include <unity.h>

#include "../../src/display/temperature_format.cpp"

namespace {
constexpr char LAYOUT[] = "DD.D.D.";
constexpr uint32_t COLOR = 0x123456;
constexpr uint16_t LED_COUNT = Modular7Segment::requiredLeds(LAYOUT, 4, 1);
static_assert(LED_COUNT == 115, "Four digits and three decimal points");

void assertFormat(float value, const char* expected) {
  char text[6] = {};
  formatTemperature(value, text);
  TEST_ASSERT_EQUAL_STRING(expected, text);
}

void assertDigit(Adafruit_NeoPixel& actual, uint8_t digit, char character) {
  Adafruit_NeoPixel reference(28, 27, NEO_GRB + NEO_KHZ800);
  reference.begin();
  Modular7Segment display(reference, "D", 4, 1);
  const char text[] = {character, '\0'};
  display.printString(text, COLOR);
  const uint16_t offsets[] = {0, 28, 57, 86};
  for (uint16_t i = 0; i < 28; ++i) {
    TEST_ASSERT_EQUAL_HEX32(reference.getPixelColor(i),
                           actual.getPixelColor(offsets[digit] + i));
  }
}
}  // namespace

void test_temperature_formatting() {
  assertFormat(25.7f, " 25.7");
  assertFormat(5.2f, "  5.2");
  assertFormat(-5.2f, " -5.2");
  assertFormat(0.0f, "  0.0");
  assertFormat(-0.0f, "  0.0");
  assertFormat(-0.04f, "  0.0");
  assertFormat(25.74f, " 25.7");
  assertFormat(25.75f, " 25.8");
  assertFormat(-5.25f, " -5.3");
  assertFormat(9.96f, " 10.0");
  assertFormat(-9.9f, " -9.9");
  assertFormat(99.9f, " 99.9");
  assertFormat(-9.91f, " ---");
  assertFormat(99.91f, " ---");
  assertFormat(100.0f, " ---");
  assertFormat(NAN, " ---");
  assertFormat(INFINITY, " ---");
}

void test_temperature_pixels_and_clearing() {
  Adafruit_NeoPixel leds(LED_COUNT, 27, NEO_GRB + NEO_KHZ800);
  leds.begin();
  Modular7Segment display(leds, LAYOUT, 4, 1);
  TEST_ASSERT_TRUE(display.isValid());
  TEST_ASSERT_EQUAL_UINT8(7, display.moduleCount());
  display.printString("88.8.8.", COLOR);
  char text[6] = {};
  formatTemperature(25.7f, text);
  display.printString(text, COLOR);
  assertDigit(leds, 0, ' ');
  assertDigit(leds, 1, '2');
  assertDigit(leds, 2, '5');
  assertDigit(leds, 3, '7');
  TEST_ASSERT_EQUAL_HEX32(0, leds.getPixelColor(56));
  TEST_ASSERT_EQUAL_HEX32(COLOR, leds.getPixelColor(85));
  TEST_ASSERT_EQUAL_HEX32(0, leds.getPixelColor(114));

  formatTemperature(100.0f, text);
  display.printString(text, COLOR);
  assertDigit(leds, 0, ' ');
  for (uint8_t digit = 1; digit < 4; ++digit) {
    assertDigit(leds, digit, '-');
  }
  TEST_ASSERT_EQUAL_HEX32(0, leds.getPixelColor(85));
}

void test_time_and_co2_decimal_regressions() {
  Adafruit_NeoPixel leds(LED_COUNT, 27, NEO_GRB + NEO_KHZ800);
  leds.begin();
  Modular7Segment display(leds, LAYOUT, 4, 1);
  const char* frames[] = {"12.34", "1234", " 750", "9999", "----"};
  for (uint8_t i = 0; i < 5; ++i) {
    display.printString("88.8.8.", COLOR);
    display.printString(frames[i], COLOR);
    TEST_ASSERT_EQUAL_HEX32(i == 0 ? COLOR : 0, leds.getPixelColor(56));
    TEST_ASSERT_EQUAL_HEX32(0, leds.getPixelColor(85));
    TEST_ASSERT_EQUAL_HEX32(0, leds.getPixelColor(114));
  }
}

void setup() {
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_temperature_formatting);
  RUN_TEST(test_temperature_pixels_and_clearing);
  RUN_TEST(test_time_and_co2_decimal_regressions);
  UNITY_END();
}

void loop() {}
