#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Perlin noise movement mode - creates base pattern using effect_static, then shifts it
void mode_perlin_move(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static bool initialized = false;
  
  // Initialize with noise pattern if not done or on update
  if (!initialized || cfg->updated) {
    // Create noise-based pattern
    for (int i = 0; i < data->pixelCount; i++) {
      // Use position-based noise for initial pattern
      float noiseValue = sin((float)i / data->pixelCount * 6.28318f * 3.0f);
      uint8_t intensity = map(noiseValue * 1000, -1000, 1000, 50, 255);
      
      // Apply user intensity scaling
      intensity = (intensity * cfg->intensity) / 100;
      
      // Use colorOne as base, or default to blue-green
      uint32_t baseColor = (cfg->colorOne != 0) ? cfg->colorOne : 0x0080FF;
      uint8_t r = ((baseColor >> 16) & 0xFF) * intensity / 255;
      uint8_t g = ((baseColor >> 8) & 0xFF) * intensity / 255;
      uint8_t b = (baseColor & 0xFF) * intensity / 255;
      
      data->setPixelColor(i, strip.Color(r, g, b));
    }
    initialized = true;
  }
  
  unsigned long now = millis();
  
  // Speed controls shift frequency
  uint32_t shiftInterval = map(cfg->speed, 1, 100, 500, 50);
  
  if (now - lastUpdate >= shiftInterval) {
    lastUpdate = now;
    
    // Use effect_shift to move the pattern
    StripData* shiftResult = effect_shift(data, cfg->direction);
    
    // Copy shifted result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, shiftResult->getPixelColor(i));
    }
    
    delete shiftResult;
  }
}