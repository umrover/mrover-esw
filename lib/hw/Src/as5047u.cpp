#include "as5047u.hpp"

// ---------- Internal helpers ----------
static uint16_t addParity(uint16_t value) {
    uint16_t cnt = 0;
    for (int i = 0; i < 15; i++) {
        if (value & (1 << i)) cnt++;
    }
    if (cnt % 2 != 0) value |= (1 << 15); // even parity
    return value;
}‘

static void cs_low(AS5047U_HandleTypeDef *hdev) {
    HAL_GPIO_WritePin(hdev->CS_Port, hdev->CS_Pin, GPIO_PIN_RESET);
}

static void cs_high(AS5047U_HandleTypeDef *hdev) {
    HAL_GPIO_WritePin(hdev->CS_Port, hdev->CS_Pin, GPIO_PIN_SET);
}

// ---------- Public API ----------
void AS5047U_Init(AS5047U_HandleTypeDef *hdev, SPI_HandleTypeDef *hspi,
                  GPIO_TypeDef *CS_Port, uint16_t CS_Pin) {
    hdev->hspi = hspi;
    hdev->CS_Port = CS_Port;
    hdev->CS_Pin = CS_Pin;
    HAL_GPIO_WritePin(CS_Port, CS_Pin, GPIO_PIN_SET);
}

uint16_t AS5047U_ReadReg(AS5047U_HandleTypeDef *hdev, uint16_t reg) {
    uint16_t cmd = (1 << 14) | (reg & 0x3FFF); // read command
    cmd = addParity(cmd);

    uint16_t tx = cmd, rx = 0;
    cs_low(hdev);
    HAL_SPI_TransmitReceive(hdev->hspi, (uint8_t*)&tx, (uint8_t*)&rx, 1, HAL_MAX_DELAY);
    cs_high(hdev);

    // dummy read to get valid data
    tx = addParity((1 << 14) | AS5047U_REG_NOP);
    cs_low(hdev);
    HAL_SPI_TransmitReceive(hdev->hspi, (uint8_t*)&tx, (uint8_t*)&rx, 1, HAL_MAX_DELAY);
    cs_high(hdev);

    return rx & 0x3FFF;
}

float AS5047U_ReadAngle(AS5047U_HandleTypeDef *hdev) {
    uint16_t raw = AS5047U_ReadReg(hdev, AS5047U_REG_ANGLECOM);
    return (raw * 360.0f) / 16384.0f;   // 14-bit → degrees
}

float AS5047U_ReadVelocity(AS5047U_HandleTypeDef *hdev) {
    uint16_t raw = AS5047U_ReadReg(hdev, AS5047U_REG_VEL);
    if (raw & 0x2000) {   // check sign bit (bit 13)
        raw |= 0xC000;    // sign-extend
    }
    int16_t vel = (int16_t)raw;
    return vel * 24.141f; // convert to deg/s (datasheet sensitivity):contentReference[oaicite:0]{index=0}
}

