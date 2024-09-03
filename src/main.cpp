#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define SERVICE_UUID "360a7a3f-8c76-4881-a7ea-344c8d7218c5"
#define CHARACTERISTIC_UUID "28884940-22b9-40ff-8586-ef22ea9e09a8"
#define DESCRIPTOR_UUID "57e50940-8bbe-4b5e-94ba-26685a420aef"

#define DEVINFO_UUID (uint16_t)0x180a
#define DEVINFO_MANUFACTURER_UUID (uint16_t)0x2a29
#define DEVINFO_NAME_UUID (uint16_t)0x2a24
#define DEVINFO_SERIAL_UUID (uint16_t)0x2a25


static bool s_deviceConnected = false;
static BLECharacteristic* s_bleChar;
static uint8_t txValue = 0;

class MyBLEServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer* server) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connected!");
    display.display();
  }

  void onDisconnect(BLEServer* server) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Disconnected!");
    display.display();
    
    //Need to advertise for new clients. This allows 1 client at a time
    //re-advertising on connect would allow multiple
    BLEDevice::startAdvertising();
  }
};

class MyBLECallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic* bleChar, esp_ble_gatts_cb_param_t* param) override {
    onWrite(bleChar);
  }

  void onWrite(BLECharacteristic* bleChar) override {
    std::string val = bleChar->getValue();
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(val.c_str());
    display.display();
  }

  void onRead(BLECharacteristic* bleChar, esp_ble_gatts_cb_param_t* param) override {
    std::string val = bleChar->getValue();
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(val.c_str());
    display.display();
  }

  void onNotify(BLECharacteristic* bleChar) override {
    std::string val = bleChar->getValue();
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(val.c_str());
    display.display();
  }

  void onStatus(BLECharacteristic* bleChar, Status s, uint32_t code) override {
    std::string val = bleChar->getValue();
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("%s :: %d\n", val.c_str(), s);
    display.display();

  }
};

void bleServer() {
 //BLE init
  display.println("Starting BLE Server...");
  display.display();

  BLEDevice::init("Jason's Bluetooth Server!");
  BLEServer* bleServer = BLEDevice::createServer();
  BLEService* bleService = bleServer->createService(SERVICE_UUID);
  BLECharacteristic* bleChar = 
  bleService->createCharacteristic(CHARACTERISTIC_UUID,
                                    BLECharacteristic::PROPERTY_NOTIFY |
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE);

  bleChar->setValue("Hello BLE World, Collin!");
  bleService->start();

  BLEAdvertising* bleAdvert = BLEDevice::getAdvertising();
  bleAdvert->addServiceUUID(SERVICE_UUID);
  bleAdvert->setScanResponse(true);
  //bleAdvert->setMinPreferred(0x06);
  //bleAdvert->setMaxPreferred(0x12);
  //BLEDevice::startAdvertising();
  bleAdvert->start();

  display.println("BLE Server Configured!");
  display.display();
}

void bleServer2() {
  BLEDevice::init("JES1");
  BLEServer* bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new MyBLEServerCallbacks());

  BLEService* bleService = bleServer->createService(SERVICE_UUID);

  s_bleChar = bleService->createCharacteristic(CHARACTERISTIC_UUID,
                                    BLECharacteristic::PROPERTY_NOTIFY
                                    | BLECharacteristic::PROPERTY_READ
                                    | BLECharacteristic::PROPERTY_WRITE
                                    | BLECharacteristic::PROPERTY_WRITE_NR);

  s_bleChar->addDescriptor(new BLE2902());
  s_bleChar->setCallbacks(new MyBLECallbacks());

  BLECharacteristic *characteristic = bleService->createCharacteristic(DEVINFO_MANUFACTURER_UUID, BLECharacteristic::PROPERTY_READ);
  characteristic->setValue("Sobotka");
  characteristic = bleService->createCharacteristic(DEVINFO_NAME_UUID, BLECharacteristic::PROPERTY_READ);
  characteristic->setValue("JES1");
  characteristic = bleService->createCharacteristic(DEVINFO_SERIAL_UUID, BLECharacteristic::PROPERTY_READ);
  String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
  characteristic->setValue(chipId.c_str());

  bleService->start();

  BLEAdvertising* adv = bleServer->getAdvertising();
  adv->addServiceUUID(bleService->getUUID());
  BLEAdvertisementData advData;
  advData.setName("JES1");
  advData.setCompleteServices(BLEUUID(SERVICE_UUID));
  adv->setAdvertisementData(advData);
  adv->start();

  display.println("Waiting for clients...");
  display.display();
}

void bleServerLoop() {
  if(s_deviceConnected) {
    display.printf("Sent value: %d\n", txValue);
    display.display();
    s_bleChar->setValue(&txValue, 1);
    s_bleChar->notify();
    txValue++;
  }
}

static BLEScan* s_bleScanner;
int scanTime = 5;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      display.display();
      delay(250);
    }
};

void bleScanner() {
  display.println("Starting BLE Scanner...");
  display.display();

  BLEDevice::init("BLEScanner");
  s_bleScanner = BLEDevice::getScan();
  s_bleScanner->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  s_bleScanner->setActiveScan(true);
  s_bleScanner->setInterval(100);
  s_bleScanner->setWindow(99);

}

void bleScanLoop() {
  BLEScanResults results = s_bleScanner->start(scanTime, false);
  display.clearDisplay();
  display.setCursor(0,0);
  display.printf("Devices Found: %d\n", results.getCount());
  display.display();
  delay(2000);
  s_bleScanner->clearResults();

}

void setup() {
  Serial.begin(115200);

  //init screen
  Wire.begin(5, 4);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();


  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  //bleServer();
  bleServer2();
  //bleScanner();
}

void loop() {
  // put your main code here, to run repeatedly:
  //bleScanLoop();
  //bleServerLoop();
  delay(1000);
}