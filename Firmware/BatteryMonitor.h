#pragma once
/*
 * BatteryMonitor.h
 *
 * Reads the MCP73871 charger's open-drain status outputs (external
 * pull-ups already on the board). These are level outputs, not edges -
 * just poll and decode the combination each time you want current status.
 *
 * Truth table (from MCP73871 datasheet):
 *   STAT1  STAT2  PG    | State
 *   HIGH   HIGH   HIGH  | Shutdown / no input power (running on battery)
 *   HIGH   HIGH   LOW   | Shutdown, input power present, charging disabled
 *   LOW    HIGH   LOW   | Charging (precondition / fast-charge / CV)
 *   HIGH   LOW    LOW   | Charge complete
 *   LOW    LOW    LOW   | Fault (timer or temperature)
 */

#include <Arduino.h>
#include "Config.h"

enum class ChargeState {
  ON_BATTERY,        // no USB power, running from battery
  CHARGING,
  CHARGE_COMPLETE,
  FAULT,
  USB_PRESENT_IDLE   // USB present but not actively charging
};

class BatteryMonitor {
public:
  void begin();
  ChargeState getState();
};
