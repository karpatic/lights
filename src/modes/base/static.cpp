#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Simple Color Swap - effect_static + colorOne
void mode_static(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
 
  // skip if not updated
  if (!cfg->updated) return;
  
  // Fill entire strip with colorOne
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, cfg->colorOne);
  }
}