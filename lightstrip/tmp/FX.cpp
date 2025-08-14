/*
  WS2812FX.cpp contains all effect methods
  Harm Aldick - 2016
  www.aldick.org

  Copyright (c) 2016  Harm Aldick
  Licensed under the EUPL v. 1.2 or later
  Adapted from code originally licensed under the MIT license

  Modified heavily
*/ 

#define IBN 5100
// paletteBlend: 0 - wrap when moving, 1 - always wrap, 2 - never wrap, 3 - none (undefined)
#define PALETTE_SOLID_WRAP   (strip.paletteBlend == 1 || strip.paletteBlend == 3)
#define PALETTE_MOVING_WRAP !(strip.paletteBlend == 2 || (strip.paletteBlend == 0 && SEGMENT.speed == 0))
 
// effect functions


/*
 * Blink/strobe function
 * Alternate between color1 and color2
 * if(strobe == true) then create a strobe effect
 */
uint16_t blink(uint32_t color1, uint32_t color2, bool strobe, bool do_palette) {
  uint32_t cycleTime = (255 - SEGMENT.speed)*20;
  uint32_t onTime = FRAMETIME;
  if (!strobe) onTime += ((cycleTime * SEGMENT.intensity) >> 8);
  cycleTime += FRAMETIME*2;
  uint32_t it = strip.now / cycleTime;
  uint32_t rem = strip.now % cycleTime;

  bool on = false;
  if (it != SEGENV.step //new iteration, force on state for one frame, even if set time is too brief
      || rem <= onTime) {
    on = true;
  }

  SEGENV.step = it; //save previous iteration

  uint32_t color = on ? color1 : color2;
  if (color == color1 && do_palette)
  {
    for (unsigned i = 0; i < SEGLEN; i++) {
      SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
    }
  } else SEGMENT.fill(color);

  return FRAMETIME;
}


/*
 * Normal blinking. Intensity sets duty cycle.
 */
uint16_t mode_blink(void) {
  return blink(SEGCOLOR(0), SEGCOLOR(1), false, true);
} 

/*
 * Classic Blink effect. Cycling through the rainbow.
 */
uint16_t mode_blink_rainbow(void) {
  return blink(SEGMENT.color_wheel(SEGENV.call & 0xFF), SEGCOLOR(1), false, false);
}


/*
 * Classic Strobe effect.
 */
uint16_t mode_strobe(void) {
  return blink(SEGCOLOR(0), SEGCOLOR(1), true, true);
}


/*
 * Classic Strobe effect. Cycling through the rainbow.
 */
uint16_t mode_strobe_rainbow(void) {
  return blink(SEGMENT.color_wheel(SEGENV.call & 0xFF), SEGCOLOR(1), true, false);
}


/*
 * Color wipe function
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (bool rev == true) then LEDs are turned off in reverse order
 */
uint16_t color_wipe(bool rev, bool useRandomColors) {
  if (SEGLEN <= 1) return mode_static();
  uint32_t cycleTime = 750 + (255 - SEGMENT.speed)*150;
  uint32_t perc = strip.now % cycleTime;
  unsigned prog = (perc * 65535) / cycleTime;
  bool back = (prog > 32767);
  if (back) {
    prog -= 32767;
    if (SEGENV.step == 0) SEGENV.step = 1;
  } else {
    if (SEGENV.step == 2) SEGENV.step = 3; //trigger color change
  }

  if (useRandomColors) {
    if (SEGENV.call == 0) {
      SEGENV.aux0 = hw_random8();
      SEGENV.step = 3;
    }
    if (SEGENV.step == 1) { //if flag set, change to new random color
      SEGENV.aux1 = get_random_wheel_index(SEGENV.aux0);
      SEGENV.step = 2;
    }
    if (SEGENV.step == 3) {
      SEGENV.aux0 = get_random_wheel_index(SEGENV.aux1);
      SEGENV.step = 0;
    }
  }

  unsigned ledIndex = (prog * SEGLEN) >> 15;
  uint16_t rem = (prog * SEGLEN) * 2; //mod 0xFFFF by truncating
  rem /= (SEGMENT.intensity +1);
  if (rem > 255) rem = 255;

  uint32_t col1 = useRandomColors? SEGMENT.color_wheel(SEGENV.aux1) : SEGCOLOR(1);
  for (unsigned i = 0; i < SEGLEN; i++)
  {
    unsigned index = (rev && back)? SEGLEN -1 -i : i;
    uint32_t col0 = useRandomColors? SEGMENT.color_wheel(SEGENV.aux0) : SEGMENT.color_from_palette(index, true, PALETTE_SOLID_WRAP, 0);

    if (i < ledIndex)
    {
      SEGMENT.setPixelColor(index, back? col1 : col0);
    } else
    {
      SEGMENT.setPixelColor(index, back? col0 : col1);
      if (i == ledIndex) SEGMENT.setPixelColor(index, color_blend(back? col0 : col1, back? col1 : col0, uint8_t(rem)));
    }
  }
  return FRAMETIME;
}


/*
 * Lights all LEDs one after another.
 */
uint16_t mode_color_wipe(void) {
  return color_wipe(false, false);
}
static const char _data_FX_MODE_COLOR_WIPE[] PROGMEM = "Wipe@!,!;!,!;!";


/*
 * Lights all LEDs one after another. Turns off opposite
 */
uint16_t mode_color_sweep(void) {
  return color_wipe(true, false);
}
static const char _data_FX_MODE_COLOR_SWEEP[] PROGMEM = "Sweep@!,!;!,!;!";


/*
 * Turns all LEDs after each other to a random color.
 * Then starts over with another color.
 */
uint16_t mode_color_wipe_random(void) {
  return color_wipe(false, true);
}
static const char _data_FX_MODE_COLOR_WIPE_RANDOM[] PROGMEM = "Wipe Random@!;;!";


/*
 * Random color introduced alternating from start and end of strip.
 */
uint16_t mode_color_sweep_random(void) {
  return color_wipe(true, true);
}
static const char _data_FX_MODE_COLOR_SWEEP_RANDOM[] PROGMEM = "Sweep Random@!;;!";


/*
 * Lights all LEDs up in one random color. Then switches them
 * to the next random color.
 */
uint16_t mode_random_color(void) {
  uint32_t cycleTime = 200 + (255 - SEGMENT.speed)*50;
  uint32_t it = strip.now / cycleTime;
  uint32_t rem = strip.now % cycleTime;
  unsigned fadedur = (cycleTime * SEGMENT.intensity) >> 8;

  uint32_t fade = 255;
  if (fadedur) {
    fade = (rem * 255) / fadedur;
    if (fade > 255) fade = 255;
  }

  if (SEGENV.call == 0) {
    SEGENV.aux0 = hw_random8();
    SEGENV.step = 2;
  }
  if (it != SEGENV.step) //new color
  {
    SEGENV.aux1 = SEGENV.aux0;
    SEGENV.aux0 = get_random_wheel_index(SEGENV.aux0); //aux0 will store our random color wheel index
    SEGENV.step = it;
  }

  SEGMENT.fill(color_blend(SEGMENT.color_wheel(SEGENV.aux1), SEGMENT.color_wheel(SEGENV.aux0), uint8_t(fade)));
  return FRAMETIME;
}
static const char _data_FX_MODE_RANDOM_COLOR[] PROGMEM = "Random Colors@!,Fade time;;!;01";


/*
 * Lights every LED in a random color. Changes all LED at the same time
 * to new random colors.
 */
uint16_t mode_dynamic(void) {
  if (!SEGENV.allocateData(SEGLEN)) return mode_static(); //allocation failed

  if(SEGENV.call == 0) {
    //SEGMENT.fill(BLACK);
    for (unsigned i = 0; i < SEGLEN; i++) SEGENV.data[i] = hw_random8();
  }

  uint32_t cycleTime = 50 + (255 - SEGMENT.speed)*15;
  uint32_t it = strip.now / cycleTime;
  if (it != SEGENV.step && SEGMENT.speed != 0) //new color
  {
    for (unsigned i = 0; i < SEGLEN; i++) {
      if (hw_random8() <= SEGMENT.intensity) SEGENV.data[i] = hw_random8(); // random color index
    }
    SEGENV.step = it;
  }

  if (SEGMENT.check1) {
    for (unsigned i = 0; i < SEGLEN; i++) {
      SEGMENT.blendPixelColor(i, SEGMENT.color_wheel(SEGENV.data[i]), 16);
    }
  } else {
    for (unsigned i = 0; i < SEGLEN; i++) {
      SEGMENT.setPixelColor(i, SEGMENT.color_wheel(SEGENV.data[i]));
    }
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_DYNAMIC[] PROGMEM = "Dynamic@!,!,,,,Smooth;;!";


/*
 * effect "Dynamic" with smooth color-fading
 */
uint16_t mode_dynamic_smooth(void) {
  bool old = SEGMENT.check1;
  SEGMENT.check1 = true;
  mode_dynamic();
  SEGMENT.check1 = old;
  return FRAMETIME;
 }
static const char _data_FX_MODE_DYNAMIC_SMOOTH[] PROGMEM = "Dynamic Smooth@!,!;;!";
 

/*
 * Blink several LEDs in random colors on, reset, repeat.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t mode_twinkle(void) {
  SEGMENT.fade_out(224);

  uint32_t cycleTime = 20 + (255 - SEGMENT.speed)*5;
  uint32_t it = strip.now / cycleTime;
  if (it != SEGENV.step)
  {
    unsigned maxOn = map(SEGMENT.intensity, 0, 255, 1, SEGLEN); // make sure at least one LED is on
    if (SEGENV.aux0 >= maxOn)
    {
      SEGENV.aux0 = 0;
      SEGENV.aux1 = hw_random(); //new seed for our PRNG
    }
    SEGENV.aux0++;
    SEGENV.step = it;
  }

  unsigned PRNG16 = SEGENV.aux1;

  for (unsigned i = 0; i < SEGENV.aux0; i++)
  {
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 13849; // next 'random' number
    uint32_t p = (uint32_t)SEGLEN * (uint32_t)PRNG16;
    unsigned j = p >> 16;
    SEGMENT.setPixelColor(j, SEGMENT.color_from_palette(j, true, PALETTE_SOLID_WRAP, 0));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TWINKLE[] PROGMEM = "Twinkle@!,!;!,!;!;;m12=0"; //pixels


/*
 * Dissolve function
 */
uint16_t dissolve(uint32_t color) {
  unsigned dataSize = sizeof(uint32_t) * SEGLEN;
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed
  uint32_t* pixels = reinterpret_cast<uint32_t*>(SEGENV.data);

  if (SEGENV.call == 0) {
    for (unsigned i = 0; i < SEGLEN; i++) pixels[i] = SEGCOLOR(1);
    SEGENV.aux0 = 1;
  }

  for (unsigned j = 0; j <= SEGLEN / 15; j++) {
    if (hw_random8() <= SEGMENT.intensity) {
      for (size_t times = 0; times < 10; times++) { //attempt to spawn a new pixel 10 times
        unsigned i = hw_random16(SEGLEN);
        if (SEGENV.aux0) { //dissolve to primary/palette
          if (pixels[i] == SEGCOLOR(1)) {
            pixels[i] = color == SEGCOLOR(0) ? SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0) : color;
            break; //only spawn 1 new pixel per frame per 50 LEDs
          }
        } else { //dissolve to secondary
          if (pixels[i] != SEGCOLOR(1)) {
            pixels[i] = SEGCOLOR(1);
            break;
          }
        }
      }
    }
  }
  // fix for #4401
  for (unsigned i = 0; i < SEGLEN; i++) SEGMENT.setPixelColor(i, pixels[i]);

  if (SEGENV.step > (255 - SEGMENT.speed) + 15U) {
    SEGENV.aux0 = !SEGENV.aux0;
    SEGENV.step = 0;
  } else {
    SEGENV.step++;
  }

  return FRAMETIME;
}


/*
 * Blink several LEDs on and then off
 */
uint16_t mode_dissolve(void) {
  return dissolve(SEGMENT.check1 ? SEGMENT.color_wheel(hw_random8()) : SEGCOLOR(0));
}
static const char _data_FX_MODE_DISSOLVE[] PROGMEM = "Dissolve@Repeat speed,Dissolve speed,,,,Random;!,!;!";


/*
 * Blink several LEDs on and then off in random colors
 */
uint16_t mode_dissolve_random(void) {
  return dissolve(SEGMENT.color_wheel(hw_random8()));
}
static const char _data_FX_MODE_DISSOLVE_RANDOM[] PROGMEM = "Dissolve Rnd@Repeat speed,Dissolve speed;,!;!";


/*
 * Blinks one LED at a time.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t mode_sparkle(void) {
  if (!SEGMENT.check2) for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1));
  }
  uint32_t cycleTime = 10 + (255 - SEGMENT.speed)*2;
  uint32_t it = strip.now / cycleTime;
  if (it != SEGENV.step)
  {
    SEGENV.aux0 = hw_random16(SEGLEN); // aux0 stores the random led index
    SEGENV.step = it;
  }

  SEGMENT.setPixelColor(SEGENV.aux0, SEGCOLOR(0));
  return FRAMETIME;
}
static const char _data_FX_MODE_SPARKLE[] PROGMEM = "Sparkle@!,,,,,,Overlay;!,!;!;;m12=0";


/*
 * Lights all LEDs in the color. Flashes single col 1 pixels randomly. (List name: Sparkle Dark)
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t mode_flash_sparkle(void) {
  if (!SEGMENT.check2) for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
  }

  if (strip.now - SEGENV.aux0 > SEGENV.step) {
    if(hw_random8((255-SEGMENT.intensity) >> 4) == 0) {
      SEGMENT.setPixelColor(hw_random16(SEGLEN), SEGCOLOR(1)); //flash
    }
    SEGENV.step = strip.now;
    SEGENV.aux0 = 255-SEGMENT.speed;
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_FLASH_SPARKLE[] PROGMEM = "Sparkle Dark@!,!,,,,,Overlay;Bg,Fx;!;;m12=0";


/*
 * Like flash sparkle. With more flash.
 * Inspired by www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
 */
uint16_t mode_hyper_sparkle(void) {
  if (!SEGMENT.check2) for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
  }

  if (strip.now - SEGENV.aux0 > SEGENV.step) {
    if (hw_random8((255-SEGMENT.intensity) >> 4) == 0) {
      int len = max(1, (int)SEGLEN/3);
      for (int i = 0; i < len; i++) {
        SEGMENT.setPixelColor(hw_random16(SEGLEN), SEGCOLOR(1));
      }
    }
    SEGENV.step = strip.now;
    SEGENV.aux0 = 255-SEGMENT.speed;
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_HYPER_SPARKLE[] PROGMEM = "Sparkle+@!,!,,,,,Overlay;Bg,Fx;!;;m12=0";


/*
 * Strobe effect with different strobe count and pause, controlled by speed.
 */
uint16_t mode_multi_strobe(void) {
  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1));
  }

  SEGENV.aux0 = 50 + 20*(uint16_t)(255-SEGMENT.speed);
  unsigned count = 2 * ((SEGMENT.intensity / 10) + 1);
  if(SEGENV.aux1 < count) {
    if((SEGENV.aux1 & 1) == 0) {
      SEGMENT.fill(SEGCOLOR(0));
      SEGENV.aux0 = 15;
    } else {
      SEGENV.aux0 = 50;
    }
  }

  if (strip.now - SEGENV.aux0 > SEGENV.step) {
    SEGENV.aux1++;
    if (SEGENV.aux1 > count) SEGENV.aux1 = 0;
    SEGENV.step = strip.now;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_MULTI_STROBE[] PROGMEM = "Strobe Mega@!,!;!,!;!;01";


/*
 * Android loading circle, refactored by @dedehai
 */
uint16_t mode_android(void) {
  if (!SEGENV.allocateData(sizeof(uint32_t))) return mode_static();
  uint32_t* counter = reinterpret_cast<uint32_t*>(SEGENV.data);
  unsigned size = SEGENV.aux1 >> 1; // upper 15 bit
  unsigned shrinking = SEGENV.aux1 & 0x01; // lowest bit
  if(strip.now >= SEGENV.step) {
    SEGENV.step = strip.now + 3 + ((8 * (uint32_t)(255 - SEGMENT.speed)) / SEGLEN);
    if (size > (SEGMENT.intensity * SEGLEN) / 255)
      shrinking = 1;
    else if (size < 2)
      shrinking = 0;
    if (!shrinking) { // growing
      if ((*counter % 3) == 1)
        SEGENV.aux0++; // advance start position
      else
        size++;
    } else { // shrinking
      SEGENV.aux0++;
      if ((*counter % 3) != 1)
        size--;
    }
    SEGENV.aux1 = size << 1 | shrinking; // save back
    (*counter)++;
    if (SEGENV.aux0 >= SEGLEN) SEGENV.aux0 = 0;
  }
  uint32_t start = SEGENV.aux0;
  uint32_t end = (SEGENV.aux0 + size) % SEGLEN;
  for (unsigned i = 0; i < SEGLEN; i++) {
    if ((start < end && i >= start && i < end) || (start >= end && (i >= start || i < end)))
      SEGMENT.setPixelColor(i, SEGCOLOR(0));
    else
      SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1));
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_ANDROID[] PROGMEM = "Android@!,Width;!,!;!;;m12=1"; //vertical

/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */
static uint16_t chase(uint32_t color1, uint32_t color2, uint32_t color3, bool do_palette) {
  uint16_t counter = strip.now * ((SEGMENT.speed >> 2) + 1);
  uint16_t a = (counter * SEGLEN) >> 16;

  bool chase_random = (SEGMENT.mode == FX_MODE_CHASE_RANDOM);
  if (chase_random) {
    if (a < SEGENV.step) //we hit the start again, choose new color for Chase random
    {
      SEGENV.aux1 = SEGENV.aux0; //store previous random color
      SEGENV.aux0 = get_random_wheel_index(SEGENV.aux0);
    }
    color1 = SEGMENT.color_wheel(SEGENV.aux0);
  }
  SEGENV.step = a;

  // Use intensity setting to vary chase up to 1/2 string length
  unsigned size = 1 + ((SEGMENT.intensity * SEGLEN) >> 10);

  uint16_t b = a + size; //"trail" of chase, filled with color1
  if (b > SEGLEN) b -= SEGLEN;
  uint16_t c = b + size;
  if (c > SEGLEN) c -= SEGLEN;

  //background
  if (do_palette)
  {
    for (unsigned i = 0; i < SEGLEN; i++) {
      SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1));
    }
  } else SEGMENT.fill(color1);

  //if random, fill old background between a and end
  if (chase_random)
  {
    color1 = SEGMENT.color_wheel(SEGENV.aux1);
    for (unsigned i = a; i < SEGLEN; i++)
      SEGMENT.setPixelColor(i, color1);
  }

  //fill between points a and b with color2
  if (a < b)
  {
    for (unsigned i = a; i < b; i++)
      SEGMENT.setPixelColor(i, color2);
  } else {
    for (unsigned i = a; i < SEGLEN; i++) //fill until end
      SEGMENT.setPixelColor(i, color2);
    for (unsigned i = 0; i < b; i++) //fill from start until b
      SEGMENT.setPixelColor(i, color2);
  }

  //fill between points b and c with color2
  if (b < c)
  {
    for (unsigned i = b; i < c; i++)
      SEGMENT.setPixelColor(i, color3);
  } else {
    for (unsigned i = b; i < SEGLEN; i++) //fill until end
      SEGMENT.setPixelColor(i, color3);
    for (unsigned i = 0; i < c; i++) //fill from start until c
      SEGMENT.setPixelColor(i, color3);
  }

  return FRAMETIME;
}


/*
 * Bicolor chase, more primary color.
 */
uint16_t mode_chase_color(void) {
  return chase(SEGCOLOR(1), (SEGCOLOR(2)) ? SEGCOLOR(2) : SEGCOLOR(0), SEGCOLOR(0), true);
}
static const char _data_FX_MODE_CHASE_COLOR[] PROGMEM = "Chase@!,Width;!,!,!;!";


/*
 * Primary running followed by random color.
 */
uint16_t mode_chase_random(void) {
  return chase(SEGCOLOR(1), (SEGCOLOR(2)) ? SEGCOLOR(2) : SEGCOLOR(0), SEGCOLOR(0), false);
}
static const char _data_FX_MODE_CHASE_RANDOM[] PROGMEM = "Chase Random@!,Width;!,,!;!";


/*
 * Primary, secondary running on rainbow.
 */
uint16_t mode_chase_rainbow(void) {
  unsigned color_sep = 256 / SEGLEN;
  if (color_sep == 0) color_sep = 1;                                           // correction for segments longer than 256 LEDs
  unsigned color_index = SEGENV.call & 0xFF;
  uint32_t color = SEGMENT.color_wheel(((SEGENV.step * color_sep) + color_index) & 0xFF);

  return chase(color, SEGCOLOR(0), SEGCOLOR(1), false);
}
static const char _data_FX_MODE_CHASE_RAINBOW[] PROGMEM = "Chase Rainbow@!,Width;!,!;!";


/*
 * Primary running on rainbow.
 */
uint16_t mode_chase_rainbow_white(void) {
  uint16_t n = SEGENV.step;
  uint16_t m = (SEGENV.step + 1) % SEGLEN;
  uint32_t color2 = SEGMENT.color_wheel(((n * 256 / SEGLEN) + (SEGENV.call & 0xFF)) & 0xFF);
  uint32_t color3 = SEGMENT.color_wheel(((m * 256 / SEGLEN) + (SEGENV.call & 0xFF)) & 0xFF);

  return chase(SEGCOLOR(0), color2, color3, false);
}
static const char _data_FX_MODE_CHASE_RAINBOW_WHITE[] PROGMEM = "Rainbow Runner@!,Size;Bg;!";


/*
 * Red - Amber - Green - Blue lights running
 */
uint16_t mode_colorful(void) {
  unsigned numColors = 4; //3, 4, or 5
  uint32_t cols[9]{0x00FF0000,0x00EEBB00,0x0000EE00,0x000077CC};
  if (SEGMENT.intensity > 160 || SEGMENT.palette) { //palette or color
    if (!SEGMENT.palette) {
      numColors = 3;
      for (size_t i = 0; i < 3; i++) cols[i] = SEGCOLOR(i);
    } else {
      unsigned fac = 80;
      if (SEGMENT.palette == 52) {numColors = 5; fac = 61;} //C9 2 has 5 colors
      for (size_t i = 0; i < numColors; i++) {
        cols[i] = SEGMENT.color_from_palette(i*fac, false, true, 255);
      }
    }
  } else if (SEGMENT.intensity < 80) //pastel (easter) colors
  {
    cols[0] = 0x00FF8040;
    cols[1] = 0x00E5D241;
    cols[2] = 0x0077FF77;
    cols[3] = 0x0077F0F0;
  }
  for (size_t i = numColors; i < numColors*2 -1U; i++) cols[i] = cols[i-numColors];

  uint32_t cycleTime = 50 + (8 * (uint32_t)(255 - SEGMENT.speed));
  uint32_t it = strip.now / cycleTime;
  if (it != SEGENV.step)
  {
    if (SEGMENT.speed > 0) SEGENV.aux0++;
    if (SEGENV.aux0 >= numColors) SEGENV.aux0 = 0;
    SEGENV.step = it;
  }

  for (unsigned i = 0; i < SEGLEN; i+= numColors)
  {
    for (unsigned j = 0; j < numColors; j++) SEGMENT.setPixelColor(i + j, cols[SEGENV.aux0 + j]);
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_COLORFUL[] PROGMEM = "Colorful@!,Saturation;1,2,3;!";


/*
 * Emulates a traffic light.
 */
uint16_t mode_traffic_light(void) {
  if (SEGLEN <= 1) return mode_static();
  for (unsigned i=0; i < SEGLEN; i++)
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1));
  uint32_t mdelay = 500;
  for (unsigned i = 0; i < SEGLEN-2 ; i+=3)
  {
    switch (SEGENV.aux0)
    {
      case 0: SEGMENT.setPixelColor(i, 0x00FF0000); mdelay = 150 + (100 * (uint32_t)(255 - SEGMENT.speed));break;
      case 1: SEGMENT.setPixelColor(i, 0x00FF0000); mdelay = 150 + (20 * (uint32_t)(255 - SEGMENT.speed)); SEGMENT.setPixelColor(i+1, 0x00EECC00); break;
      case 2: SEGMENT.setPixelColor(i+2, 0x0000FF00); mdelay = 150 + (100 * (uint32_t)(255 - SEGMENT.speed));break;
      case 3: SEGMENT.setPixelColor(i+1, 0x00EECC00); mdelay = 150 + (20 * (uint32_t)(255 - SEGMENT.speed));break;
    }
  }

  if (strip.now - SEGENV.step > mdelay)
  {
    SEGENV.aux0++;
    if (SEGENV.aux0 == 1 && SEGMENT.intensity > 140) SEGENV.aux0 = 2; //skip Red + Amber, to get US-style sequence
    if (SEGENV.aux0 > 3) SEGENV.aux0 = 0;
    SEGENV.step = strip.now;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TRAFFIC_LIGHT[] PROGMEM = "Traffic Light@!,US style;,!;!";


/*
 * Sec flashes running on prim.
 */
#define FLASH_COUNT 4
uint16_t mode_chase_flash(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned flash_step = SEGENV.call % ((FLASH_COUNT * 2) + 1);

  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
  }

  unsigned delay = 10 + ((30 * (uint16_t)(255 - SEGMENT.speed)) / SEGLEN);
  if(flash_step < (FLASH_COUNT * 2)) {
    if(flash_step % 2 == 0) {
      unsigned n = SEGENV.step;
      unsigned m = (SEGENV.step + 1) % SEGLEN;
      SEGMENT.setPixelColor( n, SEGCOLOR(1));
      SEGMENT.setPixelColor( m, SEGCOLOR(1));
      delay = 20;
    } else {
      delay = 30;
    }
  } else {
    SEGENV.step = (SEGENV.step + 1) % SEGLEN;
  }
  return delay;
}
static const char _data_FX_MODE_CHASE_FLASH[] PROGMEM = "Chase Flash@!;Bg,Fx;!";


/*
 * Prim flashes running, followed by random color.
 */
uint16_t mode_chase_flash_random(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned flash_step = SEGENV.call % ((FLASH_COUNT * 2) + 1);

  for (int i = 0; i < SEGENV.aux1; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_wheel(SEGENV.aux0));
  }

  unsigned delay = 1 + ((10 * (uint16_t)(255 - SEGMENT.speed)) / SEGLEN);
  if(flash_step < (FLASH_COUNT * 2)) {
    unsigned n = SEGENV.aux1;
    unsigned m = (SEGENV.aux1 + 1) % SEGLEN;
    if(flash_step % 2 == 0) {
      SEGMENT.setPixelColor( n, SEGCOLOR(0));
      SEGMENT.setPixelColor( m, SEGCOLOR(0));
      delay = 20;
    } else {
      SEGMENT.setPixelColor( n, SEGMENT.color_wheel(SEGENV.aux0));
      SEGMENT.setPixelColor( m, SEGCOLOR(1));
      delay = 30;
    }
  } else {
    SEGENV.aux1 = (SEGENV.aux1 + 1) % SEGLEN;

    if (SEGENV.aux1 == 0) {
      SEGENV.aux0 = get_random_wheel_index(SEGENV.aux0);
    }
  }
  return delay;
}
static const char _data_FX_MODE_CHASE_FLASH_RANDOM[] PROGMEM = "Chase Flash Rnd@!;!,!;!";


/*
 * Alternating color/sec pixels running.
 */
uint16_t mode_running_color(void) {
  return running(SEGCOLOR(0), SEGCOLOR(1));
}
static const char _data_FX_MODE_RUNNING_COLOR[] PROGMEM = "Chase 2@!,Width;!,!;!";


/*
 * Random colored pixels running. ("Stream")
 */
uint16_t mode_running_random(void) {
  uint32_t cycleTime = 25 + (3 * (uint32_t)(255 - SEGMENT.speed));
  uint32_t it = strip.now / cycleTime;
  if (SEGENV.call == 0) SEGENV.aux0 = hw_random(); // random seed for PRNG on start

  unsigned zoneSize = ((255-SEGMENT.intensity) >> 4) +1;
  uint16_t PRNG16 = SEGENV.aux0;

  unsigned z = it % zoneSize;
  bool nzone = (!z && it != SEGENV.aux1);
  for (int i=SEGLEN-1; i >= 0; i--) {
    if (nzone || z >= zoneSize) {
      unsigned lastrand = PRNG16 >> 8;
      int16_t diff = 0;
      while (abs(diff) < 42) { // make sure the difference between adjacent colors is big enough
        PRNG16 = (uint16_t)(PRNG16 * 2053) + 13849; // next zone, next 'random' number
        diff = (PRNG16 >> 8) - lastrand;
      }
      if (nzone) {
        SEGENV.aux0 = PRNG16; // save next starting seed
        nzone = false;
      }
      z = 0;
    }
    SEGMENT.setPixelColor(i, SEGMENT.color_wheel(PRNG16 >> 8));
    z++;
  }

  SEGENV.aux1 = it;
  return FRAMETIME;
}
static const char _data_FX_MODE_RUNNING_RANDOM[] PROGMEM = "Stream@!,Zone size;;!";


/*
 * K.I.T.T.
 */
uint16_t mode_larson_scanner(void) {
  if (SEGLEN <= 1) return mode_static();

  const unsigned speed  = FRAMETIME * map(SEGMENT.speed, 0, 255, 96, 2); // map into useful range
  const unsigned pixels = SEGLEN / speed; // how many pixels to advance per frame

  SEGMENT.fade_out(255-SEGMENT.intensity);

  if (SEGENV.step > strip.now) return FRAMETIME;  // we have a pause

  unsigned index = SEGENV.aux1 + pixels;
  // are we slow enough to use frames per pixel?
  if (pixels == 0) {
    const unsigned frames = speed / SEGLEN; // how many frames per 1 pixel
    if (SEGENV.step++ < frames) return FRAMETIME;
    SEGENV.step = 0;
    index++;
  }

  if (index > SEGLEN) {

    SEGENV.aux0 = !SEGENV.aux0; // change direction
    SEGENV.aux1 = 0;            // reset position
    // set delay
    if (SEGENV.aux0 || SEGMENT.check2) SEGENV.step = strip.now + SEGMENT.custom1 * 25; // multiply by 25ms
    else SEGENV.step = 0;

  } else {

    // paint as many pixels as needed
    for (unsigned i = SEGENV.aux1; i < index; i++) {
      unsigned j = (SEGENV.aux0) ? i : SEGLEN - 1 - i;
      uint32_t c = SEGMENT.color_from_palette(j, true, PALETTE_SOLID_WRAP, 0);
      SEGMENT.setPixelColor(j, c);
      if (SEGMENT.check1) {
        SEGMENT.setPixelColor(SEGLEN - 1 - j, SEGCOLOR(2) ? SEGCOLOR(2) : c);
      }
    }
    SEGENV.aux1 = index;
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_LARSON_SCANNER[] PROGMEM = "Scanner@!,Trail,Delay,,,Dual,Bi-delay;!,!,!;!;;m12=0,c1=0";

/*
 * Creates two Larson scanners moving in opposite directions
 * Custom mode by Keith Lord: https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/DualLarson.h
 */
uint16_t mode_dual_larson_scanner(void){
  SEGMENT.check1 = true;
  return mode_larson_scanner();
}
static const char _data_FX_MODE_DUAL_LARSON_SCANNER[] PROGMEM = "Scanner Dual@!,Trail,Delay,,,Dual,Bi-delay;!,!,!;!;;m12=0,c1=0";
 
/*
 * Fireworks function.
 */
uint16_t mode_fireworks() {
  if (SEGLEN <= 1) return mode_static();
  const uint16_t width  = SEGMENT.is2D() ? SEG_W : SEGLEN;
  const uint16_t height = SEG_H;

  if (SEGENV.call == 0) {
    SEGENV.aux0 = UINT16_MAX;
    SEGENV.aux1 = UINT16_MAX;
  }
  SEGMENT.fade_out(128);

  uint8_t x = SEGENV.aux0%width, y = SEGENV.aux0/width; // 2D coordinates stored in upper and lower byte
  if (!SEGENV.step) {
    // fireworks mode (blur flares)
    bool valid1 = (SEGENV.aux0 < width*height);
    bool valid2 = (SEGENV.aux1 < width*height);
    uint32_t sv1 = 0, sv2 = 0;
    if (valid1) sv1 = SEGMENT.is2D() ? SEGMENT.getPixelColorXY(x, y) : SEGMENT.getPixelColor(SEGENV.aux0); // get spark color
    if (valid2) sv2 = SEGMENT.is2D() ? SEGMENT.getPixelColorXY(x, y) : SEGMENT.getPixelColor(SEGENV.aux1);
    SEGMENT.blur(16); // used in mode_rain()
    if (valid1) { if (SEGMENT.is2D()) SEGMENT.setPixelColorXY(x, y, sv1); else SEGMENT.setPixelColor(SEGENV.aux0, sv1); } // restore spark color after blur
    if (valid2) { if (SEGMENT.is2D()) SEGMENT.setPixelColorXY(x, y, sv2); else SEGMENT.setPixelColor(SEGENV.aux1, sv2); } // restore old spark color after blur
  }

  for (int i=0; i<max(1, width/20); i++) {
    if (hw_random8(129 - (SEGMENT.intensity >> 1)) == 0) {
      uint16_t index = hw_random16(width*height);
      x = index % width;
      y = index / width;
      uint32_t col = SEGMENT.color_from_palette(hw_random8(), false, false, 0);
      if (SEGMENT.is2D()) SEGMENT.setPixelColorXY(x, y, col);
      else                SEGMENT.setPixelColor(index, col);
      SEGENV.aux1 = SEGENV.aux0;  // old spark
      SEGENV.aux0 = index;        // remember where spark occurred
    }
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_FIREWORKS[] PROGMEM = "Fireworks@,Frequency;!,!;!;12;ix=192,pal=11";

//Twinkling LEDs running. Inspired by https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/Rain.h
uint16_t mode_rain() {
  if (SEGLEN <= 1) return mode_static();
  const unsigned width  = SEG_W;
  const unsigned height = SEG_H;
  SEGENV.step += FRAMETIME;
  if (SEGENV.call && SEGENV.step > SPEED_FORMULA_L) {
    SEGENV.step = 1;
    if (SEGMENT.is2D()) {
      //uint32_t ctemp[width];
      //for (int i = 0; i<width; i++) ctemp[i] = SEGMENT.getPixelColorXY(i, height-1);
      SEGMENT.move(6, 1, true);  // move all pixels down
      //for (int i = 0; i<width; i++) SEGMENT.setPixelColorXY(i, 0, ctemp[i]); // wrap around
      SEGENV.aux0 = (SEGENV.aux0 % width) + (SEGENV.aux0 / width + 1) * width;
      SEGENV.aux1 = (SEGENV.aux1 % width) + (SEGENV.aux1 / width + 1) * width;
    } else {
      //shift all leds left
      uint32_t ctemp = SEGMENT.getPixelColor(0);
      for (unsigned i = 0; i < SEGLEN - 1; i++) {
        SEGMENT.setPixelColor(i, SEGMENT.getPixelColor(i+1));
      }
      SEGMENT.setPixelColor(SEGLEN -1, ctemp); // wrap around
      SEGENV.aux0++;  // increase spark index
      SEGENV.aux1++;
    }
    if (SEGENV.aux0 == 0) SEGENV.aux0 = UINT16_MAX; // reset previous spark position
    if (SEGENV.aux1 == 0) SEGENV.aux0 = UINT16_MAX; // reset previous spark position
    if (SEGENV.aux0 >= width*height) SEGENV.aux0 = 0;     // ignore
    if (SEGENV.aux1 >= width*height) SEGENV.aux1 = 0;
  }
  return mode_fireworks();
}
static const char _data_FX_MODE_RAIN[] PROGMEM = "Rain@!,Spawning rate;!,!;!;12;ix=128,pal=0";

/*
 * Fire flicker function
 */
uint16_t mode_fire_flicker(void) {
  uint32_t cycleTime = 40 + (255 - SEGMENT.speed);
  uint32_t it = strip.now / cycleTime;
  if (SEGENV.step == it) return FRAMETIME;

  byte w = (SEGCOLOR(0) >> 24);
  byte r = (SEGCOLOR(0) >> 16);
  byte g = (SEGCOLOR(0) >>  8);
  byte b = (SEGCOLOR(0)      );
  byte lum = (SEGMENT.palette == 0) ? MAX(w, MAX(r, MAX(g, b))) : 255;
  lum /= (((256-SEGMENT.intensity)/16)+1);
  for (unsigned i = 0; i < SEGLEN; i++) {
    byte flicker = hw_random8(lum);
    if (SEGMENT.palette == 0) {
      SEGMENT.setPixelColor(i, MAX(r - flicker, 0), MAX(g - flicker, 0), MAX(b - flicker, 0), MAX(w - flicker, 0));
    } else {
      SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0, 255 - flicker));
    }
  }

  SEGENV.step = it;
  return FRAMETIME;
}
static const char _data_FX_MODE_FIRE_FLICKER[] PROGMEM = "Fire Flicker@!,!;!;!;01";


/*
 * Gradient run base function
 */
uint16_t gradient_base(bool loading) {
  if (SEGLEN <= 1) return mode_static();
  uint16_t counter = strip.now * ((SEGMENT.speed >> 2) + 1);
  uint16_t pp = (counter * SEGLEN) >> 16;
  if (SEGENV.call == 0) pp = 0;
  int val; //0 = sec 1 = pri
  int brd = 1 + loading ? SEGMENT.intensity/2 : SEGMENT.intensity/4;
  //if (brd < 1) brd = 1;
  int p1 = pp-SEGLEN;
  int p2 = pp+SEGLEN;

  for (int i = 0; i < (int)SEGLEN; i++) {
    if (loading) {
      val = abs(((i>pp) ? p2:pp) - i);
    } else {
      val = min(abs(pp-i),min(abs(p1-i),abs(p2-i)));
    }
    val = (brd > val) ? (val * 255) / brd : 255;
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(0), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1), uint8_t(val)));
  }

  return FRAMETIME;
}


/*
 * Gradient run
 */
uint16_t mode_gradient(void) {
  return gradient_base(false);
}
static const char _data_FX_MODE_GRADIENT[] PROGMEM = "Gradient@!,Spread;!,!;!;;ix=16";


/*
 * Gradient run with hard transition
 */
uint16_t mode_loading(void) {
  return gradient_base(true);
}
static const char _data_FX_MODE_LOADING[] PROGMEM = "Loading@!,Fade;!,!;!;;ix=16";
 

/*
 * Tricolor chase function
 */
uint16_t tricolor_chase(uint32_t color1, uint32_t color2) {
  uint32_t cycleTime = 50 + ((255 - SEGMENT.speed)<<1);
  uint32_t it = strip.now / cycleTime;  // iterator
  unsigned width = (1 + (SEGMENT.intensity>>4)); // value of 1-16 for each colour
  unsigned index = it % (width*3);

  for (unsigned i = 0; i < SEGLEN; i++, index++) {
    if (index > (width*3)-1) index = 0;

    uint32_t color = color1;
    if (index > (width<<1)-1) color = SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 1);
    else if (index > width-1) color = color2;

    SEGMENT.setPixelColor(SEGLEN - i -1, color);
  }
  return FRAMETIME;
}


/*
 * Tricolor chase mode
 */
uint16_t mode_tricolor_chase(void) {
  return tricolor_chase(SEGCOLOR(2), SEGCOLOR(0));
}
static const char _data_FX_MODE_TRICOLOR_CHASE[] PROGMEM = "Chase 3@!,Size;1,2,3;!";


/*
 * ICU mode
 */
uint16_t mode_icu(void) {
  unsigned dest = SEGENV.step & 0xFFFF;
  unsigned space = (SEGMENT.intensity >> 3) +2;

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  byte pindex = map(dest, 0, SEGLEN-SEGLEN/space, 0, 255);
  uint32_t col = SEGMENT.color_from_palette(pindex, false, false, 0);

  SEGMENT.setPixelColor(dest, col);
  SEGMENT.setPixelColor(dest + SEGLEN/space, col);

  if(SEGENV.aux0 == dest) { // pause between eye movements
    if(hw_random8(6) == 0) { // blink once in a while
      SEGMENT.setPixelColor(dest, SEGCOLOR(1));
      SEGMENT.setPixelColor(dest + SEGLEN/space, SEGCOLOR(1));
      return 200;
    }
    SEGENV.aux0 = hw_random16(SEGLEN-SEGLEN/space);
    return 1000 + hw_random16(2000);
  }

  if(SEGENV.aux0 > SEGENV.step) {
    SEGENV.step++;
    dest++;
  } else if (SEGENV.aux0 < SEGENV.step) {
    SEGENV.step--;
    dest--;
  }

  SEGMENT.setPixelColor(dest, col);
  SEGMENT.setPixelColor(dest + SEGLEN/space, col);

  return SPEED_FORMULA_L;
}
static const char _data_FX_MODE_ICU[] PROGMEM = "ICU@!,!,,,,,Overlay;!,!;!";


/*
 * Custom mode by Aircoookie. Color Wipe, but with 3 colors
 */
uint16_t mode_tricolor_wipe(void) {
  uint32_t cycleTime = 1000 + (255 - SEGMENT.speed)*200;
  uint32_t perc = strip.now % cycleTime;
  unsigned prog = (perc * 65535) / cycleTime;
  unsigned ledIndex = (prog * SEGLEN * 3) >> 16;
  unsigned ledOffset = ledIndex;

  for (unsigned i = 0; i < SEGLEN; i++)
  {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 2));
  }

  if(ledIndex < SEGLEN) { //wipe from 0 to 1
    for (unsigned i = 0; i < SEGLEN; i++)
    {
      SEGMENT.setPixelColor(i, (i > ledOffset)? SEGCOLOR(0) : SEGCOLOR(1));
    }
  } else if (ledIndex < SEGLEN*2) { //wipe from 1 to 2
    ledOffset = ledIndex - SEGLEN;
    for (unsigned i = ledOffset +1; i < SEGLEN; i++)
    {
      SEGMENT.setPixelColor(i, SEGCOLOR(1));
    }
  } else //wipe from 2 to 0
  {
    ledOffset = ledIndex - SEGLEN*2;
    for (unsigned i = 0; i <= ledOffset; i++)
    {
      SEGMENT.setPixelColor(i, SEGCOLOR(0));
    }
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TRICOLOR_WIPE[] PROGMEM = "Tri Wipe@!;1,2,3;!";

 
 
//7 bytes
typedef struct Oscillator {
  uint16_t pos;
  uint8_t  size;
  int8_t   dir;
  uint8_t  speed;
} oscillator;

/*
/  Oscillating bars of color, updated with standard framerate
*/
uint16_t mode_oscillate(void) {
  constexpr unsigned numOscillators = 3;
  constexpr unsigned dataSize = sizeof(oscillator) * numOscillators;

  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  Oscillator* oscillators = reinterpret_cast<Oscillator*>(SEGENV.data);

  if (SEGENV.call == 0)
  {
    oscillators[0] = {(uint16_t)(SEGLEN/4),   (uint8_t)(SEGLEN/8),  1, 1};
    oscillators[1] = {(uint16_t)(SEGLEN/4*3), (uint8_t)(SEGLEN/8),  1, 2};
    oscillators[2] = {(uint16_t)(SEGLEN/4*2), (uint8_t)(SEGLEN/8), -1, 1};
  }

  uint32_t cycleTime = 20 + (2 * (uint32_t)(255 - SEGMENT.speed));
  uint32_t it = strip.now / cycleTime;

  for (unsigned i = 0; i < numOscillators; i++) {
    // if the counter has increased, move the oscillator by the random step
    if (it != SEGENV.step) oscillators[i].pos += oscillators[i].dir * oscillators[i].speed;
    oscillators[i].size = SEGLEN/(3+SEGMENT.intensity/8);
    if((oscillators[i].dir == -1) && (oscillators[i].pos > SEGLEN << 1)) { // use integer overflow
      oscillators[i].pos = 0;
      oscillators[i].dir = 1;
      // make bigger steps for faster speeds
      oscillators[i].speed = SEGMENT.speed > 100 ? hw_random8(2, 4):hw_random8(1, 3);
    }
    if((oscillators[i].dir == 1) && (oscillators[i].pos >= (SEGLEN - 1))) {
      oscillators[i].pos = SEGLEN - 1;
      oscillators[i].dir = -1;
      oscillators[i].speed = SEGMENT.speed > 100 ? hw_random8(2, 4):hw_random8(1, 3);
    }
  }

  for (unsigned i = 0; i < SEGLEN; i++) {
    uint32_t color = BLACK;
    for (unsigned j = 0; j < numOscillators; j++) {
      if((int)i >= (int)oscillators[j].pos - oscillators[j].size && i <= oscillators[j].pos + oscillators[j].size) {
        color = (color == BLACK) ? SEGCOLOR(j) : color_blend(color, SEGCOLOR(j), uint8_t(128));
      }
    }
    SEGMENT.setPixelColor(i, color);
  }

  SEGENV.step = it;
  return FRAMETIME;
}
static const char _data_FX_MODE_OSCILLATE[] PROGMEM = "Oscillate";


//TODO
uint16_t mode_lightning(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned ledstart = hw_random16(SEGLEN);               // Determine starting location of flash
  unsigned ledlen = 1 + hw_random16(SEGLEN -ledstart);   // Determine length of flash (not to go beyond NUM_LEDS-1)
  uint8_t bri = 255/hw_random8(1, 3);

  if (SEGENV.aux1 == 0) //init, leader flash
  {
    SEGENV.aux1 = hw_random8(4, 4 + SEGMENT.intensity/20); //number of flashes
    SEGENV.aux1 *= 2;

    bri = 52; //leader has lower brightness
    SEGENV.aux0 = 200; //200ms delay after leader
  }

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  if (SEGENV.aux1 > 3 && !(SEGENV.aux1 & 0x01)) { //flash on even number >2
    for (unsigned i = ledstart; i < ledstart + ledlen; i++)
    {
      SEGMENT.setPixelColor(i,SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0, bri));
    }
    SEGENV.aux1--;

    SEGENV.step = strip.now;
    //return hw_random8(4, 10); // each flash only lasts one frame/every 24ms... originally 4-10 milliseconds
  } else {
    if (strip.now - SEGENV.step > SEGENV.aux0) {
      SEGENV.aux1--;
      if (SEGENV.aux1 < 2) SEGENV.aux1 = 0;

      SEGENV.aux0 = (50 + hw_random8(100)); //delay between flashes
      if (SEGENV.aux1 == 2) {
        SEGENV.aux0 = (hw_random8(255 - SEGMENT.speed) * 100); // delay between strikes
      }
      SEGENV.step = strip.now;
    }
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_LIGHTNING[] PROGMEM = "Lightning@!,!,,,,,Overlay;!,!;!";

// combined function from original pride and colorwaves
uint16_t mode_colorwaves_pride_base(bool isPride2015) {
  unsigned duration = 10 + SEGMENT.speed;
  unsigned sPseudotime = SEGENV.step;
  unsigned sHue16 = SEGENV.aux0;

  uint8_t sat8 = isPride2015 ? beatsin88_t(87, 220, 250) : 255;
  unsigned brightdepth = beatsin88_t(341, 96, 224);
  unsigned brightnessthetainc16 = beatsin88_t(203, (25 * 256), (40 * 256));
  unsigned msmultiplier = beatsin88_t(147, 23, 60);

  unsigned hue16 = sHue16;
  unsigned hueinc16 = isPride2015 ? beatsin88_t(113, 1, 3000) : 
                                     beatsin88_t(113, 60, 300) * SEGMENT.intensity * 10 / 255;

  sPseudotime += duration * msmultiplier;
  sHue16 += duration * beatsin88_t(400, 5, 9);
  unsigned brightnesstheta16 = sPseudotime;

  for (unsigned i = 0; i < SEGLEN; i++) {
    hue16 += hueinc16;
    uint8_t hue8;

    if (isPride2015) {
      hue8 = hue16 >> 8;
    } else {
      unsigned h16_128 = hue16 >> 7;
      hue8 = (h16_128 & 0x100) ? (255 - (h16_128 >> 1)) : (h16_128 >> 1);
    }

    brightnesstheta16 += brightnessthetainc16;
    unsigned b16 = sin16_t(brightnesstheta16) + 32768;
    unsigned bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    if (isPride2015) {
      CRGBW newcolor = CRGB(CHSV(hue8, sat8, bri8));
      newcolor.color32 = gamma32inv(newcolor.color32);
      SEGMENT.blendPixelColor(i, newcolor, 64);
    } else {
      SEGMENT.blendPixelColor(i, SEGMENT.color_from_palette(hue8, false, PALETTE_SOLID_WRAP, 0, bri8), 128);
    }
  }

  SEGENV.step = sPseudotime;
  SEGENV.aux0 = sHue16;

  return FRAMETIME;
}

// Pride2015
// Animated, ever-changing rainbows.
// by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
uint16_t mode_pride_2015(void) {
  return mode_colorwaves_pride_base(true);
}
static const char _data_FX_MODE_PRIDE_2015[] PROGMEM = "Pride 2015@!;;";

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
uint16_t mode_colorwaves() {
  return mode_colorwaves_pride_base(false);
}
static const char _data_FX_MODE_COLORWAVES[] PROGMEM = "Colorwaves@!,Hue;!;!;;pal=26";


//eight colored dots, weaving in and out of sync with each other
uint16_t mode_juggle(void) {
  if (SEGLEN <= 1) return mode_static();

  SEGMENT.fadeToBlackBy(192 - (3*SEGMENT.intensity/4));
  CRGB fastled_col;
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    int index = 0 + beatsin88_t((16 + SEGMENT.speed)*(i + 7), 0, SEGLEN -1);
    fastled_col = CRGB(SEGMENT.getPixelColor(index));
    fastled_col |= (SEGMENT.palette==0)?CHSV(dothue, 220, 255):CRGB(ColorFromPalette(SEGPALETTE, dothue, 255));
    SEGMENT.setPixelColor(index, fastled_col);
    dothue += 32;
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_JUGGLE[] PROGMEM = "Juggle@!,Trail;;!;;sx=64,ix=128";


uint16_t mode_palette() {
  // Set up some compile time constants so that we can handle integer and float based modes using the same code base.
#ifdef ESP8266
  using mathType = int32_t;
  using wideMathType = int64_t;
  using angleType = unsigned;
  constexpr mathType sInt16Scale             = 0x7FFF;
  constexpr mathType maxAngle                = 0x8000;
  constexpr mathType staticRotationScale     = 256;
  constexpr mathType animatedRotationScale   = 1;
  constexpr int16_t (*sinFunction)(uint16_t) = &sin16_t;
  constexpr int16_t (*cosFunction)(uint16_t) = &cos16_t;
#else
  using mathType = float;
  using wideMathType = float;
  using angleType = float;
  constexpr mathType sInt16Scale           = 1.0f;
  constexpr mathType maxAngle              = M_PI / 256.0;
  constexpr mathType staticRotationScale   = 1.0f;
  constexpr mathType animatedRotationScale = M_TWOPI / double(0xFFFF);
  constexpr float (*sinFunction)(float)    = &sin_t;
  constexpr float (*cosFunction)(float)    = &cos_t;
#endif
  const bool isMatrix = strip.isMatrix;
  const int cols = SEG_W;
  const int rows = isMatrix ? SEG_H : strip.getActiveSegmentsNum();

  const int  inputShift           = SEGMENT.speed;
  const int  inputSize            = SEGMENT.intensity;
  const int  inputRotation        = SEGMENT.custom1;
  const bool inputAnimateShift    = SEGMENT.check1;
  const bool inputAnimateRotation = SEGMENT.check2;
  const bool inputAssumeSquare    = SEGMENT.check3;

  const angleType theta = (!inputAnimateRotation) ? ((inputRotation + 128) * maxAngle / staticRotationScale) : (((strip.now * ((inputRotation >> 4) +1)) & 0xFFFF) * animatedRotationScale);
  const mathType sinTheta = sinFunction(theta);
  const mathType cosTheta = cosFunction(theta);

  const mathType maxX    = std::max(1, cols-1);
  const mathType maxY    = std::max(1, rows-1);
  // Set up some parameters according to inputAssumeSquare, so that we can handle anamorphic mode using the same code base.
  const mathType maxXIn  =  inputAssumeSquare ? maxX : mathType(1);
  const mathType maxYIn  =  inputAssumeSquare ? maxY : mathType(1);
  const mathType maxXOut = !inputAssumeSquare ? maxX : mathType(1);
  const mathType maxYOut = !inputAssumeSquare ? maxY : mathType(1);
  const mathType centerX = sInt16Scale * maxXOut / mathType(2);
  const mathType centerY = sInt16Scale * maxYOut / mathType(2);
  // The basic idea for this effect is to rotate a rectangle that is filled with the palette along one axis, then map our
  // display to it, to find what color a pixel should have.
  // However, we want a) no areas of solid color (in front of or behind the palette), and b) we want to make use of the full palette.
  // So the rectangle needs to have exactly the right size. That size depends on the rotation.
  // This scale computation here only considers one dimension. You can think of it like the rectangle is always scaled so that
  // the left and right most points always match the left and right side of the display.
  const mathType scale = std::abs(sinTheta) + (std::abs(cosTheta) * maxYOut / maxXOut);
  // 2D simulation:
  // If we are dealing with a 1D setup, we assume that each segment represents one line on a 2-dimensional display.
  // The function is called once per segments, so we need to handle one line at a time.
  const int yFrom = isMatrix ? 0 : strip.getCurrSegmentId();
  const int yTo   = isMatrix ? maxY : yFrom;
  for (int y = yFrom; y <= yTo; ++y) {
    // translate, scale, rotate
    const mathType ytCosTheta = mathType((wideMathType(cosTheta) * wideMathType(y * sInt16Scale - centerY * maxYIn))/wideMathType(maxYIn * scale));
    for (int x = 0; x < cols; ++x) {
      // translate, scale, rotate
      const mathType xtSinTheta = mathType((wideMathType(sinTheta) * wideMathType(x * sInt16Scale - centerX * maxXIn))/wideMathType(maxXIn * scale));
      // Map the pixel coordinate to an imaginary-rectangle-coordinate.
      // The y coordinate doesn't actually matter, as our imaginary rectangle is filled with the palette from left to right,
      // so all points at a given x-coordinate have the same color.
      const mathType sourceX = xtSinTheta + ytCosTheta + centerX;
      // The computation was scaled just right so that the result should always be in range [0, maxXOut], but enforce this anyway
      // to account for imprecision. Then scale it so that the range is [0, 255], which we can use with the palette.
      int colorIndex = (std::min(std::max(sourceX, mathType(0)), maxXOut * sInt16Scale) * wideMathType(255)) / (sInt16Scale * maxXOut);
      // inputSize determines by how much we want to scale the palette:
      // values < 128 display a fraction of a palette,
      // values > 128 display multiple palettes.
      if (inputSize <= 128) {
        colorIndex = (colorIndex * inputSize) / 128;
      } else {
        // Linear function that maps colorIndex 128=>1, 256=>9.
        // With this function every full palette repetition is exactly 16 configuration steps wide.
        // That allows displaying exactly 2 repetitions for example.
        colorIndex = ((inputSize - 112) * colorIndex) / 16;
      }
      // Finally, shift the palette a bit.
      const int paletteOffset = (!inputAnimateShift) ? (inputShift) : (((strip.now * ((inputShift >> 3) +1)) & 0xFFFF) >> 8);
      colorIndex -= paletteOffset;
      const uint32_t color = SEGMENT.color_wheel((uint8_t)colorIndex);
      if (isMatrix) {
        SEGMENT.setPixelColorXY(x, y, color);
      } else {
        SEGMENT.setPixelColor(x, color);
      }
    }
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_PALETTE[] PROGMEM = "Palette@Shift,Size,Rotation,,,Animate Shift,Animate Rotation,Anamorphic;;!;12;ix=112,c1=0,o1=1,o2=0,o3=1";

#ifdef WLED_PS_DONT_REPLACE_FX
// WLED limitation: Analog Clock overlay will NOT work when Fire2012 is active
// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on SEGLEN; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above) (Speed = COOLING), and SPARKING (used
// in step 3 above) (Effect Intensity = Sparking).
uint16_t mode_fire_2012() {
  if (SEGLEN <= 1) return mode_static();
  const unsigned strips = SEGMENT.nrOfVStrips();
  if (!SEGENV.allocateData(strips * SEGLEN)) return mode_static(); //allocation failed
  byte* heat = SEGENV.data;

  const uint32_t it = strip.now >> 5; //div 32

  struct virtualStrip {
    static void runStrip(uint16_t stripNr, byte* heat, uint32_t it) {

      const uint8_t ignition = MAX(3,SEGLEN/10);  // ignition area: 10% of segment length or minimum 3 pixels

      // Step 1.  Cool down every cell a little
      for (unsigned i = 0; i < SEGLEN; i++) {
        uint8_t cool = (it != SEGENV.step) ? hw_random8((((20 + SEGMENT.speed/3) * 16) / SEGLEN)+2) : hw_random8(4);
        uint8_t minTemp = (i<ignition) ? (ignition-i)/4 + 16 : 0;  // should not become black in ignition area
        uint8_t temp = qsub8(heat[i], cool);
        heat[i] = temp<minTemp ? minTemp : temp;
      }

      if (it != SEGENV.step) {
        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for (int k = SEGLEN -1; k > 1; k--) {
          heat[k] = (heat[k - 1] + (heat[k - 2]<<1) ) / 3;  // heat[k-2] multiplied by 2
        }

        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        if (hw_random8() <= SEGMENT.intensity) {
          uint8_t y = hw_random8(ignition);
          uint8_t boost = (17+SEGMENT.custom3) * (ignition - y/2) / ignition; // integer math!
          heat[y] = qadd8(heat[y], hw_random8(96+2*boost,207+boost));
        }
      }

      // Step 4.  Map from heat cells to LED colors
      for (unsigned j = 0; j < SEGLEN; j++) {
        SEGMENT.setPixelColor(indexToVStrip(j, stripNr), ColorFromPalette(SEGPALETTE, min(heat[j], byte(240)), 255, NOBLEND));
      }
    }
  };

  for (unsigned stripNr=0; stripNr<strips; stripNr++)
    virtualStrip::runStrip(stripNr, &heat[stripNr * SEGLEN], it);

  if (SEGMENT.is2D()) {
    uint8_t blurAmount = SEGMENT.custom2 >> 2;
    if (blurAmount > 48) blurAmount += blurAmount-48;             // extra blur when slider > 192  (bush burn)
    if (blurAmount < 16) SEGMENT.blurCols(SEGMENT.custom2 >> 1);  // no side-burn when slider < 64 (faster)
    else SEGMENT.blur(blurAmount);
  }

  if (it != SEGENV.step)
    SEGENV.step = it;

  return FRAMETIME;
}
static const char _data_FX_MODE_FIRE_2012[] PROGMEM = "Fire 2012@Cooling,Spark rate,,2D Blur,Boost;;!;1;pal=35,sx=64,ix=160,m12=1,c2=128"; // bars
#endif // WLED_PS_DONT_REPLACE_FX
 

//Calm effect, like a lake at night
uint16_t mode_lake() {
  unsigned sp = SEGMENT.speed/10;
  int wave1 = beatsin8_t(sp +2, -64,64);
  int wave2 = beatsin8_t(sp +1, -64,64);
  int wave3 = beatsin8_t(sp +2,   0,80);

  for (unsigned i = 0; i < SEGLEN; i++)
  {
    int index = cos8_t((i*15)+ wave1)/2 + cubicwave8((i*23)+ wave2)/2;
    uint8_t lum = (index > wave3) ? index - wave3 : 0;
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, false, 0, lum));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_LAKE[] PROGMEM = "Lake@!;Fx;!";

 

//Railway Crossing / Christmas Fairy lights
uint16_t mode_railway() {
  if (SEGLEN <= 1) return mode_static();
  unsigned dur = (256 - SEGMENT.speed) * 40;
  uint16_t rampdur = (dur * SEGMENT.intensity) >> 8;
  if (SEGENV.step > dur)
  {
    //reverse direction
    SEGENV.step = 0;
    SEGENV.aux0 = !SEGENV.aux0;
  }
  unsigned pos = 255;
  if (rampdur != 0)
  {
    unsigned p0 = (SEGENV.step * 255) / rampdur;
    if (p0 < 255) pos = p0;
  }
  if (SEGENV.aux0) pos = 255 - pos;
  for (unsigned i = 0; i < SEGLEN; i += 2)
  {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(255 - pos, false, false, 255)); // do not use color 1 or 2, always use palette
    if (i < SEGLEN -1)
    {
      SEGMENT.setPixelColor(i + 1, SEGMENT.color_from_palette(pos, false, false, 255)); // do not use color 1 or 2, always use palette
    }
  }
  SEGENV.step += FRAMETIME;
  return FRAMETIME;
}
static const char _data_FX_MODE_RAILWAY[] PROGMEM = "Railway@!,Smoothness;1,2;!;;pal=3";


//Water ripple
//propagation velocity from speed
//drop rate from intensity

//4 bytes
typedef struct Ripple {
  uint8_t state;
  uint8_t color;
  uint16_t pos;
} ripple;

#ifdef ESP8266
  #define MAX_RIPPLES   56
#else
  #define MAX_RIPPLES  100
#endif
static uint16_t ripple_base(uint8_t blurAmount = 0) {
  unsigned maxRipples = min(1 + (int)(SEGLEN >> 2), MAX_RIPPLES);  // 56 max for 16 segment ESP8266
  unsigned dataSize = sizeof(ripple) * maxRipples;

  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  Ripple* ripples = reinterpret_cast<Ripple*>(SEGENV.data);

  //draw wave
  for (unsigned i = 0; i < maxRipples; i++) {
    unsigned ripplestate = ripples[i].state;
    if (ripplestate) {
      unsigned rippledecay = (SEGMENT.speed >> 4) +1; //faster decay if faster propagation
      unsigned rippleorigin = ripples[i].pos;
      uint32_t col = SEGMENT.color_from_palette(ripples[i].color, false, false, 255);
      unsigned propagation = ((ripplestate/rippledecay - 1) * (SEGMENT.speed + 1));
      int propI = propagation >> 8;
      unsigned propF = propagation & 0xFF;
      unsigned amp = (ripplestate < 17) ? triwave8((ripplestate-1)*8) : map(ripplestate,17,255,255,2);

      #ifndef WLED_DISABLE_2D
      if (SEGMENT.is2D()) {
        propI /= 2;
        unsigned cx = rippleorigin >> 8;
        unsigned cy = rippleorigin & 0xFF;
        unsigned mag = scale8(sin8_t((propF>>2)), amp);
        if (propI > 0) SEGMENT.drawCircle(cx, cy, propI, color_blend(SEGMENT.getPixelColorXY(cx + propI, cy), col, mag), true);
      } else
      #endif
      {
        int left = rippleorigin - propI -1;
        int right = rippleorigin + propI +2;
        for (int v = 0; v < 4; v++) {
          uint8_t mag = scale8(cubicwave8((propF>>2) + v * 64), amp);
          SEGMENT.setPixelColor(left + v, color_blend(SEGMENT.getPixelColor(left + v), col, mag)); // TODO
          SEGMENT.setPixelColor(right - v, color_blend(SEGMENT.getPixelColor(right - v), col, mag)); // TODO
        }
      }
      ripplestate += rippledecay;
      ripples[i].state = (ripplestate > 254) ? 0 : ripplestate;
    } else {//randomly create new wave
      if (hw_random16(IBN + 10000) <= (SEGMENT.intensity >> (SEGMENT.is2D()*3))) {
        ripples[i].state = 1;
        ripples[i].pos = SEGMENT.is2D() ? ((hw_random8(SEG_W)<<8) | (hw_random8(SEG_H))) : hw_random16(SEGLEN);
        ripples[i].color = hw_random8(); //color
      }
    }
  }
  SEGMENT.blur(blurAmount);
  return FRAMETIME;
}
#undef MAX_RIPPLES


uint16_t mode_ripple(void) {
  if (SEGLEN <= 1) return mode_static();
  if(SEGMENT.custom1 || SEGMENT.check2) // blur or overlay
    SEGMENT.fade_out(250);
  else
    SEGMENT.fill(SEGCOLOR(1));

  return ripple_base(SEGMENT.custom1>>1);
}
static const char _data_FX_MODE_RIPPLE[] PROGMEM = "Ripple@!,Wave #,Blur,,,,Overlay;,!;!;12;c1=0";


uint16_t mode_ripple_rainbow(void) {
  if (SEGLEN <= 1) return mode_static();
  if (SEGENV.call ==0) {
    SEGENV.aux0 = hw_random8();
    SEGENV.aux1 = hw_random8();
  }
  if (SEGENV.aux0 == SEGENV.aux1) {
    SEGENV.aux1 = hw_random8();
  } else if (SEGENV.aux1 > SEGENV.aux0) {
    SEGENV.aux0++;
  } else {
    SEGENV.aux0--;
  }
  SEGMENT.fill(color_blend(SEGMENT.color_wheel(SEGENV.aux0),BLACK,uint8_t(235)));
  return ripple_base();
}
static const char _data_FX_MODE_RIPPLE_RAINBOW[] PROGMEM = "Ripple Rainbow@!,Wave #;;!;12";

 

uint16_t mode_halloween_eyes()
{
  enum eyeState : uint8_t {
    initializeOn = 0,
    on,
    blink,
    initializeOff,
    off,

    count
  };
  struct EyeData {
    eyeState state;
    uint8_t color;
    uint16_t startPos;
    // duration + endTime could theoretically be replaced by a single endTime, however we would lose
    // the ability to end the animation early when the user reduces the animation time.
    uint16_t duration;
    uint32_t startTime;
    uint32_t blinkEndTime;
  };

  if (SEGLEN <= 1) return mode_static();
  const unsigned maxWidth = strip.isMatrix ? SEG_W : SEGLEN;
  const unsigned HALLOWEEN_EYE_SPACE = MAX(2, strip.isMatrix ? SEG_W>>4: SEGLEN>>5);
  const unsigned HALLOWEEN_EYE_WIDTH = HALLOWEEN_EYE_SPACE/2;
  unsigned eyeLength = (2*HALLOWEEN_EYE_WIDTH) + HALLOWEEN_EYE_SPACE;
  if (eyeLength >= maxWidth) return mode_static(); //bail if segment too short

  if (!SEGENV.allocateData(sizeof(EyeData))) return mode_static(); //allocation failed
  EyeData& data = *reinterpret_cast<EyeData*>(SEGENV.data);

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1)); //fill background

  data.state = static_cast<eyeState>(data.state % eyeState::count);
  unsigned duration = max(uint16_t{1u}, data.duration);
  const uint32_t elapsedTime = strip.now - data.startTime;

  switch (data.state) {
    case eyeState::initializeOn: {
      // initialize the eyes-on state:
      // - select eye position and color
      // - select a duration
      // - immediately switch to eyes on state.

      data.startPos = hw_random16(0, maxWidth - eyeLength - 1);
      data.color = hw_random8();
      if (strip.isMatrix) SEGMENT.offset = hw_random16(SEG_H-1); // a hack: reuse offset since it is not used in matrices
      duration = 128u + hw_random16(SEGMENT.intensity*64u);
      data.duration = duration;
      data.state = eyeState::on;
      [[fallthrough]];
    }
    case eyeState::on: {
      // eyes-on steate:
      // - fade eyes in for some time
      // - keep eyes on until the pre-selected duration is over
      // - randomly switch to the blink (sub-)state, and initialize it with a blink duration (more precisely, a blink end time stamp)
      // - never switch to the blink state if the animation just started or is about to end

      unsigned start2ndEye = data.startPos + HALLOWEEN_EYE_WIDTH + HALLOWEEN_EYE_SPACE;
      // If the user reduces the input while in this state, limit the duration.
      duration = min(duration, (128u + (SEGMENT.intensity * 64u)));

      constexpr uint32_t minimumOnTimeBegin = 1024u;
      constexpr uint32_t minimumOnTimeEnd = 1024u;
      const uint32_t fadeInAnimationState = elapsedTime * uint32_t{256u * 8u} / duration;
      const uint32_t backgroundColor = SEGCOLOR(1);
      const uint32_t eyeColor = SEGMENT.color_from_palette(data.color, false, false, 0);
      uint32_t c = eyeColor;
      if (fadeInAnimationState < 256u) {
        c = color_blend(backgroundColor, eyeColor, uint8_t(fadeInAnimationState));
      } else if (elapsedTime > minimumOnTimeBegin) {
        const uint32_t remainingTime = (elapsedTime >= duration) ? 0u : (duration - elapsedTime);
        if (remainingTime > minimumOnTimeEnd) {
          if (hw_random8() < 4u)
          {
            c = backgroundColor;
            data.state = eyeState::blink;
            data.blinkEndTime = strip.now + hw_random8(8, 128);
          }
        }
      }

      if (c != backgroundColor) {
        // render eyes
        for (unsigned i = 0; i < HALLOWEEN_EYE_WIDTH; i++) {
          if (strip.isMatrix) {
            SEGMENT.setPixelColorXY(data.startPos + i, (unsigned)SEGMENT.offset, c);
            SEGMENT.setPixelColorXY(start2ndEye   + i, (unsigned)SEGMENT.offset, c);
          } else {
            SEGMENT.setPixelColor(data.startPos + i, c);
            SEGMENT.setPixelColor(start2ndEye   + i, c);
          }
        }
      }
      break;
    }
    case eyeState::blink: {
      // eyes-on but currently blinking state:
      // - wait until the blink time is over, then switch back to eyes-on

      if (strip.now >= data.blinkEndTime) {
        data.state = eyeState::on;
      }
      break;
    }
    case eyeState::initializeOff: {
      // initialize eyes-off state:
      // - select a duration
      // - immediately switch to eyes-off state

      const unsigned eyeOffTimeBase = SEGMENT.speed*128u;
      duration = eyeOffTimeBase + hw_random16(eyeOffTimeBase);
      data.duration = duration;
      data.state = eyeState::off;
      [[fallthrough]];
    }
    case eyeState::off: {
      // eyes-off state:
      // - not much to do here

      // If the user reduces the input while in this state, limit the duration.
      const unsigned eyeOffTimeBase = SEGMENT.speed*128u;
      duration = min(duration, (2u * eyeOffTimeBase));
      break;
    }
    case eyeState::count: {
      // Can't happen, not an actual state.
      data.state = eyeState::initializeOn;
      break;
    }
  }

  if (elapsedTime > duration) {
    // The current state duration is over, switch to the next state.
    switch (data.state) {
      case eyeState::initializeOn:
      case eyeState::on:
      case eyeState::blink:
        data.state = eyeState::initializeOff;
        break;
      case eyeState::initializeOff:
      case eyeState::off:
      case eyeState::count:
      default:
        data.state = eyeState::initializeOn;
        break;
    }
    data.startTime = strip.now;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_HALLOWEEN_EYES[] PROGMEM = "Halloween Eyes@Eye off time,Eye on time,,,,,Overlay;!,!;!;12";
 
 
static uint16_t spots_base(uint16_t threshold)
{
  if (SEGLEN <= 1) return mode_static();
  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  unsigned maxZones = SEGLEN >> 2;
  unsigned zones = 1 + ((SEGMENT.intensity * maxZones) >> 8);
  unsigned zoneLen = SEGLEN / zones;
  unsigned offset = (SEGLEN - zones * zoneLen) >> 1;

  for (unsigned z = 0; z < zones; z++)
  {
    unsigned pos = offset + z * zoneLen;
    for (unsigned i = 0; i < zoneLen; i++)
    {
      unsigned wave = triwave16((i * 0xFFFF) / zoneLen);
      if (wave > threshold) {
        unsigned index = 0 + pos + i;
        unsigned s = (wave - threshold)*255 / (0xFFFF - threshold);
        SEGMENT.setPixelColor(index, color_blend(SEGMENT.color_from_palette(index, true, PALETTE_SOLID_WRAP, 0), SEGCOLOR(1), uint8_t(255-s)));
      }
    }
  }

  return FRAMETIME;
}


//Intensity slider sets number of "lights", speed sets LEDs per light
uint16_t mode_spots()
{
  return spots_base((255 - SEGMENT.speed) << 8);
}
static const char _data_FX_MODE_SPOTS[] PROGMEM = "Spots@Spread,Width,,,,,Overlay;!,!;!";


//Intensity slider sets number of "lights", LEDs per light fade in and out
uint16_t mode_spots_fade()
{
  unsigned counter = strip.now * ((SEGMENT.speed >> 2) +8);
  unsigned t = triwave16(counter);
  unsigned tr = (t >> 1) + (t >> 2);
  return spots_base(tr);
}
static const char _data_FX_MODE_SPOTS_FADE[] PROGMEM = "Spots Fade@Spread,Width,,,,,Overlay;!,!;!";

//each needs 12 bytes
typedef struct Ball {
  unsigned long lastBounceTime;
  float impactVelocity;
  float height;
} ball;

/*
*  Bouncing Balls Effect
*/
uint16_t mode_bouncing_balls(void) {
  if (SEGLEN <= 1) return mode_static();
  //allocate segment data
  const unsigned strips = SEGMENT.nrOfVStrips(); // adapt for 2D
  const size_t maxNumBalls = 16;
  unsigned dataSize = sizeof(ball) * maxNumBalls;
  if (!SEGENV.allocateData(dataSize * strips)) return mode_static(); //allocation failed

  Ball* balls = reinterpret_cast<Ball*>(SEGENV.data);

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(2) ? BLACK : SEGCOLOR(1));

  // virtualStrip idea by @ewowi (Ewoud Wijma)
  // requires virtual strip # to be embedded into upper 16 bits of index in setPixelColor()
  // the following functions will not work on virtual strips: fill(), fade_out(), fadeToBlack(), blur()
  struct virtualStrip {
    static void runStrip(size_t stripNr, Ball* balls) {
      // number of balls based on intensity setting to max of 7 (cycles colors)
      // non-chosen color is a random color
      unsigned numBalls = (SEGMENT.intensity * (maxNumBalls - 1)) / 255 + 1; // minimum 1 ball
      const float gravity = -9.81f; // standard value of gravity
      const bool hasCol2 = SEGCOLOR(2);
      const unsigned long time = strip.now;

      if (SEGENV.call == 0) {
        for (size_t i = 0; i < maxNumBalls; i++) balls[i].lastBounceTime = time;
      }

      for (size_t i = 0; i < numBalls; i++) {
        float timeSinceLastBounce = (time - balls[i].lastBounceTime)/((255-SEGMENT.speed)/64 +1);
        float timeSec = timeSinceLastBounce/1000.0f;
        balls[i].height = (0.5f * gravity * timeSec + balls[i].impactVelocity) * timeSec; // avoid use pow(x, 2) - its extremely slow !

        if (balls[i].height <= 0.0f) {
          balls[i].height = 0.0f;
          //damping for better effect using multiple balls
          float dampening = 0.9f - float(i)/float(numBalls * numBalls); // avoid use pow(x, 2) - its extremely slow !
          balls[i].impactVelocity = dampening * balls[i].impactVelocity;
          balls[i].lastBounceTime = time;

          if (balls[i].impactVelocity < 0.015f) {
            float impactVelocityStart = sqrtf(-2.0f * gravity) * hw_random8(5,11)/10.0f; // randomize impact velocity
            balls[i].impactVelocity = impactVelocityStart;
          }
        } else if (balls[i].height > 1.0f) {
          continue; // do not draw OOB ball
        }

        uint32_t color = SEGCOLOR(0);
        if (SEGMENT.palette) {
          color = SEGMENT.color_wheel(i*(256/MAX(numBalls, 8)));
        } else if (hasCol2) {
          color = SEGCOLOR(i % NUM_COLORS);
        }

        int pos = roundf(balls[i].height * (SEGLEN - 1));
        #ifdef WLED_USE_AA_PIXELS
        if (SEGLEN<32) SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
        else           SEGMENT.setPixelColor(balls[i].height + (stripNr+1)*10.0f, color);
        #else
        SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color); // encode virtual strip into index
        #endif
      }
    }
  };

  for (unsigned stripNr=0; stripNr<strips; stripNr++)
    virtualStrip::runStrip(stripNr, &balls[stripNr * maxNumBalls]);

  return FRAMETIME;
}
static const char _data_FX_MODE_BOUNCINGBALLS[] PROGMEM = "Bouncing Balls@Gravity,# of balls,,,,,Overlay;!,!,!;!;1;m12=1"; //bar

#ifdef WLED_PS_DONT_REPLACE_FX
/*
 *  bouncing balls on a track track Effect modified from Aircoookie's bouncing balls
 *  Courtesy of pjhatch (https://github.com/pjhatch)
 *  https://github.com/wled-dev/WLED/pull/1039
 */
// modified for balltrack mode
typedef struct RollingBall {
  unsigned long lastBounceUpdate;
  float mass; // could fix this to be = 1. if memory is an issue
  float velocity;
  float height;
} rball_t;

static uint16_t rolling_balls(void) {
  //allocate segment data
  const unsigned maxNumBalls = 16; // 255/16 + 1
  unsigned dataSize = sizeof(rball_t) * maxNumBalls;
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  rball_t *balls = reinterpret_cast<rball_t *>(SEGENV.data);

  // number of balls based on intensity setting to max of 16 (cycles colors)
  // non-chosen color is a random color
  unsigned numBalls = SEGMENT.intensity/16 + 1;
  bool hasCol2 = SEGCOLOR(2);

  if (SEGENV.call == 0) {
    SEGMENT.fill(hasCol2 ? BLACK : SEGCOLOR(1));                    // start clean
    for (unsigned i = 0; i < maxNumBalls; i++) {
      balls[i].lastBounceUpdate = strip.now;
      balls[i].velocity = 20.0f * float(hw_random16(1000, 10000))/10000.0f;  // number from 1 to 10
      if (hw_random8()<128) balls[i].velocity = -balls[i].velocity;    // 50% chance of reverse direction
      balls[i].height = (float(hw_random16(0, 10000)) / 10000.0f);     // from 0. to 1.
      balls[i].mass   = (float(hw_random16(1000, 10000)) / 10000.0f);  // from .1 to 1.
    }
  }

  float cfac = float(scale8(8, 255-SEGMENT.speed) +1)*20000.0f; // this uses the Aircoookie conversion factor for scaling time using speed slider

  if (SEGMENT.check3) SEGMENT.fade_out(250); // 2-8 pixel trails (optional)
  else {
  	if (!SEGMENT.check2) SEGMENT.fill(hasCol2 ? BLACK : SEGCOLOR(1)); // don't fill with background color if user wants to see trails
  }

  for (unsigned i = 0; i < numBalls; i++) {
    float timeSinceLastUpdate = float((strip.now - balls[i].lastBounceUpdate))/cfac;
    float thisHeight = balls[i].height + balls[i].velocity * timeSinceLastUpdate; // this method keeps higher resolution
    // test if intensity level was increased and some balls are way off the track then put them back
    if (thisHeight < -0.5f || thisHeight > 1.5f) {
      thisHeight = balls[i].height = (float(hw_random16(0, 10000)) / 10000.0f); // from 0. to 1.
      balls[i].lastBounceUpdate = strip.now;
    }
    // check if reached ends of the strip
    if ((thisHeight <= 0.0f && balls[i].velocity < 0.0f) || (thisHeight >= 1.0f && balls[i].velocity > 0.0f)) {
      balls[i].velocity = -balls[i].velocity; // reverse velocity
      balls[i].lastBounceUpdate = strip.now;
      balls[i].height = thisHeight;
    }
    // check for collisions
    if (SEGMENT.check1) {
      for (unsigned j = i+1; j < numBalls; j++) {
        if (balls[j].velocity != balls[i].velocity) {
          //  tcollided + balls[j].lastBounceUpdate is acutal time of collision (this keeps precision with long to float conversions)
          float tcollided = (cfac*(balls[i].height - balls[j].height) +
                balls[i].velocity*float(balls[j].lastBounceUpdate - balls[i].lastBounceUpdate))/(balls[j].velocity - balls[i].velocity);

          if ((tcollided > 2.0f) && (tcollided < float(strip.now - balls[j].lastBounceUpdate))) { // 2ms minimum to avoid duplicate bounces
            balls[i].height = balls[i].height + balls[i].velocity*(tcollided + float(balls[j].lastBounceUpdate - balls[i].lastBounceUpdate))/cfac;
            balls[j].height = balls[i].height;
            balls[i].lastBounceUpdate = (unsigned long)(tcollided + 0.5f) + balls[j].lastBounceUpdate;
            balls[j].lastBounceUpdate = balls[i].lastBounceUpdate;
            float vtmp = balls[i].velocity;
            balls[i].velocity = ((balls[i].mass - balls[j].mass)*vtmp              + 2.0f*balls[j].mass*balls[j].velocity)/(balls[i].mass + balls[j].mass);
            balls[j].velocity = ((balls[j].mass - balls[i].mass)*balls[j].velocity + 2.0f*balls[i].mass*vtmp)             /(balls[i].mass + balls[j].mass);
            thisHeight = balls[i].height + balls[i].velocity*(strip.now - balls[i].lastBounceUpdate)/cfac;
          }
        }
      }
    }

    uint32_t color = SEGCOLOR(0);
    if (SEGMENT.palette) {
      //color = SEGMENT.color_wheel(i*(256/MAX(numBalls, 8)));
      color = SEGMENT.color_from_palette(i*255/numBalls, false, PALETTE_SOLID_WRAP, 0);
    } else if (hasCol2) {
      color = SEGCOLOR(i % NUM_COLORS);
    }

    if (thisHeight < 0.0f) thisHeight = 0.0f;
    if (thisHeight > 1.0f) thisHeight = 1.0f;
    unsigned pos = round(thisHeight * (SEGLEN - 1));
    SEGMENT.setPixelColor(pos, color);
    balls[i].lastBounceUpdate = strip.now;
    balls[i].height = thisHeight;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_ROLLINGBALLS[] PROGMEM = "Rolling Balls@!,# of balls,,,,Collide,Overlay,Trails;!,!,!;!;1;m12=1"; //bar
#endif // WLED_PS_DONT_REPLACE_FX
 

// utility function that will add random glitter to SEGMENT
void glitter_base(uint8_t intensity, uint32_t col = ULTRAWHITE) {
  if (intensity > hw_random8()) SEGMENT.setPixelColor(hw_random16(SEGLEN), col);
}

//Glitter with palette background, inspired by https://gist.github.com/kriegsman/062e10f7f07ba8518af6
uint16_t mode_glitter()
{
  if (!SEGMENT.check2) { // use "* Color 1" palette for solid background (replacing "Solid glitter")
    unsigned counter = 0;
    if (SEGMENT.speed != 0) {
      counter = (strip.now * ((SEGMENT.speed >> 3) +1)) & 0xFFFF;
      counter = counter >> 8;
    }

    bool noWrap = (strip.paletteBlend == 2 || (strip.paletteBlend == 0 && SEGMENT.speed == 0));
    for (unsigned i = 0; i < SEGLEN; i++) {
      unsigned colorIndex = (i * 255 / SEGLEN) - counter;
      if (noWrap) colorIndex = map(colorIndex, 0, 255, 0, 240); //cut off blend at palette "end"
      SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(colorIndex, false, true, 255));
    }
  }
  glitter_base(SEGMENT.intensity, SEGCOLOR(2) ? SEGCOLOR(2) : ULTRAWHITE);
  return FRAMETIME;
}
static const char _data_FX_MODE_GLITTER[] PROGMEM = "Glitter@!,!,,,,,Overlay;,,Glitter color;!;;pal=11,m12=0"; //pixels


//Solid colour background with glitter (can be replaced by Glitter)
uint16_t mode_solid_glitter()
{
  SEGMENT.fill(SEGCOLOR(0));
  glitter_base(SEGMENT.intensity, SEGCOLOR(2) ? SEGCOLOR(2) : ULTRAWHITE);
  return FRAMETIME;
}
static const char _data_FX_MODE_SOLID_GLITTER[] PROGMEM = "Solid Glitter@,!;Bg,,Glitter color;;;m12=0";

//each needs 20 bytes
//Spark type is used for popcorn, 1D fireworks, and drip
typedef struct Spark {
  float pos, posX;
  float vel, velX;
  uint16_t col;
  uint8_t colIndex;
} spark;

#define maxNumPopcorn 21 // max 21 on 16 segment ESP8266
/*
*  POPCORN
*  modified from https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/Popcorn.h
*/
uint16_t mode_popcorn(void) {
  if (SEGLEN <= 1) return mode_static();
  //allocate segment data
  unsigned strips = SEGMENT.nrOfVStrips();
  unsigned usablePopcorns = maxNumPopcorn;
  if (usablePopcorns * strips * sizeof(spark) > FAIR_DATA_PER_SEG) usablePopcorns = FAIR_DATA_PER_SEG / (strips * sizeof(spark)) + 1; // at least 1 popcorn per vstrip
  unsigned dataSize = sizeof(spark) * usablePopcorns; // on a matrix 64x64 this could consume a little less than 27kB when Bar expansion is used
  if (!SEGENV.allocateData(dataSize * strips)) return mode_static(); //allocation failed

  Spark* popcorn = reinterpret_cast<Spark*>(SEGENV.data);

  bool hasCol2 = SEGCOLOR(2);
  if (!SEGMENT.check2) SEGMENT.fill(hasCol2 ? BLACK : SEGCOLOR(1));

  struct virtualStrip {
    static void runStrip(uint16_t stripNr, Spark* popcorn, unsigned usablePopcorns) {
      float gravity = -0.0001f - (SEGMENT.speed/200000.0f); // m/s/s
      gravity *= SEGLEN;

      unsigned numPopcorn = SEGMENT.intensity * usablePopcorns / 255;
      if (numPopcorn == 0) numPopcorn = 1;

      for (unsigned i = 0; i < numPopcorn; i++) {
        if (popcorn[i].pos >= 0.0f) { // if kernel is active, update its position
          popcorn[i].pos += popcorn[i].vel;
          popcorn[i].vel += gravity;
        } else { // if kernel is inactive, randomly pop it
          if (hw_random8() < 2) { // POP!!!
            popcorn[i].pos = 0.01f;

            unsigned peakHeight = 128 + hw_random8(128); //0-255
            peakHeight = (peakHeight * (SEGLEN -1)) >> 8;
            popcorn[i].vel = sqrtf(-2.0f * gravity * peakHeight);

            if (SEGMENT.palette)
            {
              popcorn[i].colIndex = hw_random8();
            } else {
              byte col = hw_random8(0, NUM_COLORS);
              if (!SEGCOLOR(2) || !SEGCOLOR(col)) col = 0;
              popcorn[i].colIndex = col;
            }
          }
        }
        if (popcorn[i].pos >= 0.0f) { // draw now active popcorn (either active before or just popped)
          uint32_t col = SEGMENT.color_wheel(popcorn[i].colIndex);
          if (!SEGMENT.palette && popcorn[i].colIndex < NUM_COLORS) col = SEGCOLOR(popcorn[i].colIndex);
          unsigned ledIndex = popcorn[i].pos;
          if (ledIndex < SEGLEN) SEGMENT.setPixelColor(indexToVStrip(ledIndex, stripNr), col);
        }
      }
    }
  };

  for (unsigned stripNr=0; stripNr<strips; stripNr++)
    virtualStrip::runStrip(stripNr, &popcorn[stripNr * usablePopcorns], usablePopcorns);

  return FRAMETIME;
} 

uint16_t mode_candle()
{
  return candle(false);
}
static const char _data_FX_MODE_CANDLE[] PROGMEM = "Candle@!,!;!,!;!;01;sx=96,ix=224,pal=0";


uint16_t mode_candle_multi()
{
  return candle(true);
}
static const char _data_FX_MODE_CANDLE_MULTI[] PROGMEM = "Candle Multi@!,!;!,!;!;;sx=96,ix=224,pal=0";

#ifdef WLED_PS_DONT_REPLACE_FX
/*
/ Fireworks in starburst effect
/ based on the video: https://www.reddit.com/r/arduino/comments/c3sd46/i_made_this_fireworks_effect_for_my_led_strips/
/ Speed sets frequency of new starbursts, intensity is the intensity of the burst
*/
#ifdef ESP8266
  #define STARBURST_MAX_FRAG   8 //52 bytes / star
#else
  #define STARBURST_MAX_FRAG  10 //60 bytes / star
#endif
//each needs 20+STARBURST_MAX_FRAG*4 bytes
typedef struct particle {
  CRGB     color;
  uint32_t birth  =0;
  uint32_t last   =0;
  float    vel    =0;
  uint16_t pos    =-1;
  float    fragment[STARBURST_MAX_FRAG];
} star;

uint16_t mode_starburst(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned maxData = FAIR_DATA_PER_SEG; //ESP8266: 256 ESP32: 640
  unsigned segs = strip.getActiveSegmentsNum();
  if (segs <= (strip.getMaxSegments() /2)) maxData *= 2; //ESP8266: 512 if <= 8 segs ESP32: 1280 if <= 16 segs
  if (segs <= (strip.getMaxSegments() /4)) maxData *= 2; //ESP8266: 1024 if <= 4 segs ESP32: 2560 if <= 8 segs
  unsigned maxStars = maxData / sizeof(star); //ESP8266: max. 4/9/19 stars/seg, ESP32: max. 10/21/42 stars/seg

  unsigned numStars = 1 + (SEGLEN >> 3);
  if (numStars > maxStars) numStars = maxStars;
  unsigned dataSize = sizeof(star) * numStars;

  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  uint32_t it = strip.now;

  star* stars = reinterpret_cast<star*>(SEGENV.data);

  float          maxSpeed                = 375.0f;  // Max velocity
  float          particleIgnition        = 250.0f;  // How long to "flash"
  float          particleFadeTime        = 1500.0f; // Fade out time

  for (unsigned j = 0; j < numStars; j++)
  {
    // speed to adjust chance of a burst, max is nearly always.
    if (hw_random8((144-(SEGMENT.speed >> 1))) == 0 && stars[j].birth == 0)
    {
      // Pick a random color and location.
      unsigned startPos = hw_random16(SEGLEN-1);
      float multiplier = (float)(hw_random8())/255.0f * 1.0f;

      stars[j].color = CRGB(SEGMENT.color_wheel(hw_random8()));
      stars[j].pos = startPos;
      stars[j].vel = maxSpeed * (float)(hw_random8())/255.0f * multiplier;
      stars[j].birth = it;
      stars[j].last = it;
      // more fragments means larger burst effect
      int num = hw_random8(3,6 + (SEGMENT.intensity >> 5));

      for (int i=0; i < STARBURST_MAX_FRAG; i++) {
        if (i < num) stars[j].fragment[i] = startPos;
        else stars[j].fragment[i] = -1;
      }
    }
  }

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  for (unsigned j=0; j<numStars; j++)
  {
    if (stars[j].birth != 0) {
      float dt = (it-stars[j].last)/1000.0;

      for (int i=0; i < STARBURST_MAX_FRAG; i++) {
        int var = i >> 1;

        if (stars[j].fragment[i] > 0) {
          //all fragments travel right, will be mirrored on other side
          stars[j].fragment[i] += stars[j].vel * dt * (float)var/3.0;
        }
      }
      stars[j].last = it;
      stars[j].vel -= 3*stars[j].vel*dt;
    }

    CRGB c = stars[j].color;

    // If the star is brand new, it flashes white briefly.
    // Otherwise it just fades over time.
    float fade = 0.0f;
    float age = it-stars[j].birth;

    if (age < particleIgnition) {
      c = CRGB(color_blend(WHITE, RGBW32(c.r,c.g,c.b,0), uint8_t(254.5f*((age / particleIgnition)))));
    } else {
      // Figure out how much to fade and shrink the star based on
      // its age relative to its lifetime
      if (age > particleIgnition + particleFadeTime) {
        fade = 1.0f;                  // Black hole, all faded out
        stars[j].birth = 0;
        c = CRGB(SEGCOLOR(1));
      } else {
        age -= particleIgnition;
        fade = (age / particleFadeTime);  // Fading star
        c = CRGB(color_blend(RGBW32(c.r,c.g,c.b,0), SEGCOLOR(1), uint8_t(254.5f*fade)));
      }
    }

    float particleSize = (1.0f - fade) * 2.0f;

    for (size_t index=0; index < STARBURST_MAX_FRAG*2; index++) {
      bool mirrored = index & 0x1;
      unsigned i = index >> 1;
      if (stars[j].fragment[i] > 0) {
        float loc = stars[j].fragment[i];
        if (mirrored) loc -= (loc-stars[j].pos)*2;
        unsigned start = loc - particleSize;
        unsigned end = loc + particleSize;
        if (start < 0) start = 0;
        if (start == end) end++;
        if (end > SEGLEN) end = SEGLEN;
        for (unsigned p = start; p < end; p++) {
          SEGMENT.setPixelColor(p, c);
        }
      }
    }
  }
  return FRAMETIME;
}
#undef STARBURST_MAX_FRAG
static const char _data_FX_MODE_STARBURST[] PROGMEM = "Fireworks Starburst@Chance,Fragments,,,,,Overlay;,!;!;;pal=11,m12=0";
#endif // WLED_PS_DONT_REPLACE_FX

 #ifdef WLED_PS_DONT_REPLACE_FX
/*
 * Exploding fireworks effect
 * adapted from: http://www.anirama.com/1000leds/1d-fireworks/
 * adapted for 2D WLED by blazoncek (Blaz Kristan (AKA blazoncek))
 */
uint16_t mode_exploding_fireworks(void)
{
  if (SEGLEN <= 1) return mode_static();
  const int cols = SEGMENT.is2D() ? SEG_W : 1;
  const int rows = SEGMENT.is2D() ? SEG_H : SEGLEN;

  //allocate segment data
  unsigned maxData = FAIR_DATA_PER_SEG; //ESP8266: 256 ESP32: 640
  unsigned segs = strip.getActiveSegmentsNum();
  if (segs <= (strip.getMaxSegments() /2)) maxData *= 2; //ESP8266: 512 if <= 8 segs ESP32: 1280 if <= 16 segs
  if (segs <= (strip.getMaxSegments() /4)) maxData *= 2; //ESP8266: 1024 if <= 4 segs ESP32: 2560 if <= 8 segs
  int maxSparks = maxData / sizeof(spark); //ESP8266: max. 21/42/85 sparks/seg, ESP32: max. 53/106/213 sparks/seg

  unsigned numSparks = min(5 + ((rows*cols) >> 1), maxSparks);
  unsigned dataSize = sizeof(spark) * numSparks;
  if (!SEGENV.allocateData(dataSize + sizeof(float))) return mode_static(); //allocation failed
  float *dying_gravity = reinterpret_cast<float*>(SEGENV.data + dataSize);

  if (dataSize != SEGENV.aux1) { //reset to flare if sparks were reallocated (it may be good idea to reset segment if bounds change)
    *dying_gravity = 0.0f;
    SEGENV.aux0 = 0;
    SEGENV.aux1 = dataSize;
  }

  SEGMENT.fade_out(252);

  Spark* sparks = reinterpret_cast<Spark*>(SEGENV.data);
  Spark* flare = sparks; //first spark is flare data

  float gravity = -0.0004f - (SEGMENT.speed/800000.0f); // m/s/s
  gravity *= rows;

  if (SEGENV.aux0 < 2) { //FLARE
    if (SEGENV.aux0 == 0) { //init flare
      flare->pos = 0;
      flare->posX = SEGMENT.is2D() ? hw_random16(2,cols-3) : (SEGMENT.intensity > hw_random8()); // will enable random firing side on 1D
      unsigned peakHeight = 75 + hw_random8(180); //0-255
      peakHeight = (peakHeight * (rows -1)) >> 8;
      flare->vel = sqrtf(-2.0f * gravity * peakHeight);
      flare->velX = SEGMENT.is2D() ? (hw_random8(9)-4)/64.0f : 0; // no X velocity on 1D
      flare->col = 255; //brightness
      SEGENV.aux0 = 1;
    }

    // launch
    if (flare->vel > 12 * gravity) {
      // flare
      if (SEGMENT.is2D()) SEGMENT.setPixelColorXY(unsigned(flare->posX), rows - uint16_t(flare->pos) - 1, flare->col, flare->col, flare->col);
      else                SEGMENT.setPixelColor((flare->posX > 0.0f) ? rows - int(flare->pos) - 1 : int(flare->pos), flare->col, flare->col, flare->col);
      flare->pos  += flare->vel;
      flare->pos  = constrain(flare->pos, 0, rows-1);
      if (SEGMENT.is2D()) {
        flare->posX += flare->velX;
        flare->posX = constrain(flare->posX, 0, cols-1);
      }
      flare->vel  += gravity;
      flare->col  -= 2;
    } else {
      SEGENV.aux0 = 2;  // ready to explode
    }
  } else if (SEGENV.aux0 < 4) {
    /*
     * Explode!
     *
     * Explosion happens where the flare ended.
     * Size is proportional to the height.
     */
    unsigned nSparks = flare->pos + hw_random8(4);
    nSparks = std::max(nSparks, 4U);  // This is not a standard constrain; numSparks is not guaranteed to be at least 4
    nSparks = std::min(nSparks, numSparks);

    // initialize sparks
    if (SEGENV.aux0 == 2) {
      for (unsigned i = 1; i < nSparks; i++) {
        sparks[i].pos  = flare->pos;
        sparks[i].posX = flare->posX;
        sparks[i].vel  = (float(hw_random16(20001)) / 10000.0f) - 0.9f; // from -0.9 to 1.1
        sparks[i].vel *= rows<32 ? 0.5f : 1; // reduce velocity for smaller strips
        sparks[i].velX = SEGMENT.is2D() ? (float(hw_random16(20001)) / 10000.0f) - 1.0f : 0; // from -1 to 1
        sparks[i].col  = 345;//abs(sparks[i].vel * 750.0); // set colors before scaling velocity to keep them bright
        //sparks[i].col = constrain(sparks[i].col, 0, 345);
        sparks[i].colIndex = hw_random8();
        sparks[i].vel  *= flare->pos/rows; // proportional to height
        sparks[i].velX *= SEGMENT.is2D() ? flare->posX/cols : 0; // proportional to width
        sparks[i].vel  *= -gravity *50;
      }
      //sparks[1].col = 345; // this will be our known spark
      *dying_gravity = gravity/2;
      SEGENV.aux0 = 3;
    }

    if (sparks[1].col > 4) {//&& sparks[1].pos > 0) { // as long as our known spark is lit, work with all the sparks
      for (unsigned i = 1; i < nSparks; i++) {
        sparks[i].pos  += sparks[i].vel;
        sparks[i].posX += sparks[i].velX;
        sparks[i].vel  += *dying_gravity;
        sparks[i].velX += SEGMENT.is2D() ? *dying_gravity : 0;
        if (sparks[i].col > 3) sparks[i].col -= 4;

        if (sparks[i].pos > 0 && sparks[i].pos < rows) {
          if (SEGMENT.is2D() && !(sparks[i].posX >= 0 && sparks[i].posX < cols)) continue;
          unsigned prog = sparks[i].col;
          uint32_t spColor = (SEGMENT.palette) ? SEGMENT.color_wheel(sparks[i].colIndex) : SEGCOLOR(0);
          CRGBW c = BLACK; //HeatColor(sparks[i].col);
          if (prog > 300) { //fade from white to spark color
            c = color_blend(spColor, WHITE, uint8_t((prog - 300)*5));
          } else if (prog > 45) { //fade from spark color to black
            c = color_blend(BLACK, spColor, uint8_t(prog - 45));
            unsigned cooling = (300 - prog) >> 5;
            c.g = qsub8(c.g, cooling);
            c.b = qsub8(c.b, cooling * 2);
          }
          if (SEGMENT.is2D()) SEGMENT.setPixelColorXY(int(sparks[i].posX), rows - int(sparks[i].pos) - 1, c);
          else                SEGMENT.setPixelColor(int(sparks[i].posX) ? rows - int(sparks[i].pos) - 1 : int(sparks[i].pos), c);
        }
      }
      if (SEGMENT.check3) SEGMENT.blur(16);
      *dying_gravity *= .8f; // as sparks burn out they fall slower
    } else {
      SEGENV.aux0 = 6 + hw_random8(10); //wait for this many frames
    }
  } else {
    SEGENV.aux0--;
    if (SEGENV.aux0 < 4) {
      SEGENV.aux0 = 0; //back to flare
    }
  }

  return FRAMETIME;
}
#undef MAX_SPARKS
static const char _data_FX_MODE_EXPLODING_FIREWORKS[] PROGMEM = "Fireworks 1D@Gravity,Firing side;!,!;!;12;pal=11,ix=128";
#endif // WLED_PS_DONT_REPLACE_FX

/*
 * Drip Effect
 * ported of: https://www.youtube.com/watch?v=sru2fXh4r7k
 */
uint16_t mode_drip(void)
{
  if (SEGLEN <= 1) return mode_static();
  //allocate segment data
  unsigned strips = SEGMENT.nrOfVStrips();
  const int maxNumDrops = 4;
  unsigned dataSize = sizeof(spark) * maxNumDrops;
  if (!SEGENV.allocateData(dataSize * strips)) return mode_static(); //allocation failed
  Spark* drops = reinterpret_cast<Spark*>(SEGENV.data);

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  struct virtualStrip {
    static void runStrip(uint16_t stripNr, Spark* drops) {

      unsigned numDrops = 1 + (SEGMENT.intensity >> 6); // 255>>6 = 3

      float gravity = -0.0005f - (SEGMENT.speed/50000.0f);
      gravity *= max(1, (int)SEGLEN-1);
      int sourcedrop = 12;

      for (unsigned j=0;j<numDrops;j++) {
        if (drops[j].colIndex == 0) { //init
          drops[j].pos = SEGLEN-1;    // start at end
          drops[j].vel = 0;           // speed
          drops[j].col = sourcedrop;  // brightness
          drops[j].colIndex = 1;      // drop state (0 init, 1 forming, 2 falling, 5 bouncing)
        }

        SEGMENT.setPixelColor(indexToVStrip(SEGLEN-1, stripNr), color_blend(BLACK,SEGCOLOR(0), uint8_t(sourcedrop)));// water source
        if (drops[j].colIndex==1) {
          if (drops[j].col>255) drops[j].col=255;
          SEGMENT.setPixelColor(indexToVStrip(uint16_t(drops[j].pos), stripNr), color_blend(BLACK,SEGCOLOR(0),uint8_t(drops[j].col)));

          drops[j].col += map(SEGMENT.speed, 0, 255, 1, 6); // swelling

          if (hw_random8() < drops[j].col/10) {               // random drop
            drops[j].colIndex=2;               //fall
            drops[j].col=255;
          }
        }
        if (drops[j].colIndex > 1) {           // falling
          if (drops[j].pos > 0) {              // fall until end of segment
            drops[j].pos += drops[j].vel;
            if (drops[j].pos < 0) drops[j].pos = 0;
            drops[j].vel += gravity;           // gravity is negative

            for (int i=1;i<7-drops[j].colIndex;i++) { // some minor math so we don't expand bouncing droplets
              unsigned pos = constrain(unsigned(drops[j].pos) +i, 0, SEGLEN-1); //this is BAD, returns a pos >= SEGLEN occasionally
              SEGMENT.setPixelColor(indexToVStrip(pos, stripNr), color_blend(BLACK,SEGCOLOR(0),uint8_t(drops[j].col/i))); //spread pixel with fade while falling
            }

            if (drops[j].colIndex > 2) {       // during bounce, some water is on the floor
              SEGMENT.setPixelColor(indexToVStrip(0, stripNr), color_blend(SEGCOLOR(0),BLACK,uint8_t(drops[j].col)));
            }
          } else {                             // we hit bottom
            if (drops[j].colIndex > 2) {       // already hit once, so back to forming
              drops[j].colIndex = 0;
              drops[j].col = sourcedrop;

            } else {

              if (drops[j].colIndex==2) {      // init bounce
                drops[j].vel = -drops[j].vel/4;// reverse velocity with damping
                drops[j].pos += drops[j].vel;
              }
              drops[j].col = sourcedrop*2;
              drops[j].colIndex = 5;           // bouncing
            }
          }
        }
      }
    }
  };

  for (unsigned stripNr=0; stripNr<strips; stripNr++)
    virtualStrip::runStrip(stripNr, &drops[stripNr*maxNumDrops]);

  return FRAMETIME;
}
static const char _data_FX_MODE_DRIP[] PROGMEM = "Drip@Gravity,# of drips,,,,,Overlay;!,!;!;;m12=1"; //bar
 

/*
 * Effects by Andrew Tuline
 */
static uint16_t phased_base(uint8_t moder) {                  // We're making sine waves here. By Andrew Tuline.

  unsigned allfreq = 16;                                          // Base frequency.
  float *phase = reinterpret_cast<float*>(&SEGENV.step);         // Phase change value gets calculated (float fits into unsigned long).
  unsigned cutOff = (255-SEGMENT.intensity);                      // You can change the number of pixels.  AKA INTENSITY (was 192).
  unsigned modVal = 5;//SEGMENT.fft1/8+1;                         // You can change the modulus. AKA FFT1 (was 5).

  unsigned index = strip.now/64;                                  // Set color rotation speed
  *phase += SEGMENT.speed/32.0;                                  // You can change the speed of the wave. AKA SPEED (was .4)

  for (unsigned i = 0; i < SEGLEN; i++) {
    if (moder == 1) modVal = (perlin8(i*10 + i*10) /16);         // Let's randomize our mod length with some Perlin noise.
    unsigned val = (i+1) * allfreq;                              // This sets the frequency of the waves. The +1 makes sure that led 0 is used.
    if (modVal == 0) modVal = 1;
    val += *phase * (i % modVal +1) /2;                          // This sets the varying phase change of the waves. By Andrew Tuline.
    unsigned b = cubicwave8(val);                                 // Now we make an 8 bit sinewave.
    b = (b > cutOff) ? (b - cutOff) : 0;                         // A ternary operator to cutoff the light.
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(index, false, false, 0), uint8_t(b)));
    index += 256 / SEGLEN;
    if (SEGLEN > 256) index ++;                                  // Correction for segments longer than 256 LEDs
  }

  return FRAMETIME;
}


uint16_t mode_phased(void) {
  return phased_base(0);
}
static const char _data_FX_MODE_PHASED[] PROGMEM = "Phased@!,!;!,!;!";


uint16_t mode_phased_noise(void) {
  return phased_base(1);
}
static const char _data_FX_MODE_PHASEDNOISE[] PROGMEM = "Phased Noise@!,!;!,!;!";


uint16_t mode_twinkleup(void) {                 // A very short twinkle routine with fade-in and dual controls. By Andrew Tuline.
  unsigned prevSeed = random16_get_seed();      // save seed so we can restore it at the end of the function
  random16_set_seed(535);                       // The randomizer needs to be re-set each time through the loop in order for the same 'random' numbers to be the same each time through.

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned ranstart = random8();               // The starting value (aka brightness) for each pixel. Must be consistent each time through the loop for this to work.
    unsigned pixBri = sin8_t(ranstart + 16 * strip.now/(256-SEGMENT.speed));
    if (random8() > SEGMENT.intensity) pixBri = 0;
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(random8()+strip.now/100, false, PALETTE_SOLID_WRAP, 0), pixBri));
  }

  random16_set_seed(prevSeed); // restore original seed so other effects can use "random" PRNG
  return FRAMETIME;
}
static const char _data_FX_MODE_TWINKLEUP[] PROGMEM = "Twinkleup@!,Intensity;!,!;!;;m12=0";


// Peaceful noise that's slow and with gradually changing palettes. Does not support WLED palettes or default colours or controls.
uint16_t mode_noisepal(void) {                                    // Slow noise palette by Andrew Tuline.
  unsigned scale = 15 + (SEGMENT.intensity >> 2); //default was 30
  //#define scale 30

  unsigned dataSize = sizeof(CRGBPalette16) * 2; //allocate space for 2 Palettes (2 * 16 * 3 = 96 bytes)
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  CRGBPalette16* palettes = reinterpret_cast<CRGBPalette16*>(SEGENV.data);

  unsigned changePaletteMs = 4000 + SEGMENT.speed *10; //between 4 - 6.5sec
  if (strip.now - SEGENV.step > changePaletteMs)
  {
    SEGENV.step = strip.now;

    unsigned baseI = hw_random8();
    palettes[1] = CRGBPalette16(CHSV(baseI+hw_random8(64), 255, hw_random8(128,255)), CHSV(baseI+128, 255, hw_random8(128,255)), CHSV(baseI+hw_random8(92), 192, hw_random8(128,255)), CHSV(baseI+hw_random8(92), 255, hw_random8(128,255)));
  }

  //EVERY_N_MILLIS(10) { //(don't have to time this, effect function is only called every 24ms)
  nblendPaletteTowardPalette(palettes[0], palettes[1], 48);               // Blend towards the target palette over 48 iterations.

  if (SEGMENT.palette > 0) palettes[0] = SEGPALETTE;

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned index = perlin8(i*scale, SEGENV.aux0+i*scale);                // Get a value from the noise function. I'm using both x and y axis.
    SEGMENT.setPixelColor(i,  ColorFromPalette(palettes[0], index, 255, LINEARBLEND));  // Use my own palette.
  }

  SEGENV.aux0 += beatsin8_t(10,1,4);                                        // Moving along the distance. Vary it a bit with a sine wave.

  return FRAMETIME;
}
static const char _data_FX_MODE_NOISEPAL[] PROGMEM = "Noise Pal@!,Scale;;!";


// Sine waves that have controllable phase change speed, frequency and cutoff. By Andrew Tuline.
// SEGMENT.speed ->Speed, SEGMENT.intensity -> Frequency (SEGMENT.fft1 -> Color change, SEGMENT.fft2 -> PWM cutoff)
//
uint16_t mode_sinewave(void) {             // Adjustable sinewave. By Andrew Tuline
  //#define qsuba(x, b)  ((x>b)?x-b:0)               // Analog Unsigned subtraction macro. if result <0, then => 0

  unsigned colorIndex = strip.now /32;//(256 - SEGMENT.fft1);  // Amount of colour change.

  SEGENV.step += SEGMENT.speed/16;                   // Speed of animation.
  unsigned freq = SEGMENT.intensity/4;//SEGMENT.fft2/8;                       // Frequency of the signal.

  for (unsigned i = 0; i < SEGLEN; i++) {                 // For each of the LED's in the strand, set a brightness based on a wave as follows:
    uint8_t pixBri = cubicwave8((i*freq)+SEGENV.step);//qsuba(cubicwave8((i*freq)+SEGENV.step), (255-SEGMENT.intensity)); // qsub sets a minimum value called thiscutoff. If < thiscutoff, then bright = 0. Otherwise, bright = 128 (as defined in qsub)..
    //setPixCol(i, i*colorIndex/255, pixBri);
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i*colorIndex/255, false, PALETTE_SOLID_WRAP, 0), pixBri));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_SINEWAVE[] PROGMEM = "Sine@!,Scale;;!";


/*
 * Best of both worlds from Palette and Spot effects. By Aircoookie
 */
uint16_t mode_flow(void)
{
  unsigned counter = 0;
  if (SEGMENT.speed != 0)
  {
    counter = strip.now * ((SEGMENT.speed >> 2) +1);
    counter = counter >> 8;
  }

  unsigned maxZones = SEGLEN / 6; //only looks good if each zone has at least 6 LEDs
  unsigned zones = (SEGMENT.intensity * maxZones) >> 8;
  if (zones & 0x01) zones++; //zones must be even
  if (zones < 2) zones = 2;
  unsigned zoneLen = SEGLEN / zones;
  unsigned offset = (SEGLEN - zones * zoneLen) >> 1;

  SEGMENT.fill(SEGMENT.color_from_palette(-counter, false, true, 255));

  for (unsigned z = 0; z < zones; z++)
  {
    unsigned pos = offset + z * zoneLen;
    for (unsigned i = 0; i < zoneLen; i++)
    {
      unsigned colorIndex = (i * 255 / zoneLen) - counter;
      unsigned led = (z & 0x01) ? i : (zoneLen -1) -i;
      if (SEGMENT.reverse) led = (zoneLen -1) -led;
      SEGMENT.setPixelColor(pos + led, SEGMENT.color_from_palette(colorIndex, false, true, 255));
    }
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_FLOW[] PROGMEM = "Flow@!,Zones;;!;;m12=1"; //vertical


/*
 * Dots waving around in a sine/pendulum motion.
 * Little pixel birds flying in a circle. By Aircoookie
 */
uint16_t mode_chunchun(void)
{
  if (SEGLEN <= 1) return mode_static();
  SEGMENT.fade_out(254); // add a bit of trail
  unsigned counter = strip.now * (6 + (SEGMENT.speed >> 4));
  unsigned numBirds = 2 + (SEGLEN >> 3);  // 2 + 1/8 of a segment
  unsigned span = (SEGMENT.intensity << 8) / numBirds;

  for (unsigned i = 0; i < numBirds; i++)
  {
    counter -= span;
    unsigned megumin = sin16_t(counter) + 0x8000;
    unsigned bird = uint32_t(megumin * SEGLEN) >> 16;
    bird = constrain(bird, 0U, SEGLEN-1U);
    SEGMENT.setPixelColor(bird, SEGMENT.color_from_palette((i * 255)/ numBirds, false, false, 0)); // no palette wrapping
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_CHUNCHUN[] PROGMEM = "Chunchun@!,Gap size;!,!;!";

#define SPOT_TYPE_SOLID       0
#define SPOT_TYPE_GRADIENT    1
#define SPOT_TYPE_2X_GRADIENT 2
#define SPOT_TYPE_2X_DOT      3
#define SPOT_TYPE_3X_DOT      4
#define SPOT_TYPE_4X_DOT      5
#define SPOT_TYPES_COUNT      6
#ifdef ESP8266
  #define SPOT_MAX_COUNT 17          //Number of simultaneous waves
#else
  #define SPOT_MAX_COUNT 49          //Number of simultaneous waves
#endif

#ifdef WLED_PS_DONT_REPLACE_FX
//13 bytes
typedef struct Spotlight {
  float speed;
  uint8_t colorIdx;
  int16_t position;
  unsigned long lastUpdateTime;
  uint8_t width;
  uint8_t type;
} spotlight;

/*
 * Spotlights moving back and forth that cast dancing shadows.
 * Shine this through tree branches/leaves or other close-up objects that cast
 * interesting shadows onto a ceiling or tarp.
 *
 * By Steve Pomeroy @xxv
 */
uint16_t mode_dancing_shadows(void)
{
  if (SEGLEN <= 1) return mode_static();
  unsigned numSpotlights = map(SEGMENT.intensity, 0, 255, 2, SPOT_MAX_COUNT);  // 49 on 32 segment ESP32, 17 on 16 segment ESP8266
  bool initialize = SEGENV.aux0 != numSpotlights;
  SEGENV.aux0 = numSpotlights;

  unsigned dataSize = sizeof(spotlight) * numSpotlights;
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed
  Spotlight* spotlights = reinterpret_cast<Spotlight*>(SEGENV.data);

  SEGMENT.fill(BLACK);

  unsigned long time = strip.now;
  bool respawn = false;

  for (size_t i = 0; i < numSpotlights; i++) {
    if (!initialize) {
      // advance the position of the spotlight
      int delta = (float)(time - spotlights[i].lastUpdateTime) *
                  (spotlights[i].speed * ((1.0 + SEGMENT.speed)/100.0));

      if (abs(delta) >= 1) {
        spotlights[i].position += delta;
        spotlights[i].lastUpdateTime = time;
      }

      respawn = (spotlights[i].speed > 0.0 && spotlights[i].position > (int)(SEGLEN + 2))
             || (spotlights[i].speed < 0.0 && spotlights[i].position < -(spotlights[i].width + 2));
    }

    if (initialize || respawn) {
      spotlights[i].colorIdx = hw_random8();
      spotlights[i].width = hw_random8(1, 10);

      spotlights[i].speed = 1.0/hw_random8(4, 50);

      if (initialize) {
        spotlights[i].position = hw_random16(SEGLEN);
        spotlights[i].speed *= hw_random8(2) ? 1.0 : -1.0;
      } else {
        if (hw_random8(2)) {
          spotlights[i].position = SEGLEN + spotlights[i].width;
          spotlights[i].speed *= -1.0;
        }else {
          spotlights[i].position = -spotlights[i].width;
        }
      }

      spotlights[i].lastUpdateTime = time;
      spotlights[i].type = hw_random8(SPOT_TYPES_COUNT);
    }

    uint32_t color = SEGMENT.color_from_palette(spotlights[i].colorIdx, false, false, 255);
    int start = spotlights[i].position;

    if (spotlights[i].width <= 1) {
      if (start >= 0 && start < (int)SEGLEN) {
        SEGMENT.blendPixelColor(start, color, 128);
      }
    } else {
      switch (spotlights[i].type) {
        case SPOT_TYPE_SOLID:
          for (size_t j = 0; j < spotlights[i].width; j++) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, 128);
            }
          }
        break;

        case SPOT_TYPE_GRADIENT:
          for (size_t j = 0; j < spotlights[i].width; j++) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, cubicwave8(map(j, 0, spotlights[i].width - 1, 0, 255)));
            }
          }
        break;

        case SPOT_TYPE_2X_GRADIENT:
          for (size_t j = 0; j < spotlights[i].width; j++) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, cubicwave8(2 * map(j, 0, spotlights[i].width - 1, 0, 255)));
            }
          }
        break;

        case SPOT_TYPE_2X_DOT:
          for (size_t j = 0; j < spotlights[i].width; j += 2) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, 128);
            }
          }
        break;

        case SPOT_TYPE_3X_DOT:
          for (size_t j = 0; j < spotlights[i].width; j += 3) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, 128);
            }
          }
        break;

        case SPOT_TYPE_4X_DOT:
          for (size_t j = 0; j < spotlights[i].width; j += 4) {
            if ((start + j) >= 0 && (start + j) < SEGLEN) {
              SEGMENT.blendPixelColor(start + j, color, 128);
            }
          }
        break;
      }
    }
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_DANCING_SHADOWS[] PROGMEM = "Dancing Shadows@!,# of shadows;!;!";
#endif // WLED_PS_DONT_REPLACE_FX

 

/*
  Image effect
  Draws a .gif image from filesystem on the matrix/strip
*/
uint16_t mode_image(void) {
  #ifndef WLED_ENABLE_GIF
  return mode_static();
  #else
  renderImageToSegment(SEGMENT);
  return FRAMETIME;
  #endif
  // if (status != 0 && status != 254 && status != 255) {
  //   Serial.print("GIF renderer return: ");
  //   Serial.println(status);
  // }
}
static const char _data_FX_MODE_IMAGE[] PROGMEM = "Image@!,;;;12;sx=128";

/*
  Blends random colors across palette
  Modified, originally by Mark Kriegsman https://gist.github.com/kriegsman/1f7ccbbfa492a73c015e
*/
uint16_t mode_blends(void) {
  unsigned pixelLen = SEGLEN > UINT8_MAX ? UINT8_MAX : SEGLEN;
  unsigned dataSize = sizeof(uint32_t) * (pixelLen + 1);  // max segment length of 56 pixels on 16 segment ESP8266
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed
  uint32_t* pixels = reinterpret_cast<uint32_t*>(SEGENV.data);
  uint8_t blendSpeed = map(SEGMENT.intensity, 0, UINT8_MAX, 10, 128);
  unsigned shift = (strip.now * ((SEGMENT.speed >> 3) +1)) >> 8;

  for (unsigned i = 0; i < pixelLen; i++) {
    pixels[i] = color_blend(pixels[i], SEGMENT.color_from_palette(shift + quadwave8((i + 1) * 16), false, PALETTE_SOLID_WRAP, 255), blendSpeed);
    shift += 3;
  }

  unsigned offset = 0;
  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, pixels[offset++]);
    if (offset >= pixelLen) offset = 0;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_BLENDS[] PROGMEM = "Blends@Shift speed,Blend speed;;!";


/*
  TV Simulator
  Modified and adapted to WLED by Def3nder, based on "Fake TV Light for Engineers" by Phillip Burgess https://learn.adafruit.com/fake-tv-light-for-engineers/arduino-sketch
*/
//43 bytes
typedef struct TvSim {
  uint32_t totalTime = 0;
  uint32_t fadeTime  = 0;
  uint32_t startTime = 0;
  uint32_t elapsed   = 0;
  uint32_t pixelNum  = 0;
  uint16_t sliderValues = 0;
  uint32_t sceeneStart    = 0;
  uint32_t sceeneDuration = 0;
  uint16_t sceeneColorHue = 0;
  uint8_t  sceeneColorSat = 0;
  uint8_t  sceeneColorBri = 0;
  uint8_t  actualColorR = 0;
  uint8_t  actualColorG = 0;
  uint8_t  actualColorB = 0;
  uint16_t pr = 0; // Prev R, G, B
  uint16_t pg = 0;
  uint16_t pb = 0;
} tvSim;

uint16_t mode_tv_simulator(void) {
  int nr, ng, nb, r, g, b, i, hue;
  uint8_t  sat, bri, j;

  if (!SEGENV.allocateData(sizeof(tvSim))) return mode_static(); //allocation failed
  TvSim* tvSimulator = reinterpret_cast<TvSim*>(SEGENV.data);

  uint8_t colorSpeed     = map(SEGMENT.speed,     0, UINT8_MAX,  1, 20);
  uint8_t colorIntensity = map(SEGMENT.intensity, 0, UINT8_MAX, 10, 30);

  i = SEGMENT.speed << 8 | SEGMENT.intensity;
  if (i != tvSimulator->sliderValues) {
    tvSimulator->sliderValues = i;
    SEGENV.aux1 = 0;
  }

    // create a new sceene
    if (((strip.now - tvSimulator->sceeneStart) >= tvSimulator->sceeneDuration) || SEGENV.aux1 == 0) {
      tvSimulator->sceeneStart    = strip.now;                                               // remember the start of the new sceene
      tvSimulator->sceeneDuration = hw_random16(60* 250* colorSpeed, 60* 750 * colorSpeed);    // duration of a "movie sceene" which has similar colors (5 to 15 minutes with max speed slider)
      tvSimulator->sceeneColorHue = hw_random16(   0, 768);                                    // random start color-tone for the sceene
      tvSimulator->sceeneColorSat = hw_random8 ( 100, 130 + colorIntensity);                   // random start color-saturation for the sceene
      tvSimulator->sceeneColorBri = hw_random8 ( 200, 240);                                    // random start color-brightness for the sceene
      SEGENV.aux1 = 1;
      SEGENV.aux0 = 0;
    }

    // slightly change the color-tone in this sceene
    if (SEGENV.aux0 == 0) {
      // hue change in both directions
      j = hw_random8(4 * colorIntensity);
      hue = (hw_random8() < 128) ? ((j < tvSimulator->sceeneColorHue)       ? tvSimulator->sceeneColorHue - j : 767 - tvSimulator->sceeneColorHue - j) :  // negative
                                ((j + tvSimulator->sceeneColorHue) < 767 ? tvSimulator->sceeneColorHue + j : tvSimulator->sceeneColorHue + j - 767) ;  // positive

      // saturation
      j = hw_random8(2 * colorIntensity);
      sat = (tvSimulator->sceeneColorSat - j) < 0 ? 0 : tvSimulator->sceeneColorSat - j;

      // brightness
      j = hw_random8(100);
      bri = (tvSimulator->sceeneColorBri - j) < 0 ? 0 : tvSimulator->sceeneColorBri - j;

      // calculate R,G,B from HSV
      // Source: https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
      { // just to create a local scope for  the variables
        uint8_t temp[5], n = (hue >> 8) % 3;
        uint8_t x = ((((hue & 255) * sat) >> 8) * bri) >> 8;
        uint8_t s = (  (256 - sat) * bri) >> 8;
        temp[0] = temp[3] =       s;
        temp[1] = temp[4] =   x + s;
        temp[2] =           bri - x;
        tvSimulator->actualColorR = temp[n + 2];
        tvSimulator->actualColorG = temp[n + 1];
        tvSimulator->actualColorB = temp[n    ];
      }
    }
    // Apply gamma correction, further expand to 16/16/16
    nr = (uint8_t)gamma8(tvSimulator->actualColorR) * 257; // New R/G/B
    ng = (uint8_t)gamma8(tvSimulator->actualColorG) * 257;
    nb = (uint8_t)gamma8(tvSimulator->actualColorB) * 257;

  if (SEGENV.aux0 == 0) {  // initialize next iteration
    SEGENV.aux0 = 1;

    // randomize total duration and fade duration for the actual color
    tvSimulator->totalTime = hw_random16(250, 2500);                   // Semi-random pixel-to-pixel time
    tvSimulator->fadeTime  = hw_random16(0, tvSimulator->totalTime);   // Pixel-to-pixel transition time
    if (hw_random8(10) < 3) tvSimulator->fadeTime = 0;                 // Force scene cut 30% of time

    tvSimulator->startTime = strip.now;
  } // end of initialization

  // how much time is elapsed ?
  tvSimulator->elapsed = strip.now - tvSimulator->startTime;

  // fade from prev color to next color
  if (tvSimulator->elapsed < tvSimulator->fadeTime) {
    r = map(tvSimulator->elapsed, 0, tvSimulator->fadeTime, tvSimulator->pr, nr);
    g = map(tvSimulator->elapsed, 0, tvSimulator->fadeTime, tvSimulator->pg, ng);
    b = map(tvSimulator->elapsed, 0, tvSimulator->fadeTime, tvSimulator->pb, nb);
  } else { // Avoid divide-by-zero in map()
    r = nr;
    g = ng;
    b = nb;
  }

  // set strip color
  for (i = 0; i < (int)SEGLEN; i++) {
    SEGMENT.setPixelColor(i, r >> 8, g >> 8, b >> 8);  // Quantize to 8-bit
  }

  // if total duration has passed, remember last color and restart the loop
  if ( tvSimulator->elapsed >= tvSimulator->totalTime) {
    tvSimulator->pr = nr; // Prev RGB = new RGB
    tvSimulator->pg = ng;
    tvSimulator->pb = nb;
    SEGENV.aux0 = 0;
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TV_SIMULATOR[] PROGMEM = "TV Simulator@!,!;;!;01";
 
 
/////////////////////////
//     Waveins         //
/////////////////////////
// Uses beatsin8() + phase shifting. By: Andrew Tuline
uint16_t mode_wavesins(void) {

  for (unsigned i = 0; i < SEGLEN; i++) {
    uint8_t bri = sin8_t(strip.now/4 + i * SEGMENT.intensity);
    uint8_t index = beatsin8_t(SEGMENT.speed, SEGMENT.custom1, SEGMENT.custom1+SEGMENT.custom2, 0, i * (SEGMENT.custom3<<3)); // custom3 is reduced resolution slider
    //SEGMENT.setPixelColor(i, ColorFromPalette(SEGPALETTE, index, bri, LINEARBLEND));
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0, bri));
  }

  return FRAMETIME;
} // mode_waveins()
static const char _data_FX_MODE_WAVESINS[] PROGMEM = "Wavesins@!,Brightness variation,Starting color,Range of colors,Color variation;!;!";


//////////////////////////////
//     Flow Stripe          //
//////////////////////////////
// By: ldirko  https://editor.soulmatelights.com/gallery/392-flow-led-stripe , modifed by: Andrew Tuline, fixed by @DedeHai
uint16_t mode_FlowStripe(void) {
  if (SEGLEN <= 1) return mode_static();
  const int hl = SEGLEN * 10 / 13;
  uint8_t hue = strip.now / (SEGMENT.speed+1);
  uint32_t t = strip.now / (SEGMENT.intensity/8+1);

  for (unsigned i = 0; i < SEGLEN; i++) {
    int c = ((abs((int)i - hl) * 127) / hl);
    c = sin8_t(c);
    c = sin8_t(c / 2 + t);
    byte b = sin8_t(c + t/8);
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(b + hue, false, true, 3));
  }

  return FRAMETIME;
} // mode_FlowStripe()
static const char _data_FX_MODE_FLOWSTRIPE[] PROGMEM = "Flow Stripe@Hue speed,Effect speed;;!;pal=11";