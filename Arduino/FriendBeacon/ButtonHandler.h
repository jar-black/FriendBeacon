#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "Config.h"

enum ButtonEvent {
  BTN_NONE,
  BTN_PRESSED,
  BTN_RELEASED,
  BTN_CLICK,
  BTN_DOUBLE_CLICK,
  BTN_LONG_PRESS,
  BTN_VERY_LONG_PRESS
};

class ButtonHandler {
private:
  uint8_t pin;
  bool lastState;
  bool currentState;
  unsigned long lastDebounceTime;
  unsigned long pressStartTime;
  unsigned long lastClickTime;
  bool isPressed;
  bool longPressTriggered;
  int clickCount;

  bool readDebounced() {
    bool reading = digitalRead(pin);

    if (reading != lastState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
      if (reading != currentState) {
        currentState = reading;
        return true; // State changed
      }
    }

    lastState = reading;
    return false; // No change
  }

public:
  ButtonHandler(uint8_t buttonPin) :
    pin(buttonPin),
    lastState(LOW),
    currentState(LOW),
    lastDebounceTime(0),
    pressStartTime(0),
    lastClickTime(0),
    isPressed(false),
    longPressTriggered(false),
    clickCount(0) {}

  void begin() {
    pinMode(pin, INPUT);
    currentState = digitalRead(pin);
    lastState = currentState;
    LOG_INFO("Button Handler initialized on pin " + String(pin));
  }

  ButtonEvent update() {
    bool stateChanged = readDebounced();

    // Button pressed
    if (stateChanged && currentState == HIGH) {
      isPressed = true;
      pressStartTime = millis();
      longPressTriggered = false;
      return BTN_PRESSED;
    }

    // Button released
    if (stateChanged && currentState == LOW) {
      isPressed = false;
      unsigned long pressDuration = millis() - pressStartTime;

      // Check if it was a long press
      if (!longPressTriggered && pressDuration >= BUTTON_LONG_PRESS_MS) {
        return BTN_LONG_PRESS;
      }

      // Short press - check for double click
      if (pressDuration < BUTTON_LONG_PRESS_MS) {
        clickCount++;

        // Check if this is a double click
        if (clickCount == 2 && (millis() - lastClickTime) < BUTTON_DOUBLE_CLICK_MS) {
          clickCount = 0;
          return BTN_DOUBLE_CLICK;
        }

        lastClickTime = millis();
      }

      return BTN_RELEASED;
    }

    // Check for long press while button is held
    if (isPressed && !longPressTriggered) {
      unsigned long pressDuration = millis() - pressStartTime;

      if (pressDuration >= BUTTON_LONG_PRESS_MS) {
        longPressTriggered = true;
        return BTN_LONG_PRESS;
      }
    }

    // Check if single click timeout expired
    if (clickCount == 1 && (millis() - lastClickTime) >= BUTTON_DOUBLE_CLICK_MS) {
      clickCount = 0;
      return BTN_CLICK;
    }

    return BTN_NONE;
  }

  bool isButtonPressed() {
    return isPressed;
  }

  unsigned long getPressDuration() {
    if (isPressed) {
      return millis() - pressStartTime;
    }
    return 0;
  }

  // Wait for button to be pressed and held for specific duration
  // Returns true if held for full duration, false if released early
  bool waitForPress(unsigned long durationMs) {
    unsigned long startTime = millis();

    while ((millis() - startTime) < durationMs) {
      update();
      if (!isButtonPressed()) {
        return false; // Released early
      }
      delay(10);
    }

    return true; // Held for full duration
  }

  void reset() {
    clickCount = 0;
    longPressTriggered = false;
  }
};

#endif // BUTTON_HANDLER_H
