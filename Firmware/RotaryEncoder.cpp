#include "RotaryEncoder.h"

void RotaryEncoder::begin() {
  // --- Button ---
  pinMode(PIN_ENC_BTN, INPUT); // external pull-up already on the board
  _btnLastState = _btnStableState = digitalRead(PIN_ENC_BTN);

  // --- Quadrature decode via PCNT ---
  pcnt_config_t cfgA = {};
  cfgA.pulse_gpio_num = PIN_ENC_A;
  cfgA.ctrl_gpio_num   = PIN_ENC_B;
  cfgA.channel         = PCNT_CHANNEL_0;
  cfgA.unit            = (pcnt_unit_t)PCNT_UNIT_ENCODER;
  cfgA.pos_mode        = PCNT_COUNT_DEC;   // rising edge on A
  cfgA.neg_mode        = PCNT_COUNT_INC;   // falling edge on A
  cfgA.lctrl_mode      = PCNT_MODE_KEEP;
  cfgA.hctrl_mode      = PCNT_MODE_REVERSE;
  cfgA.counter_h_lim   = 32000;
  cfgA.counter_l_lim   = -32000;
  pcnt_unit_config(&cfgA);

  pcnt_config_t cfgB = {};
  cfgB.pulse_gpio_num = PIN_ENC_B;
  cfgB.ctrl_gpio_num   = PIN_ENC_A;
  cfgB.channel         = PCNT_CHANNEL_1;
  cfgB.unit            = (pcnt_unit_t)PCNT_UNIT_ENCODER;
  cfgB.pos_mode        = PCNT_COUNT_INC;
  cfgB.neg_mode        = PCNT_COUNT_DEC;
  cfgB.lctrl_mode      = PCNT_MODE_REVERSE;
  cfgB.hctrl_mode      = PCNT_MODE_KEEP;
  cfgB.counter_h_lim   = 32000;
  cfgB.counter_l_lim   = -32000;
  pcnt_unit_config(&cfgB);

  // Hardware glitch filter - rejects pulses shorter than ~1us (contact bounce)
  pcnt_set_filter_value((pcnt_unit_t)PCNT_UNIT_ENCODER, 100);
  pcnt_filter_enable((pcnt_unit_t)PCNT_UNIT_ENCODER);

  pcnt_counter_pause((pcnt_unit_t)PCNT_UNIT_ENCODER);
  pcnt_counter_clear((pcnt_unit_t)PCNT_UNIT_ENCODER);
  pcnt_counter_resume((pcnt_unit_t)PCNT_UNIT_ENCODER);

  _lastCount = 0;
}

int16_t RotaryEncoder::readSteps() {
  int16_t raw = 0;
  pcnt_get_counter_value((pcnt_unit_t)PCNT_UNIT_ENCODER, &raw);

  // This encoder is 24 pulses/rev (1 detent per full quadrature cycle).
  // With X4 decoding that's 4 counts per detent - divide down accordingly.
  int16_t delta = (raw - _lastCount);
  int16_t detents = delta / 4;

  if (detents != 0) {
    _lastCount += detents * 4;
  }
  return detents; // positive = CW/right, negative = CCW/left
}

bool RotaryEncoder::buttonPressed() {
  bool reading = digitalRead(PIN_ENC_BTN);
  uint32_t now = millis();

  if (reading != _btnLastState) {
    _btnLastChangeMs = now;
    _btnLastState = reading;
  }

  if ((now - _btnLastChangeMs) > ENC_BTN_DEBOUNCE_MS) {
    if (reading != _btnStableState) {
      _btnStableState = reading;
      if (_btnStableState == LOW) { // active LOW = pressed
        _btnEventPending = true;
      }
    }
  }

  if (_btnEventPending) {
    _btnEventPending = false;
    return true;
  }
  return false;
}
