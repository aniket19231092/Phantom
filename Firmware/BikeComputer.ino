/*
 * BikeComputer.ino
 *
 * Top-level sketch. All actual logic lives in the other tabs in this
 * sketch folder (Arduino IDE shows them as tabs automatically) -
 * this file just wires the modules together.
 *
 * Required libraries (install via Library Manager):
 *   - TinyGPSPlus (Mikal Hart)
 *   - Adafruit GFX Library
 *
 * Board: ESP32S3 Dev Module (or your custom board definition), with:
 *   - USB Mode: Hardware CDC and JTAG (native USB, matches D+/D- on the
 *     board's fixed USB pins)
 *   - PSRAM: Disabled (this build targets the -N8 module, no PSRAM)
 *   - Partition scheme: Default (or Minimal SPIFFS if flash is tight)
 *
 * See README.md for the SD card folder layout and map tile format spec.
 */

#include "Config.h"
#include "SharpMemLCD.h"
#include "RotaryEncoder.h"
#include "GPSModule.h"
#include "SDStorage.h"
#include "BatteryMonitor.h"
#include "UIScreens.h"

SharpMemLCD display;
RotaryEncoder encoder;
GPSModule gps;
SDStorage storage;
BatteryMonitor battery;
UIScreens ui;

void setup() {
  Serial.begin(115200);

  display.begin();
  encoder.begin();
  gps.begin();
  battery.begin();

  if (!storage.begin()) {
    // SD card failed to mount - show a message and halt UI init, but keep
    // looping so the user can power-cycle after inserting/checking the card
    // rather than the device appearing fully dead.
    display.setTextColor(0);
    display.setTextSize(1);
    display.setCursor(10, 10);
    display.print("SD card not found.");
    display.setCursor(10, 25);
    display.print("Insert card and restart.");
    display.refresh();
    while (true) {
      delay(1000);
    }
  }

  ui.begin(&display, &encoder, &gps, &storage, &battery);
}

void loop() {
  gps.update();
  ui.update();
}
