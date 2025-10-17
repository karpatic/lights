#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Heartbeat effect - pulsing rhythm using effect_fade
void mode_heartbeat(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastBeat = 0;
  static unsigned long lastUpdate = 0;
  static int fadeLevel = 0;
  
  unsigned long now = millis();
  
  // Speed controls heartbeat rate (1-100 maps to slow-fast heart rate)
  uint32_t beatInterval = map(cfg->speed, 1, 100, 1200, 400); // Full heartbeat cycle
  
  // Update interval for smooth fading
  uint32_t updateInterval = 50;
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    // Determine which phase of heartbeat we're in
    uint32_t elapsed = now - lastBeat;
    
    if (elapsed >= beatInterval) {
      lastBeat = now;
      elapsed = 0;
    }
    
    // Calculate target fade level based on heartbeat phase
    int targetFade = 0;
    float cycleProgress = (float)elapsed / beatInterval;
    
    if (cycleProgress < 0.15f) {
      // First beat (quick pulse)
      targetFade = map(cfg->intensity, 1, 100, 50, 100);
    } else if (cycleProgress < 0.25f) {
      // Quick fade after first beat
      targetFade = 0;
    } else if (cycleProgress < 0.4f) {
      // Second beat (slightly weaker)
      targetFade = map(cfg->intensity, 1, 100, 30, 80);
    } else {
      // Rest period
      targetFade = 0;
    }
    
    // Smooth transition to target
    if (fadeLevel < targetFade) {
      fadeLevel += 5;
      if (fadeLevel > targetFade) fadeLevel = targetFade;
    } else if (fadeLevel > targetFade) {
      fadeLevel -= 3;
      if (fadeLevel < targetFade) fadeLevel = targetFade;
    }
    
    // Apply fade effect to existing strip data
    StripData* fadeResult = effect_fade(data, fadeLevel);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, fadeResult->getPixelColor(i));
    }
    
    delete fadeResult;
  }
}