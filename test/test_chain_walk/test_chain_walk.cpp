#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Chain-walk hardware validation: lights exactly one red LED at a time,
// advancing 0 -> 48 and wrapping around. No display library, no Wi-Fi/OTA.
//
// Run with: pio test -e wemos_d1_mini32 -f test_chain_walk
//
// Interpretation:
// - A single red LED walks the strip in physical order and wraps
//     -> DIN direction, data path, and the 49-LED count are all correct.
// - Walk starts mid-strip or direction looks reversed
//     -> ESP32 is feeding DOUT or modules are chained in unexpected order.
// - Multiple LEDs lit, wrong colors, or flicker
//     -> signal integrity / ground / level shifting / power; see
//        HARDWARE_CHECKLIST.md.

constexpr uint8_t DATA_PIN = 27;
constexpr uint16_t NUM_LEDS = 49;
constexpr uint32_t STEP_INTERVAL_MS = 300;

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
uint16_t current = 0;
uint32_t lastStep = 0;

void setup() {
  delay(500);

  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();
  delay(1000);
}

void loop() {
  const uint32_t now = millis();
  if (now - lastStep < STEP_INTERVAL_MS) {
    return;
  }
  lastStep = now;

  leds.clear();
  leds.setPixelColor(current, leds.Color(255, 0, 0));
  leds.show();

  current = (current + 1) % NUM_LEDS;
}
