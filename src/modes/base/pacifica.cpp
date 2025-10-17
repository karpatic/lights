#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Gentle layered ocean-style waves inspired by Adafruit's Pacifica.
// Uses speed to control wave speed (higher = faster),
// intensity to control overall brightness (1-100),
// direction to optionally reverse wave travel (0 forward, 1 reverse),
// colorOne (if non-zero) to tint highlights.
void mode_pacifica(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  // Static state for smooth animation
  static uint32_t lastMillis = 0;
  static float phase1 = 0, phase2 = 0, phase3 = 0, phase4 = 0;

  uint32_t now = millis();
  uint32_t delta = now - lastMillis;
  lastMillis = now;

  if (delta > 100) delta = 100; // clamp large gaps

  // Map speed (1-100) to phase increments (base speeds for each layer)
  float speedScale = map(cfg->speed, 1, 100, 5, 120) / 1000.0f; // overall multiplier
  // Individual layer speeds (different to create parallax)
  float inc1 = 12.0f * speedScale;
  float inc2 =  9.0f * speedScale;
  float inc3 =  5.0f * speedScale;
  float inc4 =  2.5f * speedScale;

  // Advance phases (delta compensates for loop timing)
  phase1 += inc1 * delta;
  phase2 += inc2 * delta;
  phase3 += inc3 * delta;
  phase4 += inc4 * delta;

  // Keep phases bounded
  if (phase1 > 10000) phase1 -= 10000;
  if (phase2 > 10000) phase2 -= 10000;
  if (phase3 > 10000) phase3 -= 10000;
  if (phase4 > 10000) phase4 -= 10000;

  // Intensity controls brightness ceiling
  float brightnessScale = constrain(cfg->intensity, 1, 100) / 100.0f;

  // Optional direction reverse
  int dir = (cfg->direction == 1) ? -1 : 1;

  // Simple 4-point blue/teal palette
  const uint32_t palette[] = {
    0x001020, // deep blue
    0x003060, // mid blue
    0x0078A0, // teal
    0x40D8FF  // light aqua highlight
  };
  const int paletteSize = sizeof(palette) / sizeof(palette[0]);

  auto lerp8 = [](uint8_t a, uint8_t b, float t) -> uint8_t {
    return a + (int16_t)((b - a) * t);
  };

  auto blendColor = [&](uint32_t c1, uint32_t c2, float t) -> uint32_t {
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint8_t r = lerp8(r1, r2, t);
    uint8_t g = lerp8(g1, g2, t);
    uint8_t b = lerp8(b1, b2, t);
    return strip.Color(r, g, b);
  };

  auto paletteLookup = [&](float t) -> uint32_t {
    t = fmodf(t, 1.0f);
    if (t < 0) t += 1.0f;
    float pos = t * (paletteSize - 1);
    int idx = (int)pos;
    float frac = pos - idx;
    if (idx >= paletteSize - 1) return palette[paletteSize - 1];
    return blendColor(palette[idx], palette[idx + 1], frac);
  };

  // Optional user tint (colorOne) applied to highlights if non-zero
  bool tintEnabled = (cfg->colorOne != 0);
  uint32_t tint = cfg->colorOne;

  for (int i = 0; i < data->pixelCount; i++) {
    float n = (float)i / (float)(data->pixelCount - 1);
    if (dir < 0) n = 1.0f - n;

    // Layered wave contributions
    float w1 = sinf((n * 6.28318f * 1.0f) + phase1 * 0.010f);
    float w2 = sinf((n * 6.28318f * 1.3f) + phase2 * 0.008f);
    float w3 = sinf((n * 6.28318f * 2.0f) + phase3 * 0.006f);
    float w4 = sinf((n * 6.28318f * 3.0f) + phase4 * 0.004f);

    // Weight layers
    float composite = (w1 * 0.45f) + (w2 * 0.30f) + (w3 * 0.18f) + (w4 * 0.07f);
    // Normalize composite from [-1,1] to [0,1]
    float t = (composite + 1.0f) * 0.5f;

    // Slow drift through palette with phase4 slowest
    float globalShift = fmodf((phase4 * 0.0002f), 1.0f);
    float lookup = fmodf(t * 0.7f + globalShift, 1.0f);

    uint32_t baseColor = paletteLookup(lookup);

    // Extract base
    uint8_t r = (baseColor >> 16) & 0xFF;
    uint8_t g = (baseColor >> 8) & 0xFF;
    uint8_t b = baseColor & 0xFF;

    // Apply subtle depth darkening towards edges
    float edgeDim = 0.85f + 0.15f * sinf((n - 0.5f) * 3.14159f);
    r = (uint8_t)(r * edgeDim);
    g = (uint8_t)(g * edgeDim);
    b = (uint8_t)(b * edgeDim);

    // Apply brightness scale
    r = (uint8_t)(r * brightnessScale);
    g = (uint8_t)(g * brightnessScale);
    b = (uint8_t)(b * brightnessScale);

    // Optional tint blend in highlights (higher t)
    if (tintEnabled && t > 0.65f) {
      float tt = (t - 0.65f) / 0.35f; // 0..1
      if (tt > 1) tt = 1;
      uint8_t tr = (tint >> 16) & 0xFF;
      uint8_t tg = (tint >> 8) & 0xFF;
      uint8_t tb = tint & 0xFF;
      r = lerp8(r, tr, tt * 0.6f); // partial blend
      g = lerp8(g, tg, tt * 0.6f);
      b = lerp8(b, tb, tt * 0.6f);
    }

    data->setPixelColor(i, strip.Color(r, g, b));
  }
}