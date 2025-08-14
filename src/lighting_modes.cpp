#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Forward declaration for special mode in lighting_modes_special.cpp
void mode_pacifica(StripData* data, const struct_message* config);
void mode_sunrise(StripData* data, const struct_message* config); // added
void mode_aurora(StripData* data, const struct_message* config);  // added

// 
//  Modes are standalone or a combination of Effects.
//
//  Modes are given a pointer to the real LEDs stripData 'data' variable that update immediately.
//  Effects are called by Modes and are given a copy of the LED stripData to be modified and returned to the Mode.
//  the effects should not use struct_message config directly. those are reserved for modes
//  
//  config attribributes: 
//    speed (1-100), intensity(1-100), direction - Variable attributes for effects 
//    colorOne, colorTwo, colorThree - Users colors for some effects
//
//  Composability issues:
//  - Some effects can't write directly to the stripData (eg. blink turning off loses the previous stripColors).
//  - Some effects need to be called in a specific order (eg. fade before blink).
//  - Some effects need to be called multiple times to achieve the desired effect (eg. swipe). 

// Swipe Mode - Wipes StripData on colorchange and applies new color at Stored Index. 
// Sweep Mode - Uses a black bg.

// [Shift, Breath] Inherit the previous modes colors. Do not update on color change.


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

// Static pattern - colorOne, colorTwo, colorThree
void mode_static_tri(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  
  // skip if not updated
  if (!cfg->updated) return;

  // Create triangular pattern using three colors
  // Divide strip into three equal sections
  int sectionSize = data->pixelCount / 3;
  
  for (int i = 0; i < data->pixelCount; i++) {
    uint32_t color;
    
    if (i < sectionSize) {
      color = cfg->colorOne;
    } else if (i < sectionSize * 2) {
      color = cfg->colorTwo;
    } else {
      color = cfg->colorThree;
    }
    
    data->setPixelColor(i, color);
  }
}

// Percentage display mode -  effect_range + speed + colorOne
void mode_percent(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  // skip if not updated
  if (!cfg->updated) return;
  
  // Calculate how many pixels to fill based on intensity
  int pixelsToFill = map(cfg->intensity, 0, 100, 0, data->pixelCount);
  
  // Create a strip with colorOne for the filled portion and black for the rest
  StripData* colorData = new StripData(data->pixelCount);
  for (int i = 0; i < data->pixelCount; i++) {
    if (i < pixelsToFill) {
      colorData->setPixelColor(i, cfg->colorOne);
    } else {
      colorData->setPixelColor(i, 0); // Black for unfilled pixels
    }
  }

  // Use speed as outOfRangeBrightness for unfilled pixels
  StripData* rangeResult = effect_range(colorData, 0, pixelsToFill - 1, cfg->speed);

  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, rangeResult->getPixelColor(i));
  }

  // Clean up temporary data
  delete colorData;
  delete rangeResult;
}

// Percent mode tri - effect_range + speed + colorOne, colorTwo, colorThree
void mode_percent_tri(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  // skip if not updated
  if (!cfg->updated) return;
  // Calculate how many pixels to fill based on intensity
  int pixelsToFill = map(cfg->intensity, 0, 100, 0, data->pixelCount);

  // Create a tri-color pattern for the filled portion
  StripData* triColorData = new StripData(data->pixelCount);
  int sectionSize = pixelsToFill / 3;
  for (int i = 0; i < pixelsToFill; i++) {
    uint32_t color;
    if (i < sectionSize) {
      color = cfg->colorOne;
    } else if (i < sectionSize * 2) {
      color = cfg->colorTwo;
    } else {
      color = cfg->colorThree;
    }
    triColorData->setPixelColor(i, color);
  }
  // Fill the rest with colorOne (or black, or keep as is)
  for (int i = pixelsToFill; i < data->pixelCount; i++) {
    triColorData->setPixelColor(i, 0); // 0 for black/off
  }

  // Use speed as outOfRangeBrightness for unfilled pixels (lower speed = less light)
  StripData* rangeResult = effect_range(triColorData, 0, pixelsToFill - 1, cfg->speed);

  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, rangeResult->getPixelColor(i));
  }

  // Clean up temporary data
  delete triColorData;
  delete rangeResult;
}

// - SPECIAL - Shift mode - Inherits colors. continuously shifts existing pixel colors
void mode_shift(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  
  // Speed 100 = 50ms (fast), Speed 1 = 1000ms (slow)
  uint32_t shiftInterval = map(cfg->speed, 1, 100, 1000, 50);
  
  if (now - lastUpdate >= shiftInterval) {
    lastUpdate = now;
    
    StripData* shiftResult = effect_shift(data, cfg->direction);
    
    // Copy result to actual strip data
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, shiftResult->getPixelColor(i));
    }
    
    // Clean up temporary data
    delete shiftResult;
  }
}

// - SPECIAL - Shift mode - Inherits colors. continuously shifts existing pixel colors and directions
void mode_washing_machine(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;

  // --- Pattern state ---
  static bool      patternInitialized = false;
  static int       cachedSegments     = 0;
  static int       cachedPixels       = 0;
  static uint32_t  lastColorOne       = 0;
  static uint32_t  lastColorTwo       = 0;

  // --- Motion / agitation state ---
  static unsigned long lastStepTime   = 0;
  static unsigned long dwellStart     = 0;
  static bool      dwelling           = false;
  static int       stepsThisDir       = 0;
  static int       targetSteps        = 0;
  static uint32_t  dwellDuration      = 0;
  static bool      reversedPhase      = false;  // whether we are currently reversed vs base direction

  // Trigger a pattern rebuild when config reports update or relevant inputs changed
  int desiredSegments = (cfg->count > 0) ? cfg->count :
                        map(constrain(cfg->intensity, 1, 100), 1, 100, 4, 14);
  if (desiredSegments < 2) desiredSegments = 2;

  bool needRebuild =
      !patternInitialized ||
      cfg->updated ||
      desiredSegments != cachedSegments ||
      data->pixelCount != cachedPixels ||
      cfg->colorOne != lastColorOne ||
      cfg->colorTwo != lastColorTwo;

  if (needRebuild) {
    cachedSegments  = desiredSegments;
    cachedPixels    = data->pixelCount;
    lastColorOne    = cfg->colorOne;
    lastColorTwo    = cfg->colorTwo;

    // Build alternating band pattern directly into live strip
    for (int i = 0; i < data->pixelCount; i++) {
      int seg = (long)i * cachedSegments / max(1, data->pixelCount);
      data->setPixelColor(i, (seg & 1) ? cfg->colorTwo : cfg->colorOne);
    }

    // Reset motion state
    stepsThisDir  = 0;
    targetSteps   = 0;
    dwelling      = false;
    reversedPhase = false;
    lastStepTime  = millis();
    patternInitialized = true;
  }

  if (!patternInitialized) return;

  unsigned long now = millis();

  // Shift interval: speed 1 -> 1000ms, speed 100 -> 40ms
  uint32_t shiftInterval = map(constrain(cfg->speed, 1, 100), 1, 100, 1000, 40);

  // If we are dwelling, wait it out (keeps pattern static)
  if (dwelling) {
    if (now - dwellStart >= dwellDuration) {
      dwelling = false;
      // Flip direction phase for next run
      reversedPhase = !reversedPhase;
      stepsThisDir = 0;
      targetSteps = 0; // force new run spec
    } else {
      return; // stay paused
    }
  }

  // If starting a new run direction, decide how long to run and dwell timing
  if (targetSteps == 0) {
    int intensity = constrain(cfg->intensity, 1, 100);

    // Longer runs at low intensity, shorter at high intensity (more agitation)
    // (Rewritten to avoid std::max template type mismatch)
    int calmRun = data->pixelCount * 2;      // very calm
    int agitatedBase = data->pixelCount / 4; // very agitated target
    if (agitatedBase < 8) agitatedBase = 8;

    int mapped = map(intensity, 1, 100, calmRun, agitatedBase);
    int maxRun = (mapped < 4) ? 4 : mapped;

    int minRun = max(2, maxRun / 4);
    if (minRun > maxRun) minRun = maxRun;

    targetSteps = random(minRun, maxRun + 1);

    // Occasional extended "spin cycle" when intensity high
    if (intensity > 85 && random(0, 1000) < 25) {
      targetSteps = data->pixelCount * 3;
    }

    // Dwell duration (pause after the run) inversely related to intensity
    dwellDuration = map(intensity, 1, 100, 900, 120);
  }

  // Time to execute a shift step?
  if (now - lastStepTime >= shiftInterval) {
    lastStepTime = now;

    // Determine actual direction parameter for effect_shift:
    // Base config->direction (0/1). If reversedPhase, invert.
    uint8_t effectiveDirection = reversedPhase ? (cfg->direction ? 0 : 1) : cfg->direction;

    // Perform one rotational step (kept per requirement to call effect_shift)
    StripData* shifted = effect_shift(data, effectiveDirection);

    // Copy back
    for (int i = 0; i < data->pixelCount; i++) {
      data->setPixelColor(i, shifted->getPixelColor(i));
    }
    delete shifted;

    stepsThisDir++;
    if (stepsThisDir >= targetSteps) {
      dwelling = true;
      dwellStart = now;
    }
  }
}

// blink between colorOne and black
void mode_blink(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData; 
  StripData* colorSource = createColoredStripData(data->pixelCount, cfg->colorOne);
  // Use speed to calculate blink interval
  uint32_t blinkInterval = map(cfg->speed, 1, 100, 1000, 100); // Slower speed = longer interval
  StripData* blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, blinkResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete colorSource;
  delete blinkResult;
}

// Blink between colorOne and colorTwo
void mode_blink_toggle(StripData* data, const struct_message* config) { 
  const struct_message* cfg = config ? config : &myData;
  StripData* colorSource = createColoredStripData(data->pixelCount, cfg->colorOne);
  StripData* colorSourceTwo = createColoredStripData(data->pixelCount, cfg->colorTwo);
  // Use speed to calculate blink interval
  uint32_t blinkInterval = map(cfg->speed, 1, 100, 1000, 100); // Slower speed = longer interval
  StripData* blinkResult = effect_blink(colorSource, colorSourceTwo, blinkInterval);
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, blinkResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete colorSource;
  delete colorSourceTwo;
  delete blinkResult;
}

// blink random colors
void mode_blink_random(StripData* data, const struct_message* config) {
  const struct_message* cfg = config ? config : &myData;
  static uint32_t currentRandomColor = randomColor(); // Generate once and store
  static unsigned long lastColorChange = 0;
  static bool wasOn = false; // Track previous blink state
  
  // Use speed to calculate blink interval
  uint32_t blinkInterval = map(cfg->speed, 1, 100, 1000, 100); // Slower speed = longer interval
  
  StripData* colorSource = createColoredStripData(data->pixelCount, currentRandomColor);
  StripData* blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  
  // Check if light is currently on (non-zero color means on)
  bool isOn = (blinkResult->getPixelColor(0) != 0);
  
  // Generate new color when transitioning from off to on
  if (isOn && !wasOn) {
    currentRandomColor = randomColor();
    // Recreate the color source with the new color
    delete colorSource;
    colorSource = createColoredStripData(data->pixelCount, currentRandomColor);
    delete blinkResult;
    blinkResult = effect_blink(colorSource, nullptr, blinkInterval);
  }
  
  wasOn = isOn;
  
  // Copy result to actual strip data
  for (int i = 0; i < data->pixelCount; i++) {
    data->setPixelColor(i, blinkResult->getPixelColor(i));
  }
  
  // Clean up temporary data
  delete colorSource;
  delete blinkResult;
}

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


// ~~~~~~~~~~~~~~~~~
// 
// ~~~~~~~~~~~~~~~~~


// Called on Main Loop
// It calls functions that modify the stripData based on the effect name.
// The lightstrip is then updated with the new stripData.
//  
// Some methods can't write directly to the stripData.
// (eg. blink turning off looses the previous stripColors). 
// 
void callModeFunction(const String& effect, StripData* data, const struct_message* config) {
  if (effect == "static") { mode_static(data, config); }
  else if (effect == "statictri") { mode_static_tri(data, config); }
  else if (effect == "percent") { mode_percent(data, config); }
  else if (effect == "percenttri") { mode_percent_tri(data, config); }
  else if (effect == "shift") { mode_shift(data, config); }
  else if (effect == "washingmachine") { mode_washing_machine(data, config); }
  else if (effect == "blink") { mode_blink(data, config); }
  else if (effect == "blinktoggle") { mode_blink_toggle(data, config); }
  else if (effect == "blinkrandom") { mode_blink_random(data, config); }
  else if (effect == "swipe") { mode_swipe(data, config); }
  else if (effect == "swiperandom") { mode_swipe_random(data, config); }
  else if (effect == "colorloop") { mode_colorloop(data, config); }
  else if (effect == "breath") { mode_breath(data, config); }
  else if (effect == "sweep") { mode_sweep(data, config); }
  else if (effect == "sweepdual") { mode_sweep_dual(data, config); }
  else if (effect == "pacifica") { mode_pacifica(data, config); }
  else if (effect == "sunrise") { mode_sunrise(data, config); }
  else if (effect == "aurora") { mode_aurora(data, config); }
  else if (effect == "candle") { mode_candle(data, config); } // added
  else {
    Serial.printf("Unknown effect: %s\n", effect.c_str());
  }
}