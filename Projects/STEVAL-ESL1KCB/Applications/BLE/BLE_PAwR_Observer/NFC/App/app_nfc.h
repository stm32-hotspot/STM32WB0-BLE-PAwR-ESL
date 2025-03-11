
/**
  ******************************************************************************
  * File Name          :  stmicroelectronics_x-cube-nfc4_3_0_0.h
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-NFC4.3.0.0 instances.
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
#ifndef APP_NFC_H
#define APP_NFC_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "st25dv.h"
/* Exported Functions --------------------------------------------------------*/
void MX_NFC4_NDEF_Init(void);
void MX_NFC4_NDEF_URI_Set(const char * uri);
void MX_NFC_Process(void);


extern ST25DV_UID NFC_UUID;

#ifdef __cplusplus
}
#endif
#endif /* __INIT_H */

