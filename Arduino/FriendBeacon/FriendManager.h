#ifndef FRIEND_MANAGER_H
#define FRIEND_MANAGER_H

#include <Preferences.h>
#include "Config.h"

struct Friend {
  String macAddress;     // Last 8 chars of MAC
  bool present;          // Currently detected
  bool wasPresent;       // Previous state
  int8_t rssi;          // Signal strength
  uint32_t lastSeen;    // Timestamp
  uint32_t firstSeen;   // First detection in session
  String nickname;       // Optional nickname (future use)
};

class FriendManager {
private:
  Friend friends[MAX_FRIENDS];
  int friendCount;
  Preferences preferences;
  String macPrefixes[MAC_PREFIX_MAX];
  int prefixCount;

  void loadMacPrefixes() {
    preferences.begin("macfilter", true);
    prefixCount = preferences.getUInt("prefixCount", 1);

    if (prefixCount == 0 || prefixCount > MAC_PREFIX_MAX) {
      // Load default
      prefixCount = 1;
      macPrefixes[0] = DEFAULT_MAC_PREFIX;
    } else {
      for (int i = 0; i < prefixCount; i++) {
        macPrefixes[i] = preferences.getString(("prefix" + String(i)).c_str(), DEFAULT_MAC_PREFIX);
      }
    }
    preferences.end();

    LOG_INFO("Loaded " + String(prefixCount) + " MAC prefix filters");
  }

public:
  FriendManager() : friendCount(0), prefixCount(0) {}

  void begin() {
    loadMacPrefixes();
    loadFriends();
    LOG_INFO("Friend Manager initialized with " + String(friendCount) + " friends");
  }

  void loadFriends() {
    preferences.begin("friendMacs", true);
    friendCount = preferences.getUInt("nr", 0);

    if (friendCount > MAX_FRIENDS) {
      LOG_ERROR("Friend count exceeds maximum, capping at " + String(MAX_FRIENDS));
      friendCount = MAX_FRIENDS;
    }

    for (int i = 0; i < friendCount; i++) {
      friends[i].macAddress = preferences.getString(String(i).c_str(), "");
      friends[i].present = false;
      friends[i].wasPresent = false;
      friends[i].rssi = -100;
      friends[i].lastSeen = 0;
      friends[i].firstSeen = 0;
      friends[i].nickname = "";

      LOG_DEBUG("Loaded friend " + String(i) + ": " + friends[i].macAddress);
    }

    preferences.end();
  }

  void saveFriends() {
    preferences.begin("friendMacs", false);
    preferences.putUInt("nr", friendCount);

    for (int i = 0; i < friendCount; i++) {
      preferences.putString(String(i).c_str(), friends[i].macAddress);
    }

    preferences.end();
    LOG_INFO("Saved " + String(friendCount) + " friends to preferences");
  }

  void clearFriends() {
    friendCount = 0;
    preferences.begin("friendMacs", false);
    preferences.clear();
    preferences.end();
    LOG_INFO("Cleared all friends");
  }

  bool addFriend(String macAddress) {
    if (friendCount >= MAX_FRIENDS) {
      LOG_ERROR("Cannot add friend - maximum reached");
      return false;
    }

    // Check if already exists
    for (int i = 0; i < friendCount; i++) {
      if (friends[i].macAddress == macAddress) {
        LOG_DEBUG("Friend already exists: " + macAddress);
        return false;
      }
    }

    friends[friendCount].macAddress = macAddress;
    friends[friendCount].present = false;
    friends[friendCount].wasPresent = false;
    friends[friendCount].rssi = -100;
    friends[friendCount].lastSeen = 0;
    friends[friendCount].firstSeen = 0;
    friendCount++;

    LOG_INFO("Added friend: " + macAddress + " (total: " + String(friendCount) + ")");
    return true;
  }

  bool removeFriend(int index) {
    if (index < 0 || index >= friendCount) {
      return false;
    }

    LOG_INFO("Removing friend: " + friends[index].macAddress);

    // Shift remaining friends
    for (int i = index; i < friendCount - 1; i++) {
      friends[i] = friends[i + 1];
    }

    friendCount--;
    return true;
  }

  bool removeFriend(String macAddress) {
    for (int i = 0; i < friendCount; i++) {
      if (friends[i].macAddress == macAddress) {
        return removeFriend(i);
      }
    }
    return false;
  }

  void updateFriend(String fullMacAddress, int8_t rssi) {
    String shortMac = fullMacAddress.substring(9); // Last 8 chars

    for (int i = 0; i < friendCount; i++) {
      if (friends[i].macAddress == shortMac) {
        friends[i].wasPresent = friends[i].present;
        friends[i].present = true;
        friends[i].rssi = rssi;
        friends[i].lastSeen = millis();

        if (friends[i].firstSeen == 0) {
          friends[i].firstSeen = millis();
        }

        LOG_DEBUG("Updated friend " + String(i) + ": " + shortMac + " RSSI: " + String(rssi));
        return;
      }
    }
  }

  void resetPresence() {
    for (int i = 0; i < friendCount; i++) {
      friends[i].wasPresent = friends[i].present;
      friends[i].present = false;
    }
  }

  bool isFriend(String macAddress) {
    // Check if MAC starts with any allowed prefix
    for (int i = 0; i < prefixCount; i++) {
      if (macAddress.startsWith(macPrefixes[i])) {
        return true;
      }
    }
    return false;
  }

  int getFriendCount() {
    return friendCount;
  }

  Friend* getFriend(int index) {
    if (index >= 0 && index < friendCount) {
      return &friends[index];
    }
    return nullptr;
  }

  int getPresentCount() {
    int count = 0;
    for (int i = 0; i < friendCount; i++) {
      if (friends[i].present) count++;
    }
    return count;
  }

  // Get arrays for LED display
  void getFriendStatus(bool* presentArray, int8_t* rssiArray) {
    for (int i = 0; i < friendCount; i++) {
      presentArray[i] = friends[i].present;
      rssiArray[i] = friends[i].rssi;
    }
  }

  // Check for state changes (friend arrived/left)
  bool hasChanges() {
    for (int i = 0; i < friendCount; i++) {
      if (friends[i].present != friends[i].wasPresent) {
        return true;
      }
    }
    return false;
  }

  int getNewArrivals() {
    int count = 0;
    for (int i = 0; i < friendCount; i++) {
      if (friends[i].present && !friends[i].wasPresent) {
        count++;
      }
    }
    return count;
  }

  int getDepartures() {
    int count = 0;
    for (int i = 0; i < friendCount; i++) {
      if (!friends[i].present && friends[i].wasPresent) {
        count++;
      }
    }
    return count;
  }

  // MAC prefix management
  void addMacPrefix(String prefix) {
    if (prefixCount >= MAC_PREFIX_MAX) return;

    macPrefixes[prefixCount++] = prefix;

    preferences.begin("macfilter", false);
    preferences.putUInt("prefixCount", prefixCount);
    preferences.putString(("prefix" + String(prefixCount - 1)).c_str(), prefix);
    preferences.end();

    LOG_INFO("Added MAC prefix: " + prefix);
  }

  void clearMacPrefixes() {
    preferences.begin("macfilter", false);
    preferences.clear();
    preferences.end();
    prefixCount = 0;
    LOG_INFO("Cleared MAC prefixes");
  }

  String getStats() {
    String stats = "Friends: " + String(friendCount) + "/" + String(MAX_FRIENDS);
    stats += ", Present: " + String(getPresentCount());
    return stats;
  }
};

#endif // FRIEND_MANAGER_H
