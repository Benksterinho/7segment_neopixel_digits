#include <Arduino.h>
#include <FastLED.h>
#include <Modular7Segment.h>
#include <unity.h>

// Unity's TEST_ASSERT_EQUAL compares integers only; CRGB needs operator==.
#define TEST_ASSERT_CRGB(expected, actual) \
  TEST_ASSERT_TRUE_MESSAGE(((actual) == (expected)), "CRGB mismatch")

// ---------------------------------------------------------------------------
// Compile-time requiredLeds() checks
// ---------------------------------------------------------------------------

// Backward compatibility: bare module characters use the constructor defaults.
static_assert(Modular7Segment::requiredLeds("D", 3, 1) == 21,
              "single digit with defaults");
static_assert(Modular7Segment::requiredLeds("DD:DD", 3, 1) == 86,
              "clock layout with defaults");
static_assert(Modular7Segment::requiredLeds("DD.DD", 3, 1) == 85,
              "decimal-point layout with defaults");
static_assert(Modular7Segment::requiredLeds("", 3, 1) == 0, "empty layout");
static_assert(Modular7Segment::requiredLeds(nullptr, 3, 1) == 0,
              "null layout");

// Mixed inline counts.
static_assert(Modular7Segment::requiredLeds("D4D2", 1, 1) == 42,
              "mixed digit sizes");
static_assert(Modular7Segment::requiredLeds("D4D4:D2D2", 4, 1) == 86,
              "clock layout with inline digit sizes");
static_assert(Modular7Segment::requiredLeds("D3:2D3", 3, 1) == 46,
              "inline colon size");
static_assert(Modular7Segment::requiredLeds("D3.1D2", 3, 1) == 36,
              "inline decimal-point size");
static_assert(Modular7Segment::requiredLeds("D9", 1, 1) == 63,
              "largest single-digit count");

// Invalid inline counts evaluate to 0 (static_assert on NUM_LEDS catches them).
static_assert(Modular7Segment::requiredLeds("D0", 3, 1) == 0,
              "zero inline count invalid");
static_assert(Modular7Segment::requiredLeds("D256", 3, 1) == 0,
              "count above 255 invalid");
static_assert(Modular7Segment::requiredLeds("D1234", 3, 1) == 0,
              "more than 3 digits invalid");
static_assert(Modular7Segment::requiredLeds(":0", 3, 1) == 0,
              "zero colon count invalid");
static_assert(Modular7Segment::requiredLeds(".0", 3, 1) == 0,
              "zero point count invalid");

// ---------------------------------------------------------------------------
// Runtime checks
// ---------------------------------------------------------------------------

constexpr char MIXED_LAYOUT[] = "D2D3";
constexpr uint16_t MIXED_LEDS =
    Modular7Segment::requiredLeds(MIXED_LAYOUT, 2, 1);
static_assert(MIXED_LEDS == 35, "mixed layout size");

// Module boundaries in "D2D3": module 0 covers LEDs 0..13 (7 x 2),
// module 1 covers LEDs 14..34 (7 x 3).
constexpr uint16_t MIXED_MODULE1_OFFSET = 14;

void testMixedDigitRendering() {
  CRGB leds[MIXED_LEDS];
  fill_solid(leds, MIXED_LEDS, CRGB::Black);

  Modular7Segment display(leds, MIXED_LAYOUT, 2, 1);
  TEST_ASSERT_TRUE(display.isValid());
  TEST_ASSERT_EQUAL_UINT8(2, display.moduleCount());
  TEST_ASSERT_EQUAL_UINT16(MIXED_LEDS, display.requiredLeds());

  // '8' lights every segment of the 3-pixel-per-segment digit.
  display.setModule(1, '8', CRGB::Red);
  for (uint16_t led = 0; led < MIXED_MODULE1_OFFSET; ++led) {
    TEST_ASSERT_CRGB(CRGB::Black, leds[led]);
  }
  for (uint16_t led = MIXED_MODULE1_OFFSET; led < MIXED_LEDS; ++led) {
    TEST_ASSERT_CRGB(CRGB::Red, leds[led]);
  }

  // '1' on the 2-pixel-per-segment digit lights only the right segments:
  // UPPER_RIGHT (index 5) and LOWER_RIGHT (index 6).
  display.setModule(0, '1', CRGB::Green);
  for (uint16_t led = 0; led < MIXED_MODULE1_OFFSET; ++led) {
    const bool lit = led >= 10;  // segments 5 and 6 start at LED 10
    TEST_ASSERT_CRGB(lit ? CRGB::Green : CRGB::Black, leds[led]);
  }
  // Module 1 must remain untouched by writes to module 0.
  for (uint16_t led = MIXED_MODULE1_OFFSET; led < MIXED_LEDS; ++led) {
    TEST_ASSERT_CRGB(CRGB::Red, leds[led]);
  }

  // Switching to a blank clears only the target module.
  display.setModule(0, ' ', CRGB::Green);
  for (uint16_t led = 0; led < MIXED_MODULE1_OFFSET; ++led) {
    TEST_ASSERT_CRGB(CRGB::Black, leds[led]);
  }
  for (uint16_t led = MIXED_MODULE1_OFFSET; led < MIXED_LEDS; ++led) {
    TEST_ASSERT_CRGB(CRGB::Red, leds[led]);
  }
}

void testMixedClockLayoutPrintString() {
  constexpr char CLOCK_LAYOUT[] = "D4D4:D2D2";
  constexpr uint16_t CLOCK_LEDS =
      Modular7Segment::requiredLeds(CLOCK_LAYOUT, 4, 1);
  static_assert(CLOCK_LEDS == 86, "mixed clock layout size");

  // Offsets: modules at 0, 28, 56, 58, 72. Module sizes 28, 28, 2, 14, 14.
  CRGB leds[CLOCK_LEDS];
  fill_solid(leds, CLOCK_LEDS, CRGB::Black);

  Modular7Segment display(leds, CLOCK_LAYOUT, 4, 1);
  TEST_ASSERT_TRUE(display.isValid());
  TEST_ASSERT_EQUAL_UINT8(5, display.moduleCount());

  display.printString("12:34", CRGB::Green);

  // Colon (2 LEDs with the default pixels-per-dot) is lit.
  TEST_ASSERT_CRGB(CRGB::Green, leds[56]);
  TEST_ASSERT_CRGB(CRGB::Green, leds[57]);

  // Last digit '4' (2 px/segment) lights MIDDLE, UPPER_LEFT, UPPER_RIGHT,
  // LOWER_RIGHT -> LEDs 76..79 and 82..85 relative to module offset 72.
  for (uint16_t led = 72; led < 86; ++led) {
    const uint16_t rel = led - 72;
    const bool lit = (rel >= 4 && rel <= 7) || (rel >= 10 && rel <= 13);
    TEST_ASSERT_CRGB(lit ? CRGB::Green : CRGB::Black, leds[led]);
  }
}

void testSentinelsUntouched() {
  CRGB leds[MIXED_LEDS + 2];
  fill_solid(leds, MIXED_LEDS + 2, CRGB::Blue);

  Modular7Segment display(leds, MIXED_LAYOUT, 2, 1);
  TEST_ASSERT_TRUE(display.isValid());

  display.printString("88", CRGB::Red);
  TEST_ASSERT_CRGB(CRGB::Blue, leds[MIXED_LEDS]);
  TEST_ASSERT_CRGB(CRGB::Blue, leds[MIXED_LEDS + 1]);

  display.clear();
  TEST_ASSERT_CRGB(CRGB::Blue, leds[MIXED_LEDS]);
  TEST_ASSERT_CRGB(CRGB::Blue, leds[MIXED_LEDS + 1]);
}

void testColonDecimalUseOwnSize() {
  // Regression test for setModule() passing the ModuleType enum value to
  // moduleSize() instead of the module index. Place the colon/decimal at
  // module indices where the enum value (COLON == 1, DECIMAL_POINT == 2)
  // cannot accidentally be the correct index, and call setModule() directly
  // so later printString() module writes cannot mask an overwrite.

  // Colon at module index 2: sizes 14, 14, 4, 14 -> offsets 0, 14, 28, 32.
  {
    constexpr char LAYOUT[] = "D2D2:2D2";
    constexpr uint16_t NUM = Modular7Segment::requiredLeds(LAYOUT, 2, 1);
    static_assert(NUM == 46, "colon regression layout size");

    CRGB leds[NUM];
    fill_solid(leds, NUM, CRGB::Black);

    Modular7Segment display(leds, LAYOUT, 2, 1);
    TEST_ASSERT_TRUE(display.isValid());

    display.setModule(2, ':', CRGB::Red);
    for (uint16_t led = 0; led < NUM; ++led) {
      const bool lit = led >= 28 && led < 32;
      TEST_ASSERT_CRGB(lit ? CRGB::Red : CRGB::Black, leds[led]);
    }
  }

  // Decimal point at module index 3 with a sentinel tail after the display
  // area: the buggy count overruns the module and writes past requiredLeds.
  {
    constexpr char LAYOUT[] = "D2D2D2.1";
    constexpr uint16_t NUM = Modular7Segment::requiredLeds(LAYOUT, 2, 1);
    static_assert(NUM == 43, "decimal regression layout size");
    constexpr uint16_t SENTINELS = 16;

    CRGB leds[NUM + SENTINELS];
    fill_solid(leds, NUM + SENTINELS, CRGB::Blue);

    Modular7Segment display(leds, LAYOUT, 2, 1);
    TEST_ASSERT_TRUE(display.isValid());

    display.setModule(3, '.', CRGB::Green);
    for (uint16_t led = 0; led < NUM; ++led) {
      TEST_ASSERT_CRGB(led == 42 ? CRGB::Green : CRGB::Blue, leds[led]);
    }
    for (uint16_t led = NUM; led < NUM + SENTINELS; ++led) {
      TEST_ASSERT_CRGB(CRGB::Blue, leds[led]);
    }
  }
}

void testInvalidInlineCountsRejected() {
  CRGB leds[300];
  TEST_ASSERT_FALSE(Modular7Segment(leds, "D0", 3, 1).isValid());
  TEST_ASSERT_FALSE(Modular7Segment(leds, "D256", 3, 1).isValid());
  TEST_ASSERT_FALSE(Modular7Segment(leds, "D1234", 3, 1).isValid());
  TEST_ASSERT_FALSE(Modular7Segment(leds, "D:0", 3, 1).isValid());
  TEST_ASSERT_FALSE(Modular7Segment(leds, "D.0", 3, 1).isValid());
}

void setUp() {}
void tearDown() {}

void setup() {
  delay(2000);  // Allow the serial monitor to connect.

  UNITY_BEGIN();
  RUN_TEST(testMixedDigitRendering);
  RUN_TEST(testMixedClockLayoutPrintString);
  RUN_TEST(testSentinelsUntouched);
  RUN_TEST(testColonDecimalUseOwnSize);
  RUN_TEST(testInvalidInlineCountsRejected);
  UNITY_END();
}

void loop() {}
