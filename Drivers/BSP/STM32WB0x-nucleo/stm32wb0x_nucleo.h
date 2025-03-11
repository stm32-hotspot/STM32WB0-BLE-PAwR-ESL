/**
  ******************************************************************************
  * @file    stm32wb0x_nucleo.h
  * @author  MCD Application Team
  * @brief   This file contains definitions for:
  *          - LEDs and push-button available on STM32WB0X-Nucleo Kit
  *            from STMicroelectronics
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
#ifndef STM32WB0X_NUCLEO_H
#define STM32WB0X_NUCLEO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wb0x_nucleo_conf.h"
#include "stm32wb0x_nucleo_errno.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32WB0X_NUCLEO
  * @{
  */

/** @addtogroup STM32WB0X_NUCLEO_COMMON
  * @{
  */

/** @defgroup STM32WB0X_NUCLEO_COMMON_Exported_Types STM32WB0X_NUCLEO COMMON Exported Types
  * @{
  */
typedef enum
{
  LD1 = 0,
  LED_BLUE = LD1,
  LD2 = 1,
  LED_GREEN = LD2,
  LD3 = 2,
  LED_RED = LD3
} Led_TypeDef;

typedef enum
{
  B1 = 0,
  B2 = 1,
  B3 = 2
} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

#if (USE_BSP_COM_FEATURE == 1)
typedef enum
{
  COM1 = 0
} COM_TypeDef;

typedef enum
{
  COM_WORDLENGTH_7B = UART_WORDLENGTH_7B,
  COM_WORDLENGTH_8B = UART_WORDLENGTH_8B,
  COM_WORDLENGTH_9B = UART_WORDLENGTH_9B
} COM_WordLengthTypeDef;

typedef enum
{
  COM_STOPBITS_1 = UART_STOPBITS_1,
  COM_STOPBITS_2 = UART_STOPBITS_2
} COM_StopBitsTypeDef;

typedef enum
{
  COM_PARITY_NONE = UART_PARITY_NONE,
  COM_PARITY_EVEN = UART_PARITY_EVEN,
  COM_PARITY_ODD  = UART_PARITY_ODD
} COM_ParityTypeDef;

typedef enum
{
  COM_HWCONTROL_NONE    = UART_HWCONTROL_NONE,
  COM_HWCONTROL_RTS     = UART_HWCONTROL_RTS,
  COM_HWCONTROL_CTS     = UART_HWCONTROL_CTS,
  COM_HWCONTROL_RTS_CTS = UART_HWCONTROL_RTS_CTS
} COM_HwFlowCtlTypeDef;

typedef struct
{
  uint32_t              BaudRate;
  COM_WordLengthTypeDef WordLength;
  COM_StopBitsTypeDef   StopBits;
  COM_ParityTypeDef     Parity;
  COM_HwFlowCtlTypeDef  HwFlowCtl;
} COM_InitTypeDef;

#define MX_UART_InitTypeDef COM_InitTypeDef

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
typedef struct
{
  pUART_CallbackTypeDef  pMspInitCb;
  pUART_CallbackTypeDef  pMspDeInitCb;
} BSP_COM_Cb_t;
#endif
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/** @defgroup STM32WB0X_NUCLEO_COMMON_Exported_Constants STM32WB0X_NUCLEO COMMON Exported Constants
  * @{
  */

/**
  * @brief STM32WB0x NUCLEO BSP Driver version number
  */
#define STM32WB0x_NUCLEO_BSP_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define STM32WB0x_NUCLEO_BSP_VERSION_SUB1   (0x02U) /*!< [23:16] sub1 version */
#define STM32WB0x_NUCLEO_BSP_VERSION_SUB2   (0x00U) /*!< [15:8]  sub2 version */
#define STM32WB0x_NUCLEO_BSP_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define STM32WB0x_NUCLEO_BSP_VERSION       ((STM32WB0x_NUCLEO_BSP_VERSION_MAIN << 24U)\
                                            |(STM32WB0x_NUCLEO_BSP_VERSION_SUB1 << 16U)\
                                            |(STM32WB0x_NUCLEO_BSP_VERSION_SUB2 << 8U )\
                                            |(STM32WB0x_NUCLEO_BSP_VERSION_RC))

#if defined(NUCLEO_WB09KE)
#define STM32WB0X_NUCLEO_BSP_BOARD_NAME  "NUCLEO_WB09KE"
#define STM32WB0X_NUCLEO_BSP_BOARD_ID    "NUCLEO$WB09KE"
#elif defined(NUCLEO_WB07CC)
#define STM32WB0X_NUCLEO_BSP_BOARD_NAME  "NUCLEO_WB07CC"
#define STM32WB0X_NUCLEO_BSP_BOARD_ID    "NUCLEO$WB07CC"
#elif defined(NUCLEO_WB05KZ)
#define STM32WB0X_NUCLEO_BSP_BOARD_NAME  "NUCLEO_WB05KZ"
#define STM32WB0X_NUCLEO_BSP_BOARD_ID    "NUCLEO$WB05KZ"
#endif
/** @defgroup STM32WB0X_NUCLEO_COMMON_LED STM32WB0X_NUCLEO COMMON LED
  * @{
  */
#define LEDn                                   3U
#if defined(NUCLEO_WB09KE) || defined(NUCLEO_WB05KZ)
#define LD1_GPIO_PORT                          GPIOB
#define LD1_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define LD1_PIN                                GPIO_PIN_1

#elif defined(NUCLEO_WB07CC)
#define LD1_GPIO_PORT                          GPIOB
#define LD1_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define LD1_PIN                                GPIO_PIN_0
#endif


#define LD2_GPIO_PORT                          GPIOB
#define LD2_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define LD2_PIN                                GPIO_PIN_4

#define LD3_GPIO_PORT                          GPIOB
#define LD3_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOB_CLK_ENABLE()
#define LD3_PIN                                GPIO_PIN_2


/**
  * @}
  */

/** @defgroup STM32WB0X_NUCLEO_COMMON_BUTTON STM32WB0X_NUCLEO COMMON BUTTON
  * @{
  */
#define BUTTONn                            3U

#define B1_GPIO_PORT                       GPIOA
#define B1_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define B1_PIN                             GPIO_PIN_0
#define B1_EXTI_IRQn                       GPIOA_IRQn

#define B2_GPIO_PORT                       GPIOB
#define B2_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()
#define B2_PIN                             GPIO_PIN_5
#define B2_EXTI_IRQn                       GPIOB_IRQn

#if defined(NUCLEO_WB09KE) || defined(NUCLEO_WB05KZ)

#define B3_GPIO_PORT                       GPIOB
#define B3_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()
#define B3_PIN                             GPIO_PIN_14
#define B3_EXTI_IRQn                       GPIOB_IRQn

#elif defined(NUCLEO_WB07CC)

#define B3_GPIO_PORT                       GPIOB
#define B3_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()
#define B3_PIN                             GPIO_PIN_9
#define B3_EXTI_IRQn                       GPIOB_IRQn
#endif

/**
  * @}
  */

#if (USE_BSP_COM_FEATURE == 1)
/** @defgroup STM32WB0X_NUCLEO_COMMON_COM STM32WB0X_NUCLEO COMMON COM
  * @{
  */
#define COMn                                    1U
#if defined(NUCLEO_WB07CC)
#define COM1_UART                               USART1
#define COM1_CLK_ENABLE()                       __HAL_RCC_USART1_CLK_ENABLE()
#define COM1_CLK_DISABLE()                      __HAL_RCC_USART1_CLK_DISABLE()
#define COM1_TX_GPIO_PORT                       GPIOA
#define COM1_TX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define COM1_TX_PIN                             GPIO_PIN_9
#define COM1_TX_AF                              GPIO_AF0_USART1
#define COM1_RX_GPIO_PORT                       GPIOA
#define COM1_RX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define COM1_RX_PIN                             GPIO_PIN_8
#define COM1_RX_AF                              GPIO_AF0_USART1
#else
#define COM1_UART                               USART1
#define COM1_CLK_ENABLE()                       __HAL_RCC_USART1_CLK_ENABLE()
#define COM1_CLK_DISABLE()                      __HAL_RCC_USART1_CLK_DISABLE()
#define COM1_TX_GPIO_PORT                       GPIOA
#define COM1_TX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOA_CLK_ENABLE()
#define COM1_TX_PIN                             GPIO_PIN_1
#define COM1_TX_AF                              GPIO_AF2_USART1
#define COM1_RX_GPIO_PORT                       GPIOB
#define COM1_RX_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()
#define COM1_RX_PIN                             GPIO_PIN_0
#define COM1_RX_AF                              GPIO_AF0_USART1
#endif
/**
  * @}
  */
#endif /* (USE_BSP_COM_FEATURE == 1) */

/**
  * @}
  */

/** @addtogroup STM32WB0X_NUCLEO_COMMON_Exported_Variables
  * @{
  */
#if (USE_BSP_COM_FEATURE == 1)
extern UART_HandleTypeDef hcom_uart[COMn];
#endif
/**
  * @}
  */

/** @addtogroup STM32WB0X_NUCLEO_COMMON_Exported_Functions
  * @{
  */
int32_t        BSP_GetVersion(void);
const uint8_t* BSP_GetBoardName(void);
const uint8_t* BSP_GetBoardID(void);

/** @addtogroup STM32WB0X_NUCLEO_COMMON_LED_Functions
  * @{
  */
int32_t BSP_LED_Init(Led_TypeDef Led);
int32_t BSP_LED_DeInit(Led_TypeDef Led);
int32_t BSP_LED_On(Led_TypeDef Led);
int32_t BSP_LED_Off(Led_TypeDef Led);
int32_t BSP_LED_Toggle(Led_TypeDef Led);
int32_t BSP_LED_GetState(Led_TypeDef Led);
/**
  * @}
  */

/** @addtogroup STM32WB0X_NUCLEO_COMMON_BUTTON_Functions
  * @{
  */
int32_t BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
int32_t BSP_PB_DeInit(Button_TypeDef Button);
int32_t BSP_PB_GetState(Button_TypeDef Button);
void    BSP_PB_Callback(Button_TypeDef Button);
void    BSP_PB_IRQHandler(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
  * @}
  */

#if (USE_BSP_COM_FEATURE == 1)
/** @addtogroup STM32WB0X_NUCLEO_COMMON_COM_Functions
  * @{
  */
int32_t BSP_COM_Init(COM_TypeDef COM, COM_InitTypeDef *COM_Init);
int32_t BSP_COM_DeInit(COM_TypeDef COM);
#if (USE_COM_LOG == 1)
int32_t BSP_COM_SelectLogPort(COM_TypeDef COM);
#endif
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
int32_t BSP_COM_RegisterDefaultMspCallbacks(COM_TypeDef COM);
int32_t BSP_COM_RegisterMspCallbacks(COM_TypeDef COM, BSP_COM_Cb_t *CallBacks);
#endif
HAL_StatusTypeDef MX_USART1_Init(UART_HandleTypeDef* huart, MX_UART_InitTypeDef *MXInit);
/**
  * @}
  */
#endif /* (USE_BSP_COM_FEATURE == 1) */

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

#endif /* STM32WB0X_NUCLEO_H */
