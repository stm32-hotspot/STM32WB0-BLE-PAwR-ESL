
/**
  ******************************************************************************
  * File Name          :  app_nfc.c
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

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "app_nfc.h"
#include "main.h"

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "tagtype5_wrapper.h"
#include "lib_NDEF_URI.h"

/** @defgroup ST25_Nucleo
  * @{
  */

/** @defgroup Main
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define NFC_MAX_TRIES           3
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
sURI_Info URI;
ST25DV_UID NFC_UUID;

extern sCCFileInfo CCFileStruct;

/* Private functions ---------------------------------------------------------*/
void MX_NFC4_NDEF_URI_Init(void);
void MX_NFC4_NDEF_URI_Process(void);

void MX_NFC_Init(void)
{
  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN NFC4_Library_Init_PreTreatment */

  /* USER CODE END NFC4_Library_Init_PreTreatment */

  /* Initialize the peripherals and the NFC4 components */
  MX_NFC4_NDEF_Init();

  /* USER CODE BEGIN SV */

  /* USER CODE END SV */

  /* USER CODE BEGIN NFC4_Library_Init_PostTreatment */

  /* USER CODE END NFC4_Library_Init_PostTreatment */
}
/*
 * LM background task
 */
void MX_NFC_Process(void)
{
  /* USER CODE BEGIN NFC4_Library_Process */

  /* USER CODE END NFC4_Library_Process */

  MX_NFC4_NDEF_URI_Process();

}

void MX_NFC4_NDEF_Init(void)
{
  uint8_t tries = NFC_MAX_TRIES;
  
  NFCTAG_PowerUp();

  /* Init ST25DV driver */
  while( NFCTAG_Init(NFCTAG_INSTANCE) != NFCTAG_OK && (tries--) > 0);
  
  tries = NFC_MAX_TRIES;

  /* Reset Mailbox enable to allow write to EEPROM */
  NFCTAG_ResetMBEN_Dyn(NFCTAG_INSTANCE);

  NfcTag_SelectProtocol(NFCTAG_TYPE5);

  /* Check if no NDEF detected, init mem in Tag Type 5 */
  if( NfcType5_NDEFDetection( ) != NDEF_OK )
  {
    CCFileStruct.MagicNumber = NFCT5_MAGICNUMBER_E1_CCFILE;
    CCFileStruct.Version = NFCT5_VERSION_V1_0;
    CCFileStruct.MemorySize = ( ST25DV_MAX_SIZE / 8 ) & 0xFF;
    CCFileStruct.TT5Tag = 0x05;
    /* Init of the Type Tag 5 component (M24LR) */
    while( NfcType5_TT5Init( ) != NFCTAG_OK  && (tries--) > 0);
  }
  
  NFCTAG_ReadUID(NFCTAG_INSTANCE, &NFC_UUID);
  
  NFCTAG_PowerDown();  
}

void MX_NFC4_NDEF_URI_Set(const char * uri)
{
  uint8_t tries = NFC_MAX_TRIES;
  
  /* Prepare URI NDEF message content */
  strcpy( URI.protocol,URI_ID_0x01_STRING );
  strcpy( URI.URI_Message,uri );
  strcpy( URI.Information,"\0" );
  
  NFCTAG_PowerUp();

  /* Write NDEF to EEPROM */
  while( NDEF_WriteURI( &URI ) != NDEF_OK && (tries--) > 0);
  
  NFCTAG_PowerDown();    
}

/**
  * @brief  Process of the NDEF_URI application
  * @retval None
  */
void MX_NFC4_NDEF_URI_Process(void)
{

}

#ifdef __cplusplus
}
#endif

