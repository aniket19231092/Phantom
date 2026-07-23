#pragma once
/*
 * Config.h
 * Central pin map and constants for the bike computer.
 * Pin numbers match the finalized schematic GPIO assignment pass.
 */

#include <Arduino.h>

// ---------------------------------------------------------------------------
// SD Card (SDMMC 4-bit mode)
// ---------------------------------------------------------------------------
#define PIN_SD_CLK      15
#define PIN_SD_CMD      16
#define PIN_SD_DAT0     12
#define PIN_SD_DAT1     13
#define PIN_SD_DAT2     18
#define PIN_SD_DAT3     17
#define PIN_SD_CD       14   // Card detect switch (active LOW when card present, external pull-up)

// ---------------------------------------------------------------------------
// Sharp Memory LCD (LS027B7DH01A) - software/hardware SPI + control pins
// ---------------------------------------------------------------------------
#define PIN_LCD_SCLK      35
#define PIN_LCD_SI        36
#define PIN_LCD_SCS       37
#define PIN_LCD_EXTCOMIN  38   // Hardware-toggled via LEDC (EXTMODE tied HIGH in hardware)
#define PIN_LCD_DISP      39   // DISP control - drive HIGH to turn panel on

#define LCD_WIDTH   400
#define LCD_HEIGHT  240

// EXTCOMIN toggle frequency (Hz) - typical Sharp Memory LCD spec is ~1Hz min,
// using 2Hz here for margin. Driven entirely in hardware via LEDC, no CPU time.
#define LCD_EXTCOMIN_FREQ_HZ  2

// ---------------------------------------------------------------------------
// Rotary encoder + push button
// ---------------------------------------------------------------------------
#define PIN_ENC_A       9
#define PIN_ENC_B       11
#define PIN_ENC_BTN     10   // Active LOW (external pull-up), debounced in firmware

// ---------------------------------------------------------------------------
// GPS (MAX-M10S) - UART
// ---------------------------------------------------------------------------
#define PIN_GPS_TX      43   // ESP32 TX -> GPS RXD
#define PIN_GPS_RX      44   // ESP32 RX <- GPS TXD
#define GPS_BAUD        9600
#define GPS_UART_NUM    1

// ---------------------------------------------------------------------------
// Battery charger status (MCP73871) - open-drain, external pull-ups
// ---------------------------------------------------------------------------
#define PIN_CHG_STAT1   5   // Charging / low-battery indicator
#define PIN_CHG_STAT2   7   // Charge-complete indicator
#define PIN_CHG_PG      6   // Power-good (input power present)

// ---------------------------------------------------------------------------
// Filesystem layout on SD card
// ---------------------------------------------------------------------------
#define ROUTES_DIR      "/routes"     // .gpx files live here
#define TILES_DIR       "/tiles"      // /tiles/{zoom}/{x}/{y}.bin

// Tile format (produced by a PC-side prep script, see README):
//   - Raw 1-bit-per-pixel monochrome bitmap, row-major, MSB first
//   - TILE_SIZE x TILE_SIZE pixels, no header
//   - File size must be exactly (TILE_SIZE * TILE_SIZE) / 8 bytes
#define TILE_SIZE       200
#define TILE_BYTES      ((TILE_SIZE * TILE_SIZE) / 8)

#define MIN_ZOOM        12
#define MAX_ZOOM        18
#define DEFAULT_ZOOM    16

// ---------------------------------------------------------------------------
// Ride-recording behavior
// ---------------------------------------------------------------------------
#define GPS_STOPPED_THRESHOLD_MPS   1.0f   // below this, freeze heading/speed jitter
#define GPS_FIX_TIMEOUT_MS          3000

// ---------------------------------------------------------------------------
// Encoder debounce / PCNT config
// ---------------------------------------------------------------------------
#define ENC_BTN_DEBOUNCE_MS   40
#define PCNT_UNIT_ENCODER      0   // pcnt_unit_t value used for the rotary encoder
