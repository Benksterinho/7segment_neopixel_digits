#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "D4D4:3D2D2";
constexpr uint8_t FALLBACK_PIXELS_PER_SEGMENT = 4;
constexpr uint8_t FALLBACK_PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
    LAYOUT, FALLBACK_PIXELS_PER_SEGMENT, FALLBACK_PIXELS_PER_DOT);

static_assert(NUM_LEDS == 90,
              "D4D4:3D2D2 requires 90 LEDs with the library's inline counts");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, FALLBACK_PIXELS_PER_SEGMENT,
                       FALLBACK_PIXELS_PER_DOT);

}  // namespace

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();

  Serial.print("Layout: ");
  Serial.println(LAYOUT);
  Serial.print("Compile-time LED count: ");
  Serial.println(NUM_LEDS);

  if (!display.isValid()) {
    Serial.println("Invalid display layout or LED count; LEDs remain off.");
    return;
  }

  display.printString("12:34", leds.Color(0, 40, 16));
  leds.show();
  Serial.println("Rendered 12:34. Pixel counts come from physical modules.");
}

void loop() {}
