/**
  ******************************************************************************
  * @file    nfc04a1_conf.h
  * @author  SRA Application Team
  * @brief   This file contains definitions for the NFC4 components bus interfaces
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NFC_CONF_H__
#define __NFC_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32wb0x_hal.h"
#include "steval_esl_ble_bus.h"
#include "steval_esl_ble_errno.h"

#define NFC_I2C_Init         BSP_I2C1_Init
#define NFC_I2C_DeInit       BSP_I2C1_DeInit
#define NFC_I2C_ReadReg16    BSP_I2C1_ReadReg16
#define NFC_I2C_WriteReg16   BSP_I2C1_WriteReg16
#define NFC_I2C_Recv         BSP_I2C1_Recv
#define NFC_I2C_IsReady      BSP_I2C1_IsReady

#define NFC_GetTick          HAL_GetTick

#define NFCTAG_INSTANCE         (0)

#define NFCTAG_GPO_PRIORITY     (0)

#define I2C_INSTANCE  hi2c1
extern I2C_HandleTypeDef hi2c1;

#ifdef __cplusplus
}
#endif

#endif /* __NFC_CONF_H__*/

