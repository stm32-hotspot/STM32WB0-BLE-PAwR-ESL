/**
  ******************************************************************************
  * @file    uart_cmd.c
  * @author  GPM WBL Application Team
  * @brief   Implementation of uart commands for ESL.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "uart_cmd.h"
#include "esl_profile_ap.h"
#include "app_debug.h"
#include "app_common.h"
#include "gatt_client_app.h"

#include "time_ref.h"

#define C_SIZE_CMD_STRING       256U

static uint8_t CommandString[C_SIZE_CMD_STRING];
static volatile uint16_t indexReceiveChar = 0;
static uint8_t echo_mode = 0;
static volatile uint8_t buff_lock = 0;

void UartRxCpltCallback(uint8_t * pRxDataBuff, uint16_t nDataSize)
{
  /* nDataSize always 1 in current implementation. */
  
  if(buff_lock)
  {
    /* Previous command still need to be processed. This should not happen. */
    return;
  }
  
  if(*pRxDataBuff == '\b')
  {
    if(indexReceiveChar > 0)
    {
      indexReceiveChar--;
      if(echo_mode)
      {
        putchar(*pRxDataBuff);
      }
    }
    return;
  }
  
  /* Filling buffer and wait for '\r' char */
  if (indexReceiveChar < C_SIZE_CMD_STRING - 1)
  {
    if(echo_mode)
    {
      putchar(*pRxDataBuff);
    }
    if (*pRxDataBuff == '\r')
    {
      CommandString[indexReceiveChar] = '\0';
      buff_lock = 1;
        
      //APP_DBG_MSG("received %s\n", CommandString);
      
      UART_CMD_ProcessRequestCB();
    }
    else
    {
      CommandString[indexReceiveChar++] = *pRxDataBuff;
    }
  }
}

static void print_esl_resp(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t resp_length;
  
  resp_length =  GET_LENGTH_FROM_OPCODE(resp[0]);
  
  APP_DBG_MSG("Response from ESL (%d, %d): ", group_id, esl_id);
  
  for(int i = 0; i < resp_length; i++)
  {
    APP_DBG_MSG("0x%02X ", resp[i]);
  }
  
  APP_DBG_MSG("\n");
}

static void ping_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status = 0;
  uint16_t basic_state = 0;  
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_BASIC_STATE)
  {
    basic_state = LE_TO_HOST_16(&resp[1]);    
  }
  else
  {
    status = 1;
  }
  
  printf("+STATE:%02X,%02X,%d,%d\r\n", group_id, esl_id, status, basic_state);
}

static void led_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status;
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_LED_STATE)
  {
    status = 0;
  }
  else
  {
    status = 1;
  }
  
  printf("+LED:%02X,%02X,%d\r\n", group_id, esl_id, status);
}

static void txt_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status;
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_VS_OK)
  {
    status = 0;
  }
  else
  {
    status = 1;
  }
  
  printf("+TXT:%02X,%02X,%d\r\n", group_id, esl_id, status);
}

static void price_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status;
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_VS_OK)
  {
    status = 0;
  }
  else
  {
    status = 1;
  }
  
  printf("+PRICE:%02X,%02X,%d\r\n", group_id, esl_id, status);
}

static void batt_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status;  
  uint8_t resp_param_length;
  uint16_t batt_voltage = 0;
  uint8_t sensor_index;
  
  resp_param_length =  GET_PARAM_LENGTH_FROM_OPCODE(resp[0]);
  sensor_index = resp[1];
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp_param_length != 3 || sensor_index != 0)
  {
    status = 1;
    goto fail;
  }
  
  if((resp[0] & 0x0F) == ESL_RESP_SENSOR_VALUE_TAG_NIBBLE)
  {
    status = 0;
    batt_voltage = LE_TO_HOST_16(&resp[2]);    
  }
  else
  {
    status = 1;
  }
  
fail:
  printf("+SENS:%02X,%02X,%d,%d\r\n", group_id, esl_id, status, batt_voltage);
}

static void img_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status;
  uint8_t display_index = 0;
  uint8_t img_index = 0;
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_DISPLAY_STATE)
  {
    status = 0;
    display_index = resp[1];
    img_index = resp[2];
  }
  else
  {
    status = 1;
  }
  
  printf("+DISP:%02X,%02X,%d,%d,%d\r\n", group_id, esl_id, status, display_index, img_index);
}

static void unassociated_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status = 0;
  uint16_t basic_state = 0;  
  esl_bonded_t* esl_node;
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_BASIC_STATE)
  {
    basic_state = LE_TO_HOST_16(&resp[1]); 
    /* The ESL removed all bonding information with the AP, so the AP have to 
       remove this ESL from esl_bonded_list*/
    esl_node = ESL_AP_return_ESL_bonded(group_id, esl_id);
    if (esl_node!= NULL)
    {
      ESL_AP_Remove_ESL_from_List(esl_node);
    }  
  }
  else
  {
    status = 1;
  }
  
  printf("+STATE:%02X,%02X,%d,%d\r\n", group_id, esl_id, status, basic_state);
}

static void server_reset_resp_cb(uint8_t group_id, uint8_t esl_id, uint8_t *resp)
{
  uint8_t status = 0;
  uint16_t basic_state = 0;  
  
  print_esl_resp(group_id, esl_id, resp);
  
  if(resp[0] == ESL_RESP_BASIC_STATE)
  {
    basic_state = LE_TO_HOST_16(&resp[1]);    
  }
  else
  {
    status = 1;
  }
  
  printf("+STATE:%02X,%02X,%d,%d\r\n", group_id, esl_id, status, basic_state);
}

static void factory_reset_remove_ESL(uint8_t group_id, uint8_t esl_id)
{
  esl_bonded_t* esl_node;

  /* The ESL removed all bonding information with the AP, so the AP have to 
     remove this ESL from esl_bonded_list*/
  esl_node = ESL_AP_return_ESL_bonded(group_id, esl_id);
  if (esl_node!= NULL)
  {
    ESL_AP_Remove_ESL_from_List(esl_node);
  }  
}

static int parse_cmd(void)
{
  uint32_t group_id, esl_id;
  int ret;
  
  if(strncasecmp((char *)CommandString, "ATE", 3) == 0)
  {
    uint32_t echo = 1;
 
    sscanf((char*)CommandString + 3, "%x", &echo);
    if(echo > 1)
    {
      return 1;
    }
    else
    {
      echo_mode = echo;
      
      return 0;
    }
  }
  else if(strncasecmp((char *)CommandString, "AT+PING=", 8) == 0)
  {    
    ret = sscanf((char*)CommandString + 8, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {  
      if(ESL_AP_command(ESL_CMD_PING, group_id, esl_id, ping_resp_cb) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }
  else if(strncasecmp((char *)CommandString, "AT+UNASSOC=", 11) == 0)
  {    
    ret = sscanf((char*)CommandString + 11, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_command(ESL_CMD_UNASSOCIATE_FROM_AP, group_id, esl_id, unassociated_resp_cb) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }  
  else if(strncasecmp((char *)CommandString, "AT+SRVRST=", 10) == 0)
  {
    ret = sscanf((char*)CommandString + 10, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_command(ESL_CMD_SERVICE_RESET, group_id, esl_id, server_reset_resp_cb) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }    
  else if(strncasecmp((char *)CommandString, "AT+FRST=", 8) == 0)
  {
    ret = sscanf((char*)CommandString + 8, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_factory_reset(group_id, esl_id, NULL) == 0)
      {
        factory_reset_remove_ESL(group_id, esl_id);
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }    
  else if(strncasecmp((char *)CommandString, "AT+UPDCMP=", 10) == 0)
  {    
    ret = sscanf((char*)CommandString + 10, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_update_complete(group_id, esl_id) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }  
  else if (strncasecmp((char *)CommandString, "AT+SENS=", 8) == 0)
  {
    
    ret = sscanf((char*)CommandString + 8, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS || esl_id == 0xFF)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_read_sensor_data(group_id, esl_id, batt_resp_cb, 0) == 0)
      {
        return 0;
      }
      else
      {      
        return 1;
      }
    }
  }   
  else if(strncasecmp((char *)CommandString, "AT+REFRESH=", 11) == 0)
  {
    uint32_t display_index;
    
    ret = sscanf((char*)CommandString + 11, "%x,%x,%u",  &group_id, &esl_id, &display_index);
    if(ret != 3 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_refresh_display(group_id, esl_id, display_index, img_resp_cb) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }     
  else if(strncasecmp((char *)CommandString, "AT+IMG=", 7) == 0)
  {
    uint32_t image_index, display_index;
    
    ret = sscanf((char*)CommandString + 7, "%x,%x,%u,%u", &group_id, &esl_id, &display_index, &image_index);
    if(ret != 4 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_display_image(group_id, esl_id, img_resp_cb, display_index, image_index) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }
  else if(strncasecmp((char *)CommandString, "AT+IMGTIM=", 10) == 0)
  {
    uint32_t display_index, image_index;
    uint32_t delay_ms;
    
    ret = sscanf((char*)CommandString + 10, "%x,%x,%u,%u,%u", &group_id, &esl_id, &display_index, &image_index, &delay_ms);
    if(ret != 5 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_display_timed_image(group_id, esl_id, img_resp_cb, display_index, image_index, delay_ms) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }   
  else if (strncasecmp((char *)CommandString, "AT+LED=", 7) == 0)
  {
    uint32_t led_index, led_component, off_period, on_period;
    uint64_t pattern;
    uint32_t repeat;
    
    ret = sscanf((char*)CommandString + 7, "%x,%x,%x,%x,%llx,%x,%x,%x", &group_id, &esl_id, &led_index, &led_component, &pattern, &off_period, &on_period, &repeat);
    if(ret != 8 || group_id >= MAX_GROUPS)    
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_led_control(group_id, esl_id, led_resp_cb, led_index, led_component, pattern, off_period, on_period, repeat) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }  
  else if(strncasecmp((char *)CommandString, "AT+LEDTIM=", 10) == 0)
  {    
    uint32_t led_index, led_component, off_period, on_period;
    uint64_t pattern;
    uint32_t repeat, delay_ms;
    
    ret = sscanf((char*)CommandString + 10, "%x,%x,%x,%x,%llx,%x,%x,%x,%u", &group_id, &esl_id, &led_index, &led_component, &pattern, &off_period, &on_period, &repeat, &delay_ms);
    if(ret != 9 || group_id >= MAX_GROUPS)    
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_led_timed_control(group_id, esl_id, led_resp_cb, led_index, led_component, pattern, off_period, on_period, repeat, delay_ms) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }    
  }   
  else if (strncasecmp((char *)CommandString, "AT+TXT=", 7) == 0)
  {
    char text[61];
    
    ret = sscanf((char*)CommandString + 7, "%x,%x,%[^\t\r\n]", &group_id, &esl_id, text);
    if(ret != 3 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_txt(group_id, esl_id, txt_resp_cb, text) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }
  else if (strncasecmp((char *)CommandString, "AT+PRICE=", 9) == 0)
  {
    uint32_t val_int, val_fract;
    
    ret = sscanf((char*)CommandString + 9, "%x,%x,%u.%u", &group_id, &esl_id, &val_int, &val_fract);
    if(ret != 4 || group_id >= MAX_GROUPS || val_int > 999 || val_fract > 99)
    {
      return 1;
    }
    else
    {
      if(ESL_AP_cmd_price(group_id, esl_id, price_resp_cb, val_int, val_fract) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }
  }   
  // Command to reconfigure an ESL with a new address
  else if(strncasecmp((char *)CommandString, "AT+RECONF=", 10) == 0)
  {
    uint32_t new_group_id, new_esl_id;
    
    ret = sscanf((char*)CommandString + 10, "%x,%x,%x,%x", &group_id, &esl_id, &new_group_id, &new_esl_id);
    if(ret != 4 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
    /* To transition an ESL from the Synchronized state to the Updating state, 
       the AP shall use the Periodic Advertising Connection procedure */  
      if(ESL_AP_cmd_reconfig_esl_address(group_id, esl_id, new_group_id, new_esl_id) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }    
  }
  // Command for transition from Synchronized to Updating State for an ESL
  else if(strncasecmp((char *)CommandString, "AT+CONN=", 8) == 0)
  {    
    ret = sscanf((char*)CommandString + 8, "%x,%x", &group_id, &esl_id);
    if(ret != 2 || group_id >= MAX_GROUPS)
    {
      return 1;
    }
    else
    {
    /* To transition an ESL from the Synchronized state to the Updating state, 
       the AP shall use the Periodic Advertising Connection procedure */  
      if(ESL_AP_cmd_updating_state(group_id, esl_id) == 0)
      {
        return 0;
      }
      else
      {
        return 1;
      }
    }    
  }   
  // Command to read the Display/Image/Sensor/Led Information Characteristics
  else if(strncasecmp((char *)CommandString, "AT+INFO", 7) == 0)
  {
    if(ESL_APP_Read_All_Info_Chars() == 0)
    { 
      return 0;
    }
    else
    {
      return 1;
    }   
  }    
  // Command to read the Display/Image/Sensor/Led Information Characteristics
  else if(strncasecmp((char *)CommandString, "AT+DISPLAYINFO", 14) == 0)
  {
    if(ESL_APP_Read_Display_Info_Chars() == 0)
    { 
      return 0;
    }
    else
    {
      return 1;
    }   
  }
  // Command to read the Display/Image/Sensor/Led Information Characteristics
  else if(strncasecmp((char *)CommandString, "AT+SENSORINFO", 13) == 0)
  {
    if(ESL_APP_Read_Sensor_Info_Chars() == 0)
    { 
      return 0;
    }
    else
    {
      return 1;
    }   
  }
  // Command to read the Display/Image/Sensor/Led Information Characteristics
  else if(strncasecmp((char *)CommandString, "AT+LEDINFO", 10) == 0)
  {
    if(ESL_APP_Read_Led_Info_Chars() == 0)
    { 
      return 0;
    }
    else
    {
      return 1;
    }   
  }  
  // Command to send aci_gap_clear_security_db api
  else if(strncasecmp((char *)CommandString, "AT+CLRSCDB", 10) == 0)
  {
    if(ESL_APP_Clear_Security_DB() == 0)
    { 
      return 0;
    }
    else
    {
      return 1;
    }   
  } 
  // Command to read current absolute time 
  else if(strncasecmp((char *)CommandString, "AT+ABSTIME?", 10) == 0)
  {
    uint32_t abs_time;
    
    abs_time = TIMEREF_GetCurrentAbsTime();
    printf("+ABSTIME: %d\n", abs_time);
    
    return 0;
  } 
  // Help command: list of all ESL AP commands
  if(strncasecmp((char *)CommandString, "AT+HELP", 7) == 0)
  {
    printf("Standard ESL commands: \n");
    printf("  - AT+PING=<group_id>,<esl_id>: Ping\n");
    printf("  - AT+UNASSOC=<group_id>,<esl_id>: Unassociate from AP \n");
    printf("  - AT+SRVRST=<group_id>,<esl_id>: Service Reset \n");
    printf("  - AT+FRST=<group_id>,<esl_id>: Factory Reset \n");
    printf("  - AT+UPDCMP=<group_id>,<esl_id>: Update Complete \n");
    printf("  - AT+SENS=<group_id>,<esl_id>,<sensor_index>: Read Sensor Data \n");
    printf("  - AT+REFRESH=<group_id>,<esl_id>,<display_index>: Refresh Display \n");
    printf("  - AT+IMG=<group_id>,<esl_id>,<display_index>,<image_index>: Display Image \n");
    printf("  - AT+IMGTIM=<group_id>,<esl_id>,<display_index>,<image_index>,<absolute_time>: Display Timed Image \n");
    printf("  - AT+LED=<group_id>,<esl_id>,<led_index>,<led_component>,<pattern>,<off_period>,<on_period>,<repeat>: LED Control \n");
    printf("  - AT+LEDTIM=<group_id>,<esl_id>,<led_index>,<led_component>,<pattern>,<off_period>,<on_period>,<repeat>,<absolute_time>: LED Timed Control \n");
    
    printf("\nProprietary ESL commands: \n");
    printf("  - AT+TXT=<group_id>,<esl_id>,<text>: Set text\n  - <text> can be a string with maximum 15 characters\n");
    printf("  - AT+PRICE=<group_id>,<esl_id>,<val_int>,<val_fract>: Set price \n");
    
    printf("\nCommands for special operations and for tests \n");
    printf("  - AT+RECONF=<group_id>,<esl_id>,<new_group_id>,<new_esl_id>: Reconfigure an ESL with a new address \n");
    printf("  - AT+CONN=<group_id>,<esl_id>: Connect to an ESL (ESL enters *updating state*) \n");
    printf("  - AT+INFO: Read all the Information Characteristics from the connected ESL \n");    
    printf("  - AT+DISPLAYINFO: Read the Display Information Characteristic from the connected ESL \n");
    printf("  - AT+SENSORINFO: Read the Sensor Information Characteristic from the connected ESL \n");
    printf("  - AT+LEDINFO: Read the LED Information Characteristic from the connected ESL \n");
    printf("  - AT+CLRSCDB: Clear the security db \n");
    printf("  - AT+ABSTIME?: Read current absolute time\n");
    printf("  - AT+HELP: List of AT commands \n");
    return 0;
  }    
  return 1;  
}

void UART_CMD_Process(void)
{
  if(parse_cmd() == 0)
  {
    printf("OK\r\n");   
  }
  else
  {
    printf("ERROR\r\n");
  }
  
  indexReceiveChar = 0;
  
  buff_lock = 0;
}
