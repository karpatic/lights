#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Juggle effect - uses effect_fade for trails and effect_swipe for dots
void mode_juggle(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static int dotPositions[8];
  static uint32_t dotColors[8];
  static bool initialized = false;
  
  if (!initialized || cfg->updated) {
    // Initialize dot positions and colors
    for (int i = 0; i < 8; i++) {
      dotPositions[i] = i * data->pixelCount / 8;
      dotColors[i] = randomColor();
    }
    initialized = true;
  }
  
  unsigned long now = millis();
  
  // Speed controls animation rate
  uint32_t updateInterval = map(cfg->speed, 1, 100, 100, 10);
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    // Apply fade effect first for trails
    StripData* fadeResult = effect_fade(data, 92); // 92% fade for smooth trails
    
    // Copy faded result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, fadeResult->getPixelColor(i));
    }
    delete fadeResult;
    
    // Update and draw dots using effect_swipe
    int dotCount = map(cfg->intensity, 1, 100, 3, 8);
    for (int i = 0; i < dotCount; i++) {
      // Move dot
      dotPositions[i] += 1 + (i * 1); // Different speeds for each dot
      if (dotPositions[i] >= data->pixelCount) {
        dotPositions[i] = 0;
        dotColors[i] = randomColor();
      }
      
      // Draw dot using swipe effect
      StripData* dotResult = effect_swipe(data, 0, dotColors[i], &dotPositions[i]);
      
      // Copy only the new dot pixel
      data->setPixelColor(dotPositions[i], dotResult->getPixelColor(dotPositions[i]));
      delete dotResult;
    }
  }
}