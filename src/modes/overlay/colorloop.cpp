#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Colorloop - cycles all LEDs through rainbow colors
void mode_colorloop(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  static uint8_t colorCounter = 0;
  
  unsigned long now = millis();
  
  // Calculate update interval based on speed (1-100)
  uint32_t loopInterval = map(cfg->speed, 1, 100, 500, 1); // Much faster: 1ms at max speed
  
  if (now - lastUpdate >= loopInterval) {
    lastUpdate = now;
    
    // Get rainbow color from wheel
    uint32_t rainbowColor = Wheel(colorCounter);
    
    // Apply intensity adjustment if needed
    if (cfg->intensity < 100) {
      // Blend with white/black based on intensity
      uint8_t intensityFactor = map(cfg->intensity, 1, 100, 0, 255);
      uint8_t r = (rainbowColor >> 16) & 0xFF;
      uint8_t g = (rainbowColor >> 8) & 0xFF;
      uint8_t b = rainbowColor & 0xFF;
      
      // Scale colors by intensity
      r = (r * intensityFactor) / 255;
      g = (g * intensityFactor) / 255;
      b = (b * intensityFactor) / 255;
      
      rainbowColor = strip.Color(r, g, b);
    }
    
    // Set all pixels to the same rainbow color
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, rainbowColor);
    }
    
    // Increment color counter for next cycle - larger step for faster color changes
    colorCounter += map(cfg->speed, 1, 100, 2, 8); // Variable step based on speed
  }
}