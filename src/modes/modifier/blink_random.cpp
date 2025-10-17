#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// blink random colors
void mode_blink_random(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static uint32_t currentRandomColor = randomColor(); // Generate once and store
  static unsigned long lastColorChange = 0;
  static bool wasOn = false; // Track previous blink state
  
  // Use speed to calculate blink interval
  uint32_t blinkInterval = map(cfg->speed, 1, 100, 1000, 100); // Slower speed = longer interval
  
  StripData* colorSource = createColoredStripData(data->pixelCount, currentRandomColor);
  StripData* blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  
  // Check if light is currently on (non-zero color means on)
  bool isOn = (blinkResult->getPixelColor(0) != 0);
  
  // Generate new color when transitioning from off to on
  if (isOn && !wasOn) {
    currentRandomColor = randomColor();
    // Recreate the color source with the new color
    delete colorSource;
    colorSource = createColoredStripData(data->pixelCount, currentRandomColor);
    delete blinkResult;
    blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  }
  
  wasOn = isOn;
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, blinkResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete colorSource;
  delete blinkResult;
}