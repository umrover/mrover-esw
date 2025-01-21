// IMPORTANT: SEE COMMENTS @ LINE 15 REGARDING SHIELD VS BREAKOUT BOARD USAGE.

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#ifndef _ADAFRUIT_TFTLCD_H_
#define _ADAFRUIT_TFTLCD_H_

#include "main.h"
#include "registers.h"

// TODO: Change
#define CS_BANK  GPIOA
#define CS_PIN   GPIO_PIN_1
#define CD_BANK  GPIOA
#define CD_PIN   GPIO_PIN_1
#define WR_BANK  GPIOA
#define WR_PIN   GPIO_PIN_1
#define RD_BANK  GPIOA
#define RD_PIN   GPIO_PIN_1
#define RST_BANK GPIOA
#define RST_PIN  GPIO_PIN_1

#define D0_BANK GPIOB
#define D0_PIN GPIO_PIN_1
#define D1_BANK GPIOB
#define D1_PIN GPIO_PIN_1
#define D2_BANK GPIOB
#define D2_PIN GPIO_PIN_1
#define D3_BANK GPIOB
#define D3_PIN GPIO_PIN_1
#define D4_BANK GPIOB
#define D4_PIN GPIO_PIN_1
#define D5_BANK GPIOB
#define D5_PIN GPIO_PIN_1
#define D6_BANK GPIOB
#define D6_PIN GPIO_PIN_1
#define D7_BANK GPIOB
#define D7_PIN GPIO_PIN_1

// Control signals are ACTIVE LOW (idle is HIGH)
// Command/Data: LOW = command, HIGH = data
// These are single-instruction operations and always inline
#define RD_ACTIVE 	HAL_GPIO_WritePin(RD_BANK, RD_PIN, GPIO_PIN_RESET)
#define RD_IDLE 	HAL_GPIO_WritePin(RD_BANK, RD_PIN, GPIO_PIN_SET)
#define WR_ACTIVE 	HAL_GPIO_WritePin(WR_BANK, WR_PIN, GPIO_PIN_RESET)
#define WR_IDLE 	HAL_GPIO_WritePin(WR_BANK, WR_PIN, GPIO_PIN_SET)
#define CD_COMMAND  HAL_GPIO_WritePin(CD_BANK, CD_PIN, GPIO_PIN_RESET)
#define CD_DATA 	HAL_GPIO_WritePin(CD_BANK, CD_PIN, GPIO_PIN_SET)
#define CS_ACTIVE 	HAL_GPIO_WritePin(CS_BANK, CS_PIN, GPIO_PIN_RESET)
#define CS_IDLE 	HAL_GPIO_WritePin(CS_BANK, CS_PIN, GPIO_PIN_SET)
#define RST_LOW 	HAL_GPIO_WritePin(RST_BANK, RST_PIN, GPIO_PIN_RESET)
#define RST_HIGH 	HAL_GPIO_WritePin(RST_BANK, RST_PIN, GPIO_PIN_SET)
#define WR_STROBE 	WR_ACTIVE; \
					WR_IDLE;

#define TFTWIDTH 240
#define TFTHEIGHT 320

#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

class Adafruit_TFTLCD {

public:
	Adafruit_TFTLCD(void);

	void begin();
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
	void drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
	void fillScreen(uint16_t color);
	void reset(void);
	void setRegisters8(uint8_t *ptr, uint8_t n);
	void setRegisters16(uint16_t *ptr, uint8_t n);
	void setRotation(uint8_t x);
	// These methods are public in order for BMP examples to work:
	void setAddrWindow(int x1, int y1, int x2, int y2);
	void pushColors(uint16_t *data, uint8_t len, bool first);

	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
	uint16_t readPixel(int16_t x, int16_t y);

private:
	void init();
	// These items may have previously been defined as macros
	// in pin_magic.h.  If not, function versions are declared:
	void write8(uint8_t value);
	void setWriteDir(void);
	void setReadDir(void);
	void writeRegister8(uint8_t addr, uint8_t data);
	void writeRegister16(uint16_t addr, uint16_t data);
	void writeRegister24(uint8_t addr, uint32_t data);
	void writeRegister32(uint8_t addr, uint32_t data);

	void writeRegisterPair(uint8_t addrH, uint8_t addrL, uint16_t data);

	void setLR(void);
	void flood(uint16_t color, uint32_t len);
	uint8_t driver;

	uint8_t read8fn(void);

	uint8_t rotation = 0;
	uint16_t cursor_y = 0, cursor_x = 0;
	uint16_t textcolor = 0xFFFF;
	uint16_t _width = TFTWIDTH;
	uint16_t _height = TFTHEIGHT;

};

// For compatibility with sketches written for older versions of library.
// Color function name was changed to 'color565' for parity with 2.2" LCD
// library.
#define Color565 color565

#endif
