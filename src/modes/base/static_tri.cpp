#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Static pattern - colorOne, colorTwo, colorThree
void mode_static_tri(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  // skip if not updated
  if (!cfg->updated) return;

  // Create triangular pattern using three colors
  // Divide strip into three equal sections
  int sectionSize = data->pixelCount / 3;
  
  for (int i = 0; i < data->pixelCount; i++) {
    uint32_t color;
    
    if (i < sectionSize) {
      color = cfg->colorOne;
    } else if (i < sectionSize * 2) {
      color = cfg->colorTwo;
    } else {
      color = cfg->colorThree;
    }
    
    data->setPixelColor(i, color);
  }
}