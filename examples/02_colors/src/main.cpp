#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Modular7Segment.h>

#include <cstdint>

#ifndef LED_DATA_PIN
#define LED_DATA_PIN 27
#endif

#ifndef PIXELS_PER_SEGMENT
#define PIXELS_PER_SEGMENT 4
#endif

#ifndef PIXELS_PER_DOT
#define PIXELS_PER_DOT 1
#endif

constexpr char LAYOUT[] = "DD:DD";
constexpr char DISPLAY_TEXT[] = "12:34";
constexpr uint8_t DATA_PIN = LED_DATA_PIN;
constexpr uint8_t SEGMENT_PIXELS = PIXELS_PER_SEGMENT;
constexpr uint8_t DOT_PIXELS = PIXELS_PER_DOT;
constexpr uint16_t NUM_LEDS =
    Modular7Segment::requiredLeds(LAYOUT, SEGMENT_PIXELS, DOT_PIXELS);
static_assert(NUM_LEDS > 0, "Invalid display layout or pixel counts");

constexpr uint8_t DOT_BRIGHTNESS_PERCENT = 100;
constexpr uint8_t NORMAL_BRIGHTNESS = 24;
constexpr uint8_t DIM_BRIGHTNESS = 8;
constexpr uint32_t SCENE_INTERVAL_MS = 2500;

Adafruit_NeoPixel leds(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
Modular7Segment display(leds, LAYOUT, SEGMENT_PIXELS, DOT_PIXELS,
                        DOT_BRIGHTNESS_PERCENT);

enum class Scene : uint8_t {
  UNIFORM_RGB,
  PER_MODULE_RGB,
  DIMMED_PER_MODULE_RGB,
};

Scene currentScene = Scene::UNIFORM_RGB;
uint32_t lastSceneChange = 0;
bool displayReady = false;

void renderCurrentScene() {
  const bool dimmed = currentScene == Scene::DIMMED_PER_MODULE_RGB;
  leds.setBrightness(dimmed ? DIM_BRIGHTNESS : NORMAL_BRIGHTNESS);
  display.clear();

  if (currentScene == Scene::UNIFORM_RGB) {
    display.printString(DISPLAY_TEXT, leds.Color(0, 80, 24));
  } else {
    display.setModule(0, '1', leds.Color(255, 24, 0));
    display.setModule(1, '2', leds.Color(255, 24, 0));
    display.setModule(2, ':', leds.Color(0, 64, 255));
    display.setModule(3, '3', leds.Color(0, 180, 32));
    display.setModule(4, '4', leds.Color(0, 180, 32));
  }

  leds.show();
}

void setup() {
  Serial.begin(115200);
  leds.begin();
  leds.setBrightness(NORMAL_BRIGHTNESS);
  leds.clear();
  leds.show();

  if (!display.isValid()) {
    Serial.println("Invalid display layout or LED buffer size; display remains blank.");
    return;
  }
  displayReady = true;

  Serial.println("Color lesson: uniform RGB, per-module RGB, then dimmed RGB.");
  renderCurrentScene();
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

  switch (currentScene) {
    case Scene::UNIFORM_RGB:
      currentScene = Scene::PER_MODULE_RGB;
      Serial.println("Per-module colors: red digits, blue colon, green digits.");
      break;
    case Scene::PER_MODULE_RGB:
      currentScene = Scene::DIMMED_PER_MODULE_RGB;
      Serial.println("Same per-module colors at lower global brightness.");
      break;
    case Scene::DIMMED_PER_MODULE_RGB:
      currentScene = Scene::UNIFORM_RGB;
      Serial.println("Uniform RGB color.");
      break;
  }

  renderCurrentScene();
  lastSceneChange = now;
}
