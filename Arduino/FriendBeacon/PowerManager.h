#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <esp_sleep.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include "Config.h"

class PowerManager {
private:
  unsigned long lastActivityTime;
  bool deepSleepEnabled;
  unsigned long sleepTimeout;

public:
  PowerManager() :
    lastActivityTime(0),
    deepSleepEnabled(DEEP_SLEEP_ENABLED),
    sleepTimeout(SLEEP_AFTER_NO_FRIENDS_MS) {}

  void begin() {
    // Configure wakeup sources
#if SLEEP_BUTTON_WAKEUP
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, HIGH);
#endif

    // Timer wakeup
    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);

    LOG_INFO("Power Manager initialized");

    // Check wakeup reason
    printWakeupReason();
  }

  void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_EXT0:
        LOG_INFO("Wakeup: Button press");
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        LOG_INFO("Wakeup: Timer");
        break;
      case ESP_SLEEP_WAKEUP_UNDEFINED:
      default:
        LOG_INFO("Wakeup: Power on reset");
        break;
    }
  }

  void updateActivity() {
    lastActivityTime = millis();
  }

  bool shouldSleep() {
    if (!deepSleepEnabled) return false;

    unsigned long inactiveTime = millis() - lastActivityTime;
    return inactiveTime > sleepTimeout;
  }

  void enterLightSleep(uint32_t durationMs) {
    if (!deepSleepEnabled) {
      delay(durationMs);
      return;
    }

    LOG_INFO("Entering light sleep for " + String(durationMs) + "ms");

    esp_sleep_enable_timer_wakeup(durationMs * 1000);
    esp_light_sleep_start();

    LOG_INFO("Woke from light sleep");
  }

  void enterDeepSleep() {
    if (!deepSleepEnabled) {
      LOG_INFO("Deep sleep disabled, delaying instead");
      delay(SLEEP_DURATION_US / 1000);
      return;
    }

    LOG_INFO("Entering deep sleep...");

    // Cleanup before sleep
    Serial.flush();
    delay(100);

    // Enter deep sleep
    esp_deep_sleep_start();

    // This point is never reached
  }

  void setSleepEnabled(bool enabled) {
    deepSleepEnabled = enabled;
    LOG_INFO("Deep sleep " + String(enabled ? "enabled" : "disabled"));
  }

  void setSleepTimeout(unsigned long timeoutMs) {
    sleepTimeout = timeoutMs;
  }

  unsigned long getInactiveTime() {
    return millis() - lastActivityTime;
  }

  // Prepare for sleep (save state, cleanup)
  void prepareForSleep() {
    LOG_INFO("Preparing for sleep...");

    // Disable BLE
    // BLE will be re-initialized on wake

    Serial.flush();
  }

  // Adaptive sleep based on activity
  void smartSleep(bool friendsPresent) {
    if (!deepSleepEnabled) return;

    if (friendsPresent) {
      // Short light sleep when friends are around
      enterLightSleep(FRIEND_SCAN_FAST_MS);
    } else {
      // Check if we should enter deep sleep
      if (shouldSleep()) {
        prepareForSleep();
        enterDeepSleep();
      } else {
        // Light sleep between scans
        enterLightSleep(FRIEND_SCAN_INTERVAL_MS);
      }
    }
  }

  // Get remaining time before deep sleep
  unsigned long getTimeUntilSleep() {
    if (!deepSleepEnabled) return 0;

    unsigned long inactiveTime = millis() - lastActivityTime;
    if (inactiveTime >= sleepTimeout) return 0;

    return sleepTimeout - inactiveTime;
  }
};

#endif // POWER_MANAGER_H
