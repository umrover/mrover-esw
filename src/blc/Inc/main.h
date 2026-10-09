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
void MX_IWDG_Init(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LIM_A_Pin GPIO_PIN_13
#define LIM_A_GPIO_Port GPIOC
#define LIM_B_Pin GPIO_PIN_14
#define LIM_B_GPIO_Port GPIOC
#define GATE_SS_Pin GPIO_PIN_15
#define GATE_SS_GPIO_Port GPIOC
#define EXT_SDA_Pin GPIO_PIN_0
#define EXT_SDA_GPIO_Port GPIOF
#define EXT_SCK_Pin GPIO_PIN_1
#define EXT_SCK_GPIO_Port GPIOF
#define FET_TEMP_Pin GPIO_PIN_0
#define FET_TEMP_GPIO_Port GPIOA
#define EXT_SS_Pin GPIO_PIN_1
#define EXT_SS_GPIO_Port GPIOA
#define VCP_TX_Pin GPIO_PIN_2
#define VCP_TX_GPIO_Port GPIOA
#define VCP_RX_Pin GPIO_PIN_3
#define VCP_RX_GPIO_Port GPIOA
#define BAT_ADC_Pin GPIO_PIN_4
#define BAT_ADC_GPIO_Port GPIOA
#define GATE_SCK_Pin GPIO_PIN_5
#define GATE_SCK_GPIO_Port GPIOA
#define GATE_MISO_Pin GPIO_PIN_6
#define GATE_MISO_GPIO_Port GPIOA
#define GATE_MOSI_Pin GPIO_PIN_7
#define GATE_MOSI_GPIO_Port GPIOA
#define EXT_SCL_Pin GPIO_PIN_4
#define EXT_SCL_GPIO_Port GPIOC
#define I_A_Pin GPIO_PIN_0
#define I_A_GPIO_Port GPIOB
#define I_B_Pin GPIO_PIN_1
#define I_B_GPIO_Port GPIOB
#define I_C_Pin GPIO_PIN_2
#define I_C_GPIO_Port GPIOB
#define HALL_B_Pin GPIO_PIN_10
#define HALL_B_GPIO_Port GPIOB
#define HALL_B_EXTI_IRQn EXTI15_10_IRQn
#define HALL_C_Pin GPIO_PIN_11
#define HALL_C_GPIO_Port GPIOB
#define HALL_C_EXTI_IRQn EXTI15_10_IRQn
#define PWM_CH_Pin GPIO_PIN_12
#define PWM_CH_GPIO_Port GPIOB
#define PWM_CL_Pin GPIO_PIN_13
#define PWM_CL_GPIO_Port GPIOB
#define PWM_BH_Pin GPIO_PIN_14
#define PWM_BH_GPIO_Port GPIOB
#define PWM_BL_Pin GPIO_PIN_15
#define PWM_BL_GPIO_Port GPIOB
#define GATE_EN_Pin GPIO_PIN_6
#define GATE_EN_GPIO_Port GPIOC
#define PWM_AH_Pin GPIO_PIN_8
#define PWM_AH_GPIO_Port GPIOA
#define PWM_AL_Pin GPIO_PIN_9
#define PWM_AL_GPIO_Port GPIOA
#define EXT_MISO_Pin GPIO_PIN_10
#define EXT_MISO_GPIO_Port GPIOA
#define EXT_MOSI_Pin GPIO_PIN_11
#define EXT_MOSI_GPIO_Port GPIOA
#define NFAULT_Pin GPIO_PIN_12
#define NFAULT_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define FDCAN_TX_LED_Pin GPIO_PIN_15
#define FDCAN_TX_LED_GPIO_Port GPIOA
#define FDCAN_RX_LED_Pin GPIO_PIN_10
#define FDCAN_RX_LED_GPIO_Port GPIOC
#define ENC_SS_Pin GPIO_PIN_11
#define ENC_SS_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define FDCAN_STB_Pin GPIO_PIN_4
#define FDCAN_STB_GPIO_Port GPIOB
#define FDCAN_RX_Pin GPIO_PIN_5
#define FDCAN_RX_GPIO_Port GPIOB
#define FDCAN_TX_Pin GPIO_PIN_6
#define FDCAN_TX_GPIO_Port GPIOB
#define HALL_A_Pin GPIO_PIN_9
#define HALL_A_GPIO_Port GPIOB
#define HALL_A_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
