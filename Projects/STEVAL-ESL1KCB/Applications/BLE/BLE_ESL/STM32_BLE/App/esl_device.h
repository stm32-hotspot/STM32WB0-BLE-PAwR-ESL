/**
  ******************************************************************************
  * @file    esl_device.h
  * @author  GPM WBL Application Team
  * @brief   Header file for ESL device.
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
#ifndef ESL_DEVICE_H
#define ESL_DEVICE_H

#include <stdint.h>

#define MAX_NUM_LED             1
#define MAX_NUM_SENSOR          1
#define MAX_NUM_DISPLAY         1
#define MAX_NUM_IMAGE           3

void ESL_DEVICE_Process(void);
void ESL_DEVICE_ProcessRequest(void);

/* Callbacks to be implemented. */
uint8_t ESL_DEVICE_LEDControlCmdCB(uint8_t led_index,  uint8_t led_RGB_Brigthness, uint8_t led_flash_pattern[5], uint8_t off_period, uint8_t on_period, uint16_t led_repeat);
uint8_t ESL_DEVICE_SensorDataCmdCB(uint8_t sensor_index, uint8_t *data_p, uint8_t *data_length_p);
uint8_t ESL_DEVICE_TxtVsCmdCB(uint8_t txt_length, char *txt_p);
uint8_t ESL_DEVICE_PriceVsCmdCB(uint16_t int_part, uint8_t fract_part);
uint8_t ESL_DEVICE_DisplayImageCmdCB(uint8_t display_index, uint8_t image_index);

uint8_t ESL_DEVICE_LEDTimedControlCmdCB(uint8_t led_index,  uint8_t led_RGB_Brigthness, uint8_t led_flash_pattern[5], uint8_t off_period, uint8_t on_period, uint16_t led_repeat, uint32_t abs_time);
uint8_t ESL_DEVICE_DisplayTimedImageCmdCB(uint8_t display_index, uint8_t image_index, uint32_t abs_time);
uint8_t ESL_DEVICE_ServiceResetCmdCB(void);
uint8_t ESL_DEVICE_FactoryResetCmdCB(void);
bool getFactoryReset(void);
uint8_t ESL_DEVICE_RefreshDisplayCmdCB(uint8_t display_index);
uint8_t returnIndexCurrImage(void);
void setImageDisplayed(bool bvalue);

#endif /* ESL_DEVICE_H */