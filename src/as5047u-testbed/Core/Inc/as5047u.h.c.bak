#ifndef __AS5047U_H
#define __AS5047U_H

#include "stm32g4xx_hal.h"
#include <stdint.h>

#define AS5047U_REG_NOP       0x0000
#define AS5047U_REG_ERRFL     0x0001
#define AS5047U_REG_VEL       0x3FFC
#define AS5047U_REG_ANGLEUNC  0x3FFE
#define AS5047U_REG_ANGLECOM  0x3FFF

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *CS_Port;
    uint16_t CS_Pin;
} AS5047U_HandleTypeDef;

void AS5047U_Init(AS5047U_HandleTypeDef *hdev, SPI_HandleTypeDef *hspi,
                  GPIO_TypeDef *CS_Port, uint16_t CS_Pin);

uint16_t AS5047U_ReadReg(AS5047U_HandleTypeDef *hdev, uint16_t reg);
float AS5047U_ReadAngle(AS5047U_HandleTypeDef *hdev);
float  AS5047U_ReadVelocity(AS5047U_HandleTypeDef *hdev);

#endif
