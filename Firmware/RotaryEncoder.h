#pragma once
/*
 * RotaryEncoder.h
 *
 * Reads the A/B quadrature signals using the ESP32-S3's hardware PCNT
 * (Pulse Counter) peripheral - decoding happens entirely in hardware, no
 * polling loop needed for A/B. The push button is a plain debounced GPIO.
 *
 * Direction convention (per spec): CW turn = "select right" (positive),
 * CCW turn = "select left" (negative).
 */

#include <Arduino.h>
#include "driver/pcnt.h"
#include "Config.h"

enum class EncoderDir {
  NONE,
  RIGHT,   // CW
  LEFT     // CCW
};

class RotaryEncoder {
public:
  void begin();

  // Call once per loop() iteration. Returns accumulated detent steps since
  // the last call (positive = CW/right, negative = CCW/left), then resets
  // the internal counter back to zero.
  int16_t readSteps();

  // Returns true exactly once per physical button press (debounced,
  // edge-triggered - does not repeat while held).
  bool buttonPressed();

private:
  int16_t _lastCount = 0;
  bool _btnLastState = HIGH;
  bool _btnStableState = HIGH;
  uint32_t _btnLastChangeMs = 0;
  bool _btnEventPending = false;
};
