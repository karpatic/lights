#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Meteor trail effect - uses effect_fade for trails and effect_swipe for meteor head
void mode_meteor(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static int meteorPos = 0;
  static uint32_t meteorColor = 0;
  
  unsigned long now = millis();
  
  // Speed controls meteor movement
  uint32_t meteorInterval = map(cfg->speed, 1, 100, 150, 15);
  
  if (now - lastUpdate >= meteorInterval) {
    lastUpdate = now;
    
    // Initialize or reset meteor
    if (meteorPos >= data->pixelCount || cfg->updated) {
      meteorPos = 0;
      meteorColor = (cfg->colorOne != 0) ? cfg->colorOne : randomColor();
    }
    
    // Apply fade effect for existing pixels (meteor trail)
    int fadeIntensity = map(cfg->intensity, 1, 100, 50, 90); // Trail length control
    StripData* fadeResult = effect_fade(data, fadeIntensity);
    
    // Copy faded result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, fadeResult->getPixelColor(i));
    }
    delete fadeResult;
    
    // Draw meteor head using direct pixel setting (brighter than trail)
    data->setPixelColor(meteorPos, meteorColor);
    
    meteorPos++;
  }
}