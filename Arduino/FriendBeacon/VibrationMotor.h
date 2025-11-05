#ifndef VIBRATION_MOTOR_H
#define VIBRATION_MOTOR_H

#include "Config.h"

enum VibrationType {
  VIB_NONE,
  VIB_SHORT,
  VIB_LONG,
  VIB_DOUBLE,
  VIB_TRIPLE,
  VIB_PATTERN
};

class VibrationMotor {
private:
  bool enabled;
  unsigned long vibEndTime;
  bool isVibrating;
  uint8_t intensity;

  void startVibration(uint16_t durationMs, uint8_t power) {
    if (!enabled) return;

    intensity = constrain(power, 0, 255);
    analogWrite(VIBRATION_PIN, intensity);
    vibEndTime = millis() + durationMs;
    isVibrating = true;
  }

public:
  VibrationMotor() : enabled(true), vibEndTime(0), isVibrating(false), intensity(VIB_INTENSITY) {}

  void begin() {
    pinMode(VIBRATION_PIN, OUTPUT);
    digitalWrite(VIBRATION_PIN, LOW);
    LOG_INFO("Vibration Motor initialized");
  }

  void setEnabled(bool enable) {
    enabled = enable;
    if (!enabled) {
      stop();
    }
  }

  void setIntensity(uint8_t power) {
    intensity = constrain(power, 0, 255);
  }

  void update() {
    if (isVibrating && millis() >= vibEndTime) {
      stop();
    }
  }

  void stop() {
    analogWrite(VIBRATION_PIN, 0);
    isVibrating = false;
  }

  // Vibrate for specified duration
  void vibrate(uint16_t durationMs, uint8_t power = VIB_INTENSITY) {
    startVibration(durationMs, power);
  }

  // Friend detected pattern
  void friendDetected() {
    vibrate(VIB_FRIEND_DETECTED_MS);
  }

  // Friend lost pattern
  void friendLost() {
    vibrate(VIB_FRIEND_LOST_MS);
  }

  // Config saved pattern
  void configSaved() {
    vibrate(VIB_CONFIG_SAVED_MS);
  }

  // Short pulse
  void pulse() {
    vibrate(50);
  }

  // Double pulse pattern (blocking)
  void doublePulse() {
    if (!enabled) return;
    vibrate(100);
    delay(150);
    vibrate(100);
  }

  // Triple pulse pattern (blocking)
  void triplePulse() {
    if (!enabled) return;
    for (int i = 0; i < 3; i++) {
      vibrate(80);
      delay(100);
    }
  }

  // Error pattern (blocking)
  void error() {
    if (!enabled) return;
    vibrate(200);
    delay(100);
    vibrate(200);
  }

  // Success pattern (blocking)
  void success() {
    if (!enabled) return;
    vibrate(300);
  }

  bool isActive() {
    return isVibrating;
  }
};

#endif // VIBRATION_MOTOR_H
