#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Sunrise Mode
// speed meaning (minutes / behavior):
//   0  -> static full sunrise
//   1-60 -> sunrise over 'speed' minutes (dark -> bright)
//   61-120 -> sunset over (speed-60) minutes (bright -> dark)  (if >60 given)
// intensity -> sun width / glow size (1-100)
// colorOne optional sun core color (default warm)
// colorTwo optional sky high color
// direction: 0 normal, 1 reverse strip direction (sun position mirrored)
void mode_sunrise(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  static uint32_t phaseStart = 0;
  static bool wasRunning = false;
  static uint8_t lastSpeed = 0;

  uint32_t now = millis();

  // Reset animation when config updated or speed changed
  if (cfg->updated || cfg->speed != lastSpeed) {
    phaseStart = now;
    lastSpeed = cfg->speed;
    wasRunning = false;
  }

  int pc = data->pixelCount;
  if (pc <= 0) return;

  // Determine mode
  uint8_t spd = cfg->speed;
  bool staticMode = (spd == 0);
  bool sunset = (spd > 60);
  uint16_t minutes = 0;

  if (!staticMode) {
    if (!sunset) {
      minutes = constrain(spd, 1U, 60U);
    } else {
      // map 61-120 -> 1-60 (if >100 still handled)
      minutes = constrain((uint16_t)(spd - 60), 1U, (uint16_t)60);
    }
  }

  float progress = 1.0f;
  if (!staticMode) {
    uint32_t durationMs = (uint32_t)minutes * 60000UL;
    if (durationMs == 0) durationMs = 1;
    uint32_t elapsed = now - phaseStart;
    if (elapsed >= durationMs) {
      elapsed = durationMs;
      wasRunning = true;
    }
    progress = (float)elapsed / (float)durationMs;
    if (sunset) {
      progress = 1.0f - progress; // invert for sunset
    }
    progress = constrain(progress, 0.0f, 1.0f);
  }

  // Colors
  uint32_t sunCore = (cfg->colorOne != 0) ? cfg->colorOne : 0xFF7A20;      // warm orange
  uint32_t sunEdge = 0xFFD090;
  uint32_t skyLow  = 0x000020; // pre-dawn
  uint32_t skyHigh = (cfg->colorTwo != 0) ? cfg->colorTwo : 0x87CEEB;      // light blue

  // Sun width control
  int sunWidth = map((int)cfg->intensity, 1, 100, max(2, pc / 40), max(4, pc / 3));
  float sunCenter; // position in [0, pc-1]
  // Animate sun from just below start edge to ~60% of strip
  float startPos = -sunWidth * 0.6f;
  float endPos   = (float)pc * 0.60f;
  sunCenter = startPos + progress * (endPos - startPos);

  // Optionally reverse direction
  bool reverse = (cfg->direction == 1);

  auto blend = [&](uint32_t a, uint32_t b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    uint8_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint8_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint8_t r = ar + (int16_t)((br - ar) * t);
    uint8_t g = ag + (int16_t)((bg - ag) * t);
    uint8_t b2 = ab + (int16_t)((bb - ab) * t);
    return strip.Color(r, g, b2);
  };

  // Sky gradient factor (top vs bottom)
  for (int i = 0; i < pc; i++) {
    int idx = reverse ? (pc - 1 - i) : i;
    float y = (float)i / (float)(pc - 1);

    // Base sky gradient: transition accelerates with sunrise progress
    float skyBlend = powf(y, 1.4f) * (0.3f + 0.7f * progress);
    uint32_t skyColor = blend(skyLow, skyHigh, skyBlend);

    // Sun glow
    float dist = ((float)idx - sunCenter) / (float)sunWidth;
    // Gaussian-ish falloff
    float glow = expf(-dist * dist * 3.0f);
    // Modulate glow by progress (sun appears gradually)
    glow *= progress;

    // Compose sun core and edge
    uint32_t sunColor = blend(sunEdge, sunCore, powf(glow, 0.35f));

    // Mix sky and sun (screen-like blend approximation)
    uint8_t sr = (sunColor >> 16) & 0xFF;
    uint8_t sg = (sunColor >> 8) & 0xFF;
    uint8_t sb = sunColor & 0xFF;

    uint8_t kr = (skyColor >> 16) & 0xFF;
    uint8_t kg = (skyColor >> 8) & 0xFF;
    uint8_t kb = skyColor & 0xFF;

    float sunAlpha = constrain(glow, 0.0f, 1.0f);
    // Soft blend
    uint8_t r = kr + (uint8_t)((sr - kr) * sunAlpha);
    uint8_t g = kg + (uint8_t)((sg - kg) * sunAlpha);
    uint8_t b = kb + (uint8_t)((sb - kb) * sunAlpha);

    // Slight horizon brightening near bottom as progress advances
    float horizonBoost = (1.0f - y);
    float boost = 0.15f * progress * expf(-y * 6.0f) * (1.0f + 0.5f * horizonBoost);
    r = (uint8_t)constrain(r + r * boost, 0.0f, 255.0f);
    g = (uint8_t)constrain(g + g * boost * 0.8f, 0.0f, 255.0f);
    b = (uint8_t)constrain(b + b * boost * 0.4f, 0.0f, 255.0f);

    data->setPixelColor(idx, strip.Color(r, g, b));
  }

  // Static full sunrise mode (speed == 0): ensure full progress state
  if (staticMode) {
    // Just ensure brightness scaled by intensity (optional)
    float scale = constrain(cfg->intensity, 1U, 100U) / 100.0f;
    if (scale < 0.999f) {
      for (int i = 0; i < pc; i++) {
        uint32_t c = data->getPixelColor(i);
        uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * scale);
        uint8_t g = (uint8_t)(((c >> 8) & 0xFF) * scale);
        uint8_t b = (uint8_t)((c & 0xFF) * scale);
        data->setPixelColor(i, strip.Color(r, g, b));
      }
    }
  }
}