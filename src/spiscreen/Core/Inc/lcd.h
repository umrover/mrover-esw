/*
 * lcd.h
 *
 *  Created on: Feb 11, 2025
 *      Author: AMXCh
 */

#include "main.h"

#ifndef INC_LCD_H_
#define INC_LCD_H_

#define CS_Pin GPIO_PIN_5
#define CS_Port GPIOB

#define DC_Pin GPIO_PIN_4
#define DC_Port GPIOB

#define RST_Pin GPIO_PIN_9
#define RST_Port GPIOB

#define RST_L() HAL_GPIO_WritePin(RST_Port, RST_Pin, GPIO_PIN_RESET)
#define RST_H() HAL_GPIO_WritePin(RST_Port, RST_Pin, GPIO_PIN_SET)

#define CS_L() HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_RESET)
#define CS_H() HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET)

#define DC_L() HAL_GPIO_WritePin(DC_Port, DC_Pin, GPIO_PIN_RESET)
#define DC_H() HAL_GPIO_WritePin(DC_Port, DC_Pin, GPIO_PIN_SET)

void init();
void SendByte (uint8_t data);
void SendCommand (uint8_t com);
#endif /* INC_LCD_H_ */
