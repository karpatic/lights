#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Smoothstep function for smooth transitions
static inline float smoothstep(float edge0, float edge1, float x) {
  float t = constrain((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// Aurora Mode
void mode_aurora(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  static uint32_t lastMs = 0;
  static float p1 = 0, p2 = 0, p3 = 0;

  uint32_t now = millis();
  uint32_t dt = now - lastMs;
  lastMs = now;
  if (dt > 100) dt = 100;

  float sp = map(cfg->speed, 1, 100, 5, 160) / 1000.0f;
  p1 += dt * 14.0f * sp;
  p2 += dt * 9.0f * sp;
  p3 += dt * 5.0f * sp;

  if (p1 > 10000) p1 -= 10000;
  if (p2 > 10000) p2 -= 10000;
  if (p3 > 10000) p3 -= 10000;

  float brightScale = constrain(cfg->intensity, 1U, 100U) / 100.0f;

  bool reverse = (cfg->direction == 1);

  // Palette anchors
  uint32_t darkBase = 0x000008;
  uint32_t greenMid = (cfg->colorOne != 0) ? cfg->colorOne : 0x004830;
  uint32_t greenBright = 0x00FF90;
  uint32_t purpleMid = (cfg->colorTwo != 0) ? cfg->colorTwo : 0x401080;
  uint32_t purpleBright = 0xB060FF;

  auto lerp8 = [](uint8_t a, uint8_t b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (uint8_t)(a + (int16_t)((b - a) * t));
  };
  auto blend = [&](uint32_t a, uint32_t b, float t) {
    uint8_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint8_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    return strip.Color(lerp8(ar, br, t), lerp8(ag, bg, t), lerp8(ab, bb, t));
  };

  int pc = data->pixelCount;
  for (int i = 0; i < pc; i++) {
    float n = (float)i / (float)(pc - 1);
    if (reverse) n = 1.0f - n;

    // Layered sin "noise"
    float a = sinf(n * 6.28318f * 1.2f + p1 * 0.010f);
    float b = sinf(n * 6.28318f * 2.3f + p2 * 0.008f);
    float c = sinf(n * 6.28318f * 3.7f + p3 * 0.006f);

    float composite = (a * 0.55f) + (b * 0.30f) + (c * 0.15f);
    float t = (composite + 1.0f) * 0.5f;

    // Split ribbons: greens lower half, purples higher, with crossfade
    float greenWeight = 1.0f - smoothstep(0.45f, 0.85f, n);
    float purpleWeight = smoothstep(0.15f, 0.55f, n);

    // Brightness modulation ripple
    float ripple = 0.55f + 0.45f * sinf(n * 6.28318f * 0.7f + p2 * 0.004f + sinf(p3 * 0.002f));
    float localBrightness = (0.2f + 0.8f * t) * ripple;

    // Build green ribbon color
    uint32_t gColor = blend(greenMid, greenBright, powf(t, 1.2f));
    // Build purple ribbon color
    uint32_t pColor = blend(purpleMid, purpleBright, powf(t, 0.9f));

    // Mix ribbons
    float totalW = greenWeight + purpleWeight + 0.0001f;
    float gw = greenWeight / totalW;
    float pw = purpleWeight / totalW;

    uint8_t gr = (gColor >> 16) & 0xFF;
    uint8_t gg = (gColor >> 8) & 0xFF;
    uint8_t gb = gColor & 0xFF;

    uint8_t pr = (pColor >> 16) & 0xFF;
    uint8_t pg = (pColor >> 8) & 0xFF;
    uint8_t pb = pColor & 0xFF;

    // Weighted mix
    uint32_t mixed = strip.Color(
      (uint8_t)(gr * gw + pr * pw),
      (uint8_t)(gg * gw + pg * pw),
      (uint8_t)(gb * gw + pb * pw)
    );

    // Blend with dark base (depth)
    mixed = blend(darkBase, mixed, 0.65f + 0.35f * t);

    // Apply brightness scaling
    uint8_t r = (mixed >> 16) & 0xFF;
    uint8_t g = (mixed >> 8) & 0xFF;
    uint8_t b2 = mixed & 0xFF;

    float finalScale = brightScale * localBrightness;
    if (finalScale > 1.0f) finalScale = 1.0f;

    r = (uint8_t)(r * finalScale);
    g = (uint8_t)(g * finalScale);
    b2 = (uint8_t)(b2 * finalScale);

    data->setPixelColor(i, strip.Color(r, g, b2));
  }
}