// WLED-SR effects

/////////////////////////
//     Perlin Move     //
/////////////////////////
// 16 bit perlinmove. Use Perlin Noise instead of sinewaves for movement. By Andrew Tuline.
// Controls are speed, # of pixels, faderate.
uint16_t mode_perlinmove(void) {
  if (SEGLEN <= 1) return mode_static();
  SEGMENT.fade_out(255-SEGMENT.custom1);
  for (int i = 0; i < SEGMENT.intensity/16 + 1; i++) {
    unsigned locn = perlin16(strip.now*128/(260-SEGMENT.speed)+i*15000, strip.now*128/(260-SEGMENT.speed)); // Get a new pixel location from moving noise.
    unsigned pixloc = map(locn, 50*256, 192*256, 0, SEGLEN-1);                                            // Map that to the length of the strand, and ensure we don't go over.
    SEGMENT.setPixelColor(pixloc, SEGMENT.color_from_palette(pixloc%255, false, PALETTE_SOLID_WRAP, 0));
  }

  return FRAMETIME;
} // mode_perlinmove()
static const char _data_FX_MODE_PERLINMOVE[] PROGMEM = "Perlin Move@!,# of pixels,Fade rate;!,!;!";



/*
  Aurora effect
*/

//CONFIG
#ifdef ESP8266
  #define W_MAX_COUNT  9          //Number of simultaneous waves
#else
  #define W_MAX_COUNT 20          //Number of simultaneous waves
#endif
#define W_MAX_SPEED 6             //Higher number, higher speed
#define W_WIDTH_FACTOR 6          //Higher number, smaller waves

//24 bytes
class AuroraWave {
  private:
    uint16_t ttl;
    CRGB basecolor;
    float basealpha;
    uint16_t age;
    uint16_t width;
    float center;
    bool goingleft;
    float speed_factor;
    bool alive = true;

  public:
    void init(uint32_t segment_length, CRGB color) {
      ttl = hw_random16(500, 1501);
      basecolor = color;
      basealpha = hw_random8(60, 101) / (float)100;
      age = 0;
      width = hw_random16(segment_length / 20, segment_length / W_WIDTH_FACTOR); //half of width to make math easier
      if (!width) width = 1;
      center = hw_random8(101) / (float)100 * segment_length;
      goingleft = hw_random8(0, 2) == 0;
      speed_factor = (hw_random8(10, 31) / (float)100 * W_MAX_SPEED / 255);
      alive = true;
    }

    CRGB getColorForLED(int ledIndex) {
      if(ledIndex < center - width || ledIndex > center + width) return 0; //Position out of range of this wave

      CRGB rgb;

      //Offset of this led from center of wave
      //The further away from the center, the dimmer the LED
      float offset = ledIndex - center;
      if (offset < 0) offset = -offset;
      float offsetFactor = offset / width;

      //The age of the wave determines it brightness.
      //At half its maximum age it will be the brightest.
      float ageFactor = 0.1;
      if((float)age / ttl < 0.5) {
        ageFactor = (float)age / (ttl / 2);
      } else {
        ageFactor = (float)(ttl - age) / ((float)ttl * 0.5);
      }

      //Calculate color based on above factors and basealpha value
      float factor = (1 - offsetFactor) * ageFactor * basealpha;
      rgb.r = basecolor.r * factor;
      rgb.g = basecolor.g * factor;
      rgb.b = basecolor.b * factor;

      return rgb;
    };

    //Change position and age of wave
    //Determine if its sill "alive"
    void update(uint32_t segment_length, uint32_t speed) {
      if(goingleft) {
        center -= speed_factor * speed;
      } else {
        center += speed_factor * speed;
      }

      age++;

      if(age > ttl) {
        alive = false;
      } else {
        if(goingleft) {
          if(center + width < 0) {
            alive = false;
          }
        } else {
          if(center - width > segment_length) {
            alive = false;
          }
        }
      }
    };

    bool stillAlive() {
      return alive;
    };
};

uint16_t mode_aurora(void) {
  //aux1 = Wavecount
  //aux2 = Intensity in last loop

  AuroraWave* waves;

//TODO: I am not sure this is a correct way of handling memory allocation since if it fails on 1st run
// it will display static effect but on second run it may crash ESP since data will be nullptr

  if(SEGENV.aux0 != SEGMENT.intensity || SEGENV.call == 0) {
    //Intensity slider changed or first call
    SEGENV.aux1 = map(SEGMENT.intensity, 0, 255, 2, W_MAX_COUNT);
    SEGENV.aux0 = SEGMENT.intensity;

    if(!SEGENV.allocateData(sizeof(AuroraWave) * SEGENV.aux1)) { // 26 on 32 segment ESP32, 9 on 16 segment ESP8266
      return mode_static(); //allocation failed
    }

    waves = reinterpret_cast<AuroraWave*>(SEGENV.data);

    for (int i = 0; i < SEGENV.aux1; i++) {
      waves[i].init(SEGLEN, CRGB(SEGMENT.color_from_palette(hw_random8(), false, false, hw_random8(0, 3))));
    }
  } else {
    waves = reinterpret_cast<AuroraWave*>(SEGENV.data);
  }

  for (int i = 0; i < SEGENV.aux1; i++) {
    //Update values of wave
    waves[i].update(SEGLEN, SEGMENT.speed);

    if(!(waves[i].stillAlive())) {
      //If a wave dies, reinitialize it starts over.
      waves[i].init(SEGLEN, CRGB(SEGMENT.color_from_palette(hw_random8(), false, false, hw_random8(0, 3))));
    }
  }

  uint8_t backlight = 1; //dimmer backlight if less active colors
  if (SEGCOLOR(0)) backlight++;
  if (SEGCOLOR(1)) backlight++;
  if (SEGCOLOR(2)) backlight++;
  //Loop through LEDs to determine color
  for (unsigned i = 0; i < SEGLEN; i++) {
    CRGB mixedRgb = CRGB(backlight, backlight, backlight);

    //For each LED we must check each wave if it is "active" at this position.
    //If there are multiple waves active on a LED we multiply their values.
    for (int  j = 0; j < SEGENV.aux1; j++) {
      CRGB rgb = waves[j].getColorForLED(i);

      if(rgb != CRGB(0)) {
        mixedRgb += rgb;
      }
    }

    SEGMENT.setPixelColor(i, mixedRgb[0], mixedRgb[1], mixedRgb[2]);
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_AURORA[] PROGMEM = "Aurora@!,!;1,2,3;!;;sx=24,pal=50";


/*
  Imitates a washing machine, rotating same waves forward, then pause, then backward.
  By Stefan Seegel
*/
uint16_t mode_washing_machine(void) {
  int speed = tristate_square8(strip.now >> 7, 90, 15);

  SEGENV.step += (speed * 2048) / (512 - SEGMENT.speed);

  for (unsigned i = 0; i < SEGLEN; i++) {
    uint8_t col = sin8_t(((SEGMENT.intensity / 25 + 1) * 255 * i / SEGLEN) + (SEGENV.step >> 7));
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(col, false, PALETTE_SOLID_WRAP, 3));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_WASHING_MACHINE[] PROGMEM = "Washing Machine@!,!;;!";


/*
 * Mode simulates a gradual sunrise
 */
uint16_t mode_sunrise() {
  if (SEGLEN <= 1) return mode_static();
  //speed 0 - static sun
  //speed 1 - 60: sunrise time in minutes
  //speed 60 - 120 : sunset time in minutes - 60;
  //speed above: "breathing" rise and set
  if (SEGENV.call == 0 || SEGMENT.speed != SEGENV.aux0) {
    SEGENV.step = millis(); //save starting time, millis() because strip.now can change from sync
    SEGENV.aux0 = SEGMENT.speed;
  }

  SEGMENT.fill(BLACK);
  unsigned stage = 0xFFFF;

  uint32_t s10SinceStart = (millis() - SEGENV.step) /100; //tenths of seconds

  if (SEGMENT.speed > 120) { //quick sunrise and sunset
    unsigned counter = (strip.now >> 1) * (((SEGMENT.speed -120) >> 1) +1);
    stage = triwave16(counter);
  } else if (SEGMENT.speed) { //sunrise
    unsigned durMins = SEGMENT.speed;
    if (durMins > 60) durMins -= 60;
    uint32_t s10Target = durMins * 600;
    if (s10SinceStart > s10Target) s10SinceStart = s10Target;
    stage = map(s10SinceStart, 0, s10Target, 0, 0xFFFF);
    if (SEGMENT.speed > 60) stage = 0xFFFF - stage; //sunset
  }

  for (unsigned i = 0; i <= SEGLEN/2; i++)
  {
    //default palette is Fire    
    unsigned wave = triwave16((i * stage) / SEGLEN);
    wave = (wave >> 8) + ((wave * SEGMENT.intensity) >> 15);
    uint32_t c;
    if (wave > 240) { //clipped, full white sun
      c = SEGMENT.color_from_palette( 240, false, true, 255);
    } else { //transition
      c = SEGMENT.color_from_palette(wave, false, true, 255);
    }
    SEGMENT.setPixelColor(i, c);
    SEGMENT.setPixelColor(SEGLEN - i - 1, c);
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_SUNRISE[] PROGMEM = "Sunrise@Time [min],Width;;!;;pal=35,sx=60";



//  "Pacifica"
//  Gentle, blue-green ocean waves.
//  December 2019, Mark Kriegsman and Mary Corey March.
//  For Dan.
//
//
// In this animation, there are four "layers" of waves of light.
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then
// another filter is applied that adds "whitecaps" of brightness where the
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
// Modified for WLED, based on https://github.com/FastLED/FastLED/blob/master/examples/Pacifica/Pacifica.ino
//
// Add one layer of waves into the led array
static CRGB pacifica_one_layer(uint16_t i, const CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  unsigned ci = cistart;
  unsigned waveangle = ioff;
  unsigned wavescale_half = (wavescale >> 1) + 20;

  waveangle += ((120 + SEGMENT.intensity) * i); //original 250 * i
  unsigned s16 = sin16_t(waveangle) + 32768;
  unsigned cs = scale16(s16, wavescale_half) + wavescale_half;
  ci += (cs * i);
  unsigned sindex16 = sin16_t(ci) + 32768;
  unsigned sindex8 = scale16(sindex16, 240);
  return CRGB(ColorFromPalette(p, sindex8, bri, LINEARBLEND));
}
 
uint16_t mode_pacifica()
{
  uint32_t nowOld = strip.now;

  CRGBPalette16 pacifica_palette_1 =
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
  CRGBPalette16 pacifica_palette_2 =
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
  CRGBPalette16 pacifica_palette_3 =
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

  if (SEGMENT.palette) {
    pacifica_palette_1 = SEGPALETTE;
    pacifica_palette_2 = SEGPALETTE;
    pacifica_palette_3 = SEGPALETTE;
  }

  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  unsigned sCIStart1 = SEGENV.aux0, sCIStart2 = SEGENV.aux1, sCIStart3 = SEGENV.step & 0xFFFF, sCIStart4 = (SEGENV.step >> 16);
  uint32_t deltams = (FRAMETIME >> 2) + ((FRAMETIME * SEGMENT.speed) >> 7);
  uint64_t deltat = (strip.now >> 2) + ((strip.now * SEGMENT.speed) >> 7);
  strip.now = deltat;

  unsigned speedfactor1 = beatsin16_t(3, 179, 269);
  unsigned speedfactor2 = beatsin16_t(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88_t(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88_t(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88_t(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88_t(257,4,6));
  SEGENV.aux0 = sCIStart1; SEGENV.aux1 = sCIStart2;
  SEGENV.step = (sCIStart4 << 16) | (sCIStart3 & 0xFFFF);

  // Clear out the LED array to a dim background blue-green
  //SEGMENT.fill(132618);

  unsigned basethreshold = beatsin8_t( 9, 55, 65);
  unsigned wave = beat8( 7 );

  for (unsigned i = 0; i < SEGLEN; i++) {
    CRGB c = CRGB(2, 6, 10);
    // Render each of four layers, with different scales and speeds, that vary over time
    c += pacifica_one_layer(i, pacifica_palette_1, sCIStart1, beatsin16_t(3, 11 * 256, 14 * 256), beatsin8_t(10, 70, 130), 0-beat16(301));
    c += pacifica_one_layer(i, pacifica_palette_2, sCIStart2, beatsin16_t(4,  6 * 256,  9 * 256), beatsin8_t(17, 40,  80),   beat16(401));
    c += pacifica_one_layer(i, pacifica_palette_3, sCIStart3,                         6 * 256 , beatsin8_t(9, 10,38)   , 0-beat16(503));
    c += pacifica_one_layer(i, pacifica_palette_3, sCIStart4,                         5 * 256 , beatsin8_t(8, 10,28)   ,   beat16(601));

    // Add extra 'white' to areas where the four layers of light have lined up brightly
    unsigned threshold = scale8( sin8_t( wave), 20) + basethreshold;
    wave += 7;
    unsigned l = c.getAverageLight();
    if (l > threshold) {
      unsigned overage = l - threshold;
      unsigned overage2 = qadd8(overage, overage);
      c += CRGB(overage, overage2, qadd8(overage2, overage2));
    }

    //deepen the blues and greens
    c.blue  = scale8(c.blue,  145);
    c.green = scale8(c.green, 200);
    c |= CRGB( 2, 5, 7);

    SEGMENT.setPixelColor(i, c);
  }

  strip.now = nowOld;
  return FRAMETIME;
}
static const char _data_FX_MODE_PACIFICA[] PROGMEM = "Pacifica@!,Angle;;!;;pal=51";


/*
 * Percentage display
 * Intensity values from 0-100 turn on the leds.
 */
uint16_t mode_percent(void) {

  unsigned percent = SEGMENT.intensity;
  percent = constrain(percent, 0, 200);
  unsigned active_leds = (percent < 100) ? roundf(SEGLEN * percent / 100.0f)
                                         : roundf(SEGLEN * (200 - percent) / 100.0f);

  unsigned size = (1 + ((SEGMENT.speed * SEGLEN) >> 11));
  if (SEGMENT.speed == 255) size = 255;

  if (percent <= 100) {
    for (unsigned i = 0; i < SEGLEN; i++) {
    	if (i < SEGENV.aux1) {
        if (SEGMENT.check1)
          SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(map(percent,0,100,0,255), false, false, 0));
        else
          SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
    	}
    	else {
        SEGMENT.setPixelColor(i, SEGCOLOR(1));
    	}
    }
  } else {
    for (unsigned i = 0; i < SEGLEN; i++) {
    	if (i < (SEGLEN - SEGENV.aux1)) {
        SEGMENT.setPixelColor(i, SEGCOLOR(1));
    	}
    	else {
        if (SEGMENT.check1)
          SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(map(percent,100,200,255,0), false, false, 0));
        else
          SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
    	}
    }
  }

  if(active_leds > SEGENV.aux1) {  // smooth transition to the target value
    SEGENV.aux1 += size;
    if (SEGENV.aux1 > active_leds) SEGENV.aux1 = active_leds;
  } else if (active_leds < SEGENV.aux1) {
    if (SEGENV.aux1 > size) SEGENV.aux1 -= size; else SEGENV.aux1 = 0;
    if (SEGENV.aux1 < active_leds) SEGENV.aux1 = active_leds;
  }

 	return FRAMETIME;
}
static const char _data_FX_MODE_PERCENT[] PROGMEM = "Percent@,% of fill,,,,One color;!,!;!";


/*
 * Modulates the brightness similar to a heartbeat
 * (unimplemented?) tries to draw an ECG approximation on a 2D matrix
 */
uint16_t mode_heartbeat(void) {
  unsigned bpm = 40 + (SEGMENT.speed >> 3);
  uint32_t msPerBeat = (60000L / bpm);
  uint32_t secondBeat = (msPerBeat / 3);
  uint32_t bri_lower = SEGENV.aux1;
  unsigned long beatTimer = strip.now - SEGENV.step;

  bri_lower = bri_lower * 2042 / (2048 + SEGMENT.intensity);
  SEGENV.aux1 = bri_lower;

  if ((beatTimer > secondBeat) && !SEGENV.aux0) { // time for the second beat?
    SEGENV.aux1 = UINT16_MAX; //3/4 bri
    SEGENV.aux0 = 1;
  }
  if (beatTimer > msPerBeat) { // time to reset the beat timer?
    SEGENV.aux1 = UINT16_MAX; //full bri
    SEGENV.aux0 = 0;
    SEGENV.step = strip.now;
  }

  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, color_blend(SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0), SEGCOLOR(1), uint8_t(255 - (SEGENV.aux1 >> 8))));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_HEARTBEAT[] PROGMEM = "Heartbeat@!,!;!,!;!;01;m12=1";

 


/*
/ Plasma Effect
/ adapted from https://github.com/atuline/FastLED-Demos/blob/master/plasma/plasma.ino
*/
uint16_t mode_plasma(void) {
  // initialize phases on start
  if (SEGENV.call == 0) {
    SEGENV.aux0 = hw_random8(0,2);  // add a bit of randomness
  }
  unsigned thisPhase = beatsin8_t(6+SEGENV.aux0,-64,64);
  unsigned thatPhase = beatsin8_t(7+SEGENV.aux0,-64,64);

  for (unsigned i = 0; i < SEGLEN; i++) {   // For each of the LED's in the strand, set color &  brightness based on a wave as follows:
    unsigned colorIndex = cubicwave8((i*(2+ 3*(SEGMENT.speed >> 5))+thisPhase) & 0xFF)/2   // factor=23 // Create a wave and add a phase change and add another wave with its own phase change.
                              + cos8_t((i*(1+ 2*(SEGMENT.speed >> 5))+thatPhase) & 0xFF)/2;  // factor=15 // Hey, you can even change the frequencies if you wish.
    unsigned thisBright = qsub8(colorIndex, beatsin8_t(7,0, (128 - (SEGMENT.intensity>>1))));
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(colorIndex, false, PALETTE_SOLID_WRAP, 0, thisBright));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_PLASMA[] PROGMEM = "Plasma@Phase,!;!;!";

 


/*
 * Tetris or Stacking (falling bricks) Effect
 * by Blaz Kristan (AKA blazoncek) (https://github.com/blazoncek, https://blaz.at/home)
 */
//20 bytes
typedef struct Tetris {
  float    pos;
  float    speed;
  uint8_t  col;   // color index
  uint16_t brick; // brick size in pixels
  uint16_t stack; // stack size in pixels
  uint32_t step;  // 2D-fication of SEGENV.step (state)
} tetris;

uint16_t mode_tetrix(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned strips = SEGMENT.nrOfVStrips(); // allow running on virtual strips (columns in 2D segment)
  unsigned dataSize = sizeof(tetris);
  if (!SEGENV.allocateData(dataSize * strips)) return mode_static(); //allocation failed
  Tetris* drops = reinterpret_cast<Tetris*>(SEGENV.data);

  //if (SEGENV.call == 0) SEGMENT.fill(SEGCOLOR(1));  // will fill entire segment (1D or 2D), then use drop->step = 0 below

  // virtualStrip idea by @ewowi (Ewoud Wijma)
  // requires virtual strip # to be embedded into upper 16 bits of index in setPixelcolor()
  // the following functions will not work on virtual strips: fill(), fade_out(), fadeToBlack(), blur()
  struct virtualStrip {
    static void runStrip(size_t stripNr, Tetris *drop) {
      // initialize dropping on first call or segment full
      if (SEGENV.call == 0) {
        drop->stack = 0;                  // reset brick stack size
        drop->step = strip.now + 2000;     // start by fading out strip
        if (SEGMENT.check1) drop->col = 0;// use only one color from palette
      }

      if (drop->step == 0) {              // init brick
        // speed calculation: a single brick should reach bottom of strip in X seconds
        // if the speed is set to 1 this should take 5s and at 255 it should take 0.25s
        // as this is dependant on SEGLEN it should be taken into account and the fact that effect runs every FRAMETIME s
        int speed = SEGMENT.speed ? SEGMENT.speed : hw_random8(1,255);
        speed = map(speed, 1, 255, 5000, 250); // time taken for full (SEGLEN) drop
        drop->speed = float(SEGLEN * FRAMETIME) / float(speed); // set speed
        drop->pos   = SEGLEN;             // start at end of segment (no need to subtract 1)
        if (!SEGMENT.check1) drop->col = hw_random8(0,15)<<4;   // limit color choices so there is enough HUE gap
        drop->step  = 1;                  // drop state (0 init, 1 forming, 2 falling)
        drop->brick = (SEGMENT.intensity ? (SEGMENT.intensity>>5)+1 : hw_random8(1,5)) * (1+(SEGLEN>>6));  // size of brick
      }

      if (drop->step == 1) {              // forming
        if (hw_random8()>>6) {               // random drop
          drop->step = 2;                 // fall
        }
      }

      if (drop->step == 2) {              // falling
        if (drop->pos > drop->stack) {    // fall until top of stack
          drop->pos -= drop->speed;       // may add gravity as: speed += gravity
          if (int(drop->pos) < int(drop->stack)) drop->pos = drop->stack;
          for (unsigned i = unsigned(drop->pos); i < SEGLEN; i++) {
            uint32_t col = i < unsigned(drop->pos)+drop->brick ? SEGMENT.color_from_palette(drop->col, false, false, 0) : SEGCOLOR(1);
            SEGMENT.setPixelColor(indexToVStrip(i, stripNr), col);
          }
        } else {                          // we hit bottom
          drop->step = 0;                 // proceed with next brick, go back to init
          drop->stack += drop->brick;     // increase the stack size
          if (drop->stack >= SEGLEN) drop->step = strip.now + 2000; // fade out stack
        }
      }

      if (drop->step > 2) {               // fade strip
        drop->brick = 0;                  // reset brick size (no more growing)
        if (drop->step > strip.now) {
          // allow fading of virtual strip
          for (unsigned i = 0; i < SEGLEN; i++) SEGMENT.blendPixelColor(indexToVStrip(i, stripNr), SEGCOLOR(1), 25); // 10% blend
        } else {
          drop->stack = 0;                // reset brick stack size
          drop->step = 0;                 // proceed with next brick
          if (SEGMENT.check1) drop->col += 8;   // gradually increase palette index
        }
      }
    }
  };

  for (unsigned stripNr=0; stripNr<strips; stripNr++)
    virtualStrip::runStrip(stripNr, &drops[stripNr]);

  return FRAMETIME;
}
static const char _data_FX_MODE_TETRIX[] PROGMEM = "Tetrix@!,Width,,,,One color;!,!;!;;sx=0,ix=0,pal=11,m12=1";
 
 
//values close to 100 produce 5Hz flicker, which looks very candle-y
//Inspired by https://github.com/avanhanegem/ArduinoCandleEffectNeoPixel
//and https://cpldcpu.wordpress.com/2016/01/05/reverse-engineering-a-real-candle/

uint16_t candle(bool multi)
{
  if (multi && SEGLEN > 1) {
    //allocate segment data
    unsigned dataSize = max(1, (int)SEGLEN -1) *3; //max. 1365 pixels (ESP8266)
    if (!SEGENV.allocateData(dataSize)) return candle(false); //allocation failed
  }

  //max. flicker range controlled by intensity
  unsigned valrange = SEGMENT.intensity;
  unsigned rndval = valrange >> 1; //max 127

  //step (how much to move closer to target per frame) coarsely set by speed
  unsigned speedFactor = 4;
  if (SEGMENT.speed > 252) { //epilepsy
    speedFactor = 1;
  } else if (SEGMENT.speed > 99) { //regular candle (mode called every ~25 ms, so 4 frames to have a new target every 100ms)
    speedFactor = 2;
  } else if (SEGMENT.speed > 49) { //slower fade
    speedFactor = 3;
  } //else 4 (slowest)

  unsigned numCandles = (multi) ? SEGLEN : 1;

  for (unsigned i = 0; i < numCandles; i++)
  {
    unsigned d = 0; //data location

    unsigned s = SEGENV.aux0, s_target = SEGENV.aux1, fadeStep = SEGENV.step;
    if (i > 0) {
      d = (i-1) *3;
      s = SEGENV.data[d]; s_target = SEGENV.data[d+1]; fadeStep = SEGENV.data[d+2];
    }
    if (fadeStep == 0) { //init vals
      s = 128; s_target = 130 + hw_random8(4); fadeStep = 1;
    }

    bool newTarget = false;
    if (s_target > s) { //fade up
      s = qadd8(s, fadeStep);
      if (s >= s_target) newTarget = true;
    } else {
      s = qsub8(s, fadeStep);
      if (s <= s_target) newTarget = true;
    }

    if (newTarget) {
      s_target = hw_random8(rndval) + hw_random8(rndval); //between 0 and rndval*2 -2 = 252
      if (s_target < (rndval >> 1)) s_target = (rndval >> 1) + hw_random8(rndval);
      unsigned offset = (255 - valrange);
      s_target += offset;

      unsigned dif = (s_target > s) ? s_target - s : s - s_target;

      fadeStep = dif >> speedFactor;
      if (fadeStep == 0) fadeStep = 1;
    }

    if (i > 0) {
      SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0), uint8_t(s)));

      SEGENV.data[d] = s; SEGENV.data[d+1] = s_target; SEGENV.data[d+2] = fadeStep;
    } else {
      for (unsigned j = 0; j < SEGLEN; j++) {
        SEGMENT.setPixelColor(j, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(j, true, PALETTE_SOLID_WRAP, 0), uint8_t(s)));
      }

      SEGENV.aux0 = s; SEGENV.aux1 = s_target; SEGENV.step = fadeStep;
    }
  }

  return FRAMETIME_FI


/*
* Sinelon stolen from FASTLED examples
*/
static uint16_t sinelon_base(bool dual, bool rainbow=false) {
  if (SEGLEN <= 1) return mode_static();
  SEGMENT.fade_out(SEGMENT.intensity);
  unsigned pos = beatsin16_t(SEGMENT.speed/10,0,SEGLEN-1);
  if (SEGENV.call == 0) SEGENV.aux0 = pos;
  uint32_t color1 = SEGMENT.color_from_palette(pos, true, false, 0);
  uint32_t color2 = SEGCOLOR(2);
  if (rainbow) {
    color1 = SEGMENT.color_wheel((pos & 0x07) * 32);
  }
  SEGMENT.setPixelColor(pos, color1);
  if (dual) {
    if (!color2) color2 = SEGMENT.color_from_palette(pos, true, false, 0);
    if (rainbow) color2 = color1; //rainbow
    SEGMENT.setPixelColor(SEGLEN-1-pos, color2);
  }
  if (SEGENV.aux0 != pos) {
    if (SEGENV.aux0 < pos) {
      for (unsigned i = SEGENV.aux0; i < pos ; i++) {
        SEGMENT.setPixelColor(i, color1);
        if (dual) SEGMENT.setPixelColor(SEGLEN-1-i, color2);
      }
    } else {
      for (unsigned i = SEGENV.aux0; i > pos ; i--) {
        SEGMENT.setPixelColor(i, color1);
        if (dual) SEGMENT.setPixelColor(SEGLEN-1-i, color2);
      }
    }
    SEGENV.aux0 = pos;
  }

  return FRAMETIME;
}


uint16_t mode_sinelon(void) {
  return sinelon_base(false);
}
static const char _data_FX_MODE_SINELON[] PROGMEM = "Sinelon@!,Trail;!,!,!;!";


uint16_t mode_sinelon_dual(void) {
  return sinelon_base(true);
}
static const char _data_FX_MODE_SINELON_DUAL[] PROGMEM = "Sinelon Dual@!,Trail;!,!,!;!";


uint16_t mode_sinelon_rainbow(void) {
  return sinelon_base(false, true);
}
static const char _data_FX_MODE_SINELON_RAINBOW[] PROGMEM = "Sinelon Rainbow@!,Trail;,,!;!";




uint16_t mode_tri_static_pattern()
{
  unsigned segSize = (SEGMENT.intensity >> 5) +1;
  unsigned currSeg = 0;
  unsigned currSegCount = 0;

  for (unsigned i = 0; i < SEGLEN; i++) {
    if ( currSeg % 3 == 0 ) {
      SEGMENT.setPixelColor(i, SEGCOLOR(0));
    } else if( currSeg % 3 == 1) {
      SEGMENT.setPixelColor(i, SEGCOLOR(1));
    } else {
      SEGMENT.setPixelColor(i, SEGCOLOR(2));
    }
    currSegCount += 1;
    if (currSegCount >= segSize) {
      currSeg +=1;
      currSegCount = 0;
    }
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TRI_STATIC_PATTERN[] PROGMEM = "Solid Pattern Tri@,Size;1,2,3;;;pal=0";



//Speed slider sets amount of LEDs lit, intensity sets unlit
uint16_t mode_static_pattern()
{
  unsigned lit = 1 + SEGMENT.speed;
  unsigned unlit = 1 + SEGMENT.intensity;
  bool drawingLit = true;
  unsigned cnt = 0;

  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, (drawingLit) ? SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0) : SEGCOLOR(1));
    cnt++;
    if (cnt >= ((drawingLit) ? lit : unlit)) {
      cnt = 0;
      drawingLit = !drawingLit;
    }
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_STATIC_PATTERN[] PROGMEM = "Solid Pattern@Fg size,Bg size;Fg,!;!;;pal=0";



//  TwinkleFOX by Mark Kriegsman: https://gist.github.com/kriegsman/756ea6dcae8e30845b5a
//
//  TwinkleFOX: Twinkling 'holiday' lights that fade in and out.
//  Colors are chosen from a palette. Read more about this effect using the link above!
static CRGB twinklefox_one_twinkle(uint32_t ms, uint8_t salt, bool cat)
{
  // Overall twinkle speed (changed)
  unsigned ticks = ms / SEGENV.aux0;
  unsigned fastcycle8 = uint8_t(ticks);
  uint16_t slowcycle16 = (ticks >> 8) + salt;
  slowcycle16 += sin8_t(slowcycle16);
  slowcycle16 = (slowcycle16 * 2053) + 1384;
  uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);

  // Overall twinkle density.
  // 0 (NONE lit) to 8 (ALL lit at once).
  // Default is 5.
  unsigned twinkleDensity = (SEGMENT.intensity >> 5) +1;

  unsigned bright = 0;
  if (((slowcycle8 & 0x0E)/2) < twinkleDensity) {
    unsigned ph = fastcycle8;
    // This is like 'triwave8', which produces a
    // symmetrical up-and-down triangle sawtooth waveform, except that this
    // function produces a triangle wave with a faster attack and a slower decay
    if (cat) //twinklecat, variant where the leds instantly turn on
    {
      bright = 255 - ph;
    } else { //vanilla twinklefox
      if (ph < 86) {
      bright = ph * 3;
      } else {
        ph -= 86;
        bright = 255 - (ph + (ph/2));
      }
    }
  }

  unsigned hue = slowcycle8 - salt;
  CRGB c;
  if (bright > 0) {
    c = ColorFromPalette(SEGPALETTE, hue, bright, NOBLEND);
    if (!SEGMENT.check1) {
      // This code takes a pixel, and if its in the 'fading down'
      // part of the cycle, it adjusts the color a little bit like the
      // way that incandescent bulbs fade toward 'red' as they dim.
      if (fastcycle8 >= 128)
      {
        unsigned cooling = (fastcycle8 - 128) >> 4;
        c.g = qsub8(c.g, cooling);
        c.b = qsub8(c.b, cooling * 2);
      }
    }
  } else {
    c = CRGB::Black;
  }
  return c;
}

//  This function loops over each pixel, calculates the
//  adjusted 'clock' that this pixel should use, and calls
//  "CalculateOneTwinkle" on each pixel.  It then displays
//  either the twinkle color of the background color,
//  whichever is brighter.
static uint16_t twinklefox_base(bool cat)
{
  // "PRNG16" is the pseudorandom number generator
  // It MUST be reset to the same starting value each time
  // this function is called, so that the sequence of 'random'
  // numbers that it generates is (paradoxically) stable.
  uint16_t PRNG16 = 11337;

  // Calculate speed
  if (SEGMENT.speed > 100) SEGENV.aux0 = 3 + ((255 - SEGMENT.speed) >> 3);
  else SEGENV.aux0 = 22 + ((100 - SEGMENT.speed) >> 1);

  // Set up the background color, "bg".
  CRGB bg = CRGB(SEGCOLOR(1));
  unsigned bglight = bg.getAverageLight();
  if (bglight > 64) {
    bg.nscale8_video(16); // very bright, so scale to 1/16th
  } else if (bglight > 16) {
    bg.nscale8_video(64); // not that bright, so scale to 1/4th
  } else {
    bg.nscale8_video(86); // dim, scale to 1/3rd.
  }

  unsigned backgroundBrightness = bg.getAverageLight();

  for (unsigned i = 0; i < SEGLEN; i++) {

    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    unsigned myclockoffset16= PRNG16; // use that number as clock offset
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
    // use that number as clock speed adjustment factor (in 8ths, from 8/8ths to 23/8ths)
    unsigned myspeedmultiplierQ5_3 =  ((((PRNG16 & 0xFF)>>4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
    uint32_t myclock30 = (uint32_t)((strip.now * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
    unsigned  myunique8 = PRNG16 >> 8; // get 'salt' value for this pixel

    // We now have the adjusted 'clock' for this pixel, now we call
    // the function that computes what color the pixel should be based
    // on the "brightness = f( time )" idea.
    CRGB c = twinklefox_one_twinkle(myclock30, myunique8, cat);

    unsigned cbright = c.getAverageLight();
    int deltabright = cbright - backgroundBrightness;
    if (deltabright >= 32 || (!bg)) {
      // If the new pixel is significantly brighter than the background color,
      // use the new color.
      SEGMENT.setPixelColor(i, c);
    } else if (deltabright > 0) {
      // If the new pixel is just slightly brighter than the background color,
      // mix a blend of the new color and the background color
      SEGMENT.setPixelColor(i, color_blend(RGBW32(bg.r,bg.g,bg.b,0), RGBW32(c.r,c.g,c.b,0), uint8_t(deltabright * 8)));
    } else {
      // if the new pixel is not at all brighter than the background color,
      // just use the background color.
      SEGMENT.setPixelColor(i, bg);
    }
  }
  return FRAMETIME;
}


uint16_t mode_twinklefox()
{
  return twinklefox_base(false);
}
static const char _data_FX_MODE_TWINKLEFOX[] PROGMEM = "Twinklefox@!,Twinkle rate,,,,Cool;!,!;!";


uint16_t mode_twinklecat()
{
  return twinklefox_base(true);
}
static const char _data_FX_MODE_TWINKLECAT[] PROGMEM = "Twinklecat@!,Twinkle rate,,,,Cool;!,!;!";

// meteor effect & meteor smooth (merged by @dedehai)
// send a meteor from begining to to the end of the strip with a trail that randomly decays.
// adapted from https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectMeteorRain
uint16_t mode_meteor() {
  if (SEGLEN <= 1) return mode_static();
  if (!SEGENV.allocateData(SEGLEN)) return mode_static(); //allocation failed
  const bool meteorSmooth = SEGMENT.check3;
  byte* trail = SEGENV.data;

  const unsigned meteorSize = 1 + SEGLEN / 20; // 5%
  uint16_t meteorstart;
  if(meteorSmooth) meteorstart = map((SEGENV.step >> 6 & 0xFF), 0, 255, 0, SEGLEN -1);
  else {
    unsigned counter = strip.now * ((SEGMENT.speed >> 2) + 8);
    meteorstart = (counter * SEGLEN) >> 16;
  }

  const int max = SEGMENT.palette==5 || !SEGMENT.check1 ? 240 : 255;
  // fade all leds to colors[1] in LEDs one step
  for (unsigned i = 0; i < SEGLEN; i++) {
    uint32_t col;
    if (hw_random8() <= 255 - SEGMENT.intensity) {
      if(meteorSmooth) {
        if (trail[i] > 0) {
          int change = trail[i] + 4 - hw_random8(24); //change each time between -20 and +4
          trail[i] = constrain(change, 0, max);
        }
        col = SEGMENT.check1 ? SEGMENT.color_from_palette(i, true, false, 0, trail[i]) : SEGMENT.color_from_palette(trail[i], false, true, 255);
      }
      else {
        trail[i] = scale8(trail[i], 128 + hw_random8(127));
        int index = trail[i];
        int idx = 255;
        int bri = SEGMENT.palette==35 || SEGMENT.palette==36 ? 255 : trail[i];
        if (!SEGMENT.check1) {
          idx = 0;
          index = map(i,0,SEGLEN,0,max);
          bri = trail[i];
        }
        col = SEGMENT.color_from_palette(index, false, false, idx, bri);  // full brightness for Fire
      }
      SEGMENT.setPixelColor(i, col);
    }
  }

  // draw meteor
  for (unsigned j = 0; j < meteorSize; j++) {
    unsigned index = (meteorstart + j) % SEGLEN;
    if(meteorSmooth) {
        trail[index] = max;
        uint32_t col = SEGMENT.check1 ? SEGMENT.color_from_palette(index, true, false, 0, trail[index]) : SEGMENT.color_from_palette(trail[index], false, true, 255);
        SEGMENT.setPixelColor(index, col);
    }
    else{
      int idx = 255;
      int i = trail[index] = max;
      if (!SEGMENT.check1) {
        i = map(index,0,SEGLEN,0,max);
        idx = 0;
      }
      uint32_t col = SEGMENT.color_from_palette(i, false, false, idx, 255); // full brightness
      SEGMENT.setPixelColor(index, col);
    }
  }

  SEGENV.step += SEGMENT.speed +1;
  return FRAMETIME;
}
static const char _data_FX_MODE_METEOR[] PROGMEM = "Meteor@!,Trail,,,,Gradient,,Smooth;;!;1";



// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
uint16_t mode_bpm() {
  uint32_t stp = (strip.now / 20) & 0xFF;
  uint8_t beat = beatsin8_t(SEGMENT.speed, 64, 255);
  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(stp + (i * 2), false, PALETTE_SOLID_WRAP, 0, beat - stp + (i * 10)));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_BPM[] PROGMEM = "Bpm@!;!;!;;sx=64";


uint16_t mode_fillnoise8() {
  if (SEGENV.call == 0) SEGENV.step = hw_random();
  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned index = perlin8(i * SEGLEN, SEGENV.step + i * SEGLEN);
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0));
  }
  SEGENV.step += beatsin8_t(SEGMENT.speed, 1, 6); //10,1,4

  return FRAMETIME;
}
static const char _data_FX_MODE_FILLNOISE8[] PROGMEM = "Fill Noise@!;!;!";


uint16_t mode_noise16_1() {
  unsigned scale = 320;                                       // the "zoom factor" for the noise
  SEGENV.step += (1 + SEGMENT.speed/16);

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned shift_x = beatsin8_t(11);                          // the x position of the noise field swings @ 17 bpm
    unsigned shift_y = SEGENV.step/42;                        // the y position becomes slowly incremented
    unsigned real_x = (i + shift_x) * scale;                  // the x position of the noise field swings @ 17 bpm
    unsigned real_y = (i + shift_y) * scale;                  // the y position becomes slowly incremented
    uint32_t real_z = SEGENV.step;                            // the z position becomes quickly incremented
    unsigned noise = perlin16(real_x, real_y, real_z) >> 8;   // get the noise data and scale it down
    unsigned index = sin8_t(noise * 3);                         // map LED color based on noise data

    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_NOISE16_1[] PROGMEM = "Noise 1@!;!;!;;pal=20";


uint16_t mode_noise16_2() {
  unsigned scale = 1000;                                        // the "zoom factor" for the noise
  SEGENV.step += (1 + (SEGMENT.speed >> 1));

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned shift_x = SEGENV.step >> 6;                        // x as a function of time
    uint32_t real_x = (i + shift_x) * scale;                    // calculate the coordinates within the noise field
    unsigned noise = perlin16(real_x, 0, 4223) >> 8;            // get the noise data and scale it down
    unsigned index = sin8_t(noise * 3);                           // map led color based on noise data

    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0, noise));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_NOISE16_2[] PROGMEM = "Noise 2@!;!;!;;pal=43";


uint16_t mode_noise16_3() {
  unsigned scale = 800;                                       // the "zoom factor" for the noise
  SEGENV.step += (1 + SEGMENT.speed);

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned shift_x = 4223;                                  // no movement along x and y
    unsigned shift_y = 1234;
    uint32_t real_x = (i + shift_x) * scale;                  // calculate the coordinates within the noise field
    uint32_t real_y = (i + shift_y) * scale;                  // based on the precalculated positions
    uint32_t real_z = SEGENV.step*8;
    unsigned noise = perlin16(real_x, real_y, real_z) >> 8;   // get the noise data and scale it down
    unsigned index = sin8_t(noise * 3);                         // map led color based on noise data

    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0, noise));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_NOISE16_3[] PROGMEM = "Noise 3@!;!;!;;pal=35";


//https://github.com/aykevl/ledstrip-spark/blob/master/ledstrip.ino
uint16_t mode_noise16_4() {
  uint32_t stp = (strip.now * SEGMENT.speed) >> 7;
  for (unsigned i = 0; i < SEGLEN; i++) {
    int index = perlin16(uint32_t(i) << 12, stp);
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(index, false, PALETTE_SOLID_WRAP, 0));
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_NOISE16_4[] PROGMEM = "Noise 4@!;!;!;;pal=26";


//based on https://gist.github.com/kriegsman/5408ecd397744ba0393e
uint16_t mode_colortwinkle() {
  unsigned dataSize = (SEGLEN+7) >> 3; //1 bit per LED
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed

  CRGBW col, prev;
  fract8 fadeUpAmount = strip.getBrightness()>28 ? 8 + (SEGMENT.speed>>2) : 68-strip.getBrightness();
  fract8 fadeDownAmount = strip.getBrightness()>28 ? 8 + (SEGMENT.speed>>3) : 68-strip.getBrightness();
  for (unsigned i = 0; i < SEGLEN; i++) {
    CRGBW cur = SEGMENT.getPixelColor(i);
    prev = cur;
    unsigned index = i >> 3;
    unsigned  bitNum = i & 0x07;
    bool fadeUp = bitRead(SEGENV.data[index], bitNum);

    if (fadeUp) {
      CRGBW incrementalColor = color_fade(cur, fadeUpAmount, true);
      col = color_add(cur, incrementalColor);

      if (col.r == 255 || col.g == 255 || col.b == 255) {
        bitWrite(SEGENV.data[index], bitNum, false);
      }

      if (cur == prev) {  //fix "stuck" pixels
        color_add(col, col);
        SEGMENT.setPixelColor(i, col);
      }
      else SEGMENT.setPixelColor(i, col);
    }
    else {
      col = color_fade(cur, 255 - fadeDownAmount);
      SEGMENT.setPixelColor(i, col);
    }
  }

  for (unsigned j = 0; j <= SEGLEN / 50; j++) {
    if (hw_random8() <= SEGMENT.intensity) {
      for (unsigned times = 0; times < 5; times++) { //attempt to spawn a new pixel 5 times
        int i = hw_random16(SEGLEN);
        if (SEGMENT.getPixelColor(i) == 0) {
          unsigned index = i >> 3;
          unsigned  bitNum = i & 0x07;
          bitWrite(SEGENV.data[index], bitNum, true);
          SEGMENT.setPixelColor(i, ColorFromPalette(SEGPALETTE, hw_random8(), 64, NOBLEND));
          break; //only spawn 1 new pixel per frame per 50 LEDs
        }
      }
    }
  }
  return FRAMETIME_FIXED;
}
static const char _data_FX_MODE_COLORTWINKLE[] PROGMEM = "Colortwinkles@Fade speed,Spawn speed;;!;;m12=0"; //pixels


#ifdef WLED_PS_DONT_REPLACE_FX
/*
 * Creates random comets
 * Custom mode by Keith Lord: https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/MultiComet.h
 */
#define MAX_COMETS 8
uint16_t mode_multi_comet(void) {
  uint32_t cycleTime = 10 + (uint32_t)(255 - SEGMENT.speed);
  uint32_t it = strip.now / cycleTime;
  if (SEGENV.step == it) return FRAMETIME;
  if (!SEGENV.allocateData(sizeof(uint16_t) * MAX_COMETS)) return mode_static(); //allocation failed

  SEGMENT.fade_out(SEGMENT.intensity/2 + 128);

  uint16_t* comets = reinterpret_cast<uint16_t*>(SEGENV.data);

  for (unsigned i=0; i < MAX_COMETS; i++) {
    if(comets[i] < SEGLEN) {
      unsigned index = comets[i];
      if (SEGCOLOR(2) != 0)
      {
        SEGMENT.setPixelColor(index, i % 2 ? SEGMENT.color_from_palette(index, true, PALETTE_SOLID_WRAP, 0) : SEGCOLOR(2));
      } else
      {
        SEGMENT.setPixelColor(index, SEGMENT.color_from_palette(index, true, PALETTE_SOLID_WRAP, 0));
      }
      comets[i]++;
    } else {
      if(!hw_random16(SEGLEN)) {
        comets[i] = 0;
      }
    }
  }

  SEGENV.step = it;
  return FRAMETIME;
}
static const char _data_FX_MODE_MULTI_COMET[] PROGMEM = "Multi Comet@!,Fade;!,!;!;1";
#undef MAX_COMETS
#endif // WLED_PS_DONT_REPLACE_FX

/*
 * Running random pixels ("Stream 2")
 * Custom mode by Keith Lord: https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/RandomChase.h
 */
uint16_t mode_random_chase(void) {
  if (SEGENV.call == 0) {
    SEGENV.step = RGBW32(random8(), random8(), random8(), 0);
    SEGENV.aux0 = random16();
  }
  unsigned prevSeed = random16_get_seed(); // save seed so we can restore it at the end of the function
  uint32_t cycleTime = 25 + (3 * (uint32_t)(255 - SEGMENT.speed));
  uint32_t it = strip.now / cycleTime;
  uint32_t color = SEGENV.step;
  random16_set_seed(SEGENV.aux0);

  for (int i = SEGLEN -1; i >= 0; i--) {
    uint8_t r = random8(6) != 0 ? (color >> 16 & 0xFF) : random8();
    uint8_t g = random8(6) != 0 ? (color >> 8  & 0xFF) : random8();
    uint8_t b = random8(6) != 0 ? (color       & 0xFF) : random8();
    color = RGBW32(r, g, b, 0);
    SEGMENT.setPixelColor(i, color);
    if (i == SEGLEN -1U && SEGENV.aux1 != (it & 0xFFFFU)) { //new first color in next frame
      SEGENV.step = color;
      SEGENV.aux0 = random16_get_seed();
    }
  }

  SEGENV.aux1 = it & 0xFFFF;

  random16_set_seed(prevSeed); // restore original seed so other effects can use "random" PRNG
  return FRAMETIME;
}
static const char _data_FX_MODE_RANDOM_CHASE[] PROGMEM = "Stream 2@!;;";


/*
 * Fades between 3 colors
 * Custom mode by Keith Lord: https://github.com/kitesurfer1404/WS2812FX/blob/master/src/custom/TriFade.h
 * Modified by Aircoookie
 */
uint16_t mode_tricolor_fade(void) {
  unsigned counter = strip.now * ((SEGMENT.speed >> 3) +1);
  uint16_t prog = (counter * 768) >> 16;

  uint32_t color1 = 0, color2 = 0;
  unsigned stage = 0;

  if(prog < 256) {
    color1 = SEGCOLOR(0);
    color2 = SEGCOLOR(1);
    stage = 0;
  } else if(prog < 512) {
    color1 = SEGCOLOR(1);
    color2 = SEGCOLOR(2);
    stage = 1;
  } else {
    color1 = SEGCOLOR(2);
    color2 = SEGCOLOR(0);
    stage = 2;
  }

  byte stp = prog; // % 256
  for (unsigned i = 0; i < SEGLEN; i++) {
    uint32_t color;
    if (stage == 2) {
      color = color_blend(SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 2), color2, stp);
    } else if (stage == 1) {
      color = color_blend(color1, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 2), stp);
    } else {
      color = color_blend(color1, color2, stp);
    }
    SEGMENT.setPixelColor(i, color);
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_TRICOLOR_FADE[] PROGMEM = "Tri Fade@!;1,2,3;!";




/*
 * Two dots running
 */
uint16_t mode_two_dots() {
 if (SEGLEN <= 1) return mode_static();
  unsigned delay = 1 + (FRAMETIME<<3) / SEGLEN;  // longer segments should change faster
  uint32_t it = strip.now / map(SEGMENT.speed, 0, 255, delay<<4, delay);
  unsigned offset = it % SEGLEN;
  unsigned width = ((SEGLEN*(SEGMENT.intensity+1))>>9); //max width is half the strip
  if (!width) width = 1;
  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(2));
  const uint32_t color1 = SEGCOLOR(0);
  const uint32_t color2 = (SEGCOLOR(1) == SEGCOLOR(2)) ? color1 : SEGCOLOR(1);
  for (unsigned i = 0; i < width; i++) {
    unsigned indexR = (offset + i) % SEGLEN;
    unsigned indexB = (offset + i + (SEGLEN>>1)) % SEGLEN;
    SEGMENT.setPixelColor(indexR, color1);
    SEGMENT.setPixelColor(indexB, color2);
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_TWO_DOTS[] PROGMEM = "Two Dots@!,Dot size,,,,,Overlay;1,2,Bg;!";


/*
 * Fairy, inspired by https://www.youtube.com/watch?v=zeOw5MZWq24
 */
//4 bytes
typedef struct Flasher {
  uint16_t stateStart;
  uint8_t stateDur;
  bool stateOn;
} flasher;

#define FLASHERS_PER_ZONE 6
#define MAX_SHIMMER 92

uint16_t mode_fairy() {
  //set every pixel to a 'random' color from palette (using seed so it doesn't change between frames)
  uint16_t PRNG16 = 5100 + strip.getCurrSegmentId();
  for (unsigned i = 0; i < SEGLEN; i++) {
    PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; //next 'random' number
    SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(PRNG16 >> 8, false, false, 0));
  }

  //amount of flasher pixels depending on intensity (0: none, 255: every LED)
  if (SEGMENT.intensity == 0) return FRAMETIME;
  unsigned flasherDistance = ((255 - SEGMENT.intensity) / 28) +1; //1-10
  unsigned numFlashers = (SEGLEN / flasherDistance) +1;

  unsigned dataSize = sizeof(flasher) * numFlashers;
  if (!SEGENV.allocateData(dataSize)) return FRAMETIME; //allocation failed
  Flasher* flashers = reinterpret_cast<Flasher*>(SEGENV.data);
  unsigned now16 = strip.now & 0xFFFF;

  //Up to 11 flashers in one brightness zone, afterwards a new zone for every 6 flashers
  unsigned zones = numFlashers/FLASHERS_PER_ZONE;
  if (!zones) zones = 1;
  unsigned flashersInZone = numFlashers/zones;
  uint8_t flasherBri[FLASHERS_PER_ZONE*2 -1];

  for (unsigned z = 0; z < zones; z++) {
    unsigned flasherBriSum = 0;
    unsigned firstFlasher = z*flashersInZone;
    if (z == zones-1) flashersInZone = numFlashers-(flashersInZone*(zones-1));

    for (unsigned f = firstFlasher; f < firstFlasher + flashersInZone; f++) {
      unsigned stateTime = uint16_t(now16 - flashers[f].stateStart);
      //random on/off time reached, switch state
      if (stateTime > flashers[f].stateDur * 10) {
        flashers[f].stateOn = !flashers[f].stateOn;
        if (flashers[f].stateOn) {
          flashers[f].stateDur = 12 + hw_random8(12 + ((255 - SEGMENT.speed) >> 2)); //*10, 250ms to 1250ms
        } else {
          flashers[f].stateDur = 20 + hw_random8(6 + ((255 - SEGMENT.speed) >> 2)); //*10, 250ms to 1250ms
        }
        //flashers[f].stateDur = 51 + hw_random8(2 + ((255 - SEGMENT.speed) >> 1));
        flashers[f].stateStart = now16;
        if (stateTime < 255) {
          flashers[f].stateStart -= 255 -stateTime; //start early to get correct bri
          flashers[f].stateDur += 26 - stateTime/10;
          stateTime = 255 - stateTime;
        } else {
          stateTime = 0;
        }
      }
      if (stateTime > 255) stateTime = 255; //for flasher brightness calculation, fades in first 255 ms of state
      //flasherBri[f - firstFlasher] = (flashers[f].stateOn) ? 255-SEGMENT.gamma8((510 - stateTime) >> 1) : SEGMENT.gamma8((510 - stateTime) >> 1);
      flasherBri[f - firstFlasher] = (flashers[f].stateOn) ? stateTime : 255 - (stateTime >> 0);
      flasherBriSum += flasherBri[f - firstFlasher];
    }
    //dim factor, to create "shimmer" as other pixels get less voltage if a lot of flashers are on
    unsigned avgFlasherBri = flasherBriSum / flashersInZone;
    unsigned globalPeakBri = 255 - ((avgFlasherBri * MAX_SHIMMER) >> 8); //183-255, suitable for 1/5th of LEDs flashers

    for (unsigned f = firstFlasher; f < firstFlasher + flashersInZone; f++) {
      uint8_t bri = (flasherBri[f - firstFlasher] * globalPeakBri) / 255;
      PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; //next 'random' number
      unsigned flasherPos = f*flasherDistance;
      SEGMENT.setPixelColor(flasherPos, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(PRNG16 >> 8, false, false, 0), bri));
      for (unsigned i = flasherPos+1; i < flasherPos+flasherDistance && i < SEGLEN; i++) {
        PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; //next 'random' number
        SEGMENT.setPixelColor(i, SEGMENT.color_from_palette(PRNG16 >> 8, false, false, 0, globalPeakBri));
      }
    }
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_FAIRY[] PROGMEM = "Fairy@!,# of flashers;!,!;!";


/*
 * Fairytwinkle. Like Colortwinkle, but starting from all lit and not relying on strip.getPixelColor
 * Warning: Uses 4 bytes of segment data per pixel
 */
uint16_t mode_fairytwinkle() {
  unsigned dataSize = sizeof(flasher) * SEGLEN;
  if (!SEGENV.allocateData(dataSize)) return mode_static(); //allocation failed
  Flasher* flashers = reinterpret_cast<Flasher*>(SEGENV.data);
  unsigned now16 = strip.now & 0xFFFF;
  uint16_t PRNG16 = 5100 + strip.getCurrSegmentId();

  unsigned riseFallTime = 400 + (255-SEGMENT.speed)*3;
  unsigned maxDur = riseFallTime/100 + ((255 - SEGMENT.intensity) >> 2) + 13 + ((255 - SEGMENT.intensity) >> 1);

  for (unsigned f = 0; f < SEGLEN; f++) {
    uint16_t stateTime = now16 - flashers[f].stateStart;
    //random on/off time reached, switch state
    if (stateTime > flashers[f].stateDur * 100) {
      flashers[f].stateOn = !flashers[f].stateOn;
      bool init = !flashers[f].stateDur;
      if (flashers[f].stateOn) {
        flashers[f].stateDur = riseFallTime/100 + ((255 - SEGMENT.intensity) >> 2) + hw_random8(12 + ((255 - SEGMENT.intensity) >> 1)) +1;
      } else {
        flashers[f].stateDur = riseFallTime/100 + hw_random8(3 + ((255 - SEGMENT.speed) >> 6)) +1;
      }
      flashers[f].stateStart = now16;
      stateTime = 0;
      if (init) {
        flashers[f].stateStart -= riseFallTime; //start lit
        flashers[f].stateDur = riseFallTime/100 + hw_random8(12 + ((255 - SEGMENT.intensity) >> 1)) +5; //fire up a little quicker
        stateTime = riseFallTime;
      }
    }
    if (flashers[f].stateOn && flashers[f].stateDur > maxDur) flashers[f].stateDur = maxDur; //react more quickly on intensity change
    if (stateTime > riseFallTime) stateTime = riseFallTime; //for flasher brightness calculation, fades in first 255 ms of state
    unsigned fadeprog = 255 - ((stateTime * 255) / riseFallTime);
    uint8_t flasherBri = (flashers[f].stateOn) ? 255-gamma8(fadeprog) : gamma8(fadeprog);
    unsigned lastR = PRNG16;
    unsigned diff = 0;
    while (diff < 0x4000) { //make sure colors of two adjacent LEDs differ enough
      PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; //next 'random' number
      diff = (PRNG16 > lastR) ? PRNG16 - lastR : lastR - PRNG16;
    }
    SEGMENT.setPixelColor(f, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(PRNG16 >> 8, false, false, 0), flasherBri));
  }
  return FRAMETIME;
}
static const char _data_FX_MODE_FAIRYTWINKLE[] PROGMEM = "Fairytwinkle@!,!;!,!;!;;m12=0"; //pixels


/*
 * Firing comets from one end. "Lighthouse"
 */
uint16_t mode_comet(void) {
  if (SEGLEN <= 1) return mode_static();
  unsigned counter = (strip.now * ((SEGMENT.speed >>2) +1)) & 0xFFFF;
  unsigned index = (counter * SEGLEN) >> 16;
  if (SEGENV.call == 0) SEGENV.aux0 = index;

  SEGMENT.fade_out(SEGMENT.intensity);

  SEGMENT.setPixelColor( index, SEGMENT.color_from_palette(index, true, PALETTE_SOLID_WRAP, 0));
  if (index > SEGENV.aux0) {
    for (unsigned i = SEGENV.aux0; i < index ; i++) {
       SEGMENT.setPixelColor( i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
    }
  } else if (index < SEGENV.aux0 && index < 10) {
    for (unsigned i = 0; i < index ; i++) {
       SEGMENT.setPixelColor( i, SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0));
    }
  }
  SEGENV.aux0 = index++;

  return FRAMETIME;
}
static const char _data_FX_MODE_COMET[] PROGMEM = "Lighthouse@!,Fade rate;!,!;!";




/*
 * Does the "standby-breathing" of well known i-Devices.
 */
uint16_t mode_breath(void) {
  unsigned var = 0;
  unsigned counter = (strip.now * ((SEGMENT.speed >> 3) +10)) & 0xFFFFU;
  counter = (counter >> 2) + (counter >> 4); //0-16384 + 0-2048
  if (counter < 16384) {
    if (counter > 8192) counter = 8192 - (counter - 8192);
    var = sin16_t(counter) / 103; //close to parabolic in range 0-8192, max val. 23170
  }

  uint8_t lum = 30 + var;
  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0), lum));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_BREATH[] PROGMEM = "Breathe@!;!,!;!;01";


/*
 * Fades the LEDs between two colors
 */
uint16_t mode_fade(void) {
  unsigned counter = (strip.now * ((SEGMENT.speed >> 3) +10));
  uint8_t lum = triwave16(counter) >> 8;

  for (unsigned i = 0; i < SEGLEN; i++) {
    SEGMENT.setPixelColor(i, color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0), lum));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_FADE[] PROGMEM = "Fade@!;!,!;!;01";


/*
 * Scan mode parent function
 */
uint16_t scan(bool dual) {
  if (SEGLEN <= 1) return mode_static();
  uint32_t cycleTime = 750 + (255 - SEGMENT.speed)*150;
  uint32_t perc = strip.now % cycleTime;
  int prog = (perc * 65535) / cycleTime;
  int size = 1 + ((SEGMENT.intensity * SEGLEN) >> 9);
  int ledIndex = (prog * ((SEGLEN *2) - size *2)) >> 16;

  if (!SEGMENT.check2) SEGMENT.fill(SEGCOLOR(1));

  int led_offset = ledIndex - (SEGLEN - size);
  led_offset = abs(led_offset);

  if (dual) {
    for (int j = led_offset; j < led_offset + size; j++) {
      unsigned i2 = SEGLEN -1 -j;
      SEGMENT.setPixelColor(i2, SEGMENT.color_from_palette(i2, true, PALETTE_SOLID_WRAP, (SEGCOLOR(2))? 2:0));
    }
  }

  for (int j = led_offset; j < led_offset + size; j++) {
    SEGMENT.setPixelColor(j, SEGMENT.color_from_palette(j, true, PALETTE_SOLID_WRAP, 0));
  }

  return FRAMETIME;
}


/*
 * Runs a single pixel back and forth.
 */
uint16_t mode_scan(void) {
  return scan(false);
}
static const char _data_FX_MODE_SCAN[] PROGMEM = "Scan@!,# of dots,,,,,Overlay;!,!,!;!";


/*
 * Runs two pixel back and forth in opposite directions.
 */
uint16_t mode_dual_scan(void) {
  return scan(true);
}
static const char _data_FX_MODE_DUAL_SCAN[] PROGMEM = "Scan Dual@!,# of dots,,,,,Overlay;!,!,!;!";


/*
 * Cycles all LEDs at once through a rainbow.
 */
uint16_t mode_rainbow(void) {
  unsigned counter = (strip.now * ((SEGMENT.speed >> 2) +2)) & 0xFFFF;
  counter = counter >> 8;

  if (SEGMENT.intensity < 128){
    SEGMENT.fill(color_blend(SEGMENT.color_wheel(counter),WHITE,uint8_t(128-SEGMENT.intensity)));
  } else {
    SEGMENT.fill(SEGMENT.color_wheel(counter));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_RAINBOW[] PROGMEM = "Colorloop@!,Saturation;;!;01";


/*
 * Cycles a rainbow over the entire string of LEDs.
 */
uint16_t mode_rainbow_cycle(void) {
  unsigned counter = (strip.now * ((SEGMENT.speed >> 2) +2)) & 0xFFFF;
  counter = counter >> 8;

  for (unsigned i = 0; i < SEGLEN; i++) {
    //intensity/29 = 0 (1/16) 1 (1/8) 2 (1/4) 3 (1/2) 4 (1) 5 (2) 6 (4) 7 (8) 8 (16)
    uint8_t index = (i * (16 << (SEGMENT.intensity /29)) / SEGLEN) + counter;
    SEGMENT.setPixelColor(i, SEGMENT.color_wheel(index));
  }

  return FRAMETIME;
}
static const char _data_FX_MODE_RAINBOW_CYCLE[] PROGMEM = "Rainbow@!,Size;;!";


/*
 * Alternating pixels running function.
 */
static uint16_t running(uint32_t color1, uint32_t color2, bool theatre = false) {
  int width = (theatre ? 3 : 1) + (SEGMENT.intensity >> 4);  // window
  uint32_t cycleTime = 50 + (255 - SEGMENT.speed);
  uint32_t it = strip.now / cycleTime;
  bool usePalette = color1 == SEGCOLOR(0);

  for (unsigned i = 0; i < SEGLEN; i++) {
    uint32_t col = color2;
    if (usePalette) color1 = SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0);
    if (theatre) {
      if ((i % width) == SEGENV.aux0) col = color1;
    } else {
      int pos = (i % (width<<1));
      if ((pos < SEGENV.aux0-width) || ((pos >= SEGENV.aux0) && (pos < SEGENV.aux0+width))) col = color1;
    }
    SEGMENT.setPixelColor(i,col);
  }

  if (it != SEGENV.step) {
    SEGENV.aux0 = (SEGENV.aux0 +1) % (theatre ? width : (width<<1));
    SEGENV.step = it;
  }
  return FRAMETIME;
}


/*
 * Theatre-style crawling lights.
 * Inspired by the Adafruit examples.
 */
uint16_t mode_theater_chase(void) {
  return running(SEGCOLOR(0), SEGCOLOR(1), true);
}
static const char _data_FX_MODE_THEATER_CHASE[] PROGMEM = "Theater@!,Gap size;!,!;!";


/*
 * Theatre-style crawling lights with rainbow effect.
 * Inspired by the Adafruit examples.
 */
uint16_t mode_theater_chase_rainbow(void) {
  return running(SEGMENT.color_wheel(SEGENV.step), SEGCOLOR(1), true);
}
static const char _data_FX_MODE_THEATER_CHASE_RAINBOW[] PROGMEM = "Theater Rainbow@!,Gap size;,!;!";


/*
 * Running lights effect with smooth sine transition base.
 */
static uint16_t running_base(bool saw, bool dual=false) {
  unsigned x_scale = SEGMENT.intensity >> 2;
  uint32_t counter = (strip.now * SEGMENT.speed) >> 9;

  for (unsigned i = 0; i < SEGLEN; i++) {
    unsigned a = i*x_scale - counter;
    if (saw) {
      a &= 0xFF;
      if (a < 16)
      {
        a = 192 + a*8;
      } else {
        a = map(a,16,255,64,192);
      }
      a = 255 - a;
    }
    uint8_t s = dual ? sin_gap(a) : sin8_t(a);
    uint32_t ca = color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 0), s);
    if (dual) {
      unsigned b = (SEGLEN-1-i)*x_scale - counter;
      uint8_t t = sin_gap(b);
      uint32_t cb = color_blend(SEGCOLOR(1), SEGMENT.color_from_palette(i, true, PALETTE_SOLID_WRAP, 2), t);
      ca = color_blend(ca, cb, uint8_t(127));
    }
    SEGMENT.setPixelColor(i, ca);
  }

  return FRAMETIME;
}


/*
 * Running lights in opposite directions.
 * Idea: Make the gap width controllable with a third slider in the future
 */
uint16_t mode_running_dual(void) {
  return running_base(false, true);
}
static const char _data_FX_MODE_RUNNING_DUAL[] PROGMEM = "Running Dual@!,Wave width;L,!,R;!";


/*
 * Running lights effect with smooth sine transition.
 */
uint16_t mode_running_lights(void) {
  return running_base(false);
}
static const char _data_FX_MODE_RUNNING_LIGHTS[] PROGMEM = "Running@!,Wave width;!,!;!";


/*
 * Running lights effect with sawtooth transition.
 */
uint16_t mode_saw(void) {
  return running_base(true);
}
static const char _data_FX_MODE_SAW[] PROGMEM = "Saw@!,Width;!,!;!";
