#ifndef LEDRING_H
#define LEDRING_H

#include <FastLED.h>
#include "Config.h"

enum LEDPattern {
  PATTERN_OFF,
  PATTERN_STATIC,
  PATTERN_PULSE,
  PATTERN_RAINBOW,
  PATTERN_SPINNER,
  PATTERN_BATTERY
};

class LEDRing {
private:
  CRGB leds[NUM_LEDS];
  uint8_t brightness;
  unsigned long lastUpdate;
  uint16_t animationStep;

  // Friend colors - predefined palette
  const CRGB friendColors[16] = {
    CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow,
    CRGB::Cyan, CRGB::Magenta, CRGB::Orange, CRGB::Purple,
    CRGB::Pink, CRGB::Lime, CRGB::Teal, CRGB::Lavender,
    CRGB::Brown, CRGB::Beige, CRGB::Maroon, CRGB::Mint
  };

public:
  LEDRing() : brightness(LED_BRIGHTNESS), lastUpdate(0), animationStep(0) {}

  void begin() {
    FastLED.addLeds<LED_TYPE, LED_RING_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    clear();
    LOG_INFO("LED Ring initialized");
  }

  void setBrightness(uint8_t b) {
    brightness = constrain(b, 0, 255);
    FastLED.setBrightness(brightness);
  }

  void clear() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }

  void setLED(uint8_t index, CRGB color) {
    if (index < NUM_LEDS) {
      leds[index] = color;
    }
  }

  void show() {
    FastLED.show();
  }

  CRGB getFriendColor(uint8_t friendIndex) {
    return friendColors[friendIndex % 16];
  }

  // Show friends on ring with intensity based on RSSI
  void showFriends(bool* present, int8_t* rssi, int count) {
    clear();
    int displayCount = min(count, NUM_LEDS);

    for (int i = 0; i < displayCount; i++) {
      if (present[i]) {
        CRGB color = getFriendColor(i);

        // Adjust brightness based on RSSI (proximity)
        uint8_t intensity = 255;
        if (rssi[i] < RSSI_FAR) {
          intensity = 64;  // Very dim - far away
        } else if (rssi[i] < RSSI_MEDIUM) {
          intensity = 128; // Medium - medium distance
        } else {
          intensity = 255; // Bright - nearby
        }

        color.nscale8(intensity);
        leds[i] = color;
      }
    }
    show();
  }

  // Pulse animation for a specific friend
  void pulseFriend(uint8_t friendIndex) {
    if (friendIndex >= NUM_LEDS) return;

    unsigned long now = millis();
    if (now - lastUpdate > 20) {
      lastUpdate = now;
      uint8_t breath = beatsin8(20, 50, 255); // Slow breathing effect
      CRGB color = getFriendColor(friendIndex);
      color.nscale8(breath);
      leds[friendIndex] = color;
      show();
    }
  }

  // Rainbow animation for config mode
  void rainbow() {
    unsigned long now = millis();
    if (now - lastUpdate > 20) {
      lastUpdate = now;
      fill_rainbow(leds, NUM_LEDS, animationStep, 255 / NUM_LEDS);
      animationStep += 2;
      show();
    }
  }

  // Spinner animation
  void spinner(CRGB color) {
    unsigned long now = millis();
    if (now - lastUpdate > 50) {
      lastUpdate = now;
      clear();
      leds[animationStep % NUM_LEDS] = color;
      leds[(animationStep + NUM_LEDS/2) % NUM_LEDS] = color;
      animationStep++;
      show();
    }
  }

  // Battery level display (red-yellow-green gradient)
  void showBattery(float percentage) {
    clear();
    int numLit = (percentage / 100.0) * NUM_LEDS;

    for (int i = 0; i < numLit; i++) {
      if (percentage < 20) {
        leds[i] = CRGB::Red;
      } else if (percentage < 50) {
        leds[i] = CRGB::Orange;
      } else if (percentage < 80) {
        leds[i] = CRGB::Yellow;
      } else {
        leds[i] = CRGB::Green;
      }
    }
    show();
  }

  // Flash all LEDs
  void flash(CRGB color, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
      fill_solid(leds, NUM_LEDS, color);
      show();
      delay(delayMs);
      clear();
      delay(delayMs);
    }
  }

  // Show count of friends (light up N LEDs)
  void showCount(int count, CRGB color) {
    clear();
    for (int i = 0; i < min(count, NUM_LEDS); i++) {
      leds[i] = color;
    }
    show();
  }

  // Confirmation animation
  void confirmationAnimation() {
    flash(CRGB::Green, 3, 150);
  }

  // Error animation
  void errorAnimation() {
    flash(CRGB::Red, 3, 100);
  }

  // Config saved animation
  void configSavedAnimation() {
    for (int i = 0; i < NUM_LEDS; i++) {
      clear();
      leds[i] = CRGB::Green;
      show();
      delay(30);
    }
    clear();
  }
};

#endif // LEDRING_H
