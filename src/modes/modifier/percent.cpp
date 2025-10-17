#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Percentage display mode -  effect_range + speed + colorOne
void mode_percent(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  // skip if not updated
  if (!cfg->updated) return;
  
  // Calculate how many pixels to fill based on intensity
  int pixelsToFill = map(cfg->intensity, 0, 100, 0, data->pixelCount);
  
  // Create a strip with colorOne for the filled portion and black for the rest
  StripData* colorData = new StripData(data->pixelCount);
  for (int i = 0; i < data->pixelCount; i++) {
    if (i < pixelsToFill) {
      colorData->setPixelColor(i, cfg->colorOne);
    } else {
      colorData->setPixelColor(i, 0); // Black for unfilled pixels
    }
  }

  // Use speed as outOfRangeBrightness for unfilled pixels
  StripData* rangeResult = effect_range(colorData, 0, pixelsToFill - 1, cfg->speed);

  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, rangeResult->getPixelColor(i));
  }

  // Clean up temporary data
  delete colorData;
  delete rangeResult;
}