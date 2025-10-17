#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Plasma mode - custom plasma effect (too complex for simple effects)
void mode_plasma(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static float time = 0.0f;
  
  unsigned long now = millis();
  
  // Speed controls animation rate
  uint32_t updateInterval = map(cfg->speed, 1, 100, 100, 10);
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    // Advance time
    time += 0.1f;
    
    for (int i = 0; i < data->pixelCount; i++) {
      // Create plasma effect using multiple sine waves
      float x = (float)i / data->pixelCount;
      
      float plasma = sin(x * 10.0f + time) + 
                     sin(x * 15.0f + time * 1.2f) + 
                     sin(x * 20.0f + time * 0.8f);
      
      // Normalize plasma value to 0-1
      plasma = (plasma + 3.0f) / 6.0f;
      
      // Map to rainbow colors
      uint8_t hue = (uint8_t)(plasma * 255);
      uint32_t color = Wheel(hue);
      
      // Apply intensity
      uint8_t r = ((color >> 16) & 0xFF) * cfg->intensity / 100;
      uint8_t g = ((color >> 8) & 0xFF) * cfg->intensity / 100;
      uint8_t b = (color & 0xFF) * cfg->intensity / 100;
      
      data->setPixelColor(i, strip.Color(r, g, b));
    }
  }
}