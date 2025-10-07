// | | Pacifica          | Speed, Angle                  |                | Gentle ocean waves                                                                                                                    |
#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Forward declare smoothstep so it can be used before its definition
static inline float smoothstep(float edge0, float edge1, float x);

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




// | | Sunrise           | Time [min], Width             |                | Simulates a gradual sunrise or sunset. Speed sets: 0 – static sun; 1–60 min sunrise; 60–120 min sunset; >120 “breathing” rise and set |
// | | Aurora            | Speed, Intensity              |                | Simulation of the Aurora Borealis                                                                                                     |

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

// Helper smoothstep
// (Placed after functions to keep minimal additions)
static inline float smoothstep(float edge0, float edge1, float x) {
  float t = constrain((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}
