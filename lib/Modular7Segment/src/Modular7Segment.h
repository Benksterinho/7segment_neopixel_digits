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
  static constexpr uint8_t MAX_COUNT_DIGITS = 3;

  static constexpr bool isDigitChar(char character) {
    return character >= '0' && character <= '9';
  }

  static constexpr uint16_t digitSpan(const char* text) {
    return isDigitChar(*text) ? 1 + digitSpan(text + 1) : 0;
  }

  static constexpr uint16_t numberValue(const char* text, uint16_t value) {
    return isDigitChar(*text)
               ? numberValue(text + 1,
                             static_cast<uint16_t>(value * 10 + (*text - '0')))
               : value;
  }

  // Effective pixel count for the module starting at moduleChar: the inline
  // number after the module character, or fallback when no number is present.
  // Returns 0 for invalid inline counts (0, above 255, or more than
  // MAX_COUNT_DIGITS digits).
  static constexpr uint16_t modulePixelCount(const char* moduleChar,
                                             uint8_t fallback) {
    return digitSpan(moduleChar + 1) == 0
               ? fallback
               : digitSpan(moduleChar + 1) > MAX_COUNT_DIGITS
                     ? 0
                     : numberValue(moduleChar + 1, 0) > 255
                           ? 0
                           : numberValue(moduleChar + 1, 0);
  }

  static constexpr uint16_t requiredLedsForModule(
      const char* layout, uint8_t pixelsPerSegment, uint8_t pixelsPerDot,
      uint16_t ledCount, uint8_t moduleCount, uint8_t fallbackCount,
      uint16_t pixelsPerModule) {
    return modulePixelCount(layout, fallbackCount) == 0
               ? 0
               : requiredLeds(layout + 1 + digitSpan(layout + 1),
                              pixelsPerSegment, pixelsPerDot,
                              ledCount + pixelsPerModule *
                                             modulePixelCount(layout,
                                                              fallbackCount),
                              moduleCount + 1);
  }

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
                           ? requiredLedsForModule(layout, pixelsPerSegment,
                                                   pixelsPerDot, ledCount,
                                                   moduleCount,
                                                   pixelsPerSegment,
                                                   SEGMENT_COUNT)
                           : *layout == ':'
                                 ? requiredLedsForModule(
                                       layout, pixelsPerSegment, pixelsPerDot,
                                       ledCount, moduleCount, pixelsPerDot, 2)
                                 : *layout == '.'
                                       ? requiredLedsForModule(
                                             layout, pixelsPerSegment,
                                             pixelsPerDot, ledCount,
                                             moduleCount, pixelsPerDot, 1)
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
  // DIGIT modules: pixels per segment; COLON/DECIMAL_POINT: pixels per dot.
  uint8_t modulePixelCounts_[MAX_MODULES];
  uint8_t moduleCount_;
  uint16_t requiredLeds_;
  bool valid_;

  static uint8_t characterMask(char character);
  uint16_t moduleSize(uint8_t moduleIndex) const;
  bool moduleFitsBuffer(uint8_t moduleIndex) const;
  void clearModule(uint8_t moduleIndex);
};