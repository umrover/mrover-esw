// IMPORTANT: LIBRARY MUST BE SPECIFICALLY CONFIGURED FOR EITHER TFT SHIELD
// OR BREAKOUT BOARD USAGE.  SEE RELEVANT COMMENTS IN Adafruit_TFTLCD.h

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#include "Adafruit_TFTLCD.h"

//#define TFTWIDTH   320
//#define TFTHEIGHT  480


// LCD controller chip identifiers
#define ID_932X 0
#define ID_7575 1
#define ID_9341 2
#define ID_HX8357D 3
#define ID_UNKNOWN 0xFF

#include "registers.h"



// Constructor for shield (fixed LCD control lines)
Adafruit_TFTLCD::Adafruit_TFTLCD(void) {
	init();
}

// Initialization common to both shield & breakout configs
void Adafruit_TFTLCD::init(void) {

	// TODO: Adapt to STM32.
  CS_IDLE; // Set all control bits to idle state
  WR_IDLE;
  RD_IDLE;
  CD_DATA;
	HAL_GPIO_WritePin(RST_BANK, RST_PIN, GPIO_PIN_SET);

	// Ignore this for now \/\/
	setWriteDir(); // Set up LCD data port(s) for WRITE operations

	rotation = 0;
	cursor_y = cursor_x = 0;
	textcolor = 0xFFFF;
	_width = TFTWIDTH;
	_height = TFTHEIGHT;
}

// Initialization command tables for different LCD controllers
#define TFTLCD_DELAY 0xFF


static const uint8_t HX8357D_regValues[] = {
	HX8357_SWRESET,
	0,
	HX8357D_SETC,
	3,
	0xFF,
	0x83,
	0x57,
	TFTLCD_DELAY,
	250,
	HX8357_SETRGB,
	4,
	0x00,
	0x00,
	0x06,
	0x06,
	HX8357D_SETCOM,
	1,
	0x25, // -1.52V
	HX8357_SETOSC,
	1,
	0x68,// Normal mode 70Hz, Idle mode 55 Hz
	HX8357_SETPANEL,
	1,
	0x05,// BGR, Gate direction swapped
	HX8357_SETPWR1,
	6,
	0x00,
	0x15,
	0x1C,
	0x1C,
	0x83,
	0xAA,
	HX8357D_SETSTBA,
	6,
	0x50,
	0x50,
	0x01,
	0x3C,
	0x1E,
	0x08,
	// MEME GAMMA HERE
	HX8357D_SETCYC,
	7,
	0x02,
	0x40,
	0x00,
	0x2A,
	0x2A,
	0x0D,
	0x78,
	HX8357_COLMOD,
	1,
	0x55,
	HX8357_MADCTL,
	1,
	0xC0,
	HX8357_TEON,
	1,
	0x00,
	HX8357_TEARLINE,
	2,
	0x00,
	0x02,
	HX8357_SLPOUT,
	0,
	TFTLCD_DELAY,
	150,
	HX8357_DISPON,
	0,
	TFTLCD_DELAY,
	50,
};


void Adafruit_TFTLCD::begin() {
	uint8_t i = 0;

	reset();

	HAL_Delay(200);

	// HX8357D
	driver = ID_HX8357D;
	CS_ACTIVE;
	while (i < sizeof(HX8357D_regValues)) {
		uint8_t r = HX8357D_regValues[i++];
		uint8_t len = HX8357D_regValues[i++];
		if (r == TFTLCD_DELAY) {
			HAL_Delay(len);
		} else {
			// Serial.print("Register $"); Serial.print(r, HEX);
			// Serial.print(" datalen "); Serial.println(len);

			CS_ACTIVE;
			CD_COMMAND;
			write8(r);
			CD_DATA;
			for (uint8_t d = 0; d < len; d++) {
				uint8_t x = HX8357D_regValues[i++];
				write8(x);
			}
			CS_IDLE;
		}
	}
	return;

}

void Adafruit_TFTLCD::reset(void) {

	CS_IDLE;
	//  CD_DATA;
	WR_IDLE;
	RD_IDLE;


	RST_LOW;
	HAL_Delay(2);
	RST_HIGH;

	// Data transfer sync
	CS_ACTIVE;
	CD_COMMAND;
	write8(0x00);
	for (uint8_t i = 0; i < 3; i++)
		WR_STROBE; // Three extra 0x00s
	CS_IDLE;
}

// Sets the LCD address window (and address counter, on 932X).
// Relevant to rect/screen fills and H/V lines.  Input coordinates are
// assumed pre-sorted (e.g. x2 >= x1).
void Adafruit_TFTLCD::setAddrWindow(int x1, int y1, int x2, int y2) {
	CS_ACTIVE;

	uint32_t t;

	t = x1;
	t <<= 16;
	t |= x2;
	writeRegister32(ILI9341_COLADDRSET, t); // HX8357D uses same registers!
	t = y1;
	t <<= 16;
	t |= y2;
	writeRegister32(ILI9341_PAGEADDRSET, t); // HX8357D uses same registers!

	CS_IDLE;
}

// Unlike the 932X drivers that set the address window to the full screen
// by default (using the address counter for drawPixel operations), the
// 7575 needs the address window set on all graphics operations.  In order
// to save a few register writes on each pixel drawn, the lower-right
// corner of the address window is reset after most fill operations, so
// that drawPixel only needs to change the upper left each time.
void Adafruit_TFTLCD::setLR(void) {
	CS_ACTIVE;
	writeRegisterPair(HX8347G_COLADDREND_HI, HX8347G_COLADDREND_LO, _width - 1);
	writeRegisterPair(HX8347G_ROWADDREND_HI, HX8347G_ROWADDREND_LO,
			_height - 1);
	CS_IDLE;
}

// Fast block fill operation for fillScreen, fillRect, H/V line, etc.
// Requires setAddrWindow() has previously been called to set the fill
// bounds.  'len' is inclusive, MUST be >= 1.
void Adafruit_TFTLCD::flood(uint16_t color, uint32_t len) {
	uint16_t blocks;
	uint8_t i, hi = color >> 8, lo = color;

	CS_ACTIVE;
	CD_COMMAND;
	write8(HX8357_RAMWR);

	// Write first pixel normally, decrement counter by 1
	CD_DATA;
	write8(hi);
	write8(lo);
	len--;

	blocks = (uint16_t)(len / 64); // 64 pixels/block
	if (hi == lo) {
		// High and low bytes are identical.  Leave prior data
		// on the port(s) and just toggle the write strobe.
		while (blocks--) {
			i = 16; // 64 pixels/block / 4 pixels/pass
			do {
				WR_STROBE
				;
				WR_STROBE
				;
				WR_STROBE
				;
				WR_STROBE
				; // 2 bytes/pixel
				WR_STROBE
				;
				WR_STROBE
				;
				WR_STROBE
				;
				WR_STROBE
				; // x 4 pixels
			} while (--i);
		}
		// Fill any remaining pixels (1 to 64)
		for (i = (uint8_t) len & 63; i--;) {
			WR_STROBE
			;
			WR_STROBE
			;
		}
	} else {
		while (blocks--) {
			i = 16; // 64 pixels/block / 4 pixels/pass
			do {
				write8(hi);
				write8(lo);
				write8(hi);
				write8(lo);
				write8(hi);
				write8(lo);
				write8(hi);
				write8(lo);
			} while (--i);
		}
		for (i = (uint8_t) len & 63; i--;) {
			write8(hi);
			write8(lo);
		}
	}
	CS_IDLE;
}

void Adafruit_TFTLCD::drawFastHLine(int16_t x, int16_t y, int16_t length,
		uint16_t color) {
	int16_t x2;

	// Initial off-screen clipping
	if ((length <= 0) || (y < 0) || (y >= _height) || (x >= _width)
			|| ((x2 = (x + length - 1)) < 0))
		return;

	if (x < 0) { // Clip left
		length += x;
		x = 0;
	}
	if (x2 >= _width) { // Clip right
		x2 = _width - 1;
		length = x2 - x + 1;
	}

	setAddrWindow(x, y, x2, y);
	flood(color, length);
	setLR();
}

void Adafruit_TFTLCD::drawFastVLine(int16_t x, int16_t y, int16_t length,
		uint16_t color) {
	int16_t y2;

	// Initial off-screen clipping
	if ((length <= 0) || (x < 0) || (x >= _width) || (y >= _height)
			|| ((y2 = (y + length - 1)) < 0))
		return;
	if (y < 0) { // Clip top
		length += y;
		y = 0;
	}
	if (y2 >= _height) { // Clip bottom
		y2 = _height - 1;
		length = y2 - y + 1;
	}

	setAddrWindow(x, y, x, y2);
	flood(color, length);
	setLR();
}

void Adafruit_TFTLCD::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h,
		uint16_t fillcolor) {
	int16_t x2, y2;

	// Initial off-screen clipping
	if ((w <= 0) || (h <= 0) || (x1 >= _width) || (y1 >= _height)
			|| ((x2 = x1 + w - 1) < 0) || ((y2 = y1 + h - 1) < 0))
		return;
	if (x1 < 0) { // Clip left
		w += x1;
		x1 = 0;
	}
	if (y1 < 0) { // Clip top
		h += y1;
		y1 = 0;
	}
	if (x2 >= _width) { // Clip right
		x2 = _width - 1;
		w = x2 - x1 + 1;
	}
	if (y2 >= _height) { // Clip bottom
		y2 = _height - 1;
		h = y2 - y1 + 1;
	}

	setAddrWindow(x1, y1, x2, y2);
	flood(fillcolor, (uint32_t) w * (uint32_t) h);
	setLR();
}

void Adafruit_TFTLCD::fillScreen(uint16_t color) {

	// For these, there is no settable address pointer, instead the
	// address window must be set for each drawing operation.  However,
	// this display takes rotation into account for the parameters, no
	// need to do extra rotation math here.
	setAddrWindow(0, 0, _width - 1, _height - 1);

	flood(color, (long) TFTWIDTH * (long) TFTHEIGHT);
}

void Adafruit_TFTLCD::drawPixel(int16_t x, int16_t y, uint16_t color) {

	// Clip
	if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
		return;

	CS_ACTIVE;

	setAddrWindow(x, y, _width - 1, _height - 1);
	CS_ACTIVE;
	CD_COMMAND;
	write8(0x2C);
	CD_DATA;
	write8(color >> 8);
	write8(color);


	CS_IDLE;
}

// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
void Adafruit_TFTLCD::pushColors(uint16_t *data, uint8_t len, bool first) {
	uint16_t color;
	uint8_t hi, lo;
	CS_ACTIVE;
	if (first == true) { // Issue GRAM write command only on first call
		CD_COMMAND;
		write8(0x2C);
	}
	CD_DATA;
	while (len--) {
		color = *data++;
		hi = color >> 8; // Don't simplify or merge these
		lo = color;      // lines, there's macro shenanigans
		write8(hi);      // going on.
		write8(lo);
	}
	CS_IDLE;
}

void Adafruit_TFTLCD::setRotation(uint8_t x) {

	// Then perform hardware-specific rotation operations...

	CS_ACTIVE;
	// MEME, HX8357D uses same registers as 9341 but different values
	uint16_t t = 0;

	switch (rotation) {
	case 2:
		t = HX8357B_MADCTL_RGB;
		break;
	case 3:
		t = HX8357B_MADCTL_MX | HX8357B_MADCTL_MV | HX8357B_MADCTL_RGB;
		break;
	case 0:
		t = HX8357B_MADCTL_MX | HX8357B_MADCTL_MY | HX8357B_MADCTL_RGB;
		break;
	case 1:
		t = HX8357B_MADCTL_MY | HX8357B_MADCTL_MV | HX8357B_MADCTL_RGB;
		break;
	}
	writeRegister8(ILI9341_MADCTL, t); // MADCTL
	// For 8357, init default full-screen address window:
	setAddrWindow(0, 0, _width - 1, _height - 1); // CS_IDLE happens here

}


// Unsupported.
uint16_t Adafruit_TFTLCD::readPixel(int16_t x, int16_t y) {

	return 0;
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t Adafruit_TFTLCD::color565(uint8_t r, uint8_t g, uint8_t b) {
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// For I/O macros that were left undefined, declare function
// versions that reference the inline macros just once:

void Adafruit_TFTLCD::write8(uint8_t value) {
//  {                                                                            \
//    PORTD = (PORTD & B00000011) | ((d)&B11111100);                             \
//    PORTB = (PORTB & B11111100) | ((d)&B00000011);                             \
//    WR_STROBE;                                                                 \
//  }
	// Write, then do WR strobe
	HAL_GPIO_WritePin(D0_BANK, D0_PIN, static_cast<GPIO_PinState>(value & 0x1));
	HAL_GPIO_WritePin(D1_BANK, D1_PIN, static_cast<GPIO_PinState>((value >> 1) & 0x1));
	HAL_GPIO_WritePin(D2_BANK, D2_PIN, static_cast<GPIO_PinState>((value >> 2) & 0x1));
	HAL_GPIO_WritePin(D3_BANK, D3_PIN, static_cast<GPIO_PinState>((value >> 3) & 0x1));
	HAL_GPIO_WritePin(D4_BANK, D4_PIN, static_cast<GPIO_PinState>((value >> 4) & 0x1));
	HAL_GPIO_WritePin(D5_BANK, D5_PIN, static_cast<GPIO_PinState>((value >> 5) & 0x1));
	HAL_GPIO_WritePin(D6_BANK, D6_PIN, static_cast<GPIO_PinState>((value >> 6) & 0x1));
	HAL_GPIO_WritePin(D7_BANK, D7_PIN, static_cast<GPIO_PinState>((value >> 7) & 0x1));

	WR_STROBE;
}


// TODO: Figure out what this is and if we need it.
void Adafruit_TFTLCD::setWriteDir(void) {
//	setWriteDirInline();
}

// TODO: Figure out what this is and if we need it.
void Adafruit_TFTLCD::setReadDir(void) {
}

void Adafruit_TFTLCD::writeRegister8(uint8_t a, uint8_t d) {
	CD_COMMAND;
	write8(a);
	CD_DATA;
	write8(d);
}

void Adafruit_TFTLCD::writeRegister16(uint16_t a, uint16_t d) {
	uint8_t hi, lo;
	hi = (a) >> 8;
	lo = (a);
	CD_COMMAND;
	write8(hi);
	write8(lo);
	hi = (d) >> 8;
	lo = (d);
	CD_DATA;
	write8(hi);
	write8(lo);
}

void Adafruit_TFTLCD::writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d) {
	uint8_t hi = (d) >> 8, lo = (d);
	CD_COMMAND;
	write8(aH);
	CD_DATA;
	write8(hi);
	CD_COMMAND;
	write8(aL);
	CD_DATA;
	write8(lo);
}

void Adafruit_TFTLCD::writeRegister24(uint8_t r, uint32_t d) {
	CS_ACTIVE;
	CD_COMMAND;
	write8(r);
	CD_DATA;
	HAL_Delay(10000);
	write8(d >> 16);
	HAL_Delay(10000);
	write8(d >> 8);
	HAL_Delay(10000);
	write8(d);
	CS_IDLE;
}

void Adafruit_TFTLCD::writeRegister32(uint8_t r, uint32_t d) {
	CS_ACTIVE;
	CD_COMMAND;
	write8(r);
	CD_DATA;
	HAL_Delay(10000);
	write8(d >> 24);
	HAL_Delay(10000);
	write8(d >> 16);
	HAL_Delay(10000);
	write8(d >> 8);
	HAL_Delay(10000);
	write8(d);
	CS_IDLE;
}
