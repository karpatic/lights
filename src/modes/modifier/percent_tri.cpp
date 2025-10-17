#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Percent mode tri - effect_range + speed + colorOne, colorTwo, colorThree
void mode_percent_tri(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  // skip if not updated
  if (!cfg->updated) return;
  // Calculate how many pixels to fill based on intensity
  int pixelsToFill = map(cfg->intensity, 0, 100, 0, data->pixelCount);

  // Create a tri-color pattern for the filled portion
  StripData* triColorData = new StripData(data->pixelCount);
  int sectionSize = pixelsToFill / 3;
  for (int i = 0; i < pixelsToFill; i++) {
    uint32_t color;
    if (i < sectionSize) {
      color = cfg->colorOne;
    } else if (i < sectionSize * 2) {
      color = cfg->colorTwo;
    } else {
      color = cfg->colorThree;
    }
    triColorData->setPixelColor(i, color);
  }
  // Fill the rest with colorOne (or black, or keep as is)
  for (int i = pixelsToFill; i < data->pixelCount; i++) {
    triColorData->setPixelColor(i, 0); // 0 for black/off
  }

  // Use speed as outOfRangeBrightness for unfilled pixels (lower speed = less light)
  StripData* rangeResult = effect_range(triColorData, 0, pixelsToFill - 1, cfg->speed);

  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, rangeResult->getPixelColor(i));
  }

  // Clean up temporary data
  delete triColorData;
  delete rangeResult;
}