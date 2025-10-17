#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Swipe effect with random colors
void mode_swipe_random(StripData* data, const struct_message* config) { 
  const struct_message* cfg = config ? config : &myData;
  static uint32_t randColor = randomColor();
  static int randPixelsFilled = 0;
  
  StripData* swipeResult = effect_swipe(data, cfg->direction, randColor);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, swipeResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete swipeResult;
  
  if (++randPixelsFilled >= data->pixelCount) {
    randColor = randomColor();
    randPixelsFilled = 0;
  }
}