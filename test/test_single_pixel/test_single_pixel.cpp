#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

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
constexpr uint16_t NUM_LEDS = 49;

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  delay(500);

  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();
  delay(1000);

  leds.setPixelColor(0, leds.Color(255, 0, 0));
  leds.show();
}

void loop() {}
