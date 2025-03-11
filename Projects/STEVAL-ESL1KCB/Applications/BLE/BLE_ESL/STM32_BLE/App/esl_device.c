/**
  ******************************************************************************
  * @file    esl_app.c
  * @author  GPM WBL Application Team
  * @brief   Implementation of callbacks needed by ESL profile
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
#include "esl_app.h"
#include "esl_device.h"
#include "stm32wb0x.h"
#include "stm32wb0x_esl1kcb.h"
#include "esl_display.h"
#include "app_nfc.h"
#include "app_common.h"
#include "time_ref.h"
#include "app_ble.h"    

/* Global variables used for LED Timed Control Command Pending execution */
uint8_t led_index_pending, led_RGB_Brigthness_pending;
uint8_t led_flash_pattern_pending[5];
uint8_t led_off_period_pending, led_on_period_pending;
uint16_t led_repeat_pending; /* Repeat type and duration. */
uint32_t led_abs_time_pending;

/* Global variables used for Display Timed Image Command Pending execution */
uint8_t display_index_pending, image_index_pending;
uint32_t img_abs_time_pending;
Led_TypeDef Led;

bool bFactoryReset = false;
bool bImageDisplayed = true;

extern ADC_HandleTypeDef hadc;

static void LED_Timed_Cmd_timeout_cb(void *arg);
static void Img_Timed_Cmd_timeout_cb(void *arg);

#define LED_OFF                                     0
#define LED_ON                                      1

#define MAX_TXT_LENGHT                           (11U)

#define LED_BLINK_ON_TIME_MS            (10)
#define LED_BLINK_OFF_TIME_MS           (1000)

#define MAX_TIMED_CMD_DELAY_MS          (4147200000)    // 48 days

typedef struct
{
  uint16_t int_part;
  uint8_t fract_part;  
}price_t;

typedef struct
{
  char curr_txt[MAX_TXT_LENGHT + 1];
  char next_txt[MAX_TXT_LENGHT + 1];
  price_t curr_price;
  price_t next_price;  
  uint8_t curr_img_index;
  uint8_t next_img_index;
  bool refresh;
} ESL_DEVICE_Context_t;

ESL_DEVICE_Context_t ESL_DEVICE_Context = 
{
  .curr_txt = {0},
  .next_txt = {0},
  .curr_price = {0},
  .next_price = {0},
  .curr_img_index = 0xFF,
  .next_img_index = 0xFF,
  .refresh = false,
};

VTIMER_HandleType Img_Timed_Cmd_timer_Id= {
  .callback = Img_Timed_Cmd_timeout_cb,
};

VTIMER_HandleType LED_Timed_Cmd_timer_Id= {
  .callback = LED_Timed_Cmd_timeout_cb,
};

uint8_t led_state = LED_OFF;

uint8_t ESL_DEVICE_LEDControlCmdCB(uint8_t led_index, uint8_t led_RGB_Brigthness, uint8_t led_flash_pattern[5], uint8_t off_period, uint8_t on_period, uint16_t led_repeat)
{
  uint8_t repeat_type = (uint8_t)(led_repeat && 0x0001);        //the first bit of led_repeat
  uint16_t repeat_duration = led_repeat >> 1;                   //other 15 bits of led_repeat
  
  //PTS configuration with "One LED"  
  if (led_index == 0)
  {
    Led = LD1;
  }
  else
  {
    return ERROR_INVALID_PARAMETERS;
  }
  
  APP_DBG_MSG("LED Index: 0x%02x \n", led_index);
  /* For now led_RGB_Brigthness component is not used, in fact: 
     if the LED selected by the Index value is a monochrome LED, the value 
     of the Color fields shall be ignored upon receipt.*/
  APP_DBG_MSG("LED Component: 0x%02x [Brightness: %d]\n", led_RGB_Brigthness, (led_RGB_Brigthness >> 6)); 
  /* Flashing pattern is ignored for the moment, because Repeats_Duration = 0 */
  APP_DBG_MSG("LED Flashing Pattern: 0x%02x%02x%02x%02x%02x \n", led_flash_pattern[4], 
                                                                 led_flash_pattern[3],
                                                                 led_flash_pattern[2],
                                                                 led_flash_pattern[1],
                                                                 led_flash_pattern[0]);
  APP_DBG_MSG("LED Flashing Pattern OFF Period: 0x%02x \n", off_period);
  APP_DBG_MSG("LED Flashing Pattern ON Period:  0x%02x \n", on_period);
  
  APP_DBG_MSG("LED Repeats:  0x%04x [Repeat type: %d - Repeat duration: 0x%04x]\n", 
              led_repeat, repeat_type, repeat_duration);
  
  if (repeat_duration != 0)
  {  
    /* A Bit_Off_Period  or Bit_On_Period value of 0 ms is invalid */
    if ((off_period == 0) || (on_period == 0))
    {
      return ERROR_INVALID_PARAMETERS;
    }  
    /* Value multiplied by 2 ms. The valid range is 2 ms to 510 ms*/
    if ((((off_period * 2) < 2) || ((off_period * 2) > 510)) ||
        (((on_period * 2) < 2) || ((on_period * 2) > 510)))
    {
      return ERROR_INVALID_PARAMETERS;
    }    
  }
  else // repeat_duration == 0
  { 
    /* Special Value:
    If Repeats_Duration = 0, the Flashing_Pattern field shall be ignored
    and the Repeat_Type field shall have the following interpretation:
    • If Repeat_Type = 0, then the LED shall be turned off continuously.
    • If Repeat_Type = 1, then the LED shall be turned on continuously. */
	if(repeat_type == 0)
	{
	  BSP_LED_Off(Led);
	  APP_DBG_MSG("LED OFF\n");
	  //Unset the Active LED bit on Basic State Bitmap
	  ESL_APP_Unset_Basic_State_Bitmap(BASIC_STATE_ACTIVE_LED_BIT);     
    }
	else // repeat_type == 1
	{
	  BSP_LED_On(Led);
	  APP_DBG_MSG("LED ON\n");
	  //Set the Active LED bit on Basic State Bitmap
	  ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_ACTIVE_LED_BIT); 
	}    
  }  
  
  return 0;
}

uint8_t ESL_DEVICE_TxtVsCmdCB(uint8_t txt_length, char *txt_p)
{
  memset(ESL_DEVICE_Context.next_txt, 0, sizeof(ESL_DEVICE_Context.next_txt));
  
  /* Here use MIN only for better robustness. */
  memcpy(ESL_DEVICE_Context.next_txt, txt_p, MIN(sizeof(ESL_DEVICE_Context.next_txt) - 1, txt_length));
  
  ESL_DEVICE_ProcessRequest();
  
  return 0;
}

uint8_t ESL_DEVICE_PriceVsCmdCB(uint16_t int_part, uint8_t fract_part)
{
  ESL_DEVICE_Context.next_price.int_part = int_part;
  ESL_DEVICE_Context.next_price.fract_part = fract_part;
  
  ESL_DEVICE_ProcessRequest();
  
  return 0;
}

uint8_t ESL_DEVICE_DisplayImageCmdCB(uint8_t display_index, uint8_t image_index)
{
  ESL_DEVICE_Context.next_img_index = image_index;
  APP_DBG_MSG("Display Index: %d - Image Index: %d\n", display_index, image_index);
  if (display_index >= MAX_NUM_DISPLAY)
  {  
    bImageDisplayed = false;
    return ERROR_INVALID_PARAMETERS;
  }  
  if (image_index >= MAX_NUM_IMAGE)
  {
    bImageDisplayed = false;
    return ERROR_INVALID_IMAGE_INDEX;
  }  
  ESL_DEVICE_ProcessRequest();
  
  bImageDisplayed = true;
  
  return 0;
}

uint8_t ESL_DEVICE_SensorDataCmdCB(uint8_t sensor_index, uint8_t *data_p, uint8_t *data_length_p)
{
  uint32_t adc_val = 0;
  uint16_t batt_voltage = 0;
      
  *data_length_p = 0;  
  
  HAL_ADC_Start(&hadc);
  
  if(HAL_ADC_PollForConversion(&hadc, 3) != HAL_OK)
  {
    APP_DBG_MSG("Sensor read timeout\n");
    
    return ERROR_RETRY;
  }
  
  adc_val = HAL_ADC_GetValue(&hadc);
  
  batt_voltage = __LL_ADC_CALC_DATA_TO_VOLTAGE(LL_ADC_VIN_RANGE_3V6, adc_val, LL_ADC_DS_DATA_WIDTH_16_BIT);
  
  HOST_TO_LE_16(data_p, batt_voltage);
  *data_length_p = 2;
  
  HAL_ADC_Stop(&hadc);
  
  return 0;
}

uint8_t ESL_DEVICE_LEDTimedControlCmdCB(uint8_t led_index,  uint8_t led_RGB_Brigthness, uint8_t led_flash_pattern[5], uint8_t off_period, uint8_t on_period, uint16_t led_repeat, uint32_t abs_time)
{
  uint32_t curr_abs_time;
  uint32_t delay;
  
  if (led_index > (MAX_NUM_LED-1))
  {
    return ERROR_INVALID_PARAMETERS;
  }
  
  /* If an LED Timed Control command is received while an LED Timed Control command is already pending */
  if(ESL_APP_Get_Basic_State_Bitmap() & BASIC_STATE_PENDING_LED_UPDATE_BIT)
  {  
    if (abs_time == 0x00000000) 
    {
      /* If the value of the Absolute Time parameter is zero (0x00000000), 
      then the pending LED Timed Control command shall be deleted.*/
      HAL_RADIO_TIMER_StopVirtualTimer(&LED_Timed_Cmd_timer_Id);
    }  
    return 0;
  }
  
  curr_abs_time = TIMEREF_GetCurrentAbsTime();
  delay = abs_time - curr_abs_time;
  
  APP_DBG_MSG("*** GET CurrentAbsTime: %d\n", curr_abs_time); 
  APP_DBG_MSG("Absolute Time: %d\n", abs_time);
  
  if (delay > MAX_TIMED_CMD_DELAY_MS) 
  {
    //Absolute time is more than 48 days
    return ERROR_IMPLAUSIBLE_ABSOLUTE_TIME;
  }
  
  if (delay == 0)
  {
    //LED Timed Control command must be executed
    ESL_DEVICE_LEDControlCmdCB(led_index, led_RGB_Brigthness, led_flash_pattern, off_period, on_period, led_repeat); 
  }
  else
  {   
    //LED Timed Control command is Pending
    /* The ESL shall set the value of the Pending LED Update bit in the Basic 
    State response to True; otherwise the bit shall be set to False. */
    APP_DBG("Pending LED Update bit is set to True\n");
    
    APP_DBG_MSG("LED Index: 0x%02x \n", led_index);
    APP_DBG_MSG("LED Component: 0x%02x \n", led_RGB_Brigthness); 
    APP_DBG_MSG("LED Flashing Pattern: 0x%02x%02x%02x%02x%02x \n", led_flash_pattern[4], 
                led_flash_pattern[3],
                led_flash_pattern[2],
                led_flash_pattern[1],
                led_flash_pattern[0]);
    APP_DBG_MSG("LED Flashing Pattern OFF Period: 0x%02x \n", off_period);
    APP_DBG_MSG("LED Flashing Pattern ON Period:  0x%02x \n", on_period);        
    APP_DBG_MSG("LED Repeats:  0x%04x [Repeat type: %d - Repeat duration: 0x%04x]\n", 
                led_repeat, (led_repeat && 0x0001), (led_repeat >> 1));
    
    if (ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_PENDING_LED_UPDATE_BIT) == 0) 
    {  
      //No other pending command
      led_index_pending = led_index;
      led_RGB_Brigthness_pending = led_RGB_Brigthness;
      memcpy(led_flash_pattern_pending, led_flash_pattern, 5);
      led_off_period_pending = off_period;
      led_on_period_pending = on_period;
      led_repeat_pending = led_repeat;
      led_abs_time_pending = abs_time;
      
      HAL_RADIO_TIMER_StartVirtualTimer(&LED_Timed_Cmd_timer_Id, delay);
    } 
    else
    {
      //There is an other pending command
      /* If the value of the Absolute_Time parameter in the new command is equal to the 
      value of the Absolute_Time parameter in the pending LED Timed Control command,
      then the newly received command shall replace the old pending command. */
      if (abs_time == led_abs_time_pending)
      {
        APP_DBG_MSG("*** Pending command is replaced by the newly received!\n");
        
        led_index_pending = led_index;
        led_RGB_Brigthness_pending = led_RGB_Brigthness;
        memcpy(led_flash_pattern_pending, led_flash_pattern, 5);
        led_off_period_pending = off_period;
        led_on_period_pending = on_period;
        led_repeat_pending = led_repeat;
        led_abs_time_pending = abs_time;
      }  
      else
      {
        /* The ESL shall send the Error response: Queue Full. The LED Timed 
        Control command that was already pending remains unchanged. */
        return ERROR_QUEUE_FULL;
      }  
    }  
  }  
  
  return 0;
}

static void LED_Timed_Cmd_timeout_cb(void *arg)
{
  /* Set the value of the Pending LED Update bit in the Basic State response to False */
  ESL_APP_Unset_Basic_State_Bitmap(BASIC_STATE_PENDING_LED_UPDATE_BIT);
  APP_DBG("Pending LED Update bit is set to False\n");
  /* LED Timed Control command must be executed */
  ESL_DEVICE_LEDControlCmdCB(led_index_pending, led_RGB_Brigthness_pending, led_flash_pattern_pending, led_off_period_pending, led_on_period_pending, led_repeat_pending);
}

uint8_t ESL_DEVICE_DisplayTimedImageCmdCB(uint8_t display_index, uint8_t image_index, uint32_t abs_time)
{
  uint32_t curr_abs_time;
  uint32_t delay;
  
  if (display_index >= MAX_NUM_DISPLAY)
    return ERROR_INVALID_PARAMETERS;
  
  if (image_index >= MAX_NUM_IMAGE)
    return ERROR_INVALID_IMAGE_INDEX;
  
  /* If an LED Timed Control command is received while an LED Timed Control command is already pending */
  if(ESL_APP_Get_Basic_State_Bitmap() & BASIC_STATE_PENDING_DISPLAY_UPDATE_BIT)
  {  
    if (abs_time == 0x00000000) 
    {
      /* If the value of the Absolute Time parameter is zero (0x00000000), 
      then the pending LED Timed Control command shall be deleted.*/
      HAL_RADIO_TIMER_StopVirtualTimer(&LED_Timed_Cmd_timer_Id);
    }  
    return 0;
  }
  
  curr_abs_time = TIMEREF_GetCurrentAbsTime();
  delay = abs_time - curr_abs_time;
  
  APP_DBG_MSG("*** GET CurrentAbsTime: %d\n", curr_abs_time); 
  APP_DBG_MSG("Absolute Time: %d\n", abs_time);

  if (delay > MAX_TIMED_CMD_DELAY_MS) 
  {
    //Absolute time is more than 48 days
    return ERROR_IMPLAUSIBLE_ABSOLUTE_TIME;
  }
  
  if (delay == 0)
  {
    //Display Timed Image command must be executed
    ESL_DEVICE_DisplayImageCmdCB(display_index, image_index);
  }
  else
  {   
    //Display Timed Image command is Pending
    /* The ESL shall set the value of the Pending Display Update bit in the Basic 
    State response to True; otherwise the bit shall be set to False. */
    APP_DBG("Pending LED Update bit is set to True\n");
    
    APP_DBG_MSG("Display Index: 0x%02x \n", display_index);
    APP_DBG_MSG("Image Index: 0x%02x \n", image_index); 
    
    if (ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_PENDING_DISPLAY_UPDATE_BIT) == 0)
    {
      //No other pending command
      display_index_pending = display_index;
      image_index_pending = image_index; 
      img_abs_time_pending = abs_time;
      
      HAL_RADIO_TIMER_StartVirtualTimer(&Img_Timed_Cmd_timer_Id, delay);
    }  
    else
    {
      //There is an other pending command
      /* If the value of the Absolute_Time parameter in the new command is equal to the 
      value of the Absolute_Time parameter in the pending LED Timed Control command,
      then the newly received command shall replace the old pending command. */
      if (abs_time == img_abs_time_pending)
      {        
        APP_DBG_MSG("*** Pending command is replaced by the newly received!\n");
        
        display_index_pending = display_index;
        image_index_pending = image_index; 
        img_abs_time_pending = abs_time;
      }  
      else
      {
        /* The ESL shall send the Error response: Queue Full. The Display Timed 
        Image command that was already pending remains unchanged. */
        return ERROR_QUEUE_FULL;
      }
    }  
  }
    
  return 0;
}

static void Img_Timed_Cmd_timeout_cb(void *arg)
{
  /* Set the value of the Pending LED Update bit in the Basic State response to False */
  ESL_APP_Unset_Basic_State_Bitmap(BASIC_STATE_PENDING_DISPLAY_UPDATE_BIT);
  /* LED Timed Control command must be executed */
  ESL_DEVICE_DisplayImageCmdCB(display_index_pending, image_index_pending);
}

uint8_t ESL_DEVICE_ServiceResetCmdCB(void)
{
  /* If the Service Needed state is no longer True, then the ESL shall set the value
     that is reported in the Service Needed bit of the Basic State response to False. */
  if (!get_Service_Needed_State())
  {  
    ESL_APP_Unset_Basic_State_Bitmap(BASIC_STATE_SERVICE_NEEDED_BIT);
    APP_DBG_MSG("RESET Service Needed bit of the Basic State response to FALSE! \n");
  }
  else
  {     
    /* If a condition persists that requires the ESL to keep the Service Needed 
       state set to True, then the ESL shall set the value that is reported in  
       the Service Needed bit of the Basic State response to True */
    ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_SERVICE_NEEDED_BIT);
    APP_DBG_MSG("SET Service Needed bit of the Basic State response to TRUE! \n");
  }    
  
  return 0;
}

uint8_t ESL_DEVICE_FactoryResetCmdCB(void)
{
  /* If an ESL in the Synchronized state receives a Factory Reset command, 
     then the ESL shall send the Error response: Invalid State.*/
  if (ESL_APP_Get_ESL_State() == ESL_STATE_SYNCHRONIZED)
  {
    bFactoryReset = false;
    return ERROR_INVALID_STATE;
  }  
  if ((ESL_APP_Get_ESL_State() == ESL_STATE_CONFIGURING) ||
      (ESL_APP_Get_ESL_State() == ESL_STATE_UPDATING)) 
  {
    bFactoryReset = true;
    /* The ESL shall initiate disconnection of the link with the AP */
    APP_BLE_Procedure_Gap_General(PROC_GAP_GEN_CONN_TERMINATE);
  }
  
  return 0;
}

bool getFactoryReset(void)
{
  return bFactoryReset;
}

uint8_t ESL_DEVICE_RefreshDisplayCmdCB(uint8_t display_index)
{ 
  APP_DBG_MSG("Refresh Display Command - Index: %d\n", display_index);

  if (display_index >= MAX_NUM_DISPLAY)
    return ERROR_INVALID_PARAMETERS;
  if (!bImageDisplayed)
    return ERROR_IMAGE_NOT_AVAILABLE;   //ERROR_INVALID_PARAMETERS  //TBR???
  
  ESL_DEVICE_ProcessRequest();
  
  return 0; 
}

uint8_t returnIndexCurrImage(void)
{
  return ESL_DEVICE_Context.curr_img_index;
}

void ESL_DEVICE_Process(void)
{  
  char uri[255] = "myst25.com/esl/?data=";
  
  bool update_nfc = false;
  bool update_display = false;
  
  APP_DBG_MSG("ESL_DEVICE_Process\n");

  if(strcmp(ESL_DEVICE_Context.curr_txt, ESL_DEVICE_Context.next_txt) != 0)
  {
    ESL_DISPLAY_SetText(ESL_DEVICE_Context.next_txt);
    update_display = true;
    strcpy(ESL_DEVICE_Context.curr_txt, ESL_DEVICE_Context.next_txt);
  }
  
  if(memcmp(&ESL_DEVICE_Context.curr_price, &ESL_DEVICE_Context.next_price, sizeof(price_t)) != 0)
  {
    ESL_DISPLAY_SetPrice(ESL_DEVICE_Context.next_price.int_part, ESL_DEVICE_Context.next_price.fract_part);
    update_display = true;
    update_nfc = true;    
    ESL_DEVICE_Context.curr_price = ESL_DEVICE_Context.next_price;
  }
  
  if(ESL_DEVICE_Context.curr_img_index != ESL_DEVICE_Context.next_img_index)
  {
    ESL_DISPLAY_SetIcon(ESL_DEVICE_Context.next_img_index);
    update_display = true;
    update_nfc = true;
    ESL_DEVICE_Context.curr_img_index = ESL_DEVICE_Context.next_img_index;
  }
  
  if(ESL_DEVICE_Context.refresh)
  {
    update_display = true;
    ESL_DEVICE_Context.refresh = false;
  }
  
  if(update_display)
  {
    ESL_DISPLAY_Show();
  }  
  if(update_nfc)
  {
    uint8_t group_id = 0xFF, esl_id = 0xFF;
    
    ESL_APP_GetAddress(&group_id, &esl_id);
    
    sprintf(uri + strlen(uri),
            "%02X%02X%04X%02X%02X%08X%08X", group_id, esl_id,
                                            ESL_DEVICE_Context.curr_price.int_part, ESL_DEVICE_Context.curr_price.fract_part,
                                            ESL_DEVICE_Context.curr_img_index,
                                            NFC_UUID.MsbUid, NFC_UUID.LsbUid);  
    APP_DBG_MSG("%s", uri);
    APP_DBG_MSG("\n");
    MX_NFC4_NDEF_URI_Set(uri);
  }  
}

void setImageDisplayed(bool bvalue)
{
  bImageDisplayed = bvalue;
}
