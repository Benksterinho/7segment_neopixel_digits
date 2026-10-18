#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#include <cstdio>

#include "display/temperature_format.h"

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

namespace {

constexpr char LAYOUT[] = "DD.DD";
constexpr uint8_t PIXELS_PER_SEGMENT = 4;
constexpr uint8_t PIXELS_PER_DOT = 1;
constexpr uint16_t NUM_LEDS =
    Modular7Segment::requiredLeds(LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);
constexpr uint32_t SCENE_INTERVAL_MS = 2500;

static_assert(NUM_LEDS == 113, "DD.DD requires 113 LEDs");

Adafruit_NeoPixel leds(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, PIXELS_PER_SEGMENT, PIXELS_PER_DOT);

struct Scene {
  const char* label;
  float celsius;
  bool blank;
};

constexpr Scene SCENES[] = {
    {"waiting: blank display", 0.0f, true},
    {"round down: 25.74 -> 25.7", 25.74f, false},
    {"round up: 25.75 -> 25.8", 25.75f, false},
    {"negative zero: -0.04 -> 0.0", -0.04f, false},
    {"negative rounding: -5.25 -> -5.3", -5.25f, false},
    {"lower supported edge: -9.9", -9.9f, false},
    {"out of range: 100.0 -> overflow dashes", 100.0f, false},
    {"blank interval", 0.0f, true},
};
constexpr uint8_t SCENE_COUNT = sizeof(SCENES) / sizeof(SCENES[0]);

uint8_t sceneIndex = 0;
uint32_t lastSceneAt = 0;

void renderScene(uint8_t index) {
  display.clear();
  const Scene& scene = SCENES[index];
  if (!scene.blank) {
    char formatted[6] = {};
    char displayText[6] = {};
    formatTemperature(scene.celsius, formatted);

    // The formatter returns " dd.d"; place its sign/integer before the
    // physical decimal point in DD.DD and keep the final digit blank.
    if (formatted[4] == '\0') {
      snprintf(displayText, sizeof(displayText), "%s", formatted);
    } else {
      displayText[0] = formatted[1];
      displayText[1] = formatted[2];
      displayText[2] = '.';
      displayText[3] = formatted[4];
      displayText[4] = ' ';
    }
    display.printString(displayText, leds.Color(32, 20, 0));
    Serial.print("Formatter output: \"");
    Serial.print(formatted);
    Serial.println("\"");
  }
  leds.show();
  Serial.print("Scene: ");
  Serial.println(scene.label);
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

  Serial.println("Signed-decimal formatter demonstration; each scene lasts 2.5 s.");
  Serial.println("Physical layout DD.DD: four digit modules and a decimal point.");
  renderScene(sceneIndex);
  lastSceneAt = millis();
}

void loop() {
  if (!display.isValid()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastSceneAt < SCENE_INTERVAL_MS) {
    return;
  }

  sceneIndex = (sceneIndex + 1) % SCENE_COUNT;
  renderScene(sceneIndex);
  lastSceneAt = now;
}
