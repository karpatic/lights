#include "lighting.h"
#include "communications.h"
#include <Arduino.h>

// Include all base layer mode files
#include "modes/base/pacifica.cpp"
#include "modes/base/sunrise.cpp"
#include "modes/base/aurora.cpp"
#include "modes/base/static.cpp"
#include "modes/base/static_tri.cpp"
#include "modes/base/perlin_move.cpp"
#include "modes/base/stream.cpp"
#include "modes/base/palette.cpp"
#include "modes/base/plasma.cpp"

// Include all modifier layer mode files
#include "modes/modifier/percent.cpp"
#include "modes/modifier/percent_tri.cpp"
#include "modes/modifier/shift.cpp"
#include "modes/modifier/washing_machine.cpp"
#include "modes/modifier/blink.cpp"
#include "modes/modifier/blink_toggle.cpp"
#include "modes/modifier/blink_random.cpp"
#include "modes/modifier/heartbeat.cpp"
#include "modes/modifier/twinkles.cpp"

// Include overlay layer mode files
#include "modes/overlay/swipe.cpp"
#include "modes/overlay/swipe_random.cpp"
#include "modes/overlay/colorloop.cpp"
#include "modes/overlay/breath.cpp"
#include "modes/overlay/sweep.cpp"
#include "modes/overlay/sweep_dual.cpp"
#include "modes/overlay/theater.cpp"
#include "modes/overlay/fireworks.cpp"
#include "modes/overlay/juggle.cpp"
#include "modes/overlay/bouncing_balls.cpp"
#include "modes/overlay/tetrix.cpp"
#include "modes/overlay/meteor.cpp"
#include "modes/overlay/candle.cpp"
 

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

//
// Modifier Layer Modes
//

//
// Overlay Modes
//


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
  else if (effect == "heartbeat") { mode_heartbeat(data, config); }
  else if (effect == "twinkles") { mode_twinkles(data, config); }
  else if (effect == "swipe") { mode_swipe(data, config); }
  else if (effect == "swiperandom") { mode_swipe_random(data, config); }
  else if (effect == "colorloop") { mode_colorloop(data, config); }
  else if (effect == "breath") { mode_breath(data, config); }
  else if (effect == "sweep") { mode_sweep(data, config); }
  else if (effect == "sweepdual") { mode_sweep_dual(data, config); }
  else if (effect == "theater") { mode_theater(data, config); }
  else if (effect == "fireworks") { mode_fireworks(data, config); }
  else if (effect == "juggle") { mode_juggle(data, config); }
  else if (effect == "bouncingballs") { mode_bouncing_balls(data, config); }
  else if (effect == "meteor") { mode_meteor(data, config); }
  else if (effect == "tetrix") { mode_tetrix(data, config); }
  else if (effect == "perlinmove") { mode_perlin_move(data, config); }
  else if (effect == "stream") { mode_stream(data, config); }
  else if (effect == "palette") { mode_palette(data, config); }
  else if (effect == "plasma") { mode_plasma(data, config); }
  else if (effect == "pacifica") { mode_pacifica(data, config); }
  else if (effect == "sunrise") { mode_sunrise(data, config); }
  else if (effect == "aurora") { mode_aurora(data, config); }
  else if (effect == "candle") { mode_candle(data, config); } 
  else {
    Serial.printf("Unknown effect: %s\n", effect.c_str());
  }
}