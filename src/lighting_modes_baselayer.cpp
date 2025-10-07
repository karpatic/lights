#include "lighting.h"
#include "communications.h"
#include <Arduino.h>


// Simple Color Swap - effect_static + colorOne
void mode_static(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
 
  // skip if not updated
  if (!cfg->updated) return;
  
  // Fill entire strip with colorOne
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, cfg->colorOne);
  }
}

// Static pattern - colorOne, colorTwo, colorThree
void mode_static_tri(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  // skip if not updated
  if (!cfg->updated) return;

  // Create triangular pattern using three colors
  // Divide strip into three equal sections
  int sectionSize = data->pixelCount / 3;
  
  for (int i = 0; i < data->pixelCount; i++) {
    uint32_t color;
    
    if (i < sectionSize) {
      color = cfg->colorOne;
    } else if (i < sectionSize * 2) {
      color = cfg->colorTwo;
    } else {
      color = cfg->colorThree;
    }
    
    data->setPixelColor(i, color);
  }
}

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