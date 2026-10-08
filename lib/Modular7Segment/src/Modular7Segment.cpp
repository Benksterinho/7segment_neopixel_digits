#include "Modular7Segment.h"

namespace {
constexpr uint8_t segmentBit(Modular7Segment::Segment segment) {
  return 1U << static_cast<uint8_t>(segment);
}

constexpr uint8_t BOTTOM_MASK = segmentBit(Modular7Segment::BOTTOM);
constexpr uint8_t LOWER_LEFT_MASK = segmentBit(Modular7Segment::LOWER_LEFT);
constexpr uint8_t MIDDLE_MASK = segmentBit(Modular7Segment::MIDDLE);
constexpr uint8_t UPPER_LEFT_MASK = segmentBit(Modular7Segment::UPPER_LEFT);
constexpr uint8_t TOP_MASK = segmentBit(Modular7Segment::TOP);
constexpr uint8_t UPPER_RIGHT_MASK = segmentBit(Modular7Segment::UPPER_RIGHT);
constexpr uint8_t LOWER_RIGHT_MASK = segmentBit(Modular7Segment::LOWER_RIGHT);

constexpr uint8_t ZERO = TOP_MASK | UPPER_RIGHT_MASK | LOWER_RIGHT_MASK |
                         BOTTOM_MASK | LOWER_LEFT_MASK | UPPER_LEFT_MASK;
constexpr uint8_t ONE = UPPER_RIGHT_MASK | LOWER_RIGHT_MASK;
constexpr uint8_t TWO = TOP_MASK | UPPER_RIGHT_MASK | MIDDLE_MASK |
                        LOWER_LEFT_MASK | BOTTOM_MASK;
constexpr uint8_t THREE = TOP_MASK | UPPER_RIGHT_MASK | MIDDLE_MASK |
                          LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t FOUR = UPPER_LEFT_MASK | MIDDLE_MASK | UPPER_RIGHT_MASK |
                         LOWER_RIGHT_MASK;
constexpr uint8_t FIVE = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK |
                         LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t SIX = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK |
                        LOWER_LEFT_MASK | LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t SEVEN = TOP_MASK | UPPER_RIGHT_MASK | LOWER_RIGHT_MASK;
constexpr uint8_t EIGHT = ZERO | MIDDLE_MASK;
constexpr uint8_t NINE = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK |
                         UPPER_RIGHT_MASK | LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t A = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK |
                      UPPER_RIGHT_MASK | LOWER_RIGHT_MASK | LOWER_LEFT_MASK;
constexpr uint8_t B = UPPER_LEFT_MASK | MIDDLE_MASK | LOWER_LEFT_MASK |
                      LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t C = TOP_MASK | UPPER_LEFT_MASK | LOWER_LEFT_MASK | BOTTOM_MASK;
constexpr uint8_t D = UPPER_RIGHT_MASK | MIDDLE_MASK | LOWER_LEFT_MASK |
                      LOWER_RIGHT_MASK | BOTTOM_MASK;
constexpr uint8_t E = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK |
                      LOWER_LEFT_MASK | BOTTOM_MASK;
constexpr uint8_t F = TOP_MASK | UPPER_LEFT_MASK | MIDDLE_MASK | LOWER_LEFT_MASK;
}  // namespace

Modular7Segment::Modular7Segment(Adafruit_NeoPixel& leds, const char* layout,
                                 uint8_t pixelsPerSegment,
                                 uint8_t pixelsPerDot)
    : leds_(leds),
      pixelsPerSegment_(pixelsPerSegment),
      pixelsPerDot_(pixelsPerDot),
      moduleCount_(0),
      requiredLeds_(0),
      valid_(layout != nullptr && pixelsPerSegment > 0 && pixelsPerDot > 0) {
  if (!valid_) {
    return;
  }

  for (const char* module = layout; *module != '\0';
       module += 1 + digitSpan(module + 1)) {
    ModuleType moduleType;
    uint8_t fallbackCount;
    if (*module == 'D') {
      moduleType = DIGIT;
      fallbackCount = pixelsPerSegment;
    } else if (*module == ':') {
      moduleType = COLON;
      fallbackCount = pixelsPerDot;
    } else if (*module == '.') {
      moduleType = DECIMAL_POINT;
      fallbackCount = pixelsPerDot;
    } else {
      valid_ = false;
      return;
    }

    if (moduleCount_ == MAX_MODULES) {
      valid_ = false;
      return;
    }

    const uint16_t pixelCount = modulePixelCount(module, fallbackCount);
    if (pixelCount == 0) {
      valid_ = false;
      return;
    }

    moduleOffsets_[moduleCount_] = requiredLeds_;
    moduleTypes_[moduleCount_] = moduleType;
    modulePixelCounts_[moduleCount_] = static_cast<uint8_t>(pixelCount);
    requiredLeds_ += moduleSize(moduleCount_);
    ++moduleCount_;
  }

  valid_ = moduleCount_ > 0 && requiredLeds_ <= leds_.numPixels();
}

void Modular7Segment::setModule(uint8_t moduleIndex, char character,
                                uint32_t color) {
  if (!valid_ || !moduleFitsBuffer(moduleIndex)) {
    return;
  }

  clearModule(moduleIndex);
  const uint16_t moduleOffset = moduleOffsets_[moduleIndex];

  if (moduleTypes_[moduleIndex] == DIGIT) {
    const uint8_t mask = characterMask(character);
    const uint8_t count = modulePixelCounts_[moduleIndex];
    for (uint8_t segment = 0; segment < SEGMENT_COUNT; ++segment) {
      if ((mask & segmentBit(static_cast<Segment>(segment))) == 0) {
        continue;
      }

      const uint16_t segmentOffset = moduleOffset + segment * count;
      for (uint8_t pixel = 0; pixel < count; ++pixel) {
        leds_.setPixelColor(segmentOffset + pixel, color);
      }
    }
    return;
  }

  const char requiredCharacter =
      moduleTypes_[moduleIndex] == COLON ? ':' : '.';
  if (character != requiredCharacter) {
    return;
  }

  // moduleSize() takes a module index, not the ModuleType enum value.
  const uint16_t count = moduleSize(moduleIndex);
  for (uint16_t pixel = 0; pixel < count; ++pixel) {
    leds_.setPixelColor(moduleOffset + pixel, color);
  }
}

void Modular7Segment::printString(const char* text, uint32_t color) {
  if (!valid_) {
    return;
  }

  const char* input = text == nullptr ? "" : text;
  for (uint8_t module = 0; module < moduleCount_; ++module) {
    const ModuleType moduleType = moduleTypes_[module];
    if (moduleType == DIGIT) {
      const char character = *input == '\0' ? ' ' : *input++;
      setModule(module, character, color);
      continue;
    }

    const char punctuation = moduleType == COLON ? ':' : '.';
    if (*input == punctuation) {
      ++input;
      setModule(module, punctuation, color);
    } else {
      setModule(module, ' ', color);
    }
  }
}

void Modular7Segment::clear() {
  if (!valid_) {
    return;
  }

  for (uint16_t led = 0; led < requiredLeds_; ++led) {
    leds_.setPixelColor(led, 0);
  }
}

uint8_t Modular7Segment::moduleCount() const { return moduleCount_; }

uint16_t Modular7Segment::requiredLeds() const { return requiredLeds_; }

bool Modular7Segment::isValid() const { return valid_; }

uint8_t Modular7Segment::characterMask(char character) {
  switch (character) {
    case '0': return ZERO;
    case '1': return ONE;
    case '2': return TWO;
    case '3': return THREE;
    case '4': return FOUR;
    case '5': return FIVE;
    case '6': return SIX;
    case '7': return SEVEN;
    case '8': return EIGHT;
    case '9': return NINE;
    case 'A':
    case 'a': return A;
    case 'B':
    case 'b': return B;
    case 'C':
    case 'c': return C;
    case 'D':
    case 'd': return D;
    case 'E':
    case 'e': return E;
    case 'F':
    case 'f': return F;
    default: return 0;
  }
}

uint16_t Modular7Segment::moduleSize(uint8_t moduleIndex) const {
  const ModuleType moduleType = moduleTypes_[moduleIndex];
  if (moduleType == DIGIT) {
    return SEGMENT_COUNT * modulePixelCounts_[moduleIndex];
  }
  return (moduleType == COLON ? 2 : 1) * modulePixelCounts_[moduleIndex];
}

bool Modular7Segment::moduleFitsBuffer(uint8_t moduleIndex) const {
  return moduleIndex < moduleCount_ &&
         moduleOffsets_[moduleIndex] + moduleSize(moduleIndex) <=
             leds_.numPixels();
}

void Modular7Segment::clearModule(uint8_t moduleIndex) {
  const uint16_t moduleOffset = moduleOffsets_[moduleIndex];
  const uint16_t count = moduleSize(moduleIndex);
  for (uint16_t pixel = 0; pixel < count; ++pixel) {
    leds_.setPixelColor(moduleOffset + pixel, 0);
  }
}