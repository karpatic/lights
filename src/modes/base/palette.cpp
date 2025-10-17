#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Palette mode - uses effect_static to create solid colors, cycling through palette
void mode_palette(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static uint8_t paletteIndex = 0;
  
  // Define a color palette (can be expanded)
  static const uint32_t palette[] = {
    0xFF0000, // Red
    0xFF8000, // Orange  
    0xFFFF00, // Yellow
    0x80FF00, // Yellow-Green
    0x00FF00, // Green
    0x00FF80, // Green-Cyan
    0x00FFFF, // Cyan
    0x0080FF, // Cyan-Blue
    0x0000FF, // Blue
    0x8000FF, // Blue-Purple
    0xFF00FF, // Purple
    0xFF0080  // Purple-Red
  };
  const int paletteSize = sizeof(palette) / sizeof(palette[0]);
  
  unsigned long now = millis();
  
  // Speed controls palette cycling speed
  uint32_t cycleInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= cycleInterval) {
    lastUpdate = now;
    
    // Cycle through palette
    paletteIndex = (paletteIndex + 1) % paletteSize;
    
    // Use intensity to blend between palette colors
    uint32_t currentColor = palette[paletteIndex];
    uint32_t nextColor = palette[(paletteIndex + 1) % paletteSize];
    
    // Simple blend based on intensity
    uint8_t blendAmount = map(cfg->intensity, 1, 100, 0, 255);
    
    uint8_t r1 = (currentColor >> 16) & 0xFF;
    uint8_t g1 = (currentColor >> 8) & 0xFF;
    uint8_t b1 = currentColor & 0xFF;
    
    uint8_t r2 = (nextColor >> 16) & 0xFF;
    uint8_t g2 = (nextColor >> 8) & 0xFF;
    uint8_t b2 = nextColor & 0xFF;
    
    uint8_t r = r1 + ((r2 - r1) * blendAmount / 255);
    uint8_t g = g1 + ((g2 - g1) * blendAmount / 255);
    uint8_t b = b1 + ((b2 - b1) * blendAmount / 255);
    
    uint32_t blendedColor = strip.Color(r, g, b);
    
    // Use effect_static to fill all pixels with the current palette color
    StripData* staticResult = effect_static(blendedColor);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, staticResult->getPixelColor(i));
    }
    
    delete staticResult;
  }
}