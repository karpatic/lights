#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Candle flicker mode
void mode_candle(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  static unsigned long lastUpdate = 0;
  static uint32_t* lastColors = nullptr;
  static int lastPixelCount = 0;

  int pc = data->pixelCount;
  if (pc <= 0) return;

  // (Re)allocate cache if size changed
  if (pc != lastPixelCount) {
    if (lastColors) delete[] lastColors;
    lastColors = new uint32_t[pc];
    for (int i = 0; i < pc; i++) lastColors[i] = 0;
    lastPixelCount = pc;
  }

  // Map speed to update interval (slow=120ms fast=18ms)
  uint32_t interval = map(constrain(cfg->speed, 1, 100), 1, 100, 120, 18);
  unsigned long now = millis();
  bool doUpdate = (now - lastUpdate) >= interval;
  if (!doUpdate) {
    // Reuse previous frame
    for (int i = 0; i < pc; i++) data->setPixelColor(i, lastColors[i]);
    return;
  }
  lastUpdate = now;

  // Base (fallback) candle color
  uint32_t base = (cfg->colorOne != 0) ? cfg->colorOne : 0xFF8A2C; // warm amber
  uint8_t baseR = (base >> 16) & 0xFF;
  uint8_t baseG = (base >> 8) & 0xFF;
  uint8_t baseB = base & 0xFF;

  // Determine candle cluster centers
  int clusterCount = cfg->count > 0 ? cfg->count : max(1, pc / 60);
  if (clusterCount > 16) clusterCount = 16;

  // Precompute cluster centers
  static int centers[16];
  static int prevPc = -1;
  static int prevClusters = -1;
  if (cfg->updated || prevPc != pc || prevClusters != clusterCount) {
    for (int c = 0; c < clusterCount; c++) {
      centers[c] = (int)(( (float)c + 0.5f) * pc / clusterCount);
    }
    prevPc = pc;
    prevClusters = clusterCount;
  }

  // Smoothing factor (higher speed = less smoothing)
  float smooth = map(constrain(cfg->speed, 1, 100), 1, 100, 90, 30) / 100.0f;

  // Overall brightness ceiling from intensity
  float intensityScale = constrain(cfg->intensity, 1, 100) / 100.0f;

  // For each pixel compute distance to nearest candle center for spatial falloff
  for (int i = 0; i < pc; i++) {
    int nearestDist = pc;
    for (int c = 0; c < clusterCount; c++) {
      int d = abs(i - centers[c]);
      if (d < nearestDist) nearestDist = d;
    }
    // Spatial falloff (soft)
    float falloff = expf(-(nearestDist * nearestDist) / (float)(pc)); // gentle spread
    if (falloff < 0.02f) falloff = 0.02f;

    // Random flicker components
    // Base random between 0.55 and 1.00
    float rnd = (random(55, 101)) / 100.0f;

    // Occasional deep dip
    if (random(0, 1000) < 3) rnd *= 0.35f;

    // Occasional brief surge
    if (random(0, 1000) < 5) rnd = min(1.15f, rnd * 1.15f);

    // Combine
    float target = rnd * falloff * intensityScale;

    // Previous color brightness estimate
    uint32_t prev = lastColors[i];
    uint8_t pr = (prev >> 16) & 0xFF;
    uint8_t pg = (prev >> 8) & 0xFF;
    uint8_t pb = prev & 0xFF;
    float prevBrightness = 0.0f;
    if (baseR) prevBrightness = max(prevBrightness, pr / (float)baseR);
    if (baseG) prevBrightness = max(prevBrightness, pg / (float)baseG);
    if (baseB) prevBrightness = max(prevBrightness, pb / (float)baseB);
    if (prevBrightness > 1.5f) prevBrightness = 1.5f;

    // Smooth
    float brightness = prevBrightness * smooth + target * (1.0f - smooth);
    if (brightness > 1.2f) brightness = 1.2f;

    // Slight warm color shift (more red when dimmer)
    float warmth = 0.15f + 0.85f * brightness;
    uint8_t r = (uint8_t)constrain(baseR * brightness, 0.0f, 255.0f);
    uint8_t g = (uint8_t)constrain(baseG * brightness * (0.85f + 0.15f * warmth), 0.0f, 255.0f);
    uint8_t b = (uint8_t)constrain(baseB * brightness * (0.70f + 0.30f * warmth), 0.0f, 255.0f);

    uint32_t out = strip.Color(r, g, b);
    data->setPixelColor(i, out);
    lastColors[i] = out;
  }
}