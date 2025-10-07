#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

//
// Modifier Layer Modes
//

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