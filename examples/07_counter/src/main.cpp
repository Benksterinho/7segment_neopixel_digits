#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "DDDD";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS =
    Modular7Segment::requiredLeds(LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
constexpr uint32_t TICK_INTERVAL_MS = 1000;

static_assert(NUM_LEDS == 112, "Four digits require 112 LEDs");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
uint16_t count = 0;
uint32_t lastTickAt = 0;

void renderCount() {
  char text[5];
  snprintf(text, sizeof(text), "%04u", static_cast<unsigned int>(count));
  display.clear();
  display.printString(text, leds.Color(0, 40, 0));
  leds.show();

  Serial.print("Counter: ");
  Serial.println(text);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout or LED count; LEDs remain off.");
    return;
  }

  Serial.println("Four-digit counter: 0000 through 9999, then back to 0000.");
  renderCount();
  lastTickAt = millis();
}

void loop() {
  if (!display.isValid()) {
    return;
  }

  const uint32_t now = millis();
  const uint32_t elapsedTicks = (now - lastTickAt) / TICK_INTERVAL_MS;
  if (elapsedTicks == 0) {
    return;
  }

  lastTickAt += elapsedTicks * TICK_INTERVAL_MS;
  count = (count + elapsedTicks % 10000) % 10000;
  renderCount();
}
