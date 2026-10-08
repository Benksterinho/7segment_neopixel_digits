#include <Arduino.h>
#include <FastLED.h>

// Minimal hardware validation: no display library, no Wi-Fi, no OTA.
// Expected result: only physical LED #0 lights dim red.
//
// Run with: pio test -e wemos_d1_mini32 -f test_single_pixel
//
// Interpretation:
// - First LED red        -> hardware/data path works; debug the app.
// - All LEDs stay white  -> hardware/data problem; work HARDWARE_CHECKLIST.md.
// - Random colors/flicker -> signal integrity, ground, level shifting, power.

constexpr uint8_t DATA_PIN = 27;
constexpr uint16_t NUM_LEDS = 3;

CRGB leds[NUM_LEDS];

void setup() {
  delay(500);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(32);

  FastLED.clear(true);
  delay(1000);

  leds[0] = CRGB::Red;
  FastLED.show();
}

void loop() {}
