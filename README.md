# LED Lighting Effects and Utility Functions

I find that swipe is actually both an light mode AND also a transition effect.

chrome://flags/ -> Enable BT 

## Effects Table

| **Effect Name**           | **Description**                                          | **Parameters**                       |
| ------------------------- | -------------------------------------------------------- | ------------------------------------ |
| **COLOR**                | Static color, no animation                               | `color`                              |
| **BLINK**                 | Blinking effect, color blinks on and off                 | `color`, `speed`                     |
| **BREATH**                | Smooth breathing (fade in and out)                       | `color`, `speed`, `intensity`        |
| **COLOR_WIPE**            | Wipe effect where a color "wipes" across the LEDs        | `color`, `speed`, `direction`        |
| **COLOR_WIPE_RANDOM**     | Random color wipe                                        | `speed`, `direction`                 |
| **RANDOM_COLOR**          | Randomly changes the colors of the LEDs                  | `speed`                              |
| **COLOR_SWIPE**           | Smooth transition of a color across the strip            | `color`, `speed`, `direction`        |
| **DYNAMIC**               | Continuously changing colors in a dynamic pattern        | `speed`, `intensity`                 |
| **RAINBOW**               | Traditional rainbow cycling effect                       | `speed`, `direction`                 |
| **RAINBOW_CYCLE**         | Rainbow colors continuously shift                        | `speed`, `direction`                 |
| **SCAN**                  | A "scan" effect that moves across the LEDs               | `speed`, `direction`                 |
| **FADE**                  | Smooth fade effect from one color to another             | `color`, `speed`, `intensity`        |
| **THEATER_CHASE**         | Classic theater chase effect, lights chase in a sequence | `color`, `speed`, `direction`        |
| **THEATER_CHASE_RAINBOW** | Rainbow version of the theater chase                     | `speed`                              |
| **RUNNING_LIGHTS**        | Lights "run" across the strip                            | `color`, `speed`, `direction`        |
| **SAW**                   | Sawtooth pattern, lights cycle in a saw-tooth wave       | `speed`                              |
| **TWINKLE**               | Twinkling stars effect                                   | `color`, `speed`, `intensity`        |
| **DISSOLVE**              | Dissolve effect, colors fade in and out                  | `speed`, `intensity`                 |
| **DISSOLVE_RANDOM**       | Dissolve with random colors                              | `speed`, `intensity`                 |
| **SPARKLE**               | Sparkling effect with random bright spots                | `color`, `speed`, `intensity`        |
| **FLASH_SPARKLE**         | Flashing sparkling effect                                | `color`, `speed`, `intensity`        |
| **HYPER_SPARKLE**         | Intense sparkle effect                                   | `color`, `speed`, `intensity`        |
| **STROBE**                | Strobe light effect with fast flashing                   | `color`, `speed`, `intensity`        |
| **STROBE_RAINBOW**        | Strobe effect with rainbow flashing                      | `speed`                              |
| **MULTI_STROBE**          | Multiple strobe effects with varying speeds              | `color`, `speed`, `intensity`        |
| **BLINK_RAINBOW**         | Rainbow color blink effect                               | `speed`                              |
| **ANDROID**               | Simulates the Android robot "eye" pattern                | `color`, `speed`                     |
| **CHASE_COLOR**           | A chase effect with a single color                       | `color`, `speed`, `direction`        |
| **CHASE_RANDOM**          | A chase effect with random colors                        | `speed`, `direction`                 |
| **CHASE_RAINBOW**         | A chase effect with rainbow colors                       | `speed`, `direction`                 |
| **CHASE_FLASH**           | Chase effect where LEDs flash in a sequence              | `color`, `speed`, `direction`        |
| **CHASE_FLASH_RANDOM**    | Randomized flashing chase effect                         | `speed`, `direction`                 |
| **CHASE_RAINBOW_WHITE**   | Chase effect with rainbow and white colors               | `speed`, `direction`                 |
| **COLORFUL**              | A colorful effect with multi-colored transitions         | `speed`, `intensity`                 |
| **TRAFFIC_LIGHT**         | Simulates a traffic light pattern                        | `speed`, `intensity`                 |
| **COLOR_SWIPE_RANDOM**    | Randomized color sweep effect                            | `speed`, `direction`                 |
| **RUNNING_COLOR**         | A running color pattern across the LEDs                  | `color`, `speed`, `direction`        |
| **AURORA**                | Aurora-style wave effect                                 | `speed`, `intensity`                 |
| **RUNNING_RANDOM**        | Random running lights effect                             | `speed`, `direction`                 |
| **LARSON_SCANNER**        | Larson scanner effect (back-and-forth motion)            | `color`, `speed`                     |
| **COMET**                 | Simulates a comet-like effect with a light trail         | `color`, `speed`, `direction`        |
| **FIREWORKS**             | Simulates fireworks display                              | `speed`, `intensity`                 |
| **RAIN**                  | Raindrop or water drop effect                            | `speed`, `intensity`                 |
| **TETRIX**                | Grid-like wipe effect (formerly Merry Christmas)         | `color`, `speed`, `direction`        |
| **FIRE_FLICKER**          | Simulates the flickering of a fire                       | `speed`, `intensity`                 |
| **GRADIENT**              | Gradient effect transitioning through multiple colors    | `speed`, `direction`, `color`        |
| **LOADING**               | Loading-style effect where lights gradually fill up      | `color`, `speed`                     |
| **ROLLINGBALLS**          | Simulates balls rolling along the LED strip              | `color`, `speed`                     |
| **FAIRY**                 | Fairy-style twinkling lights effect                      | `speed`, `intensity`                 |
| **TWO_DOTS**              | Two dots moving across the strip with color variation    | `speed`, `color`                     |
| **FAIRYTWINKLE**          | Fairy twinkle effect with magical light spots            | `speed`, `color`                     |
| **RUNNING_DUAL**          | Two running effects happening simultaneously             | `speed`, `color`                     |
| **IMAGE**                 | Image-based effect simulating an image across LEDs       | `image_data`, `speed`                |
| **TRICOLOR_CHASE**        | A chase effect with three colors                         | `speed`, `direction`                 |
| **TRICOLOR_WIPE**         | Wipe effect with three colors                            | `speed`, `direction`                 |
| **TRICOLOR_FADE**         | Fade effect with three colors                            | `speed`, `direction`                 |
| **LIGHTNING**             | Lightning bolt effect with quick flashes                 | `color`, `speed`, `intensity`        |
| **ICU**                   | A simulated “ICU” effect with blinking lights            | `color`, `speed`                     |
| **MULTI_COMET**           | Multiple comet effects running in parallel               | `color`, `speed`, `intensity`        |
| **DUAL_LARSON_SCANNER**   | A dual Larson scanner effect                             | `color`, `speed`                     |
| **RANDOM_CHASE**          | A randomized chase effect with no fixed pattern          | `color`, `speed`, `direction`        |
| **OSCILLATE**             | Oscillating lights moving in a wave-like pattern         | `speed`, `intensity`                 |
| **PRIDE_2015**            | Pride flag-style rainbow effect                          | `speed`, `direction`                 |
| **JUGGLE**                | Juggling pattern with lights bouncing back and forth     | `speed`, `intensity`                 |
| **PALETTE**               | Palette-based effect with blended colors                 | `speed`, `intensity`, `palette_id`   |
| **FIRE_2012**             | A fire effect simulating fire-like behavior              | `speed`, `intensity`                 |
| **COLORWAVES**            | Waves of color moving across the strip                   | `speed`, `direction`                 |
| **BPM**                   | Beats per minute-based effect (rhythm-based)             | `speed`, `intensity`                 |
| **COLORTWINKLE**          | Similar to twinkle but with color variations             | `speed`, `intensity`                 |
| **LAKE**                  | A "lake" effect with waves of light                      | `speed`, `intensity`                 |
| **METEOR**                | Simulates meteors shooting across the strip              | `speed`, `intensity`                 |

## Utility Functions Overview

### 1. **Color Management**:
   - RGB to HSV Conversion
   - HSV to RGB Conversion
   - Gamma Correction for LED color adjustments

### 2. **Brightness Control**:
   - Adjust individual LED brightness
   - Adjust global brightness for the entire LED strip

### 3. **Segment Management**:
   - Define LED segments based on lengths
   - Apply different lighting effects to individual segments

### 4. **Color Temperature (Kelvin) Adjustment**:
   - Convert Kelvin values to RGB colors for adjusting color temperature

### 5. **Lighting Effects**:
   - Apply effects like Static Color, Blinking, Fade, Rainbow Cycle, etc., across the LED strip
   - Implement specific effects like Rainbow Cycle, Color Wipe, Twinkle, and more

### 7. **Advanced Effects Management**:
   - Render images or patterns across the LED strip
   - Handle multiple effects running simultaneously
   - Reset the LED strip to its default state after effects

### 8. **Additional Utility Functions**:
   - Generate random or noise-based patterns for dynamic lighting effects
   - Combine multiple lighting patterns or effects for complex designs