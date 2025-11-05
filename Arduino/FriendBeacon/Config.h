#ifndef CONFIG_H
#define CONFIG_H

// Version
#define FIRMWARE_VERSION "2.0.0"

// Pin Configuration
#define ONBOARD_LED 8
#define BUTTON_PIN 10        // HIGH when pressed
#define LED_RING_PIN 7       // Data pin for WS2812B LED ring
#define BATTERY_ADC_PIN 0    // ADC pin for battery monitoring
#define VIBRATION_PIN 9      // PWM pin for vibration motor

// LED Ring Configuration
#define NUM_LEDS 16          // Number of LEDs in the ring
#define LED_BRIGHTNESS 128   // 0-255, default brightness
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Battery Configuration
#define BATTERY_MAX_VOLTAGE 4.2  // Fully charged LiPo
#define BATTERY_MIN_VOLTAGE 3.3  // Minimum safe voltage
#define BATTERY_WARNING_VOLTAGE 3.5  // Warning threshold
#define BATTERY_ADC_SAMPLES 10   // Number of samples to average
#define VOLTAGE_DIVIDER_RATIO 2.0  // Adjust based on your voltage divider

// Power Management
#define DEEP_SLEEP_ENABLED true
#define SLEEP_DURATION_US 30000000  // 30 seconds in microseconds
#define SLEEP_AFTER_NO_FRIENDS_MS 300000  // 5 minutes
#define SLEEP_BUTTON_WAKEUP true

// BLE Configuration
#define BLE_SCAN_TIME 5      // Seconds
#define BLE_SCAN_INTERVAL 100
#define BLE_SCAN_WINDOW 99
#define MAX_FRIENDS 50       // Maximum number of friends to store
#define MAC_PREFIX_MAX 5     // Maximum MAC prefixes to filter

// Default MAC prefix filter (ESP32 vendor)
#define DEFAULT_MAC_PREFIX "dc:06:75"

// RSSI Thresholds (signal strength)
#define RSSI_NEAR -60        // Very close
#define RSSI_MEDIUM -75      // Medium distance
#define RSSI_FAR -90         // Far away

// Timing Configuration
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_LONG_PRESS_MS 3000
#define BUTTON_DOUBLE_CLICK_MS 500
#define CONFIG_MODE_TIMEOUT_MS 60000  // 1 minute
#define FRIEND_SCAN_INTERVAL_MS 6000  // 6 seconds
#define FRIEND_SCAN_FAST_MS 3000      // 3 seconds when friends nearby

// Vibration Patterns
#define VIB_FRIEND_DETECTED_MS 200
#define VIB_FRIEND_LOST_MS 100
#define VIB_CONFIG_SAVED_MS 500
#define VIB_INTENSITY 200    // 0-255 PWM value

// Event Logging
#define MAX_LOG_ENTRIES 100
#define LOG_ENABLED true

// Debug Configuration
#define DEBUG_LEVEL 2        // 0=OFF, 1=ERROR, 2=INFO, 3=DEBUG
#define SERIAL_BAUD 115200

// Debug Macros
#if DEBUG_LEVEL >= 1
  #define LOG_ERROR(x) Serial.println("[ERROR] " + String(x))
#else
  #define LOG_ERROR(x)
#endif

#if DEBUG_LEVEL >= 2
  #define LOG_INFO(x) Serial.println("[INFO] " + String(x))
#else
  #define LOG_INFO(x)
#endif

#if DEBUG_LEVEL >= 3
  #define LOG_DEBUG(x) Serial.println("[DEBUG] " + String(x))
#else
  #define LOG_DEBUG(x)
#endif

// OTA Configuration
#define OTA_ENABLED true
#define OTA_PASSWORD "friendbeacon"

#endif // CONFIG_H
