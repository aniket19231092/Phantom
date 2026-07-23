#pragma once
/*
 * SharpMemLCD.h
 *
 * Minimal framebuffer driver for the Sharp Memory LCD (LS027B7DH01A, 400x240).
 *
 * Notes on this specific hardware configuration:
 *  - EXTMODE is tied HIGH in hardware, meaning COM inversion (EXTCOMIN) is
 *    driven by a free-running hardware square wave (LEDC peripheral), NOT by
 *    software toggling over the SPI bus. This class does not touch EXTCOMIN
 *    at all after begin() sets up the LEDC channel - it runs autonomously.
 *  - SPI here is bit-banged (software SPI) since the display's clock
 *    requirement is modest (low MHz) and it simplifies pin assignment
 *    flexibility. Swap to hardware SPI later if refresh rate becomes a
 *    bottleneck.
 *  - Display is write-only: no MISO/readback line exists on this panel.
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "Config.h"

class SharpMemLCD : public Adafruit_GFX {
public:
  SharpMemLCD();

  void begin();

  // Adafruit_GFX required override
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;

  void clearDisplay();          // Clears framebuffer AND pushes an all-clear frame
  void refresh();               // Pushes the full framebuffer to the panel
  void setPower(bool on);       // Drives the DISP pin

private:
  uint8_t _fb[LCD_HEIGHT][LCD_WIDTH / 8];

  void spiBeginTransaction();
  void spiEndTransaction();
  void spiWriteByte(uint8_t b);
  void setupExtcominPWM();
};
