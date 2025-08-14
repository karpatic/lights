#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Utility functions
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

StripData* createColoredStripData(int pixelCount, uint32_t color) {
  StripData* newData = new StripData(pixelCount);
  for (int i = 0; i < pixelCount; i++) {
    newData->setPixelColor(i, color);
  }
  return newData;
}

StripData* cloneStripData(StripData* source) {
  StripData* clone = new StripData(source->pixelCount);
  for (int i = 0; i < source->pixelCount; i++) {
    clone->setPixelColor(i, source->getPixelColor(i));
  }
  return clone;
}

uint32_t randomColor() {
  return strip.Color(random(0, 255), random(0, 255), random(0, 255));
}

// ~~~~~~~~~~~~~~~~~
// Effect Functions
// ~~~~~~~~~~~~~~~~~

StripData* effect_static(uint32_t color) { 
  StripData* result = createColoredStripData(myData.pixelCount, color); 
  return result;
}

// Fade effect - dims colors  (0-100%) based on intensity value
StripData* effect_fade(StripData* data, unsigned intensity) { 
  StripData* result = cloneStripData(data);
  uint8_t fadeFactor = map(intensity, 0, 100, 0, 255); // (0-255)
  
  for (int i = 0; i < result->pixelCount; i++) {
    uint32_t color = result->getPixelColor(i);
    
    // Extract RGB components
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Apply fade factor
    r = (r * fadeFactor) / 255;
    g = (g * fadeFactor) / 255;
    b = (b * fadeFactor) / 255;
    
    result->setPixelColor(i, strip.Color(r, g, b));
  }
  return result;
}

// Percentage effect - Fades out of range LEDs based on intensity value (0-100%)
StripData* effect_range(StripData* data, int startPixel, int endPixel, unsigned outOfRangeBrightness) { 
  StripData* result = cloneStripData(data);
  // Ensure valid range
  if (startPixel < 0) startPixel = 0;
  if (endPixel >= result->pixelCount) endPixel = result->pixelCount - 1;
  if (startPixel > endPixel) {
    int temp = startPixel;
    startPixel = endPixel;
    endPixel = temp;
  }
  
  // Calculate brightness factor for pixels outside the range
  uint8_t brightnessFactor = map(outOfRangeBrightness, 0, 100, 0, 255);
  
  // Apply brightness to pixels outside the specified range
  for (int i = 0; i < result->pixelCount; i++) {
    if (i < startPixel || i > endPixel) {
      uint32_t color = result->getPixelColor(i);
      
      // Extract RGB components
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      // Apply brightness factor
      r = (r * brightnessFactor) / 255;
      g = (g * brightnessFactor) / 255;
      b = (b * brightnessFactor) / 255;
      
      result->setPixelColor(i, strip.Color(r, g, b));
    }
    // Pixels in range keep their original color (no change needed)
  }
  
  return result;
}

// Shift effect - shifts all pixel colors by one position
StripData* effect_shift(StripData* data, unsigned direction) {
  // Create a copy to return
  StripData* result = new StripData(data->pixelCount);
  
  if (data->pixelCount <= 1) {
    // If only one pixel or empty strip, just copy the data
    for (int i = 0; i < data->pixelCount; i++) {
      result->setPixelColor(i, data->getPixelColor(i));
    }
    return result;
  }
  
  if (direction == 1) {
    // Shift left (direction = 1)
    for (int i = 0; i < data->pixelCount; i++) {
      int sourceIndex = (i + 1) % data->pixelCount;
      result->setPixelColor(i, data->getPixelColor(sourceIndex));
    }
  } else {
    // Shift right (direction = 0 or any other value)
    for (int i = 0; i < data->pixelCount; i++) {
      int sourceIndex = (i - 1 + data->pixelCount) % data->pixelCount;
      result->setPixelColor(i, data->getPixelColor(sourceIndex));
    }
  }
  
  return result;
}

// Can do blink - returns modified stripdata copy
StripData* effect_blink(StripData* data, StripData* data2, uint32_t blinkInterval) {
  static unsigned long lastToggle = 0;
  static bool isOn = true;
  
  uint32_t now = millis();
  uint32_t interval = (blinkInterval > 100) ? blinkInterval : 100; // Minimum 100ms interval
  
  // Simple toggle logic: check if enough time has passed
  if (now - lastToggle >= interval) {
    isOn = !isOn;
    lastToggle = now;
  }
  
  // Create a copy to return
  StripData* result = new StripData(data->pixelCount);
  
  // Set colors based on blink state
  for (int i = 0; i < data->pixelCount; i++) {
    result->setPixelColor(i, isOn ? data->getPixelColor(i) : (data2 ? data2->getPixelColor(i) : 0x000000));
  }
  
  return result;
}

// Flexible swipe effect - An LED is lit at a time, moving across the strip till it all are lit.
StripData* effect_swipe(StripData* data, unsigned direction, uint32_t color, int* overridePixelIndex) {
  static int pixel = 0;
  if (overridePixelIndex) { pixel = *overridePixelIndex; }
  
  // Create a copy to return
  StripData* result = new StripData(data->pixelCount); 
  for (int i = 0; i < data->pixelCount; i++) {
    result->setPixelColor(i, data->getPixelColor(i));
  }
  
  result->setPixelColor(pixel, color);
  // Move to next pixel based on direction
  pixel = (pixel + (direction == 1 ? -1 : 1) + data->pixelCount) % data->pixelCount;
  
  return result;
}

// Sweep - n leds are lit at a time, moving across the strip without changing colors for good.
StripData* effect_sweep(StripData* data, unsigned direction, uint32_t color, unsigned intensity, unsigned count, int* overridePixelIndex) {
  static int pixel = 0;
  static int bounceDirection = 1; // 1 for forward, -1 for backward
  
  if (overridePixelIndex) { 
    pixel = *overridePixelIndex; 
    bounceDirection = 1; // Reset bounce direction when pixel is overridden
  }
  
  // Create a copy to return
  StripData* result = new StripData(data->pixelCount);
  
  // Clear all pixels first (sweep shows only intensity pixels at a time)
  for (int i = 0; i < data->pixelCount; i++) {
    result->setPixelColor(i, 0x000000);
  }
  
  // Calculate spacing between instances
  int spacing = count > 1 ? data->pixelCount / count : data->pixelCount;
  
  // Create multiple sweep instances
  for (unsigned instance = 0; instance < count; instance++) {
    int basePixel = (pixel + instance * spacing) % data->pixelCount;
    
    // Set the current pixel and trailing pixels based on intensity for this instance
    for (unsigned i = 0; i < intensity && i < (unsigned)data->pixelCount; i++) {
      int pixelIndex = (basePixel - i + data->pixelCount) % data->pixelCount;
      
      // Calculate fade intensity for trailing pixels (full brightness for main pixel, dimmer for trail)
      uint8_t fadeIntensity = 255 - (i * 255 / intensity);
      
      // Extract RGB components and apply fade
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      r = (r * fadeIntensity) / 255;
      g = (g * fadeIntensity) / 255;
      b = (b * fadeIntensity) / 255;

      result->setPixelColor(pixelIndex, strip.Color(r, g, b));
    }
  }
  
  // Move to next pixel based on direction
  if (direction == 2) {
    // Bounce mode - check for bounce before moving
    if (pixel >= data->pixelCount - 1) {
      bounceDirection = -1;
    } else if (pixel <= 0) {
      bounceDirection = 1;
    }
    
    pixel += bounceDirection;
  } else {
    // Original behavior for direction 0 and 1
    pixel = (pixel + (direction == 1 ? -1 : 1) + data->pixelCount) % data->pixelCount;
  }
  
  return result;
}
