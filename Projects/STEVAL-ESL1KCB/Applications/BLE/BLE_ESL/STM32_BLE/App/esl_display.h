/**
  ******************************************************************************
  * @file    esl_display.h
  * @author  GPM WBL Application Team
  * @brief   Header file for library to handle display.
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
  
#ifndef ESL_DISPLAY_H
#define ESL_DISPLAY_H

#define ESL_STATE_OFF   0
#define ESL_STATE_ADV   1
#define ESL_STATE_SYNC  2

void ESL_DISPLAY_Init(uint8_t init_image);

void ESL_DISPLAY_Clear(void);

void ESL_DISPLAY_Show(void);

void ESL_DISPLAY_SetImage(const uint8_t *new_image, uint32_t size);

int ESL_DISPLAY_SetText(const char *txt);

int ESL_DISPLAY_SetPrice(uint16_t price_int, uint8_t price_fract);

void ESL_DISPLAY_SetState(uint8_t state);

void ESL_DISPLAY_SetID(uint8_t group_id, uint8_t esl_id);

#endif /* ESL_DISPLAY_H */