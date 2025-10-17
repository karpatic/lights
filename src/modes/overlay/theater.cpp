#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Theater effect with rainbow colors - uses effect_sweep with rainbow colors
void mode_theater(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  static uint8_t colorCounter = 0;
  
  unsigned long now = millis();
  
  // Speed controls update interval
  uint32_t theaterInterval = map(cfg->speed, 1, 100, 200, 20);
  
  if (now - lastUpdate >= theaterInterval) {
    lastUpdate = now;
    
    // Calculate gap size based on intensity (1-100 maps to 1-10)
    unsigned gapSize = map(cfg->intensity, 1, 100, 1, 10);
    
    // Get rainbow color and cycle it
    uint32_t rainbowColor = Wheel(colorCounter);
    colorCounter += 8; // Change color for next cycle
    
    // Use effect_sweep with the rainbow color and gap size
    StripData* sweepResult = effect_sweep(data, cfg->direction, rainbowColor, gapSize, cfg->count ? cfg->count : 1);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, sweepResult->getPixelColor(i));
    }
    
    delete sweepResult;
  }
}