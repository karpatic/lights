#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Tetrix effect - uses effect_static for blocks and custom stacking logic
void mode_tetrix(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static int blockPosition = 0;
  static uint32_t blockColor = 0;
  static bool blockActive = false;
  static bool* stackPixels = nullptr;
  static int stackHeight = 0;
  
  // Initialize stack if needed
  if (stackPixels == nullptr || cfg->updated) {
    if (stackPixels) delete[] stackPixels;
    stackPixels = new bool[data->pixelCount];
    for (int i = 0; i < data->pixelCount; i++) {
      stackPixels[i] = false;
    }
    stackHeight = 0;
  }
  
  unsigned long now = millis();
  
  // Speed controls fall rate
  uint32_t fallInterval = map(cfg->speed, 1, 100, 300, 30);
  
  if (now - lastUpdate >= fallInterval) {
    lastUpdate = now;
    
    if (!blockActive) {
      // Spawn new block
      blockPosition = data->pixelCount - 1;
      blockColor = (cfg->colorOne != 0) ? cfg->colorOne : randomColor();
      blockActive = true;
    } else {
      // Move block down
      blockPosition--;
      
      // Check if block hits stack or bottom
      if (blockPosition <= stackHeight || blockPosition < 0) {
        // Block lands
        int landPos = max(0, blockPosition + 1);
        if (landPos < data->pixelCount && !stackPixels[landPos]) {
          stackPixels[landPos] = true;
          stackHeight = max(stackHeight, landPos + 1);
        }
        blockActive = false;
        
        // Check for game over (stack reaches top)
        if (stackHeight >= data->pixelCount * 0.9f) {
          // Reset stack
          for (int i = 0; i < data->pixelCount; i++) {
            stackPixels[i] = false;
          }
          stackHeight = 0;
        }
      }
    }
    
    // Clear strip first
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, 0);
    }
    
    // Draw stack using effect_static for each stack pixel
    uint32_t stackColor = (cfg->colorTwo != 0) ? cfg->colorTwo : 0x404040;
    for (int i = 0; i < stackHeight; i++) {
      if (stackPixels[i]) {
        data->setPixelColor(i, stackColor);
      }
    }
    
    // Draw falling block
    if (blockActive && blockPosition >= 0 && blockPosition < data->pixelCount) {
      data->setPixelColor(blockPosition, blockColor);
    }
  }
}