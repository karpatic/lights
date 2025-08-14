#ifndef LIGHTING_EFFECTS_H
#define LIGHTING_EFFECTS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "communications.h"

// Strip data structure to hold RGB values for each pixel
struct StripData {
  uint32_t* pixels;
  int pixelCount;
  
  StripData(int count) : pixelCount(count) {
    pixels = new uint32_t[count];
    clear();
  }
  
  ~StripData() {
    delete[] pixels;
  }
  
  void clear() {
    for (int i = 0; i < pixelCount; i++) {
      pixels[i] = 0;
    }
  }
  
  void setPixelColor(int index, uint32_t color) {
    if (index >= 0 && index < pixelCount) {
      pixels[index] = color;
    }
  }
  
  uint32_t getPixelColor(int index) {
    if (index >= 0 && index < pixelCount) {
      return pixels[index];
    }
    return 0;
  }
};

extern struct_message myData;
extern Adafruit_NeoPixel strip;

// Strip data arrays for transition blending
extern StripData* stripData;
extern StripData* stripDataOld;

// Transition management
extern int transitionValue; 

// Utility functions
uint32_t Wheel(byte WheelPos);
StripData* createColoredStripData(int pixelCount, uint32_t color);
StripData* cloneStripData(StripData* source);
uint32_t randomColor();

// Effect functions
StripData* effect_static(uint32_t color);
StripData* effect_fade(StripData* data, unsigned intensity);
StripData* effect_range(StripData* data, int startPixel, int endPixel, unsigned fadeIntensity);
StripData* effect_shift(StripData* data, unsigned direction);
StripData* effect_blink(StripData* sourceData, StripData* backgroundData, uint32_t blinkInterval);
StripData* effect_swipe(StripData* data, unsigned direction, uint32_t color, int* overridePixelIndex = nullptr);
StripData* effect_sweep(StripData* data, unsigned direction, uint32_t color, unsigned dragLength, unsigned count = 1, int* overridePixelIndex = nullptr);
StripData* effect_breath(StripData* data, unsigned intensity);


// Effect dispatcher function
void callModeFunction(const String& effect, StripData* data, const struct_message* config = nullptr);

#endif