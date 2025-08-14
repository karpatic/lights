#include "communications.h"
#include <Adafruit_NeoPixel.h>

// BLE globals
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c3319123"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b2623"
#define ESPNAME "Music Strip"

// Convert 0-100 scale to 0-255 PWM scale
uint8_t convertBrightness(int valu) {
  if (valu <= 0) return 0;  // Off
  if (valu >= 100) return 255;  // Max
  return (uint8_t)(valu * 2.55);  // Linear scale: 1=2.55, 50=127.5, 100=255
}

// Add function to clone struct data
void cloneData(const struct_message& source, struct_message& destination) {
  memcpy(&destination, &source, sizeof(struct_message));
}

bool checkMemory() {
  size_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 15000) {
    Serial.print(F("WARNING: Low memory: "));
    Serial.println(freeHeap);
    return false;
  }
  return true;
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0],
           macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void gotBroadcast(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print(F("gotBroadcast: "));
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print(F("Data received: "));
  Serial.println(len); 

  if (deviceConnected) {
    pCharacteristic->setValue("TRIGGERED");
    pCharacteristic->notify();
  }
}

void sentBroadcast(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print(F("gotBroadcast: "));
  char macStr[18];
  Serial.print(F("Packet Sent to: "));
  formatMacAddress(macAddr, macStr, 18);
}

void broadcast(struct struct_message message) {
  Serial.print(F("Broadcast: "));
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress)) {
    esp_now_add_peer(&peerInfo);
  }

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println(F("Broadcast message success"));
  } else {
    Serial.println(F("Unknown error"));
  }
}

class BleConnectionCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    BLE2902 *desc = (BLE2902 *)pCharacteristic->getDescriptorByUUID(
        BLEUUID((uint16_t)0x2902));
    if (desc) {
      desc->setNotifications(true);
    }
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    pServer->startAdvertising();
    Serial.println("Device disconnected");
  }
};

void debugParsedData(const struct_message &data) {
  Serial.println(F("=== Parsed JSON Data ==="));
  Serial.printf("brightness: %d | lightMode: %s\n", data.brightness, data.lightMode);
  Serial.printf("colors: one=0x%06X, two=0x%06X, three=0x%06X\n", data.colorOne, data.colorTwo, data.colorThree);
  Serial.printf("speed: %d | intensity: %d | direction: %d\n", data.speed, data.intensity, data.direction);
  Serial.printf("hardware: maxCurrent=%d | colorOrder: 0x%04X\n", data.maxCurrent, data.colorOrder);
  Serial.printf("pins: pixelPin=%d | pixelCount=%d\n", data.pixelPin, data.pixelCount);
  checkMemory();
  Serial.println(F("========================"));
}

uint16_t parseColorOrder(const char* orderStr) {
  uint16_t order = strip.str2order(orderStr);
  return order + NEO_KHZ800;
}

uint32_t parseColorString(const char* colorStr) {
  int r, g, b;
  if (sscanf(colorStr, "%d,%d,%d", &r, &g, &b) == 3) {
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
  }
  return 0; // Default to black if parsing fails
}

void parseAndUpdateData(const std::string &jsonString, struct_message &data) {
  if (!checkMemory()) {
    Serial.println(F("Cannot parse data - insufficient memory"));
    return;
  }
  
  Serial.print(F("Received JSON: "));
  Serial.println(jsonString.c_str());
  
  DynamicJsonDocument jsonDoc(512);
  DeserializationError error = deserializeJson(jsonDoc, jsonString);

  if (error) {
    Serial.print(F("JSON parsing failed: "));
    Serial.println(error.c_str());
    return;
  } 

  extern struct_message myOldData;
  memcpy(&myOldData, &data, sizeof(struct_message));

  if (jsonDoc.containsKey("brightness")) {
    int brightness = jsonDoc["brightness"];
    data.brightness = constrain(brightness, 0, 100);
  }
  if (jsonDoc.containsKey("lightMode")) {
    strlcpy(data.lightMode, (const char*)jsonDoc["lightMode"], sizeof(data.lightMode));
  }
  if (jsonDoc.containsKey("colorOne")) {
    const char* colorStr = jsonDoc["colorOne"];
    data.colorOne = parseColorString(colorStr);
  }
  if (jsonDoc.containsKey("colorTwo")) {
    const char* colorStr = jsonDoc["colorTwo"];
    data.colorTwo = parseColorString(colorStr);
  }
  if (jsonDoc.containsKey("colorThree")) {
    const char* colorStr = jsonDoc["colorThree"];
    data.colorThree = parseColorString(colorStr);
  }
  if (jsonDoc.containsKey("ledCount")) {
    data.ledCount = jsonDoc["ledCount"];
  }
  if (jsonDoc.containsKey("colorOrder")) {
    const char* orderStr = jsonDoc["colorOrder"];
    data.colorOrder = parseColorOrder(orderStr);
  } else {
    // Default color order if not specified
    data.colorOrder = NEO_RGB + NEO_KHZ800;
  } 
  if (jsonDoc.containsKey("maxCurrent")) {
    data.maxCurrent = jsonDoc["maxCurrent"];
  }
  if (jsonDoc.containsKey("pixelPin")) {
    data.pixelPin = jsonDoc["pixelPin"];
  }
  if (jsonDoc.containsKey("pixelCount")) {
    data.pixelCount = jsonDoc["pixelCount"];
  }
  if (jsonDoc.containsKey("speed")) {
    int speed = jsonDoc["speed"];
    data.speed = constrain(speed, 0, 100);
  }
  if (jsonDoc.containsKey("intensity")) {
    int intensity = jsonDoc["intensity"];
    data.intensity = constrain(intensity, 0, 100);
  }
  if (jsonDoc.containsKey("count")) {
    int count = jsonDoc["count"];
    data.count = constrain(count, 0, 100);
  } 
  if (jsonDoc.containsKey("direction")) {
    int direction = jsonDoc["direction"];
    data.direction = constrain(direction, 0, 2);
  } 
  data.updated = true;
  debugParsedData(data); 
}

class BleEventCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    Serial.print(F("BLE Write received, length: "));
    Serial.println(rxValue.length());
    
    if (rxValue.length() > 0) { 
      parseAndUpdateData(rxValue, myData); 
      
      if (deviceConnected) {
        pCharacteristic->setValue("OK");
        pCharacteristic->notify();
        Serial.println(F("Sent confirmation to client"));
        Serial.println(F("========================"));
      }
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) {
    // Implementation if needed
  }
}; 

void initializeBLE() {
  Serial.println(F("Initializing BLE..."));
  delay(1000);
  
  BLEDevice::init(ESPNAME);
  delay(500);
  
  pServer = BLEDevice::createServer();
  if (pServer) {
    pServer->setCallbacks(new BleConnectionCallbacks());
    delay(200);

    BLEService *pService = pServer->createService(SERVICE_UUID);
    if (pService) {
      delay(200);
      pCharacteristic = pService->createCharacteristic(
          CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE |
                                    BLECharacteristic::PROPERTY_NOTIFY);
      if (pCharacteristic) {
        pCharacteristic->addDescriptor(new BLE2902());
        pCharacteristic->setCallbacks(new BleEventCallbacks());
        pCharacteristic->setValue("Ready");
        delay(200);
        
        pService->start();
        delay(200);
        pServer->getAdvertising()->start();
        Serial.println(F("BLE initialized successfully"));
      }
    }
  }
}

void initializeESPNow() {
  // ESP-NOW initialization code would go here
  Serial.println(F("ESP-NOW initialization skipped for stability"));
}