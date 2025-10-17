#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Fireworks effect - uses effect_swipe to create rocket, then custom explosion
void mode_fireworks(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static unsigned long lastLaunch = 0;
  static bool rocketActive = false;
  static bool exploding = false;
  static int explosionFrame = 0;
  static int explosionCenter = 0;
  static uint32_t explosionColor = 0;
  static int rocketPixel = 0;
  
  unsigned long now = millis();
  
  // Speed controls animation rate
  uint32_t updateInterval = map(cfg->speed, 1, 100, 100, 10);
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    if (!rocketActive && !exploding) {
      // Launch interval based on intensity
      uint32_t launchInterval = map(cfg->intensity, 1, 100, 3000, 500);
      if (now - lastLaunch >= launchInterval) {
        rocketActive = true;
        rocketPixel = 0;
        lastLaunch = now;
      }
    }
    
    if (rocketActive) {
      // Use effect_swipe to create rocket trail
      uint32_t rocketColor = (cfg->colorOne != 0) ? cfg->colorOne : 0xFFFFFF;
      StripData* swipeResult = effect_swipe(data, cfg->direction, rocketColor, &rocketPixel);
      
      // Copy swipe result
      for (int i = 0; i < data->pixelCount; i++) {
        data->setPixelColor(i, swipeResult->getPixelColor(i));
      }
      delete swipeResult;
      
      rocketPixel++;
      if (rocketPixel >= data->pixelCount * 0.7f) {
        // Start explosion
        rocketActive = false;
        exploding = true;
        explosionFrame = 0;
        explosionCenter = rocketPixel;
        explosionColor = randomColor();
      }
    }
    
    if (exploding) {
      // Clear strip first
      for (int i = 0; i < data->pixelCount; i++) {
        data->setPixelColor(i, 0);
      }
      
      // Draw explosion manually (too complex for simple effects)
      int radius = explosionFrame / 2;
      for (int i = max(0, explosionCenter - radius); 
           i <= min(data->pixelCount - 1, explosionCenter + radius); i++) {
        uint8_t brightness = 255 - (explosionFrame * 15);
        if (brightness > 0) {
          uint8_t r = ((explosionColor >> 16) & 0xFF) * brightness / 255;
          uint8_t g = ((explosionColor >> 8) & 0xFF) * brightness / 255;
          uint8_t b = (explosionColor & 0xFF) * brightness / 255;
          data->setPixelColor(i, strip.Color(r, g, b));
        }
      }
      
      explosionFrame++;
      if (explosionFrame > 15) {
        exploding = false;
      }
    }
  }
}