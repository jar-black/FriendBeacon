#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>

// Own configuration
#define ONBOARD_LED 8
#define BUTTON_PIN 10 // HIGH when pressed


enum states {
  INIT = 1,
  LOAD_CONFIG = 2,
  NEW_CONFIG = 3,
  STORE_CONFIG = 4,
  LOOKING_FOR_FRIENDS = 5
};

int scanTime = 5;  //In seconds
BLEScan *pBLEScan;
Preferences preferences;
String *macAddresses;
enum states programState;
int friend1;
BLEScanResults *foundFriends;
long delayTime;
int nrOfFriends;
String *myOldFriends;
String *myNewFriends;
bool *friendsPresent;


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String discoveredAddress = advertisedDevice.getAddress().toString();
    if (discoveredAddress.startsWith("dc:06:75")) {
      String mac = advertisedDevice.getAddress().toString();
      if(programState == NEW_CONFIG) {
        bool isInList = false;
        for(int c = 0; c < nrOfFriends; c++) if(mac.endsWith(myOldFriends[c])) isInList = true;
        if (isInList) Serial.println("Friend is already in list"); else {
          Serial.printf("Adding new friend to list, %s\n", mac.substring(9));
          myNewFriends[nrOfFriends] = mac.substring(9);
          nrOfFriends += 1;
        }
      } else {
        Serial.println("Lighting (up a led)/(adding a color) for a friend");
        for(int c = 0; c < nrOfFriends; c++) {
          if(mac.endsWith(myOldFriends[c])) {
            Serial.printf("I found my friend: %s\n", mac.substring(9).c_str());
            friendsPresent[c] = true;
          }
        }
      }
    }
  }
};


void setup() {
  programState = INIT;

  Serial.begin(115200);
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  delay(3000);

  BLEDevice::init("");
  BLEAddress address = BLEDevice::getAddress();
  BLEDevice::startAdvertising();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  Serial.printf("My address: %s\n", address.toString().c_str());
}

enum states holdingButton(int button, int timeMs, enum states oldState, enum states nextState) {
  if (digitalRead(button) == HIGH) {
    digitalWrite(ONBOARD_LED, HIGH);
    long counter = millis() + timeMs; // 3 seconds
      while(counter > millis()) {
        if (digitalRead(button) != HIGH) {
          digitalWrite(ONBOARD_LED, LOW);
          Serial.println("Returning to old state");
          return oldState;
        }
        digitalWrite(ONBOARD_LED, LOW);
        delay(200);
      }
        digitalWrite(ONBOARD_LED, LOW);
      Serial.println("Going to next state");
      return nextState;
  }
  digitalWrite(ONBOARD_LED, LOW);
  return oldState;
}


void loop() {
  switch(programState) {
    case INIT: 
      Serial.println("INIT:");
      programState = LOAD_CONFIG;
      break;
    case LOAD_CONFIG:
      Serial.println("LOAD_CONFIG:");
      preferences.begin("friendMacs", false);
      nrOfFriends = preferences.getUInt("nr", 0);
      myOldFriends = (String *)calloc(nrOfFriends, sizeof(char[8]));
      friendsPresent = (bool *)calloc(nrOfFriends, sizeof(bool));
      for(int a=0; a<nrOfFriends; a++) {
        myOldFriends[a] = preferences.getString("" + a, "");
        Serial.printf("My stored friend: %s\n", myOldFriends[a]);
      }
//      myOldFriends[0] = "f4:5e:a2";
      if (true) {
        programState = LOOKING_FOR_FRIENDS;
      } else {
        programState = NEW_CONFIG;
      }
      break;
    case NEW_CONFIG:
      Serial.println("NEW_CONFIG:");
      nrOfFriends = 0;
      myNewFriends = (String *)calloc(10, 3*sizeof(char[2])); // Allocate memory for 5 mac addresses
      while(programState == NEW_CONFIG) {
        foundFriends = pBLEScan->start(scanTime, false);
        delayTime = millis() + 1000*3; // 6 seconds
        while (delayTime > millis() && programState == NEW_CONFIG) {
          programState = holdingButton(BUTTON_PIN, 100, NEW_CONFIG, STORE_CONFIG);
          delay(100);
        }
      }
      break;
    case STORE_CONFIG: 
      Serial.println("STORE_CONFIG:");
      preferences.putUInt("nr", nrOfFriends);
      for(int a=0; a<nrOfFriends; a++) {
        preferences.putString("" + a, myNewFriends[a]);
      }
      free(myOldFriends);
      myOldFriends = myNewFriends;
      free(friendsPresent);
      friendsPresent = (bool *)calloc(nrOfFriends, sizeof(bool));
      for(int a=0; a<nrOfFriends; a++) friendsPresent[a] = false;
      programState = LOOKING_FOR_FRIENDS;
      break;
    case LOOKING_FOR_FRIENDS: 
      Serial.println("LOOKING_FOR_FRIENDS:");
      for(int a=0; a<nrOfFriends; a++) friendsPresent[a] = false;
      foundFriends = pBLEScan->start(scanTime, false);
      pBLEScan->clearResults();
      digitalWrite(ONBOARD_LED, LOW);
      for(int a=0; a<nrOfFriends; a++) if(friendsPresent[a]) digitalWrite(ONBOARD_LED, HIGH);
      delayTime = millis() + 1000*6; // 6 seconds
      while (delayTime > millis() && programState == LOOKING_FOR_FRIENDS) {
        programState = holdingButton(BUTTON_PIN, 3000, LOOKING_FOR_FRIENDS, NEW_CONFIG);
        delay(100);
      }
      break;
  }
}
