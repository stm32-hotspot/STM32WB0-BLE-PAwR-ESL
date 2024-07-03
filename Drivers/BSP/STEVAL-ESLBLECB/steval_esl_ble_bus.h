/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : steval_esl_ble_bus.h
  * @brief          : header file for the BSP BUS IO driver
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#ifndef STEVAL_ESL_BLE_BUS_H
#define STEVAL_ESL_BLE_BUS_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "steval_esl_ble_conf.h"
#include "steval_esl_ble_errno.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STEVAL_ESL_BLE
  * @{
  */

/** @defgroup STEVAL_ESL_BLE_BUS STEVAL_ESL_BLE BUS
  * @{
  */

/** @defgroup STEVAL_ESL_BLE_BUS_Exported_Constants STEVAL_ESL_BLE BUS Exported Constants
  * @{
  */   
    
#define I2Cx                                            I2C1
#define I2Cx_SCL_PORT                                   GPIOA
#define I2Cx_SCL_PWR_PORT                               LL_PWR_GPIO_A
#define I2Cx_SCL_PIN                                    GPIO_PIN_0
#define I2Cx_SCL_AF                                     GPIO_AF0_I2C1
#define I2Cx_SDA_PORT                                   GPIOB
#define I2Cx_SDA_PWR_PORT                               LL_PWR_GPIO_B
#define I2Cx_SDA_PIN                                    GPIO_PIN_7
#define I2Cx_SDA_AF                                     GPIO_AF0_I2C1
#define __HAL_RCC_I2Cx_CLK_ENABLE                       __HAL_RCC_I2C1_CLK_ENABLE
#define __HAL_RCC_I2Cx_CLK_DISABLE                      __HAL_RCC_I2C1_CLK_DISABLE
#define __HAL_RCC_I2Cx_SCL_GPIO_CLK_ENABLE              __HAL_RCC_GPIOA_CLK_ENABLE
#define __HAL_RCC_I2Cx_SDA_GPIO_CLK_ENABLE              __HAL_RCC_GPIOB_CLK_ENABLE
#define I2Cx_IRQn                                       I2C1_IRQn
#define I2Cx_IRQHandler                                 I2C1_IRQHandler
    
    
    
#define I2C_FASTMODEPLUS_I2C1   I2C_FASTMODEPLUS_PA0

#ifndef BUS_I2C1_POLL_TIMEOUT
   #define BUS_I2C1_POLL_TIMEOUT                0x1000U
#endif
/* I2C1 Frequeny in Hz  */
#ifndef BUS_I2C1_FREQUENCY
   #define BUS_I2C1_FREQUENCY  1000000U /* Frequency of I2Cn = 100 KHz*/
#endif

/**
  * @}
  */

/** @defgroup STEVAL_ESL_BLE_BUS_Private_Types STEVAL_ESL_BLE BUS Private types
  * @{
  */
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1U)
typedef struct
{
  pI2C_CallbackTypeDef  pMspInitCb;
  pI2C_CallbackTypeDef  pMspDeInitCb;
}BSP_I2C_Cb_t;
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1U) */
/**
  * @}
  */

/** @defgroup STEVAL_ESL_BLE_LOW_LEVEL_Exported_Variables LOW LEVEL Exported Constants
  * @{
  */

extern I2C_HandleTypeDef hi2c1;

/**
  * @}
  */

/** @addtogroup STEVAL_ESL_BLE_BUS_Exported_Functions
  * @{
  */

/* BUS IO driver over I2C Peripheral */
HAL_StatusTypeDef MX_I2C1_Init(I2C_HandleTypeDef* hi2c);
int32_t BSP_I2C1_Init(void);
int32_t BSP_I2C1_DeInit(void);
int32_t BSP_I2C1_IsReady(uint16_t DevAddr, uint32_t Trials);
int32_t BSP_I2C1_WriteReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_WriteReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_ReadReg16(uint16_t Addr, uint16_t Reg, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_Send(uint16_t DevAddr, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_Recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length);
int32_t BSP_I2C1_SendRecv(uint16_t DevAddr, uint8_t *pTxdata, uint8_t *pRxdata, uint16_t Length);
#if (USE_HAL_I2C_REGISTER_CALLBACKS == 1U)
int32_t BSP_I2C1_RegisterDefaultMspCallbacks (void);
int32_t BSP_I2C1_RegisterMspCallbacks (BSP_I2C_Cb_t *Callbacks);
#endif /* (USE_HAL_I2C_REGISTER_CALLBACKS == 1U) */

int32_t BSP_GetTick(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /* STEVAL_ESL_BLE_BUS_H */

