#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Breath effect - smooth global brightness modulation of a captured base frame
void mode_breath(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  // Static state
  static StripData*   baseFrame      = nullptr;
  static int          basePixelCount = 0;
  static uint32_t     lastColorOne   = 0;
  static uint32_t     lastColorTwo   = 0;
  static uint32_t     lastColorThree = 0;
  static String       lastMode;               // To detect entry
  static unsigned long cycleStart    = 0;
  static uint32_t     cycleMillis    = 4000;  // full in+out duration

  // Helper: (re)build base frame
  auto rebuildBase = [&]() {
    if (baseFrame) { delete baseFrame; baseFrame = nullptr; }
    baseFrame = new StripData(data->pixelCount);
    basePixelCount = data->pixelCount;

    // If existing data already has non‑black pixels, keep them (inherit previous mode)
    bool anyLit = false;
    for (int i = 0; i < data->pixelCount; i++) {
      if (data->getPixelColor(i) != 0) { anyLit = true; break; }
    }

    if (anyLit) {
      for (int i = 0; i < data->pixelCount; i++) {
        baseFrame->setPixelColor(i, data->getPixelColor(i));
      }
    } else {
      // Fallback: build a simple tri pattern from user colors
      int section = max(1, data->pixelCount / 3);
      for (int i = 0; i < data->pixelCount; i++) {
        uint32_t c = (i < section) ? cfg->colorOne :
                     (i < section * 2) ? cfg->colorTwo : cfg->colorThree;
        baseFrame->setPixelColor(i, c);
      }
    }

    cycleStart = millis();
  };

  bool entering = (lastMode != "breath");
  bool colorChanged = (cfg->colorOne != lastColorOne ||
                       cfg->colorTwo != lastColorTwo ||
                       cfg->colorThree != lastColorThree);
  bool sizeChanged = (basePixelCount != data->pixelCount);

  if (entering || colorChanged || sizeChanged || cfg->updated) {
    rebuildBase();
    lastMode       = "breath";
    lastColorOne   = cfg->colorOne;
    lastColorTwo   = cfg->colorTwo;
    lastColorThree = cfg->colorThree;
  }

  if (!baseFrame) return;

  // Map speed (1..100) to full cycle length (ms)
  // Slow (1) ≈ 9000 ms, Fast (100) ≈ 1500 ms
  cycleMillis = map(constrain(cfg->speed, 1, 100), 1, 100, 9000, 1500);

  unsigned long now = millis();
  unsigned long elapsed = (now - cycleStart) % cycleMillis;
  float phase = (float)elapsed / (float)cycleMillis; // 0..1

  // Sine wave 0..1
  float wave = (sinf(phase * TWO_PI) + 1.0f) * 0.5f;

  // Intensity sets minimum brightness floor (5%..70%)
  float minFloor = 0.05f + (constrain(cfg->intensity, 1, 100) / 100.0f) * 0.65f;

  // Optional ease in/out (make breathing softer)
  // Apply a smoothstep to wave
  wave = wave * wave * (3.0f - 2.0f * wave);

  float brightness = minFloor + wave * (1.0f - minFloor);

  // Gamma compensation (simple 2.2)
  auto applyGamma = [](uint8_t v, float g) -> uint8_t {
    if (v == 0) return 0;
    float nv = powf(v / 255.0f, g);
    return (uint8_t)(nv * 255.0f + 0.5f);
  };
  const float gamma = 2.2f;

  for (int i = 0; i < data->pixelCount; i++) {
    uint32_t baseCol = baseFrame->getPixelColor(i);
    if (baseCol == 0) {
      data->setPixelColor(i, 0);
      continue;
    }
    uint8_t r = (baseCol >> 16) & 0xFF;
    uint8_t g = (baseCol >> 8)  & 0xFF;
    uint8_t b =  baseCol        & 0xFF;

    // Scale by brightness (float) then gamma
    r = applyGamma((uint8_t)(r * brightness), gamma);
    g = applyGamma((uint8_t)(g * brightness), gamma);
    b = applyGamma((uint8_t)(b * brightness), gamma);

    data->setPixelColor(i, strip.Color(r, g, b));
  }
}