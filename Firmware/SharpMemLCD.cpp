#include "SharpMemLCD.h"
#include "driver/ledc.h"

// Sharp Memory LCD command bits (sent as the first byte of each line-write)
#define SHARPMEM_CMD_WRITE_LINE   0x80
#define SHARPMEM_CMD_CLEAR        0x20

// The Sharp protocol transmits the LINE ADDRESS least-significant-bit first,
// while command bits above are fixed-position. We bit-reverse address bytes
// before shifting them out MSB-first over the bit-banged bus.
// NOTE: cross check this against the LS027B7DH01A timing diagram before
// relying on this in a final production build - protocol bit-order details
// like this are easy to get subtly wrong without hardware-in-loop testing.
static uint8_t reverseBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

SharpMemLCD::SharpMemLCD()
  : Adafruit_GFX(LCD_WIDTH, LCD_HEIGHT) {}

void SharpMemLCD::begin() {
  pinMode(PIN_LCD_SCLK, OUTPUT);
  pinMode(PIN_LCD_SI, OUTPUT);
  pinMode(PIN_LCD_SCS, OUTPUT);
  pinMode(PIN_LCD_DISP, OUTPUT);

  digitalWrite(PIN_LCD_SCLK, LOW);
  digitalWrite(PIN_LCD_SI, LOW);
  digitalWrite(PIN_LCD_SCS, LOW);

  setPower(true);
  setupExtcominPWM();

  memset(_fb, 0xFF, sizeof(_fb)); // Sharp Memory LCD: 1 = white, 0 = black
  refresh();
}

void SharpMemLCD::setPower(bool on) {
  digitalWrite(PIN_LCD_DISP, on ? HIGH : LOW);
}

void SharpMemLCD::setupExtcominPWM() {
  // EXTMODE is tied HIGH in hardware -> COM inversion is driven purely by a
  // free-running square wave on EXTCOMIN. Configure one LEDC channel for
  // this and never touch it again; it runs independently of the CPU.
  ledc_timer_config_t timerCfg = {};
  timerCfg.speed_mode      = LEDC_LOW_SPEED_MODE;
  timerCfg.duty_resolution = LEDC_TIMER_1_BIT;
  timerCfg.timer_num       = LEDC_TIMER_0;
  timerCfg.freq_hz         = LCD_EXTCOMIN_FREQ_HZ;
  timerCfg.clk_cfg         = LEDC_AUTO_CLK;
  ledc_timer_config(&timerCfg);

  ledc_channel_config_t chCfg = {};
  chCfg.gpio_num   = PIN_LCD_EXTCOMIN;
  chCfg.speed_mode = LEDC_LOW_SPEED_MODE;
  chCfg.channel    = LEDC_CHANNEL_0;
  chCfg.timer_sel  = LEDC_TIMER_0;
  chCfg.duty       = 1;   // 50% duty at 1-bit resolution (1 out of 2)
  chCfg.hpoint     = 0;
  ledc_channel_config(&chCfg);
}

void SharpMemLCD::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;

  uint8_t &byte = _fb[y][x / 8];
  uint8_t mask = 0x80 >> (x % 8);
  if (color) {
    byte |= mask;   // white
  } else {
    byte &= ~mask;  // black
  }
}

void SharpMemLCD::spiWriteByte(uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(PIN_LCD_SI, (b & 0x80) ? HIGH : LOW);
    b <<= 1;
    digitalWrite(PIN_LCD_SCLK, HIGH);
    digitalWrite(PIN_LCD_SCLK, LOW);
  }
}

void SharpMemLCD::spiBeginTransaction() {
  digitalWrite(PIN_LCD_SCS, HIGH);
  delayMicroseconds(3); // tsSCS setup time, see datasheet AC characteristics
}

void SharpMemLCD::spiEndTransaction() {
  // Two trailing dummy clocks required at end of transfer per datasheet
  spiWriteByte(0x00);
  digitalWrite(PIN_LCD_SCS, LOW);
  delayMicroseconds(3);
}

void SharpMemLCD::refresh() {
  spiBeginTransaction();
  spiWriteByte(SHARPMEM_CMD_WRITE_LINE);

  for (uint16_t line = 0; line < LCD_HEIGHT; line++) {
    spiWriteByte(reverseBits((uint8_t)(line + 1))); // 1-indexed line address
    for (uint16_t col = 0; col < LCD_WIDTH / 8; col++) {
      spiWriteByte(_fb[line][col]);
    }
    spiWriteByte(0x00); // trailing dummy byte between lines
  }

  spiEndTransaction();
}

void SharpMemLCD::clearDisplay() {
  memset(_fb, 0xFF, sizeof(_fb));

  spiBeginTransaction();
  spiWriteByte(SHARPMEM_CMD_CLEAR);
  spiWriteByte(0x00);
  spiEndTransaction();
}
