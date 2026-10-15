#include "temperature_format.h"

#include <math.h>
#include <stdio.h>

void formatTemperature(float celsius, char (&text)[6]) {
  if (!isfinite(celsius) || celsius < -9.9f || celsius > 99.9f) {
    snprintf(text, sizeof(text), " ---");
    return;
  }

  const int tenths = static_cast<int>(roundf(celsius * 10.0f));
  const unsigned int magnitude =
      static_cast<unsigned int>(tenths < 0 ? -tenths : tenths);
  if (tenths < 0) {
    snprintf(text, sizeof(text), " -%u.%u", magnitude / 10, magnitude % 10);
  } else {
    snprintf(text, sizeof(text), " %2u.%u", magnitude / 10, magnitude % 10);
  }
}
