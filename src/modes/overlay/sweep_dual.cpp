#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Sweep effect with dual colors sweeping in opposite directions.
void mode_sweep_dual(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed 100 = 50ms (fast), Speed 1 = 1000ms (slow)
  uint32_t sweepInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= sweepInterval) {
    lastUpdate = now;
    
    // Use intensity to control drag length (1-100 maps to 1-10 pixels)
    unsigned dragLength = map(cfg->intensity, 1, 100, 1, 10);
    
    // Use count parameter from config
    unsigned count = cfg->count;
    
    // Create two sweep effects going in opposite directions
    // Direction 0 = forward, Direction 1 = reverse
    unsigned direction1 = 0; // Forward
    unsigned direction2 = 1; // Reverse
    
    // Create first sweep with colorOne going forward
    StripData* sweep1 = effect_sweep(data, direction1, cfg->colorOne, dragLength, count);
    
    // Create second sweep with colorTwo going reverse  
    StripData* sweep2 = effect_sweep(data, direction2, cfg->colorTwo, dragLength, count);
    
    // Combine both sweeps - blend the colors where they overlap
    for (int i = 0; i < data->pixelCount; i++) {
      uint32_t color1 = sweep1->getPixelColor(i);
      uint32_t color2 = sweep2->getPixelColor(i);
      
      if (color1 != 0 && color2 != 0) {
        // Both sweeps have a pixel here - blend them
        uint8_t r1 = (color1 >> 16) & 0xFF;
        uint8_t g1 = (color1 >> 8) & 0xFF;
        uint8_t b1 = color1 & 0xFF;
        
        uint8_t r2 = (color2 >> 16) & 0xFF;
        uint8_t g2 = (color2 >> 8) & 0xFF;
        uint8_t b2 = color2 & 0xFF;
        
        // Average the colors for blending
        uint8_t r = (r1 + r2) / 2;
        uint8_t g = (g1 + g2) / 2;
        uint8_t b = (b1 + b2) / 2;
        
        data->setPixelColor(i, strip.Color(r, g, b));
      } else if (color1 != 0) {
        // Only first sweep has a pixel here
        data->setPixelColor(i, color1);
      } else if (color2 != 0) {
        // Only second sweep has a pixel here
        data->setPixelColor(i, color2);
      } else {
        // Neither sweep has a pixel here - keep it black
        data->setPixelColor(i, 0);
      }
    }
    
    // Clean up temporary data
    delete sweep1;
    delete sweep2;
  }
}