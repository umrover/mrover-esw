/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_HRTIM_MspPostInit(HRTIM_HandleTypeDef *hhrtim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define I2C_SDA_Pin GPIO_PIN_0
#define I2C_SDA_GPIO_Port GPIOF
#define ENC_SPI_SCK_Pin GPIO_PIN_1
#define ENC_SPI_SCK_GPIO_Port GPIOF
#define ENC_ONBOARD_SS_Pin GPIO_PIN_0
#define ENC_ONBOARD_SS_GPIO_Port GPIOA
#define ENC_EXT_SS_Pin GPIO_PIN_1
#define ENC_EXT_SS_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_3
#define VCP_RX_GPIO_Port GPIOA
#define GATE_SS_Pin GPIO_PIN_4
#define GATE_SS_GPIO_Port GPIOA
#define GATE_SPI_SCK_Pin GPIO_PIN_5
#define GATE_SPI_SCK_GPIO_Port GPIOA
#define GATE_SPI_MISO_Pin GPIO_PIN_6
#define GATE_SPI_MISO_GPIO_Port GPIOA
#define GATE_SPI_MOSI_Pin GPIO_PIN_7
#define GATE_SPI_MOSI_GPIO_Port GPIOA
#define I2C_SCL_Pin GPIO_PIN_4
#define I2C_SCL_GPIO_Port GPIOC
#define CURRENT_A_Pin GPIO_PIN_0
#define CURRENT_A_GPIO_Port GPIOB
#define CURRENT_B_Pin GPIO_PIN_1
#define CURRENT_B_GPIO_Port GPIOB
#define CURRENT_C_Pin GPIO_PIN_2
#define CURRENT_C_GPIO_Port GPIOB
#define HALL_B_Pin GPIO_PIN_10
#define HALL_B_GPIO_Port GPIOB
#define HALL_C_Pin GPIO_PIN_11
#define HALL_C_GPIO_Port GPIOB
#define PWM_CH_Pin GPIO_PIN_12
#define PWM_CH_GPIO_Port GPIOB
#define PWM_CL_Pin GPIO_PIN_13
#define PWM_CL_GPIO_Port GPIOB
#define PWM_BH_Pin GPIO_PIN_14
#define PWM_BH_GPIO_Port GPIOB
#define PWM_BL_Pin GPIO_PIN_15
#define PWM_BL_GPIO_Port GPIOB
#define CAN_STBY_Pin GPIO_PIN_6
#define CAN_STBY_GPIO_Port GPIOC
#define PWM_AH_Pin GPIO_PIN_8
#define PWM_AH_GPIO_Port GPIOA
#define PWM_AL_Pin GPIO_PIN_9
#define PWM_AL_GPIO_Port GPIOA
#define ENC_SPI_MISO_Pin GPIO_PIN_10
#define ENC_SPI_MISO_GPIO_Port GPIOA
#define ENC_SPI_MOSI_Pin GPIO_PIN_11
#define ENC_SPI_MOSI_GPIO_Port GPIOA
#define CAN_RX_LED_Pin GPIO_PIN_12
#define CAN_RX_LED_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define CAN_TX_LED_Pin GPIO_PIN_15
#define CAN_TX_LED_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define GATE_EN_Pin GPIO_PIN_4
#define GATE_EN_GPIO_Port GPIOB
#define FDCAN_RX_Pin GPIO_PIN_5
#define FDCAN_RX_GPIO_Port GPIOB
#define FDCAN_TX_Pin GPIO_PIN_6
#define FDCAN_TX_GPIO_Port GPIOB
#define HALL_A_Pin GPIO_PIN_9
#define HALL_A_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
