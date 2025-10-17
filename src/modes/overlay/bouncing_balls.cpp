#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Bouncing balls effect - uses effect_fade for trails and manual ball physics
void mode_bouncing_balls(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  static unsigned long lastUpdate = 0;
  static float ballPositions[8];
  static float ballVelocities[8];
  static uint32_t ballColors[8];
  static bool initialized = false;
  
  if (!initialized || cfg->updated) {
    // Initialize balls
    int ballCount = map(cfg->intensity, 1, 100, 2, 8);
    for (int i = 0; i < ballCount; i++) {
      ballPositions[i] = random(0, data->pixelCount);
      ballVelocities[i] = random(50, 150) / 100.0f;
      ballColors[i] = (i < 3) ? (i == 0 ? cfg->colorOne : (i == 1 ? cfg->colorTwo : cfg->colorThree)) : randomColor();
      if (ballColors[i] == 0) ballColors[i] = randomColor(); // Ensure non-zero colors
    }
    initialized = true;
  }
  
  unsigned long now = millis();
  
  // Speed controls animation rate
  uint32_t updateInterval = map(cfg->speed, 1, 100, 50, 5);
  
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    
    // Apply fade effect for trails
    StripData* fadeResult = effect_fade(data, 85); // 85% fade for ball trails
    
    // Copy faded result back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, fadeResult->getPixelColor(i));
    }
    delete fadeResult;
    
    // Update and draw balls
    int ballCount = map(cfg->intensity, 1, 100, 2, 8);
    float gravity = 0.05f;
    
    for (int i = 0; i < ballCount; i++) {
      // Apply gravity
      ballVelocities[i] -= gravity;
      ballPositions[i] += ballVelocities[i];
      
      // Bounce off ground
      if (ballPositions[i] <= 0) {
        ballPositions[i] = 0;
        ballVelocities[i] *= -0.8f; // Energy loss on bounce
        if (ballVelocities[i] < 0.5f) {
          ballVelocities[i] = random(50, 120) / 100.0f; // Reset if too slow
        }
      }
      
      // Bounce off ceiling
      if (ballPositions[i] >= data->pixelCount - 1) {
        ballPositions[i] = data->pixelCount - 1;
        ballVelocities[i] *= -0.8f;
      }
      
      // Draw ball
      int pos = (int)ballPositions[i];
      if (pos >= 0 && pos < data->pixelCount) {
        data->setPixelColor(pos, ballColors[i]);
      }
    }
  }
}