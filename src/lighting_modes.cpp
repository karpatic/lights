#include "lighting.h"
#include "communications.h"
#include <Arduino.h>
 

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

//
// Base Layer Modes 
//

// Forward declaration for special mode in lighting_modes_special.cpp
void mode_pacifica(StripData* data, const struct_message* config);
void mode_sunrise(StripData* data, const struct_message* config); 
void mode_aurora(StripData* data, const struct_message* config);  
void mode_static(StripData* data, const struct_message* config);
void mode_static_tri(StripData* data, const struct_message* config); 

void mode_stream(StripData* data, const struct_message* config) {}
void mode_palette(StripData* data, const struct_message* config) {}
void mode_plasma(StripData* data, const struct_message* config) {}
void mode_perlin_move(StripData* data, const struct_message* config) {}

//
// Modifier Layer Modes
//

void mode_percent(StripData* data, const struct_message* config); 
void mode_percent_tri(StripData* data, const struct_message* config);


// - SPECIAL - Shift mode - Inherits colors. continuously shifts existing pixel colors
void mode_shift(StripData* data, const struct_message* config);

// - SPECIAL - Shift mode - Inherits colors. continuously shifts existing pixel colors and directions
void mode_washing_machine(StripData* data, const struct_message* config);

// blink between colorOne and black
void mode_blink(StripData* data, const struct_message* config);

// Blink between colorOne and colorTwo
void mode_blink_toggle(StripData* data, const struct_message* config);

// blink random colors
void mode_blink_random(StripData* data, const struct_message* config);

void mode_heartbeat(StripData* data, const struct_message* config) {}
void mode_twinkles(StripData* data, const struct_message* config) {}

// Breath effect - smooth global brightness modulation of a captured base frame
void mode_breath(StripData* data, const struct_message* config);

//
// Overlay Modes
//

// Swipe effect that alternates between colorOne and colorTwo
void mode_swipe(StripData* data, const struct_message* config);

// Swipe effect with random colors
void mode_swipe_random(StripData* data, const struct_message* config);

// Colorloop - cycles all LEDs through rainbow colors
void mode_colorloop(StripData* data, const struct_message* config);
 
// Sweep effect using colorOne - one LED lit at a time moving across strip
void mode_sweep(StripData* data, const struct_message* config);

// Sweep effect with dual colors sweeping in opposite directions.
void mode_sweep_dual(StripData* data, const struct_message* config);

// Candle flicker mode
void mode_candle(StripData* data, const struct_message* config); 
void mode_sinelon_dual(StripData* data, const struct_message* config) {} 
void mode_theater(StripData* data, const struct_message* config) {} 
void mode_fireworks(StripData* data, const struct_message* config) {}
void mode_juggle(StripData* data, const struct_message* config) {}
void mode_bouncing_balls(StripData* data, const struct_message* config) {}
void mode_meteor(StripData* data, const struct_message* config) {}
void mode_tetrix(StripData* data, const struct_message* config) {}


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