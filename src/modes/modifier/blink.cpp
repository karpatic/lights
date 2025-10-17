#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// blink between colorOne and black
void mode_blink(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData; 
  StripData* colorSource = createColoredStripData(data->pixelCount, cfg->colorOne);
  // Use speed to calculate blink interval
  uint32_t blinkInterval = map(cfg->speed, 1, 100, 1000, 100); // Slower speed = longer interval
  StripData* blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, blinkResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete colorSource;
  delete blinkResult;
}