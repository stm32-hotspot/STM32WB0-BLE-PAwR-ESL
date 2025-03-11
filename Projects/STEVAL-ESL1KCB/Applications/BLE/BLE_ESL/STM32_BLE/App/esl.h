/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    esl.h
  * @author  MCD Application Team
  * @brief   Header for esl_service.c
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
#ifndef ESL_H
#define ESL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ble_status.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported defines ----------------------------------------------------------*/
/* USER CODE BEGIN ED */

#define ST_COMPANY_ID          0x0030
#define BATT_SENSOR_ID         0x0090
/* USER CODE END ED */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  ESL_SERVICE_ADDR,
  ESL_SERVICE_SYNC_KEY_MATERIAL,
  ESL_SERVICE_RESP_KEY_MATERIAL,
  ESL_SERVICE_CURR_ABS_TIME,
  ESL_SERVICE_CONTROL_POINT,
  ESL_SERVICE_SENSOR_INFO,  
  ESL_SERVICE_LED_INFO,
  /* USER CODE BEGIN Service1_CharOpcode_t */

  /* USER CODE END Service1_CharOpcode_t */

  ESL_SERVICE_CHAROPCODE_LAST
} ESL_SERVICE_CharOpcode_t;

typedef enum
{
  ESL_SERVICE_ADDR_WRITE_EVT,
  ESL_SERVICE_SYNC_KEY_MATERIAL_WRITE_EVT,
  ESL_SERVICE_RESP_KEY_MATERIAL_WRITE_EVT,
  ESL_SERVICE_CURR_ABS_TIME_WRITE_EVT,
  ESL_SERVICE_CONTROL_POINT_WRITE_NO_RESP_EVT,
  ESL_SERVICE_CONTROL_POINT_WRITE_EVT,  
  ESL_SERVICE_CONTROL_POINT_NOTIFY_ENABLED_EVT,
  ESL_SERVICE_CONTROL_POINT_NOTIFY_DISABLED_EVT,

  ESL_SERVICE_LED_INFO_TIME_READ_EVT,
  /* USER CODE BEGIN Service1_OpcodeEvt_t */

  /* USER CODE END Service1_OpcodeEvt_t */

  ESL_SERVICE_BOOT_REQUEST_EVT
} ESL_SERVICE_OpcodeEvt_t;

typedef struct
{
  uint8_t *p_Payload;
  uint8_t Length;

  /* USER CODE BEGIN Service1_Data_t */

  /* USER CODE END Service1_Data_t */

} ESL_SERVICE_Data_t;

typedef struct
{
  ESL_SERVICE_OpcodeEvt_t       EvtOpcode;
  ESL_SERVICE_Data_t             DataTransfered;
  uint16_t                ConnectionHandle;
  uint16_t                AttributeHandle;
  uint8_t                 ServiceInstance;

  /* USER CODE BEGIN Service1_NotificationEvt_t */

  /* USER CODE END Service1_NotificationEvt_t */

} ESL_SERVICE_NotificationEvt_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions ------------------------------------------------------- */
void ESL_SERVICE_Init(void);
void ESL_SERVICE_Notification(ESL_SERVICE_NotificationEvt_t *p_Notification);
tBleStatus ESL_SERVICE_UpdateValue(ESL_SERVICE_CharOpcode_t CharOpcode, ESL_SERVICE_Data_t *pData);
tBleStatus ESL_SERVICE_NotifyValue(ESL_SERVICE_CharOpcode_t CharOpcode, ESL_SERVICE_Data_t *pData, uint16_t ConnectionHandle);
/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif

#endif /*ESL_H */
