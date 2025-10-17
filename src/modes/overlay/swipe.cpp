#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Swipe effect that alternates between colorOne and colorTwo
void mode_swipe(StripData* data, const struct_message* config) { 
  const struct_message* cfg = config ? config : &myData;
  static int pixelsFilled = 0;
  static bool usecolorOne = true;
  
  // Default behavior: alternate between colorOne and colorTwo
  uint32_t color = usecolorOne ? cfg->colorOne : cfg->colorTwo;
  StripData* swipeResult = effect_swipe(data, cfg->direction, color);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, swipeResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete swipeResult;
  
  // Track when we've filled all pixels
  if (++pixelsFilled >= data->pixelCount) { 
    usecolorOne = !usecolorOne; 
    pixelsFilled = 0; 
  }
}