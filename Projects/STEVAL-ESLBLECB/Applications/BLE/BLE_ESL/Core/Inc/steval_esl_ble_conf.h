/**
  ******************************************************************************
  * @file    steval_esl_ble_conf.h
  * @author  MCD Application Team
  * @brief   STM32WB0X ESL board configuration file.
  *          This file should be copied to the application folder and renamed
  *          to steval_esl_ble_conf.h .
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STEVAL_ESL_BLE_CONF_H
#define STEVAL_ESL_BLE_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wb0x_hal.h"

/* Usage of COM feature */
#define USE_BSP_COM_FEATURE 1U
#define USE_COM_LOG         1U

/* Button interrupt priorities */
#define BSP_B1_IT_PRIORITY 0x0FUL  /* Default is lowest priority level */
#define BSP_B2_IT_PRIORITY 0x0FUL  /* Default is lowest priority level */
#define BSP_B3_IT_PRIORITY 0x0FUL  /* Default is lowest priority level */

#ifdef __cplusplus
}
#endif

#endif /* STEVAL_ESL_BLE_CONF_H */
