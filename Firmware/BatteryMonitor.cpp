#include "BatteryMonitor.h"

void BatteryMonitor::begin() {
  pinMode(PIN_CHG_STAT1, INPUT); // external pull-ups already on board
  pinMode(PIN_CHG_STAT2, INPUT);
  pinMode(PIN_CHG_PG, INPUT);
}

ChargeState BatteryMonitor::getState() {
  bool s1 = digitalRead(PIN_CHG_STAT1);
  bool s2 = digitalRead(PIN_CHG_STAT2);
  bool pg = digitalRead(PIN_CHG_PG);

  if (s1 && s2 && pg)   return ChargeState::ON_BATTERY;
  if (s1 && s2 && !pg)  return ChargeState::USB_PRESENT_IDLE;
  if (!s1 && s2 && !pg) return ChargeState::CHARGING;
  if (s1 && !s2 && !pg) return ChargeState::CHARGE_COMPLETE;
  if (!s1 && !s2 && !pg) return ChargeState::FAULT;

  return ChargeState::ON_BATTERY; // fallback for any unexpected combination
}
