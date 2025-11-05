/*
 * FriendBeacon v2.0
 * A light beacon for friends with enhanced features
 *
 * Features:
 * - RGB LED Ring with proximity-based intensity
 * - Battery monitoring and low-battery warnings
 * - Vibration feedback
 * - Deep sleep power management
 * - Improved button handling (click, double-click, long press)
 * - RSSI-based proximity detection
 * - Enhanced friend management (up to 50 friends)
 * - Configurable MAC filtering
 * - Event logging
 * - OTA updates
 * - BLE app integration
 */

#include "Config.h"
#include "LEDRing.h"
#include "BatteryMonitor.h"
#include "VibrationMotor.h"
#include "ButtonHandler.h"
#include "FriendManager.h"
#include "EventLogger.h"
#include "PowerManager.h"
#include "BLEManager.h"
#include "OTAUpdate.h"

// Component instances
LEDRing ledRing;
BatteryMonitor battery;
VibrationMotor vibration;
ButtonHandler button(BUTTON_PIN);
FriendManager friendManager;
EventLogger eventLogger;
PowerManager powerManager;
BLEManager bleManager(&friendManager, &eventLogger);
OTAUpdate otaUpdate;

// State variables
unsigned long lastBatteryCheck = 0;
unsigned long lastScanTime = 0;
unsigned long configModeStartTime = 0;
bool lowBatteryWarningShown = false;

// Initialize watchdog timer
void setupWatchdog() {
  // ESP32 has built-in watchdog, configure if needed
  // For now, we'll rely on software reset on critical errors
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);

  // Print startup banner
  Serial.println("\n\n========================================");
  Serial.println("    FriendBeacon v" + String(FIRMWARE_VERSION));
  Serial.println("========================================\n");

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  // Initialize components in order
  LOG_INFO("Initializing components...");

  // LED Ring first for visual feedback
  ledRing.begin();
  ledRing.spinner(CRGB::Blue); // Show we're starting up

  // Button handler
  button.begin();

  // Battery monitor
  battery.begin();

  // Vibration motor
  vibration.begin();

  // Event logger
  eventLogger.begin();

  // Friend manager
  friendManager.begin();

  // Power manager
  powerManager.begin();

  // BLE Manager (sets global pointer)
  g_bleManager = &bleManager;
  if (!bleManager.begin()) {
    LOG_ERROR("BLE initialization failed!");
    ledRing.errorAnimation();
    vibration.error();
    delay(5000);
    ESP.restart();
  }

  // OTA Updates
#if OTA_ENABLED
  otaUpdate.begin("FriendBeacon-" + bleManager.getMyAddress().substring(12));
#endif

  LOG_INFO("Initialization complete!");
  LOG_INFO("My BLE Address: " + bleManager.getMyAddress());
  LOG_INFO(friendManager.getStats());
  LOG_INFO(battery.getStatus());

  // Show battery level on LED ring for 2 seconds
  ledRing.showBattery(battery.getPercentage());
  delay(2000);

  // Initial state
  bleManager.programState = BLEManager::LOOKING_FOR_FRIENDS;
  vibration.success();
  ledRing.clear();

  // Check if button is held during boot for config mode
  if (button.isButtonPressed()) {
    LOG_INFO("Button held during boot - entering config mode");
    enterConfigMode();
  }
}

void enterConfigMode() {
  bleManager.programState = BLEManager::NEW_CONFIG;
  configModeStartTime = millis();

  LOG_INFO("Entering CONFIG MODE");
  eventLogger.logEvent(EVENT_CONFIG_MODE);

  ledRing.rainbow();
  vibration.doublePulse();

  // Clear current friend list in memory (not saved yet)
  friendManager.clearFriends();
}

void exitConfigMode(bool save) {
  if (save) {
    LOG_INFO("Saving configuration...");
    friendManager.saveFriends();
    eventLogger.logEvent(EVENT_CONFIG_SAVED, String(friendManager.getFriendCount()) + " friends");

    ledRing.configSavedAnimation();
    vibration.configSaved();

    delay(1000);
  } else {
    LOG_INFO("Configuration cancelled");
    friendManager.loadFriends(); // Reload old configuration
    ledRing.errorAnimation();
  }

  bleManager.programState = BLEManager::LOOKING_FOR_FRIENDS;
  ledRing.clear();
}

void handleConfigMode() {
  // Show rainbow animation
  ledRing.rainbow();

  // Scan for new friends
  unsigned long now = millis();
  if (now - lastScanTime > BLE_SCAN_TIME * 1000) {
    LOG_INFO("Scanning for friends... (Found: " + String(friendManager.getFriendCount()) + ")");

    bleManager.scan(BLE_SCAN_TIME);
    bleManager.clearScanResults();

    lastScanTime = now;

    // Show count of friends found
    ledRing.showCount(friendManager.getFriendCount(), CRGB::Green);
    vibration.pulse();
    delay(1000);
  }

  // Check for timeout
  if ((now - configModeStartTime) > CONFIG_MODE_TIMEOUT_MS) {
    LOG_INFO("Config mode timeout");
    exitConfigMode(true); // Auto-save on timeout
  }
}

void handleLookingForFriends() {
  // Scan for friends
  unsigned long now = millis();
  bool friendsPresent = friendManager.getPresentCount() > 0;

  // Adaptive scan interval
  unsigned long scanInterval = friendsPresent ? FRIEND_SCAN_FAST_MS : FRIEND_SCAN_INTERVAL_MS;

  if (now - lastScanTime > scanInterval) {
    friendManager.resetPresence();

    LOG_DEBUG("Scanning for friends...");
    bleManager.scan(BLE_SCAN_TIME);
    bleManager.clearScanResults();

    lastScanTime = now;

    // Check for changes
    if (friendManager.hasChanges()) {
      int arrivals = friendManager.getNewArrivals();
      int departures = friendManager.getDepartures();

      LOG_INFO("Changes detected - Arrivals: " + String(arrivals) + ", Departures: " + String(departures));

      if (arrivals > 0) {
        vibration.friendDetected();
        powerManager.updateActivity();
      }

      if (departures > 0) {
        vibration.friendLost();
      }
    }

    // Update LED ring
    bool presentArray[MAX_FRIENDS];
    int8_t rssiArray[MAX_FRIENDS];
    friendManager.getFriendStatus(presentArray, rssiArray);
    ledRing.showFriends(presentArray, rssiArray, friendManager.getFriendCount());

    // Update BLE characteristics for app
    bleManager.updateFriendsCharacteristic();

    LOG_INFO(friendManager.getStats());
  }

  // Check battery periodically
  if (now - lastBatteryCheck > 30000) {
    battery.update();
    lastBatteryCheck = now;

    LOG_DEBUG(battery.getStatus());

    // Update battery characteristic for app
    bleManager.updateBatteryCharacteristic(battery.getVoltage(), battery.getPercentage());

    // Show low battery warning
    if (battery.isLow() && !lowBatteryWarningShown) {
      ledRing.flash(CRGB::Red, 3, 200);
      vibration.error();
      lowBatteryWarningShown = true;
      eventLogger.logEvent(EVENT_LOW_BATTERY, battery.getStatus());

      // Restore friend display
      bool presentArray[MAX_FRIENDS];
      int8_t rssiArray[MAX_FRIENDS];
      friendManager.getFriendStatus(presentArray, rssiArray);
      ledRing.showFriends(presentArray, rssiArray, friendManager.getFriendCount());
    }

    // Enter deep sleep if battery critical
    if (battery.isCritical()) {
      LOG_ERROR("CRITICAL BATTERY - Entering deep sleep");
      ledRing.flash(CRGB::Red, 5, 100);
      ledRing.clear();
      powerManager.enterDeepSleep();
    }
  }
}

void handleButtonEvents() {
  ButtonEvent event = button.update();

  switch (event) {
    case BTN_CLICK:
      LOG_INFO("Button: Single click");

      if (bleManager.programState == BLEManager::LOOKING_FOR_FRIENDS) {
        // Show battery level
        ledRing.showBattery(battery.getPercentage());
        vibration.pulse();
        delay(2000);

        // Restore friend display
        bool presentArray[MAX_FRIENDS];
        int8_t rssiArray[MAX_FRIENDS];
        friendManager.getFriendStatus(presentArray, rssiArray);
        ledRing.showFriends(presentArray, rssiArray, friendManager.getFriendCount());
      }
      break;

    case BTN_DOUBLE_CLICK:
      LOG_INFO("Button: Double click");

      // Show friend count
      ledRing.showCount(friendManager.getFriendCount(), CRGB::Blue);
      vibration.doublePulse();
      delay(1500);

      // Restore display
      if (bleManager.programState == BLEManager::LOOKING_FOR_FRIENDS) {
        bool presentArray[MAX_FRIENDS];
        int8_t rssiArray[MAX_FRIENDS];
        friendManager.getFriendStatus(presentArray, rssiArray);
        ledRing.showFriends(presentArray, rssiArray, friendManager.getFriendCount());
      }
      break;

    case BTN_LONG_PRESS:
      LOG_INFO("Button: Long press");

      if (bleManager.programState == BLEManager::LOOKING_FOR_FRIENDS) {
        // Enter config mode
        enterConfigMode();
      } else if (bleManager.programState == BLEManager::NEW_CONFIG) {
        // Exit and save config
        exitConfigMode(true);
      }
      break;

    case BTN_PRESSED:
      // Visual feedback on press
      digitalWrite(ONBOARD_LED, HIGH);
      break;

    case BTN_RELEASED:
      digitalWrite(ONBOARD_LED, LOW);
      break;

    default:
      break;
  }
}

void loop() {
  // Handle OTA updates
  otaUpdate.handle();

  // Don't process other tasks during OTA update
  if (otaUpdate.isUpdateInProgress()) {
    return;
  }

  // Handle button events
  handleButtonEvents();

  // Update vibration motor
  vibration.update();

  // State machine
  switch (bleManager.programState) {
    case BLEManager::NEW_CONFIG:
      handleConfigMode();
      break;

    case BLEManager::LOOKING_FOR_FRIENDS:
      handleLookingForFriends();
      break;

    default:
      bleManager.programState = BLEManager::LOOKING_FOR_FRIENDS;
      break;
  }

  // Small delay to prevent tight loops
  delay(10);
}
