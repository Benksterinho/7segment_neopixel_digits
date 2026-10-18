#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#include <cstdio>
#include <cstddef>
#include <cstdint>

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

#ifndef EXAMPLE_LAYOUT
#define EXAMPLE_LAYOUT D
#endif

#ifndef EXAMPLE_TEXT
#define EXAMPLE_TEXT 7
#endif

#ifndef PIXELS_PER_SEGMENT
#define PIXELS_PER_SEGMENT 4
#endif

#ifndef PIXELS_PER_DOT
#define PIXELS_PER_DOT 1
#endif

#define STRINGIFY_VALUE_INNER(value) #value
#define STRINGIFY_VALUE(value) STRINGIFY_VALUE_INNER(value)

constexpr char LAYOUT[] = STRINGIFY_VALUE(EXAMPLE_LAYOUT);
constexpr char DISPLAY_TEXT[] = STRINGIFY_VALUE(EXAMPLE_TEXT);
constexpr uint8_t DATA_PIN = LED_DATA_PIN;
constexpr uint8_t SEGMENT_PIXELS = PIXELS_PER_SEGMENT;
constexpr uint8_t DOT_PIXELS = PIXELS_PER_DOT;
constexpr uint16_t NUM_LEDS =
    Modular7Segment::requiredLeds(LAYOUT, SEGMENT_PIXELS, DOT_PIXELS);
static_assert(NUM_LEDS > 0, "Invalid display layout or pixel counts");

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, SEGMENT_PIXELS, DOT_PIXELS);

constexpr uint32_t SCENE_INTERVAL_MS = 3000;
bool displayReady = false;
bool showingText = true;
uint32_t lastSceneChange = 0;

bool formatPaddedNumber(uint32_t value, uint8_t width, char* buffer,
                        size_t capacity) {
  if (capacity == 0) {
    return false;
  }

  const int written =
      snprintf(buffer, capacity, "%0*u", static_cast<int>(width),
               static_cast<unsigned int>(value));
  return written >= 0 && static_cast<size_t>(written) < capacity;
}

void renderText(const char* text) {
  display.clear();
  display.printString(text, leds.Color(0, 48, 0));
  leds.show();
}

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(32);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout or LED buffer size; display remains blank.");
    return;
  }
  displayReady = true;

  char paddedNumber[12];
  if (formatPaddedNumber(7, 2, paddedNumber, sizeof(paddedNumber))) {
    Serial.print("Bounded number conversion (width 2): ");
    Serial.println(paddedNumber);
  } else {
    Serial.println("Could not format the number into the bounded buffer.");
  }

  Serial.print("Physical layout: ");
  Serial.println(LAYOUT);
  Serial.print("Rendered text: ");
  Serial.println(DISPLAY_TEXT);
  renderText(DISPLAY_TEXT);
  lastSceneChange = millis();
}

void loop() {
  if (!displayReady) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastSceneChange < SCENE_INTERVAL_MS) {
    return;
  }

  showingText = !showingText;
  renderText(showingText ? DISPLAY_TEXT : "");
  Serial.println(showingText ? "Showing configured text." : "Display blanked.");
  lastSceneChange = now;
}
