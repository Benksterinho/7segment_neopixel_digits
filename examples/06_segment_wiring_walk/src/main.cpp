#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "D:.";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS =
    Modular7Segment::requiredLeds(LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
constexpr uint32_t STEP_INTERVAL_MS = 1200;
constexpr uint8_t BRIGHTNESS = 12;
constexpr uint32_t COLOR_CHANNEL = 48;

static_assert(NUM_LEDS == 31, "D:. requires 31 LEDs at the configured sizes");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

struct WalkStep {
  const char* description;
  int8_t segment;
  int8_t pixel;
  uint8_t moduleOffset;
};

constexpr WalkStep WALK[] = {
    {"digit segment BOTTOM", Modular7Segment::BOTTOM, -1, 0},
    {"digit segment LOWER_LEFT", Modular7Segment::LOWER_LEFT, -1, 0},
    {"digit segment MIDDLE", Modular7Segment::MIDDLE, -1, 0},
    {"digit segment UPPER_LEFT", Modular7Segment::UPPER_LEFT, -1, 0},
    {"digit segment TOP", Modular7Segment::TOP, -1, 0},
    {"digit segment UPPER_RIGHT", Modular7Segment::UPPER_RIGHT, -1, 0},
    {"digit segment LOWER_RIGHT", Modular7Segment::LOWER_RIGHT, -1, 0},
    {"colon module pixel 1", -1, 0, 28},
    {"colon module pixel 2", -1, 1, 28},
    {"decimal-point module pixel 1", -1, 0, 30},
};
constexpr uint8_t WALK_COUNT = sizeof(WALK) / sizeof(WALK[0]);

uint8_t walkIndex = 0;
uint32_t lastStepAt = 0;

void renderStep(uint8_t index) {
  display.clear();
  const WalkStep& step = WALK[index];
  if (step.segment >= 0) {
    const uint16_t firstLed =
        static_cast<uint16_t>(step.segment) * PIXELS_PER_SEGMENT;
    for (uint8_t offset = 0; offset < PIXELS_PER_SEGMENT; ++offset) {
      leds.setPixelColor(firstLed + offset,
                         leds.Color(0, COLOR_CHANNEL, 0));
    }
  } else {
    leds.setPixelColor(step.moduleOffset + step.pixel,
                       leds.Color(0, COLOR_CHANNEL, 0));
  }
  leds.show();

  Serial.print("LED walk: ");
  Serial.println(step.description);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(BRIGHTNESS);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout or LED count; LEDs remain off.");
    return;
  }

  Serial.println("Low-brightness segment and wiring walk; one item every 1.2 s.");
  Serial.println("Digit segment order follows Modular7Segment::Segment.");
  renderStep(walkIndex);
  lastStepAt = millis();
}

void loop() {
  if (!display.isValid()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastStepAt < STEP_INTERVAL_MS) {
    return;
  }

  walkIndex = (walkIndex + 1) % WALK_COUNT;
  renderStep(walkIndex);
  lastStepAt = now;
}
