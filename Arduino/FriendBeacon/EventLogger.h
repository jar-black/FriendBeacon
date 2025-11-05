#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <Preferences.h>
#include "Config.h"

enum EventType {
  EVENT_BOOT,
  EVENT_FRIEND_DETECTED,
  EVENT_FRIEND_LOST,
  EVENT_CONFIG_MODE,
  EVENT_CONFIG_SAVED,
  EVENT_LOW_BATTERY,
  EVENT_SLEEP,
  EVENT_WAKE,
  EVENT_ERROR
};

struct LogEntry {
  EventType type;
  uint32_t timestamp;
  String data;
};

class EventLogger {
private:
  LogEntry logs[MAX_LOG_ENTRIES];
  int logIndex;
  int logCount;
  Preferences preferences;
  bool enabled;

  String getEventTypeName(EventType type) {
    switch (type) {
      case EVENT_BOOT: return "BOOT";
      case EVENT_FRIEND_DETECTED: return "FRIEND_DETECTED";
      case EVENT_FRIEND_LOST: return "FRIEND_LOST";
      case EVENT_CONFIG_MODE: return "CONFIG_MODE";
      case EVENT_CONFIG_SAVED: return "CONFIG_SAVED";
      case EVENT_LOW_BATTERY: return "LOW_BATTERY";
      case EVENT_SLEEP: return "SLEEP";
      case EVENT_WAKE: return "WAKE";
      case EVENT_ERROR: return "ERROR";
      default: return "UNKNOWN";
    }
  }

public:
  EventLogger() : logIndex(0), logCount(0), enabled(LOG_ENABLED) {}

  void begin() {
    if (!enabled) return;

    logEvent(EVENT_BOOT, "FriendBeacon v" + String(FIRMWARE_VERSION));
    LOG_INFO("Event Logger initialized");
  }

  void logEvent(EventType type, String data = "") {
    if (!enabled) return;

    logs[logIndex].type = type;
    logs[logIndex].timestamp = millis();
    logs[logIndex].data = data;

    logIndex = (logIndex + 1) % MAX_LOG_ENTRIES;
    if (logCount < MAX_LOG_ENTRIES) {
      logCount++;
    }

    // Print to serial
    String logMsg = "[" + String(millis()) + "ms] " + getEventTypeName(type);
    if (data.length() > 0) {
      logMsg += ": " + data;
    }
    LOG_DEBUG(logMsg);
  }

  void logFriendDetected(String mac, int8_t rssi) {
    logEvent(EVENT_FRIEND_DETECTED, mac + " (" + String(rssi) + " dBm)");
  }

  void logFriendLost(String mac) {
    logEvent(EVENT_FRIEND_LOST, mac);
  }

  void logError(String error) {
    logEvent(EVENT_ERROR, error);
  }

  int getLogCount() {
    return logCount;
  }

  LogEntry* getLog(int index) {
    if (index >= logCount) return nullptr;

    int actualIndex = (logIndex - logCount + index + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    return &logs[actualIndex];
  }

  void printLogs(int count = 10) {
    if (!enabled) return;

    Serial.println("\n--- Event Log (last " + String(min(count, logCount)) + " entries) ---");

    for (int i = max(0, logCount - count); i < logCount; i++) {
      LogEntry* entry = getLog(i);
      if (entry) {
        Serial.print("[");
        Serial.print(entry->timestamp);
        Serial.print("ms] ");
        Serial.print(getEventTypeName(entry->type));

        if (entry->data.length() > 0) {
          Serial.print(": ");
          Serial.print(entry->data);
        }
        Serial.println();
      }
    }
    Serial.println("--- End Log ---\n");
  }

  void clearLogs() {
    logIndex = 0;
    logCount = 0;
    LOG_INFO("Logs cleared");
  }

  void setEnabled(bool enable) {
    enabled = enable;
  }

  bool isEnabled() {
    return enabled;
  }
};

#endif // EVENT_LOGGER_H
