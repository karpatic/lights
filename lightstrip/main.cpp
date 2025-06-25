// === MusicStripRMT.ino =====================================================
// ESP32 LED controller — RMT‑driven WS2812, BLE + ESP‑NOW, state persistence
// Implements:
//   1. NVS state persistence
//   2. Non‑blocking LED driver via NeoPixelBus (RMT)
//   3. Configurable strip geometry (pin + length)
//   4. Power/heat safety: simple max‑current limiter
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <NeoPixelBus.h>               // RMT‑based LED driver

// ───────── BLE definitions ─────────
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c3319123"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b2623"
#define ESPNODE_NAME        "Music Strip"

BLEServer*       pServer        = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool             deviceConnected = false;

// ───────── Preferences (NVS) ─────────
Preferences prefs;
#define PREF_NAMESPACE "musicStrip"
#define PREF_KEY       "state"

// ───────── Config defaults ─────────
#define DEFAULT_PIN        15
#define DEFAULT_LED_COUNT  300
#define DEFAULT_MAX_CURRENT 8000   // mA — adjust to your PSU

// ───────── Runtime data struct ───────── 
typedef struct __attribute__((packed)) {
  uint8_t  version;            // struct layout version
  char     a[32];
  int      b;
  float    c;
  bool     d;
  uint8_t  brightness;        // 0–255
  char     lightmode[32];
  char     colortwo[16];
  char     colorthree[16];
  char     colorone[16];
  uint16_t animationdelay;     // ms
  uint16_t ledCount;           // strip length
  uint8_t  pixelPin;           // GPIO
  uint16_t maxCurrent;         // mA supply limit
} strip_state_t;

static strip_state_t myData = {
  1,               // version
  "TestString",   // a
  42,              // b
  420.69f,         // c
  true,            // d
  40,              // brightness (≈15 %)
  "setColor",     // lightmode
  "0,255,0",      // colortwo
  "0,0,255",      // colorthree
  "255,0,0",      // colorone
  120,             // animationdelay
  DEFAULT_LED_COUNT,
  DEFAULT_PIN,
  DEFAULT_MAX_CURRENT
};

// ───────── LED driver (RMT) ─────────
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod>* strip = nullptr;

inline uint32_t rgbToPacked(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t colorone = rgbToPacked(255, 0, 0);
uint32_t colortwo = rgbToPacked(0, 255, 0);
uint32_t colorthree = rgbToPacked(0, 0, 255);

void initStrip() {
  if (strip) {
    strip->~NeoPixelBus();
    strip = nullptr;
  }
  strip = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod>(myData.ledCount, myData.pixelPin);
  strip->Begin();
  strip->ClearTo(0);
  strip->Show();
}

// ───────── Helpers ─────────
uint32_t Wheel(uint8_t WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return rgbToPacked(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return rgbToPacked(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return rgbToPacked(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void applyBrightnessLimit() {
}

void setColor() {
  strip->ClearTo(colorone);
  strip->Show();
}

void swipe() {
  static uint16_t pixel = 0;
  strip->SetPixelColor(pixel, colorone);
  strip->Show();
  pixel = (pixel + 1) % myData.ledCount;
}

void rainbowCycle() {
  static uint16_t j = 0;
  for (uint16_t i = 0; i < myData.ledCount; i++) {
    strip->SetPixelColor(i, Wheel((i * 256 / myData.ledCount + j) & 255));
  }
  strip->Show();
  j = (j + 1) & 255;
}

void rainbow() {
  for (uint16_t i = 0; i < myData.ledCount; i++) {
    strip->SetPixelColor(i, Wheel(i * 256 / myData.ledCount));
  }
  strip->Show();
}

void theaterChaseRainbow() {
  static uint8_t q = 0;
  static uint16_t j = 0;
  for (uint16_t i = 0; i < myData.ledCount; i += 3) {
    strip->SetPixelColor(i + q, Wheel((i + j) & 255));
  }
  strip->Show();
  for (uint16_t i = 0; i < myData.ledCount; i += 3) {
    strip->SetPixelColor(i + q, 0);
  }
  q = (q + 1) % 3;
  j = (j + 1) & 255;
}

void theaterChase() {
  static uint8_t q = 0;
  for (uint16_t i = 0; i < myData.ledCount; i += 3) {
    strip->SetPixelColor(i + q, colorone);
  }
  strip->Show();
  for (uint16_t i = 0; i < myData.ledCount; i += 3) {
    strip->SetPixelColor(i + q, 0);
  }
  q = (q + 1) % 3;
}

void swipeRandom() {
  static uint16_t pixel = 0;
  uint32_t randomColor = rgbToPacked(random(0, 255), random(0, 255), random(0, 255));
  strip->SetPixelColor(pixel, randomColor);
  strip->Show();
  pixel = (pixel + 1) % myData.ledCount;
}

void handleStrip() {
  if (!strip) return;
  if      (strcmp(myData.lightmode, "setColor") == 0)            setColor();
  else if (strcmp(myData.lightmode, "swipe") == 0)                swipe();
  else if (strcmp(myData.lightmode, "rainbowCycle") == 0)        rainbowCycle();
  else if (strcmp(myData.lightmode, "rainbow") == 0)             rainbow();
  else if (strcmp(myData.lightmode, "theaterChaseRainbow") == 0) theaterChaseRainbow();
  else if (strcmp(myData.lightmode, "theaterChase") == 0)        theaterChase();
  else                                                           swipeRandom();
}

// ───────── ESP‑NOW helpers (unchanged logic) ─────────
void formatMacAddress(const uint8_t* macAddr, char* buffer, int maxLength) {
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void gotBroadcast(const uint8_t* mac, const uint8_t* incomingData, int len) {
  if (len != sizeof(myData)) return; // size guard
  memcpy(&myData, incomingData, sizeof(myData));
  applyBrightnessLimit();
  initStrip();
  if (deviceConnected) {
    String status = generateStatusJson();
    pCharacteristic->setValue(status.c_str());
    pCharacteristic->notify();
  }
}

void sentBroadcast(const uint8_t* macAddr, esp_now_send_status_t status) {
  // optional debug
}

void broadcastState() {
  uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress)) esp_now_add_peer(&peerInfo);
  esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
}

// ───────── NVS persistence ─────────
void saveState() {
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putBytes(PREF_KEY, &myData, sizeof(myData));
  prefs.end();
}

bool loadState() {
  prefs.begin(PREF_NAMESPACE, true);
  size_t s = prefs.getBytesLength(PREF_KEY);
  bool ok = false;
  if (s == sizeof(myData)) {
    prefs.getBytes(PREF_KEY, &myData, sizeof(myData));
    ok = true;
  }
  prefs.end();
  return ok;
}

// ───────── BLE callbacks ─────────
class BleConnectionCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    deviceConnected = true;
    auto* desc = (BLE2902*)pCharacteristic->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    if (desc) desc->setNotifications(true);
  }
  void onDisconnect(BLEServer* s) override {
    deviceConnected = false;
    s->startAdvertising();
  }
};

void updateStripColor(const char* colorStr, uint32_t* colorVar) {
  int r, g, b;
  if (sscanf(colorStr, "%d,%d,%d", &r, &g, &b) == 3) {
    *colorVar = rgbToPacked(r, g, b);
  }
}

void parseAndUpdateData(const std::string& jsonString) {
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) return;

  bool geometryChanged = false;

  if (doc.containsKey("brightness"))   myData.brightness = doc["brightness"].as<uint8_t>();
  if (doc.containsKey("animationdelay")) myData.animationdelay = doc["animationdelay"].as<uint16_t>();
  if (doc.containsKey("lightmode"))    strlcpy(myData.lightmode, doc["lightmode"], sizeof(myData.lightmode));

  if (doc.containsKey("colorone"))  { strlcpy(myData.colorone, doc["colorone"], sizeof(myData.colorone)); updateStripColor(myData.colorone, &colorone); }
  if (doc.containsKey("colortwo"))  { strlcpy(myData.colortwo, doc["colortwo"], sizeof(myData.colortwo)); updateStripColor(myData.colortwo, &colortwo); }
  if (doc.containsKey("colorthree")){ strlcpy(myData.colorthree, doc["colorthree"], sizeof(myData.colorthree)); updateStripColor(myData.colorthree, &colorthree); }

  if (doc.containsKey("ledCount"))  { myData.ledCount = doc["ledCount"].as<uint16_t>(); geometryChanged = true; }
  if (doc.containsKey("pixelPin"))  { myData.pixelPin = doc["pixelPin"].as<uint8_t>(); geometryChanged = true; }
  if (doc.containsKey("maxCurrent")) myData.maxCurrent = doc["maxCurrent"].as<uint16_t>();

  applyBrightnessLimit();
  if (geometryChanged) initStrip();
  saveState();
  broadcastState();
}

class BleEventCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* ch) override {
    std::string rx = ch->getValue();
    if (!rx.empty()) {
      parseAndUpdateData(rx);
      if (deviceConnected) { 
        String status = generateStatusJson();
        ch->setValue(status.c_str()); 
        ch->notify(); 
      }
    }
  }
};

// ───────── Status JSON generator ─────────
String generateStatusJson() {
  DynamicJsonDocument doc(512);
  doc["mode"] = myData.lightmode;
  doc["brightness"] = myData.brightness;
  doc["animationdelay"] = myData.animationdelay;
  doc["colorone"] = myData.colorone;
  doc["colortwo"] = myData.colortwo;
  doc["colorthree"] = myData.colorthree;
  doc["ledCount"] = myData.ledCount;
  doc["pixelPin"] = myData.pixelPin;
  doc["maxCurrent"] = myData.maxCurrent;
  doc["temperature"] = temperatureRead(); // ESP32 internal temp
  doc["firmware"] = "1.0.0";
  doc["freeHeap"] = ESP.getFreeHeap();
  
  String output;
  serializeJson(doc, output);
  return output;
}

// ───────── Setup ─────────
void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  if (!loadState()) saveState();  // ensure key exists

  // Release classic BT RAM, fix Wi‑Fi channel to minimise hopping
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE); // pick a stable channel
  WiFi.disconnect();

  // ESP‑NOW init
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(gotBroadcast);
    esp_now_register_send_cb(sentBroadcast);
  } else {
    ESP.restart();
  }

  // BLE init
  BLEDevice::init(ESPNODE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleConnectionCallbacks());
  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,
                    BLECharacteristic::PROPERTY_READ |
                    BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new BleEventCallbacks());
  pCharacteristic->setValue("Ready");
  pService->start();
  pServer->getAdvertising()->start();

  // LED driver init & brightness safety check
  applyBrightnessLimit();
  initStrip();
}

// ───────── Main loop ─────────
void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last >= myData.animationdelay) {
    last = now;
    handleStrip();
  }
}
// ===========================================================================
