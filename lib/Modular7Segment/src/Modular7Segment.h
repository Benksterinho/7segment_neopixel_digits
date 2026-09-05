#pragma once

#include <Arduino.h>
#include <FastLED.h>

class Modular7Segment {
 public:
  enum Segment : uint8_t {
    BOTTOM = 0,
    LOWER_LEFT,
    MIDDLE,
    UPPER_LEFT,
    TOP,
    UPPER_RIGHT,
    LOWER_RIGHT,
  };

  static constexpr uint16_t requiredLeds(const char* layout,
                                         uint8_t pixelsPerSegment,
                                         uint8_t pixelsPerDot) {
    return requiredLeds(layout, pixelsPerSegment, pixelsPerDot, 0, 0);
  }

  template <size_t N>
  Modular7Segment(CRGB (&leds)[N], const char* layout,
                  uint8_t pixelsPerSegment, uint8_t pixelsPerDot)
      : Modular7Segment(leds, static_cast<uint16_t>(N), layout,
                        pixelsPerSegment, pixelsPerDot) {}

  void setModule(uint8_t moduleIndex, char character, CRGB color);
  void printString(const char* text, CRGB color);
  void clear();

  uint8_t moduleCount() const;
  uint16_t requiredLeds() const;
  bool isValid() const;

 private:
  enum ModuleType : uint8_t {
    DIGIT,
    COLON,
    DECIMAL_POINT,
  };

  static constexpr uint8_t SEGMENT_COUNT = 7;
  static constexpr uint8_t MAX_MODULES = 32;

    static constexpr uint16_t requiredLeds(const char* layout,
                   uint8_t pixelsPerSegment,
                   uint8_t pixelsPerDot,
                   uint16_t ledCount,
                   uint8_t moduleCount) {
      return layout == nullptr || pixelsPerSegment == 0 || pixelsPerDot == 0 ||
         moduleCount == MAX_MODULES
         ? 0
         : *layout == '\0'
           ? (moduleCount == 0 ? 0 : ledCount)
           : *layout == 'D'
             ? requiredLeds(layout + 1, pixelsPerSegment,
                pixelsPerDot,
                ledCount +
                    SEGMENT_COUNT * pixelsPerSegment,
                moduleCount + 1)
             : *layout == ':'
               ? requiredLeds(layout + 1, pixelsPerSegment,
                  pixelsPerDot,
                  ledCount + 2 * pixelsPerDot,
                  moduleCount + 1)
               : *layout == '.'
                 ? requiredLeds(layout + 1,
                    pixelsPerSegment,
                    pixelsPerDot,
                    ledCount + pixelsPerDot,
                    moduleCount + 1)
                 : 0;
    }

  Modular7Segment(CRGB* leds, uint16_t numLeds, const char* layout,
                  uint8_t pixelsPerSegment, uint8_t pixelsPerDot);

  CRGB* leds_;
  uint16_t numLeds_;
  uint8_t pixelsPerSegment_;
  uint8_t pixelsPerDot_;
  uint16_t moduleOffsets_[MAX_MODULES];
  ModuleType moduleTypes_[MAX_MODULES];
  uint8_t moduleCount_;
  uint16_t requiredLeds_;
  bool valid_;

  static uint8_t characterMask(char character);
  uint16_t moduleSize(ModuleType moduleType) const;
  bool moduleFitsBuffer(uint8_t moduleIndex) const;
  void clearModule(uint8_t moduleIndex);
};