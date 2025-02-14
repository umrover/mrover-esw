/*
 * lcd.c
 *
 *  Created on: Feb 11, 2025
 *      Author: AMXCh
 */
#include "lcd.h"
#include "main.h"

extern SPI_HandleTypeDef hspi2;

void init () {
	RST_L();
	HAL_Delay(10);
	RST_H();
	HAL_Delay(50);

//	SendCommand(0x11);
//	HAL_Delay(120);
//
//	SendCommand(0x36);
//	SendData(0xC0);
//	SendCommand (0x3A);
//	SendData(0x55);
//	SendCommand (0x29);

	SendCommand(HX8357_SWRESET);
	SendData(0x80);
	HAL_Delay(10);
	SendCommand(HX8357D_SETC);
	SendData(0xFF);
	SendData(0x83);
	SendData(0x57);
	HAL_Delay(300);

	SendCommand(HX8357D_SETRGB);
	SendData(0x80);
	SendData(0x00);
	SendData(0x06);
	SendData (0x06);

	SendCommand(HX8357D_SETCOM);
	SendData (0x25);

	SendCommand(HX8357_SETOSC);
	SendData(0x68);

	SendCommand(HX8357_SETPANEL);
	SendData(0x05);

	SendCommand(HX8357_SETPWR1);
	SendData(0x00);
	SendData(0x15);
	SendData(0x1C);
	SendData(0x1C);
	SendData(0x83);
	SendData(0xAA);

	SendCommand(HX8357D_SETSTBA);
	SendData(0x50);
	SendData(0x50);
	SendData(0x01);
	SendData(0x3C);
	SendData(0x1E);
	SendData(0x08);

	SendCommand(HX8357D_SETCYC);
	SendData(0x02);
	SendData(0x40);
	SendData (0x00);
	SendData(0x2A);
	SendData(0x2A);
	SendData     (0x0D);
	SendData(0x78);

	SendCommand (   HX8357D_SETGAMMA); //34
	SendData (0x02);
	SendData(0x0A);
	SendData (0x11);
	SendData (0x1d);
	SendData (0x23);
	SendData(0x35);
	SendData (0x41);
	SendData(0x4b);
	SendData(0x4b);
	SendData(0x42);
	SendData(0x3A);
	SendData(0x27);
	SendData(0x1B);
	SendData(0x08);
	SendData(0x09);
	SendData(0x03);
	SendData(0x02);
	SendData(0x0A);
	SendData(0x11);
	SendData(0x1d);
	SendData(0x23);
	SendData(0x35);
	SendData(0x41);
	SendData(0x4b);
	SendData(0x4b);
	SendData(0x42);
	SendData(0x3A);
	SendData(0x27);
	SendData(0x1B);
	SendData(0x08);
	SendData(0x09);
	SendData(0x03);
	SendData(0x00);
	SendData(0x01);

	SendCommand(HX8357_COLMOD); //1
	SendData(0x55); // 16 bit

	SendCommand(HX8357_MADCTL);// 1,
	SendData(0xC0);
	SendCommand(HX8357_TEON); // 1
	SendData(0x00);    // TW off
	SendCommand(HX8357_TEARLINE); // 2
	SendData(0x00);
	SendData(0x02);

	SendCommand(HX8357_SLPOUT);
	SendData(0x80);
	HAL_Delay(150); // Exit Sleep, then delay 150 ms

	SendCommand(HX8357_DISPON);
	SendData(0x80);
	HAL_Delay(50); // Main screen turn on, delay 50 ms
//	    0,                           // END OF COMMAND LIST

}



void SendByte (uint8_t data) {
	SPI2->DR = data;
}

void SendCommand (uint8_t com) {
	uint8_t tmpCmd = com;
	DC_H();
	CS_L();
	HAL_SPI_Transmit (&hspi2, &tmpCmd, 1, 1);
	CS_H();
}

void SendData (uint8_t data) {
	uint8_t tmpCmd = data;
	DC_L();
	CS_L();
	HAL_SPI_Transmit (&hspi2, &tmpCmd, 1, 1);
	CS_H();
}

