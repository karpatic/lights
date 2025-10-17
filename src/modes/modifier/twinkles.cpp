#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Twinkles effect - uses effect_fade and randomly sets new pixels
void mode_twinkles(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed controls update rate
  uint32_t updateInterval = map(cfg->speed, 1, 100, 100, 10);
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    // Apply fade effect first to existing pixels
    int fadeAmount = map(cfg->intensity, 1, 100, 95, 85); // Higher intensity = slower fade
    StripData* fadeResult = effect_fade(data, fadeAmount);
    
    // Copy faded result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, fadeResult->getPixelColor(i));
    }
    delete fadeResult;
    
    // Randomly spawn new twinkles based on intensity
    int spawnChance = map(cfg->intensity, 1, 100, 2, 20);
    for (int i = 0; i < data->pixelCount; i++) {
      if (data->getPixelColor(i) == 0 && random(0, 1000) < spawnChance) {
        // Start new twinkle
        uint32_t twinkleColor;
        if (cfg->colorOne != 0) {
          twinkleColor = cfg->colorOne;
        } else {
          twinkleColor = randomColor();
        }
        data->setPixelColor(i, twinkleColor);
      }
    }
  }
}