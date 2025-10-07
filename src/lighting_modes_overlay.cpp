
#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Swipe effect that alternates between colorOne and colorTwo
void mode_swipe(StripData* data, const struct_message* config) { 
  const struct_message* cfg = config ? config : &myData;
  static int pixelsFilled = 0;
  static bool usecolorOne = true;
  
  // Default behavior: alternate between colorOne and colorTwo
  uint32_t color = usecolorOne ? cfg->colorOne : cfg->colorTwo;
  StripData* swipeResult = effect_swipe(data, cfg->direction, color);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, swipeResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete swipeResult;
  
  // Track when we've filled all pixels
  if (++pixelsFilled >= data->pixelCount) { 
    usecolorOne = !usecolorOne; 
    pixelsFilled = 0; 
  }
}

// Swipe effect with random colors
void mode_swipe_random(StripData* data, const struct_message* config) { 
  const struct_message* cfg = config ? config : &myData;
  static uint32_t randColor = randomColor();
  static int randPixelsFilled = 0;
  
  StripData* swipeResult = effect_swipe(data, cfg->direction, randColor);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, swipeResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete swipeResult;
  
  if (++randPixelsFilled >= data->pixelCount) {
    randColor = randomColor();
    randPixelsFilled = 0;
  }
}

// Colorloop - cycles all LEDs through rainbow colors
void mode_colorloop(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  static uint8_t colorCounter = 0;
  
  unsigned long now = millis();
  
  // Calculate update interval based on speed (1-100)
  uint32_t loopInterval = map(cfg->speed, 1, 100, 500, 1); // Much faster: 1ms at max speed
  
  if (now - lastUpdate >= loopInterval) {
    lastUpdate = now;
    
    // Get rainbow color from wheel
    uint32_t rainbowColor = Wheel(colorCounter);
    
    // Apply intensity adjustment if needed
    if (cfg->intensity < 100) {
      // Blend with white/black based on intensity
      uint8_t intensityFactor = map(cfg->intensity, 1, 100, 0, 255);
      uint8_t r = (rainbowColor >> 16) & 0xFF;
      uint8_t g = (rainbowColor >> 8) & 0xFF;
      uint8_t b = rainbowColor & 0xFF;
      
      // Scale colors by intensity
      r = (r * intensityFactor) / 255;
      g = (g * intensityFactor) / 255;
      b = (b * intensityFactor) / 255;
      
      rainbowColor = strip.Color(r, g, b);
    }
    
    // Set all pixels to the same rainbow color
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, rainbowColor);
    }
    
    // Increment color counter for next cycle - larger step for faster color changes
    colorCounter += map(cfg->speed, 1, 100, 2, 8); // Variable step based on speed
  }
}

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

// Sweep effect using colorOne - one LED lit at a time moving across strip
void mode_sweep(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed 100 = 50ms (fast), Speed 1 = 1000ms (slow)
  uint32_t sweepInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= sweepInterval) {
    lastUpdate = now;
    
    // Use intensity to control drag length (1-100 maps to 1-10 pixels)
    unsigned dragLength = map(cfg->intensity, 1, 100, 1, 10);
    
    // Fix: Use the count parameter from config instead of calculating from direction
    unsigned count = cfg->count; // Use the actual count parameter
    
    StripData* sweepResult = effect_sweep(data, cfg->direction, cfg->colorOne, dragLength, count);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, sweepResult->getPixelColor(i));
    }
    
    // Clean up temporary data
    delete sweepResult;
  }
}

// Sweep effect with dual colors sweeping in opposite directions.
void mode_sweep_dual(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  static uint32_t lastColorOne = 0;
  static uint32_t lastColorTwo = 0;
  static int lastPixelCount = 0;
  static int lastCount = 0;
  static unsigned long phase = 0; // advances each frame

  unsigned long now = millis();
  // Speed 100 = 50ms, Speed 1 = 1000ms
  uint32_t interval = map(constrain(cfg->speed, 1, 100), 1, 100, 1000, 50);
  if (now - lastUpdate < interval) return;
  lastUpdate = now;

  int pc = data->pixelCount;
  if (pc <= 0) return;

  // Number of heads per direction (default 1). Clamp.
  int heads = cfg->count > 0 ? cfg->count : 1;
  if (heads > pc / 2) heads = pc / 2;
  if (heads < 1) heads = 1;

  // Tail/drag length from intensity (1..10)
  int tail = map(constrain(cfg->intensity, 1, 100), 1, 100, 1, 10);

  // Clear strip each frame
  for (int i = 0; i < pc; i++) data->setPixelColor(i, 0);

  // Precompute colors
  uint8_t r1 = (cfg->colorOne >> 16) & 0xFF;
  uint8_t g1 = (cfg->colorOne >> 8)  & 0xFF;
  uint8_t b1 =  cfg->colorOne        & 0xFF;
  uint8_t r2 = (cfg->colorTwo >> 16) & 0xFF;
  uint8_t g2 = (cfg->colorTwo >> 8)  & 0xFF;
  uint8_t b2 =  cfg->colorTwo        & 0xFF;

  // Spacing between heads
  int spacing = pc / heads;

  // Direction handling: cfg->direction == 0 normal, ==1 swap roles
  bool swapDir = (cfg->direction != 0);

  auto drawHeadGroup = [&](bool forward, uint8_t cr, uint8_t cg, uint8_t cb) {
    for (int h = 0; h < heads; h++) {
      // Base position for this head
      int basePos = ( (int)(phase) + h * spacing ) % pc;
      int pos = forward ? basePos : (pc - 1 - basePos);

      // Draw head + tail
      for (int t = 0; t < tail; t++) {
        int offset = forward ? (pos - t) : (pos + t);
        // Wrap
        while (offset < 0) offset += pc;
        while (offset >= pc) offset -= pc;

        // Fade factor (head brightest)
        float f = 1.0f - (float)t / tail;
        if (f < 0.05f) f = 0.05f;

        uint8_t r = (uint8_t)(cr * f);
        uint8_t g = (uint8_t)(cg * f);
        uint8_t b = (uint8_t)(cb * f);

        uint32_t prev = data->getPixelColor(offset);
        uint8_t pr = (prev >> 16) & 0xFF;
        uint8_t pg = (prev >> 8)  & 0xFF;
        uint8_t pb =  prev        & 0xFF;

        // Additive with clamp
        uint8_t nr = min(255, pr + r);
        uint8_t ng = min(255, pg + g);
        uint8_t nb = min(255, pb + b);

        data->setPixelColor(offset, strip.Color(nr, ng, nb));
      }
    }
  };

  if (!swapDir) {
    // colorOne moves forward, colorTwo backward
    drawHeadGroup(true,  r1, g1, b1);
    drawHeadGroup(false, r2, g2, b2);
  } else {
    // swapped
    drawHeadGroup(true,  r2, g2, b2);
    drawHeadGroup(false, r1, g1, b1);
  }

  phase++; // advance
  // Reset phase if large to avoid overflow (not critical but tidy)
  if (phase > 100000UL) phase = phase % pc;

  lastColorOne = cfg->colorOne;
  lastColorTwo = cfg->colorTwo;
  lastPixelCount = pc;
  lastCount = heads;  // fixed: was stray 'lastCount'
}

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
