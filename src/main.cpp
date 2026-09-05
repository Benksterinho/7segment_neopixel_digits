#include <Arduino.h>
#include <FastLED.h>
#include <Modular7Segment.h>

constexpr uint8_t DATA_PIN = 27;
constexpr char LAYOUT[] = "D";
constexpr uint8_t PIXELS_PER_SEGMENT = 3;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS = Modular7Segment::requiredLeds(
  LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
static_assert(NUM_LEDS > 0, "Invalid display layout");
constexpr uint32_t GLYPH_INTERVAL_MS = 1000;
constexpr uint8_t BRIGHTNESS = 25;

CRGB leds[NUM_LEDS];
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

constexpr char GLYPHS[] = "0123456789ABCDEF ";
constexpr uint8_t GLYPH_COUNT = sizeof(GLYPHS) - 1;

uint8_t currentGlyph = 0;
uint32_t lastGlyphChange = 0;

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  if (!display.isValid()) {
    return;
  }

  display.setModule(0, GLYPHS[currentGlyph], CRGB::Red);
  FastLED.show();
}

void loop() {
  if (!display.isValid()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastGlyphChange < GLYPH_INTERVAL_MS) {
    return;
  }

  lastGlyphChange = now;
  currentGlyph = (currentGlyph + 1) % GLYPH_COUNT;
  display.setModule(0, GLYPHS[currentGlyph], CRGB::Red);
  FastLED.show();
}