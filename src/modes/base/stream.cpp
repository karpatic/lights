#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Stream mode - creates random color bands and uses effect_shift to move them
void mode_stream(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static unsigned long lastColorChange = 0;
  static bool initialized = false;
  
  // Initialize with random colors if not already done
  if (!initialized || cfg->updated) {
    for (int i = 0; i < data->pixelCount; i++) {
      // Create bands of random hues
      uint32_t randomHue = randomColor();
      data->setPixelColor(i, randomHue);
    }
    initialized = true;
  }
  
  unsigned long now = millis();
  
  // Speed controls shift frequency
  uint32_t shiftInterval = map(cfg->speed, 1, 100, 500, 50);
  
  if (now - lastUpdate >= shiftInterval) {
    lastUpdate = now;
    
    // Use effect_shift to move existing colors
    StripData* shiftResult = effect_shift(data, cfg->direction);
    
    // Copy shifted result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, shiftResult->getPixelColor(i));
    }
    
    delete shiftResult;
  }
  
  // Occasionally inject new random colors at the edge
  uint32_t colorChangeInterval = map(cfg->intensity, 1, 100, 2000, 200);
  if (now - lastColorChange >= colorChangeInterval) {
    lastColorChange = now;
    
    // Add new random color at the leading edge
    int newPixel = (cfg->direction == 1) ? 0 : data->pixelCount - 1;
    data->setPixelColor(newPixel, randomColor());
  }
}