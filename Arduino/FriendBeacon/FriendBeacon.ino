/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <EEPROM.h>


int scanTime = 5;  //In seconds
BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String discoveredAddress = advertisedDevice.getAddress().toString();
    if (discoveredAddress.startsWith("dc:06:75")) Serial.printf("%s \n", advertisedDevice.getAddress().toString().c_str());
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  BLEAddress address = BLEDevice::getAddress();
  uint8_t storedValue = EEPROM.read(12);
  sleep(1);
  EEPROM.begin(1);
  EEPROM.write(12, 44);
  EEPROM.commit();
  Serial.printf("Stored value: %d\n", storedValue);
  BLEDevice::startAdvertising();
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  Serial.printf("My address: %s\n", address.toString().c_str());
}

void loop() {
  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
  delay(2000);
}
