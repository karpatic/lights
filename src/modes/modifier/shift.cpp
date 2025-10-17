#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// - SPECIAL - Shift mode - Inherits colors. continuously shifts existing pixel colors
void mode_shift(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed 100 = 50ms (fast), Speed 1 = 1000ms (slow)
  uint32_t shiftInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= shiftInterval) {
    lastUpdate = now;
    
    StripData* shiftResult = effect_shift(data, cfg->direction);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, shiftResult->getPixelColor(i));
    }
    
    // Clean up temporary data
    delete shiftResult;
  }
}