#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "Config.h"
#include "FriendManager.h"
#include "EventLogger.h"

// Service UUIDs for app integration
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BATTERY_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define FRIENDS_CHAR_UUID      "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define CONFIG_CHAR_UUID       "9a8ca5cf-3f4b-4d1e-af12-6e1c8b3d5f2a"
#define COMMAND_CHAR_UUID      "7d3f8e2a-5c6b-4a1e-9f3d-2b5e8c4a6f1b"

class BLEManager;

// Forward declarations
class MyAdvertisedDeviceCallbacks;
class MyServerCallbacks;
class CommandCharacteristicCallbacks;

// Global pointer for callbacks
extern BLEManager* g_bleManager;

class BLEManager {
private:
  BLEScan* pBLEScan;
  BLEServer* pServer;
  FriendManager* friendManager;
  EventLogger* eventLogger;

  BLECharacteristic* batteryCharacteristic;
  BLECharacteristic* friendsCharacteristic;
  BLECharacteristic* configCharacteristic;
  BLECharacteristic* commandCharacteristic;

  bool serverStarted;
  bool scanning;
  String myAddress;

public:
  enum ProgramState {
    INIT = 1,
    LOAD_CONFIG = 2,
    NEW_CONFIG = 3,
    STORE_CONFIG = 4,
    LOOKING_FOR_FRIENDS = 5
  };

  ProgramState programState;

  BLEManager(FriendManager* fm, EventLogger* el) :
    pBLEScan(nullptr),
    pServer(nullptr),
    friendManager(fm),
    eventLogger(el),
    batteryCharacteristic(nullptr),
    friendsCharacteristic(nullptr),
    configCharacteristic(nullptr),
    commandCharacteristic(nullptr),
    serverStarted(false),
    scanning(false),
    programState(INIT) {}

  bool begin() {
    try {
      BLEDevice::init("FriendBeacon");
      BLEAddress address = BLEDevice::getAddress();
      myAddress = address.toString().c_str();

      // Start advertising
      BLEDevice::startAdvertising();

      // Setup scan
      pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
      pBLEScan->setActiveScan(true);
      pBLEScan->setInterval(BLE_SCAN_INTERVAL);
      pBLEScan->setWindow(BLE_SCAN_WINDOW);

      LOG_INFO("BLE initialized. Address: " + myAddress);

#if OTA_ENABLED
      setupBLEServer();
#endif

      return true;
    } catch (...) {
      LOG_ERROR("BLE initialization failed");
      return false;
    }
  }

  void setupBLEServer() {
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);

    // Battery characteristic (read/notify)
    batteryCharacteristic = pService->createCharacteristic(
                              BATTERY_CHAR_UUID,
                              BLECharacteristic::PROPERTY_READ |
                              BLECharacteristic::PROPERTY_NOTIFY
                            );
    batteryCharacteristic->addDescriptor(new BLE2902());

    // Friends characteristic (read/notify)
    friendsCharacteristic = pService->createCharacteristic(
                              FRIENDS_CHAR_UUID,
                              BLECharacteristic::PROPERTY_READ |
                              BLECharacteristic::PROPERTY_NOTIFY
                            );
    friendsCharacteristic->addDescriptor(new BLE2902());

    // Config characteristic (read/write)
    configCharacteristic = pService->createCharacteristic(
                             CONFIG_CHAR_UUID,
                             BLECharacteristic::PROPERTY_READ |
                             BLECharacteristic::PROPERTY_WRITE
                           );

    // Command characteristic (write)
    commandCharacteristic = pService->createCharacteristic(
                              COMMAND_CHAR_UUID,
                              BLECharacteristic::PROPERTY_WRITE
                            );
    commandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());

    pService->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    serverStarted = true;
    LOG_INFO("BLE Server started for app integration");
  }

  BLEScanResults* scan(int scanTime = BLE_SCAN_TIME) {
    if (!pBLEScan) return nullptr;

    scanning = true;
    BLEScanResults* results = pBLEScan->start(scanTime, false);
    scanning = false;

    return results;
  }

  void clearScanResults() {
    if (pBLEScan) {
      pBLEScan->clearResults();
    }
  }

  String getMyAddress() {
    return myAddress;
  }

  bool isScanning() {
    return scanning;
  }

  // Update BLE characteristics for app
  void updateBatteryCharacteristic(float voltage, float percentage) {
    if (!serverStarted || !batteryCharacteristic) return;

    String batteryData = String(voltage, 2) + "," + String(percentage, 1);
    batteryCharacteristic->setValue(batteryData.c_str());
    batteryCharacteristic->notify();
  }

  void updateFriendsCharacteristic() {
    if (!serverStarted || !friendsCharacteristic) return;

    String friendsData = "";
    for (int i = 0; i < friendManager->getFriendCount(); i++) {
      Friend* f = friendManager->getFriend(i);
      if (f) {
        if (i > 0) friendsData += ";";
        friendsData += f->macAddress + "," +
                       String(f->present ? "1" : "0") + "," +
                       String(f->rssi);
      }
    }

    friendsCharacteristic->setValue(friendsData.c_str());
    friendsCharacteristic->notify();
  }

  void handleDeviceFound(BLEAdvertisedDevice advertisedDevice) {
    String discoveredAddress = advertisedDevice.getAddress().toString().c_str();

    if (friendManager->isFriend(discoveredAddress)) {
      int8_t rssi = advertisedDevice.getRSSI();

      if (programState == NEW_CONFIG) {
        String shortMac = discoveredAddress.substring(9);
        if (friendManager->addFriend(shortMac)) {
          LOG_INFO("Adding new friend to list: " + shortMac);
          if (eventLogger) {
            eventLogger->logEvent(EVENT_FRIEND_DETECTED, shortMac);
          }
        }
      } else {
        friendManager->updateFriend(discoveredAddress, rssi);

        Friend* f = nullptr;
        for (int i = 0; i < friendManager->getFriendCount(); i++) {
          Friend* temp = friendManager->getFriend(i);
          if (temp && temp->macAddress == discoveredAddress.substring(9)) {
            f = temp;
            break;
          }
        }

        if (f && f->present && !f->wasPresent && eventLogger) {
          eventLogger->logFriendDetected(f->macAddress, rssi);
        }
      }
    }
  }

  void cleanup() {
    if (pBLEScan) {
      pBLEScan->stop();
    }
  }
};

// Global pointer
BLEManager* g_bleManager = nullptr;

// Callback implementations
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (g_bleManager) {
      g_bleManager->handleDeviceFound(advertisedDevice);
    }
  }
};

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    LOG_INFO("BLE client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    LOG_INFO("BLE client disconnected");
    BLEDevice::startAdvertising();
  }
};

class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      String command = value.c_str();
      LOG_INFO("Received command: " + command);

      // Handle commands from app
      // Format: "CMD:param1,param2"
      // Examples: "CLEAR_FRIENDS", "ADD_PREFIX:aa:bb:cc", "REBOOT"
    }
  }
};

#endif // BLE_MANAGER_H
