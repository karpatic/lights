// Include Libraries
#include <Arduino.h>
#include "lighting.h"
#include "communications.h"

// Available Methods: sine8(), gamma8(), str2order(), ColorHSV(), Color(), 
// rainbow(), getPixelColor, setPixelColor, updateLength(), updateType()
#include <Adafruit_NeoPixel.h>

// Add state tracking 
unsigned long lastHeapCheck = 0;
void handleStrip(); // Forward declarations
void blendAndShow();
void show();

struct_message myData = {
    20,           // brightness (of 100)
    "static",      // lightMode
    0xFF0000,     // colorOne (red)
    0x00FF00,     // colorTwo (green)
    0x0000FF,     // colorThree (blue)
    300,          // ledCount
    8000,         // maxCurrent
    NEO_GRB + NEO_KHZ800,  // colorOrder
    15,           // pixelPin
    300,          // pixelCount
    50,           // speed (default 50)
    75,           // intensity (default 75)
    0,            // direction (default forward)
    false,        // To know if myData was updated
    2             // count (default 2)
}; 
struct_message myOldData; 
Adafruit_NeoPixel strip(myData.pixelCount, myData.pixelPin, myData.colorOrder);
 
void setup() { 
  Serial.begin(115200); 
  Serial.println(F("=== LED Strip Controller Starting ==="));
  Serial.print(F("Initial free heap: "));
  Serial.println(ESP.getFreeHeap()); 
  strip.begin();  
  strip.updateLength(myData.pixelCount);
  strip.setBrightness( convertBrightness(myData.brightness) );
  strip.clear();
  strip.show();
  
  // Initialize strip data arrays
  stripData = new StripData(myData.pixelCount);
  stripDataOld = new StripData(myData.pixelCount);
  // Remove: currentMode = myData.lightMode;
  
  initializeBLE(); // Initialize BLE using communication module
  delay(2000); 
  Serial.println(F("Skipping WiFi/ESP-NOW for now..."));
  Serial.print(F("Final free heap: "));
  Serial.println(ESP.getFreeHeap()); 
  Serial.println(F("=== Initialization Complete ==="));
} 

long oldMillis = 0; // Used to track time for loopInterval
void loop() {  
  long currentMillis = millis();    
  if (currentMillis - oldMillis > 50 ) {
    oldMillis = currentMillis;
    handleStrip();
  } 
}

// For transition blending
StripData* stripData = nullptr;
StripData* stripDataOld = nullptr;
int transitionValue = 0; // Global transition state

// Helper: which effects are passive (no animation over time)
static bool isStaticMode(const String& e) {
  return (e == "static" || e == "statictri" || e == "percent" || e == "percenttri");
}

// callModeFn to update stripData and blend w stripDataOld.
// SPECIAL Instructions: ["shift""] Modes Inherits the LED data from the previous mode
void handleStrip() { 
  // Update strip settings.
  if (myData.updated) { 
    // Save the current stripData as stripDataOld, then create new stripData
    delete stripDataOld;
    stripDataOld = stripData;
    stripData = new StripData(myData.ledCount);

    // If switching to "shift" or "breath" mode, copy colors from old to new stripData
    if ((String(myData.lightMode) == "shift" || String(myData.lightMode) == "breath") && stripDataOld) {
      int count = min(stripDataOld->pixelCount, stripData->pixelCount);
      for (int i = 0; i < count; i++) {
        stripData->setPixelColor(i, stripDataOld->getPixelColor(i));
      }
    }

    cloneData(myData, myOldData);
    Serial.println(F("Updating strip settings...")); 
    strip.setBrightness(convertBrightness(myData.brightness));
    strip.updateLength(myData.ledCount);
    strip.updateType(myData.colorOrder);
    strip.clear();
    strip.show();  
    if (String(myOldData.lightMode) != myData.lightMode) { 
      transitionValue = 100; // Start transition
      Serial.print(F("Starting transition."));
    }
  } 

  String currentMode = String(myData.lightMode);
  // Only run passive effects when an update occurred; dynamic effects every loop
  if (isStaticMode(currentMode)) {
    if (myData.updated) {
      callModeFunction(currentMode, stripData, &myData);
    }
  } else {
    callModeFunction(currentMode, stripData, &myData);
  }

  if (transitionValue < 2) {
    transitionValue = 0;
    show();
  } else {
    transitionValue -= 2;
    String oldEffect = String(myOldData.lightMode);
    // Only advance old passive effect if it is dynamic (i.e., not passive)
    if (!isStaticMode(oldEffect)) {
      callModeFunction(oldEffect, stripDataOld, &myOldData);
    }
    blendAndShow();
  }

  if (myData.updated) { 
    myData.updated = false; // Reset update flag
  }
}

uint32_t blendColors(uint32_t color1, uint32_t color2, int blend) {
  // Extract RGB components
  uint8_t r1 = (color1 >> 16) & 0xFF;
  uint8_t g1 = (color1 >> 8) & 0xFF;
  uint8_t b1 = color1 & 0xFF;
  
  uint8_t r2 = (color2 >> 16) & 0xFF;
  uint8_t g2 = (color2 >> 8) & 0xFF;
  uint8_t b2 = color2 & 0xFF;
  
  // Blend (blend 0-100, where 100 is fully color2)
  uint8_t r = r1 + ((r2 - r1) * blend / 100);
  uint8_t g = g1 + ((g2 - g1) * blend / 100);
  uint8_t b = b1 + ((b2 - b1) * blend / 100);
  
  return strip.Color(r, g, b);
}

void show() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, stripData->getPixelColor(i));
  }
  strip.show();
}

void blendAndShow() {
  // Blend old and new strip data during transition
  for (int i = 0; i < strip.numPixels(); i++) {
    uint32_t oldColor = stripDataOld->getPixelColor(i);
    uint32_t newColor = stripData->getPixelColor(i);
    uint32_t blendedColor = blendColors(oldColor, newColor, 100 - transitionValue);
    strip.setPixelColor(i, blendedColor);
  }
  strip.show();
}