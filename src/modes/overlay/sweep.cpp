#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Sweep effect using colorOne - one LED lit at a time moving across strip
void mode_sweep(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed 100 = 50ms (fast), Speed 1 = 1000ms (slow)
  uint32_t sweepInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= sweepInterval) {
    lastUpdate = now;
    
    // Use intensity to control drag length (1-100 maps to 1-10 pixels)
    unsigned dragLength = map(cfg->intensity, 1, 100, 1, 10);
    
    // Fix: Use the count parameter from config instead of calculating from direction
    unsigned count = cfg->count; // Use the actual count parameter
    
    StripData* sweepResult = effect_sweep(data, cfg->direction, cfg->colorOne, dragLength, count);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, sweepResult->getPixelColor(i));
    }
    
    // Clean up temporary data
    delete sweepResult;
  }
}