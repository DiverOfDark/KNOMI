#pragma once
#include "../config/Config.h"
#include "../hal/pinout.h"
#include "./network/WifiManager.h"
#include <Arduino.h>
#include <Ticker.h>

class Button final {

public:
  explicit Button(WifiManager *mgr, Config *config) {
    this->config = config;
    this->manager = mgr;
#if KNOMIV1
    pinMode(BOOT_PIN, INPUT); // input mode; default -> high, pressed -> low.
#else
#if KNOMIV2
    pinMode(BOOT_PIN, INPUT_PULLUP); // input mode; default -> high, pressed -> low.
#endif
#endif
    delay(200);
    currentState = HIGH;
    lastState = HIGH;

    timer.attach(0.001, Button::KeyScan, this);
  }

  void KeyScan() {
    currentState = (uint8_t)digitalRead(0); // RESET_PIN

    if (lastState == HIGH && currentState == LOW) { // button is pressed
      LV_LOG_INFO("Button pressed");
      pressedTime = millis();
    } else if (lastState == LOW && currentState == HIGH) { // button is released
      LV_LOG_INFO("Button released");
      releasedTime = millis();

      unsigned long pressDuration = releasedTime - pressedTime;

      if (pressDuration > WIFI_RESET_LONG_PRESS_MS) {
#if !DEBUG
        ESP.restart();
#endif
        manager->resetWifi();
      }
    }
    lastState = currentState;
  }

  static void KeyScan(Button *button) { button->KeyScan(); }

  bool isPressed() const { return lastState == LOW; }

private:
  Config *config;
  WifiManager *manager;
  Ticker timer;

  const int WIFI_RESET_LONG_PRESS_MS = 4000; // long press for ~4 sec

  int lastState = LOW; // the previous state from the input pin
  int currentState;    // the current reading from the input pin

  unsigned long pressedTime = 0;
  unsigned long releasedTime = 0;
};