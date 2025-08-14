#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <string>

// Define the message structure
typedef struct struct_message {
  int brightness;
  char lightMode[32];
  uint32_t colorOne;
  uint32_t colorTwo;
  uint32_t colorThree;
  int ledCount;
  int maxCurrent;
  uint16_t colorOrder;
  int pixelPin;
  int pixelCount;
  int speed;        // Animation speed (0-100)
  int intensity;    // Effect intensity (0-100)
  int direction;    // Direction: 0=forward, 1=reverse, 2=bounce
  int count;        // Count parameter for effects
  bool updated;
} struct_message;

// Forward declarations
class Adafruit_NeoPixel;

// External data reference
extern struct_message myData;
extern struct_message myOldData;
extern bool deviceConnected;

// External references to strip
extern Adafruit_NeoPixel strip;

// External function references 
extern uint8_t convertBrightness(int valu);

// Data management functions
void cloneData(const struct_message& source, struct_message& destination);

// BLE functions
void initializeBLE();
void parseAndUpdateData(const std::string &jsonString, struct_message &data);

// ESP-NOW functions
void initializeESPNow();
void gotBroadcast(const uint8_t *mac, const uint8_t *incomingData, int len);
void sentBroadcast(const uint8_t *macAddr, esp_now_send_status_t status);
void broadcast(struct struct_message message);
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);

// Utility functions
bool checkMemory();

#endif