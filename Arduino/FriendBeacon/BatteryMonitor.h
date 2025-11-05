#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "Config.h"

class BatteryMonitor {
private:
  float lastVoltage;
  float lastPercentage;
  unsigned long lastCheck;
  bool lowBatteryWarning;

  float readVoltage() {
    long sum = 0;
    for (int i = 0; i < BATTERY_ADC_SAMPLES; i++) {
      sum += analogRead(BATTERY_ADC_PIN);
      delay(10);
    }
    float adcValue = sum / (float)BATTERY_ADC_SAMPLES;

    // ESP32 ADC: 0-4095 maps to 0-3.3V, then account for voltage divider
    float voltage = (adcValue / 4095.0) * 3.3 * VOLTAGE_DIVIDER_RATIO;
    return voltage;
  }

  float voltageToPercentage(float voltage) {
    if (voltage >= BATTERY_MAX_VOLTAGE) return 100.0;
    if (voltage <= BATTERY_MIN_VOLTAGE) return 0.0;

    // Linear approximation (could use LiPo discharge curve for better accuracy)
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) /
                       (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0;
    return constrain(percentage, 0.0, 100.0);
  }

public:
  BatteryMonitor() : lastVoltage(0), lastPercentage(0), lastCheck(0), lowBatteryWarning(false) {}

  void begin() {
    pinMode(BATTERY_ADC_PIN, INPUT);
    update(); // Initial reading
    LOG_INFO("Battery Monitor initialized - Voltage: " + String(lastVoltage) + "V");
  }

  void update() {
    lastVoltage = readVoltage();
    lastPercentage = voltageToPercentage(lastVoltage);
    lastCheck = millis();

    if (lastVoltage < BATTERY_WARNING_VOLTAGE && !lowBatteryWarning) {
      lowBatteryWarning = true;
      LOG_ERROR("Low battery warning: " + String(lastVoltage) + "V");
    } else if (lastVoltage >= BATTERY_WARNING_VOLTAGE) {
      lowBatteryWarning = false;
    }
  }

  float getVoltage() {
    return lastVoltage;
  }

  float getPercentage() {
    return lastPercentage;
  }

  bool isLow() {
    return lastVoltage < BATTERY_WARNING_VOLTAGE;
  }

  bool isCritical() {
    return lastVoltage < BATTERY_MIN_VOLTAGE;
  }

  bool shouldShowWarning() {
    return lowBatteryWarning;
  }

  String getStatus() {
    String status = "Battery: ";
    status += String(lastVoltage, 2);
    status += "V (";
    status += String(lastPercentage, 0);
    status += "%)";

    if (isCritical()) {
      status += " CRITICAL";
    } else if (isLow()) {
      status += " LOW";
    }

    return status;
  }

  // Check if it's time to update (avoid excessive ADC reads)
  bool shouldUpdate(unsigned long intervalMs = 30000) {
    return (millis() - lastCheck) > intervalMs;
  }
};

#endif // BATTERY_MONITOR_H
