/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32wb0x_hal.h"
#include "app_entry.h"
#include "app_common.h"
#include "app_debug.h"
#include "compiler.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "steval_esl_ble.h"
#include "app_nfc.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
  
/* Pin configuration for display */
  
/**SPI_MASTER GPIO Configuration
  PB3/AF4    ------> SPI3_SCK
  PA11/AF3   ------> SPI3_MOSI
  */
#define SPI_MASTER                              SPI3
#define GPIO_MASTER_SCK                         GPIOB
#define GPIO_MASTER_MOSI                        GPIOA
#define GPIO_AF_SPI_MASTER_SCK                  GPIO_AF4_SPI3
#define GPIO_AF_SPI_MASTER_MOSI                 GPIO_AF3_SPI3
#define GPIO_PIN_SPI_MASTER_SCK                 GPIO_PIN_3
#define GPIO_PIN_SPI_MASTER_MOSI                GPIO_PIN_11
#define __HAL_RCC_SPI_MASTER_CLK_ENABLE         __HAL_RCC_SPI3_CLK_ENABLE
#define __HAL_RCC_SPI_MASTER_FORCE_RESET        __HAL_RCC_SPI3_FORCE_RESET
#define __HAL_RCC_SPI_MASTER_RELEASE_RESET      __HAL_RCC_SPI3_RELEASE_RESET
#define __HAL_RCC_SPI_MASTER_CLK_DISABLE        __HAL_RCC_SPI3_CLK_DISABLE

#define RST_Pin                                 GPIO_PIN_14
#define RST_GPIO_Port                           GPIOB
#define DC_Pin                                  GPIO_PIN_5
#define DC_GPIO_Port                            GPIOB
#define BUSY_Pin                                GPIO_PIN_4
#define BUSY_GPIO_Port                          GPIOB
#define SPI_CS_Pin                              GPIO_PIN_9
#define SPI_CS_GPIO_Port                        GPIOA

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
