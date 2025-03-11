/**
  ******************************************************************************
  * @file    esl_app.c
  * @author  GPM WBL Application Team
  * @brief   Implementation of ESL packet management.
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
#include <string.h>
#include "stm32wb0x.h"
#include "ble.h"
#include "app_common.h"
#include "esl.h"
#include "esl_app.h"
#include "esl_device.h"
#include "app_ble.h"
#include "time_ref.h"

#define MAX_ESL_PAYLOAD_SIZE                    (48U)
#define EAD_MIC_SIZE                            (4U)
#define EAD_RANDOMZER_SIZE                      (5U)
#define MAX_ADV_PAYLOAD                         (MAX_ESL_PAYLOAD_SIZE + 2 + EAD_RANDOMZER_SIZE + EAD_MIC_SIZE + 2)

#define AD_TYPE_ENCRYPTED_ADVERTISING_DATA      (0x31)
#define AD_TYPE_ELECTRONIC_SHELF_LABEL          (0x34)

/* Codes for Commands */
#define ESL_CMD_PING                            (0x00)
#define ESL_CMD_UPDATE_COMPLETE                 (0x04)
#define ESL_CMD_READ_SENSOR_DATA                (0x10)
#define ESL_CMD_DISPLAY_IMG                     (0x20)
#define ESL_CMD_LED_CONTROL                     (0xB0)

#define ESL_CMD_UNASSOCIATE_FROM_AP             (0x01)
#define ESL_CMD_SERVICE_RESET                   (0x02)
#define ESL_CMD_FACTORY_RESET                   (0x03)
#define ESL_CMD_REFRESH_DISPLAY                 (0x11)
#define ESL_CMD_DISPLAY_TIMED_IMG               (0x60)
#define ESL_CMD_LED_TIMED_CONTROL               (0xF0)

/* Code for vendor-specific commands */
#define ESL_CMD_VS_PRICE                        (0x3F)
#define ESL_CMD_VS_TXT                          (0xBF)

/* Code for response data types */
#define ESL_RESP_ERROR                          (0x00)
#define ESL_RESP_LED_STATE                      (0x01)
#define ESL_RESP_BASIC_STATE                    (0x10)
#define ESL_RESP_DISPLAY_STATE                  (0x11)
#define ESL_RESP_SENSOR_VALUE_TAG_NIBBLE        (0x0E) // This is only the value of the Tag nibble. The Length is variable
#define ESL_RESP_VS_OK                          (0x0F)

#define BRC_ESL_ID                              (0xFF)

#define MAX_TXT_LENGHT                           (11U)

#define GET_PARAM_LENGTH_FROM_OPCODE(opcode)          ((((opcode) & 0xF0) >> 4) + 1)

#define GET_LENGTH_FROM_OPCODE(opcode)                (GET_PARAM_LENGTH_FROM_OPCODE(opcode) + 1)

#define SET_LENGTH_TO_OPCODE(tag, param_length)       (((tag) & 0x0F) | ((((param_length) - 1) & 0x0F) << 4))


#define CONFIG_STATE_FLAG_ADDRESS           0x01
#define CONFIG_STATE_FLAG_SYNC_KEY          0x02
#define CONFIG_STATE_FLAG_RESPONSE_KEY      0x04
#define CONFIG_STATE_FLAG_ABSOLUTE_TIME     0x08
#define CONFIG_STATE_FLAG_ALL_SET           0x0F

#define SYNC_TIMEOUT_MS                     (60 * 60 * 1000)  // 60 minutes
#define UNSYNC_TIMEOUT_MS                   (60 * 60 * 1000)  // 60 minutes

#define C_SIZE_CMD_STRING       20


typedef enum
{
  ECP_NOTIFICATION_OFF,
  ECP_NOTIFICATION_ON,
  /* USER CODE BEGIN Service1_APP_SendInformation_t */

  /* USER CODE END Service1_APP_SendInformation_t */
  ESL_APP_SENDINFORMATION_LAST
} ESL_APP_SendInformation_t;

typedef struct
{
  uint8_t Session_Key[16];
  uint8_t IV[8];
}ESL_APP_KeyMaterial_t;

typedef struct
{
  ESL_APP_SendInformation_t ECP_Notification_Status;
  /* USER CODE BEGIN Service1_APP_Context_t */  
  ESL_APP_State_t state;
  uint8_t group_id;
  uint8_t esl_id;
  ESL_APP_KeyMaterial_t ap_sync_key_material;
  ESL_APP_KeyMaterial_t esl_resp_key_material;
  uint16_t sync_handle;
  uint16_t basic_state;
  uint8_t a_resp[MAX_ADV_PAYLOAD];
  uint8_t config_state_flags;   /* Flags for written characteristics during configuration (CONFIG_STATE_FLAG macros) */
  bool service_needed_state; /* The Service Needed state is reported via the Service Needed bit of the Basic State response */
  
  /* Synchronized timeout timerID*/
  /* If the ESL has not received a valid ESL message in a synchronization message from
     the AP for 60 minutes then the ESL shall transition to the Unsynchronized state. */
  VTIMER_HandleType Synchronized_timer_Id;  
  /* If the ESL is not moved to the Updating state for 60 minutes, then the 
     ESL shall transition to the Unassociated state */
  VTIMER_HandleType Unsynchronized_timer_Id;
  /* USER CODE END Service1_APP_Context_t */  
  uint16_t              ConnectionHandle;  
} ESL_APP_Context_t;

ESL_APP_Context_t ESL_APP_Context;
bool b_group_id_changed = false;
VTIMER_HandleType Disconnection_timer_Id;

extern uint8_t AP_bonded_Peer_Address[6];
extern uint8_t AP_bonded_Peer_Address_Type;
extern bool ESL_synchronized;
extern bool bFactoryReset;
extern bool ESL_connected;

/* USER CODE BEGIN PV */
static uint8_t CommandString[C_SIZE_CMD_STRING];
static volatile uint16_t indexReceiveChar = 0;
/* USER CODE END PV */

static uint8_t TLV_OpCode_handling(uint8_t *p_cmd, uint8_t opcode, uint8_t esl_cmd_id, uint8_t * esl_payload_resp, uint8_t param_length, uint8_t resp_idx);
static void synch_packet_received(uint16_t pa_event, uint8_t *p_esl_data, uint8_t size);
static void send_resp(uint16_t pa_event, uint8_t resp_slot, uint32_t *p_esl_resp, uint8_t resp_size);
static void disconnection_delay(void *arg);
static void ESL_APP_Unsynchronized_State(void);
static void ESL_APP_Unsynchronized_State_Transition(void *arg);
static void ESL_APP_Unsassociated_State_Transition(void *arg);

/* Functions Definition ------------------------------------------------------*/
void ESL_SERVICE_Notification(ESL_SERVICE_NotificationEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service2_Notification_1 */

  /* USER CODE END Service2_Notification_1 */
  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_Notification_Service1_EvtOpcode */

    /* USER CODE END Service1_Notification_Service1_EvtOpcode */
    case ESL_SERVICE_ADDR_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */

      /* USER CODE END Service1Char1_WRITE_EVT */
      break;

    case ESL_SERVICE_SYNC_KEY_MATERIAL_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */

      /* USER CODE END Service1Char1_WRITE_EVT */
      break;

    case ESL_SERVICE_RESP_KEY_MATERIAL_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */

      /* USER CODE END Service1Char1_WRITE_EVT */
      break;
 
    case ESL_SERVICE_CONTROL_POINT_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */

      /* USER CODE END Service1Char1_WRITE_EVT */
      break;
      
    case ESL_SERVICE_CURR_ABS_TIME_WRITE_EVT:
      /* USER CODE BEGIN Service1Char1_WRITE_EVT */

      /* USER CODE END Service1Char1_WRITE_EVT */
      break;
      
    case ESL_SERVICE_CONTROL_POINT_NOTIFY_ENABLED_EVT:
      /* USER CODE BEGIN Service1Char5_NOTIFY_ENABLED_EVT */
        APP_DBG_MSG("ESL_SERVICE_CONTROL_POINT_NOTIFY_ENABLED_EVT\n");
        ESL_APP_Context.ECP_Notification_Status = ECP_NOTIFICATION_ON;
      /* USER CODE END Service2Char3_NOTIFY_ENABLED_EVT */
      break;

    case ESL_SERVICE_CONTROL_POINT_NOTIFY_DISABLED_EVT:
      /* USER CODE BEGIN Service1Char5_NOTIFY_DISABLED_EVT */
        APP_DBG_MSG("ESL_SERVICE_CONTROL_POINT_NOTIFY_DISABLED_EVT\n");
        ESL_APP_Context.ECP_Notification_Status = ECP_NOTIFICATION_OFF;
      /* USER CODE END Service2Char3_NOTIFY_DISABLED_EVT */
      break;

    case ESL_SERVICE_LED_INFO_TIME_READ_EVT: 
      /* USER CODE BEGIN Service1Char6_READ_EVT */

      /* USER CODE END Service1Char6_READ_EVT */
      break;    
      
    default:
      /* USER CODE BEGIN Service2_Notification_default */

      /* USER CODE END Service2_Notification_default */
      break;
  }
  /* USER CODE BEGIN Service2_Notification_2 */

  /* USER CODE END Service2_Notification_2 */
  return;
}


void ESL_APP_EvtRx(ESL_APP_ConnHandleNotEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service1_APP_EvtRx_1 */

  /* USER CODE END Service1_APP_EvtRx_1 */

  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_APP_EvtRx_Service1_EvtOpcode */

    /* USER CODE END Service1_APP_EvtRx_Service1_EvtOpcode */
  case   ESL_CONN_HANDLE_EVT:
      ESL_APP_Context.ConnectionHandle = p_Notification->ConnectionHandle;
      /* USER CODE BEGIN Service1_APP_CENTR_CONN_HANDLE_EVT */
      ESL_APP_ConnectionComplete(p_Notification->ConnectionHandle);
      /* USER CODE END Service1_APP_CENTR_CONN_HANDLE_EVT */
      break;
    case ESL_DISCON_HANDLE_EVT :
      ESL_APP_Context.ConnectionHandle = 0xFFFF;
      /* USER CODE BEGIN Service1_APP_DISCON_HANDLE_EVT */
      ESL_APP_DisconnectionComplete(p_Notification->ConnectionHandle);
      /* USER CODE END Service1_APP_DISCON_HANDLE_EVT */
      break;

    default:
      /* USER CODE BEGIN Service1_APP_EvtRx_default */

      /* USER CODE END Service1_APP_EvtRx_default */
      break;
  }

  /* USER CODE BEGIN Service1_APP_EvtRx_2 */

  /* USER CODE END Service1_APP_EvtRx_2 */

  return;
}

void ESL_APP_Init(void)
{
  tBleStatus ret;
  
  APP_DBG_MSG("*** Unassociated State\n");
  ESL_APP_Context.state = ESL_STATE_UNASSOCIATED;
  
  ESL_APP_Context.config_state_flags = 0;
  
  set_Service_Needed_State(false);
  
  ESL_SERVICE_Init();
 
  aci_gap_clear_security_db();
  
  APP_DBG_MSG("hci_le_set_default_periodic_advertising_sync_transfer_parameters:");
  
  ret = hci_le_set_default_periodic_advertising_sync_transfer_parameters(0x01, /* Mode: reports disabled */
                                                                         0x0000, /* Skip */
                                                                         1000, /* Sync_Timeout */
                                                                         0); /* CTE_Type*/
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG(" fail (0x%02x)\n", ret);
  }
  else
  {
    APP_DBG_MSG(" success\n");
  }  
  
  /* Create timer to manage Synchronized timeout timerID*/
  /* If the ESL has not received a valid ESL message in a synchronization message from
     the AP for 60 minutes then the ESL shall transition to the Unsynchronized state. */  
  ESL_APP_Context.Synchronized_timer_Id.callback = ESL_APP_Unsynchronized_State_Transition;   
  /* Create timer to manage Unsynchronized state timeout timerID*/
  /* If the ESL is not moved to the Updating state for 60 minutes, then the 
     ESL shall transition to the Unassociated state */
  ESL_APP_Context.Unsynchronized_timer_Id.callback = ESL_APP_Unsassociated_State_Transition;
  
  Disconnection_timer_Id.callback = disconnection_delay;
    
}

void ESL_APP_ConnectionComplete(uint16_t connection_handle)
{
  ESL_APP_Context.ConnectionHandle = connection_handle;
}

/* To be called when a bond has been established. */
void ESL_APP_PairingComplete(uint16_t connection_handle)
{
  //TODO: allow only one bonded AP.
  if(ESL_APP_Context.state == ESL_STATE_UNASSOCIATED)
  {
    APP_DBG_MSG("*** Configuring State\n");
    ESL_APP_Context.state = ESL_STATE_CONFIGURING;
    ESL_APP_Context.ConnectionHandle = connection_handle;
  }
}

void ESL_APP_DisconnectionComplete(uint16_t connection_handle)
{
  if(ESL_APP_Context.state == ESL_STATE_CONFIGURING)
  {
     /* If the connection is lost owing to link loss occurring in the Configuring 
        state before the configuration of the ESL has been completed, then the ESL 
        shall transition to the Unassociated state */
     if(ESL_APP_Context.config_state_flags != CONFIG_STATE_FLAG_ALL_SET)
     {
       ESL_APP_Context.state = ESL_STATE_UNASSOCIATED;
       APP_DBG_MSG("*** Unassociated state from Configuring state for link loss \n");
       // Enter GAP undirected connectable mode.
       APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
     }
     else 
     {
       /* if the connection is lost owing to link loss after the configuration 
          of the ESL has been successfully completed, the ESL shall transition 
          to the Unsynchronized state */
       APP_DBG_MSG("*** Unsynchronized state from Configuring state for link loss \n");
       ESL_APP_Unsynchronized_State();
     }
  } 
  /* If the connection is lost owing to link loss occurring in the Updating state, 
     then the ESL shall transition to the Unsynchronized state */
  else if(ESL_APP_Context.state == ESL_STATE_UPDATING)
  {
    APP_DBG_MSG("*** Unsynchronized state from Updating state for link loss \n");
    ESL_APP_Unsynchronized_State();
  }
  else if(ESL_APP_Context.state == ESL_STATE_UNASSOCIATED)
  {
    /* If a disconnection event is received before moving to configuring state, we go back advertising. */
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
  }    
}

void ESL_APP_SyncLost(void)
{
  APP_DBG_MSG("*** Unsynchronized state for sync lost\n");
  ESL_APP_Unsynchronized_State();
}

/* Set the group_id and esl_id by ESL Address and return true if group_id is 
   changed from the previous one */
uint8_t ESL_APP_SetESLAddress(uint16_t address)
{  
  if (ESL_APP_ConfiguringOrUpdatingState())
  {
    uint8_t esl_id = address & 0x00FF;
    if (esl_id == 0xFF)
    {
      return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
    } 
    
    uint8_t prev_group_id = ESL_APP_Context.group_id;
    
    ESL_APP_Context.group_id = (address & 0x7F00) >> 8;
    ESL_APP_Context.esl_id = esl_id;
//    APP_DBG_MSG("*** ESL_APP_Context.group_id: 0X%02X\n", ESL_APP_Context.group_id);
//    APP_DBG_MSG("*** ESL_APP_Context.esl_id: 0X%04X\n", ESL_APP_Context.esl_id);
    
    ESL_APP_Context.config_state_flags |= CONFIG_STATE_FLAG_ADDRESS;
    if (ESL_APP_Context.group_id != prev_group_id)
      b_group_id_changed = true;
    else
      b_group_id_changed = false;
    
    return BLE_ATT_ERR_NONE;
  }
  return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
}

void ESL_APP_SetAPSyncKeyMaterial(uint8_t key_material[24])
{
  if (ESL_APP_ConfiguringOrUpdatingState())
  {
    memcpy(&ESL_APP_Context.ap_sync_key_material.Session_Key, key_material, 16);
    memcpy(&ESL_APP_Context.ap_sync_key_material.IV, &key_material[16], 8);
    
    APP_DBG_MSG("*** ESL_APP_Context.ap_sync_key_material: \n");
    for(uint8_t i=0; i<16; i++)
    {
      APP_DBG_MSG("%02X", ESL_APP_Context.ap_sync_key_material.Session_Key[i]);
          if (i < 15) APP_DBG_MSG(":");
    }
    APP_DBG_MSG("\n*** IV: \n");
    for(uint8_t i=0; i<8; i++)
    {
      APP_DBG_MSG("%02X", ESL_APP_Context.ap_sync_key_material.IV[i]);
          if (i < 7) APP_DBG_MSG(":");
    }
    APP_DBG_MSG("\n");
    ESL_APP_Context.config_state_flags |= CONFIG_STATE_FLAG_SYNC_KEY;
  }
}

void ESL_APP_SetESLResponseKeyMaterial(uint8_t key_material[24])
{
  if (ESL_APP_ConfiguringOrUpdatingState())
  {
    memcpy(&ESL_APP_Context.esl_resp_key_material.Session_Key, key_material, 16);
    memcpy(&ESL_APP_Context.esl_resp_key_material.IV, &key_material[16], 8);
    
    APP_DBG_MSG("*** ESL_APP_Context.esl_resp_key_material: \n");
    for(uint8_t i=0; i<16; i++)
    {
      APP_DBG_MSG("%02X", ESL_APP_Context.esl_resp_key_material.Session_Key[i]);
          if (i < 15) APP_DBG_MSG(":");
    }
    APP_DBG_MSG("\n*** IV: \n");
    for(uint8_t i=0; i<8; i++)
    {
      APP_DBG_MSG("%02X", ESL_APP_Context.esl_resp_key_material.IV[i]);
          if (i < 7) APP_DBG_MSG(":");
    }
    APP_DBG_MSG("\n");
    ESL_APP_Context.config_state_flags |= CONFIG_STATE_FLAG_RESPONSE_KEY;
  }
}

void ESL_APP_SetCurrentAbsoluteTime(uint32_t curr_absolute_time)
{
  if (ESL_APP_ConfiguringOrUpdatingState())
  {
    TIMEREF_SetAbsoluteTime(curr_absolute_time);
    ESL_APP_Context.config_state_flags |= CONFIG_STATE_FLAG_ABSOLUTE_TIME;
    
    APP_DBG_MSG("*** SET CurrentAbsTime: %d\n", TIMEREF_GetCurrentAbsTime());
  }
}

void ESL_APP_SyncInfoReceived(uint16_t sync_handle)
{
  tBleStatus ret;
    
  ESL_APP_Context.sync_handle = sync_handle;
  
  APP_DBG_MSG("hci_le_set_periodic_sync_subevent:");
  
  ret = hci_le_set_periodic_sync_subevent(sync_handle,
                                          0, /* Periodic_Advertising_Properties */
                                          1, /* Num_Subevents */
                                          &ESL_APP_Context.group_id);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG(" fail (0x%02x)\n", ret);
  }
  else
  {
    APP_DBG_MSG(" success\n");
  }  
  
  APP_DBG_MSG("hci_le_set_periodic_advertising_receive_enable:");
  
  ret = hci_le_set_periodic_advertising_receive_enable(sync_handle,
                                                       1 /* Enable*/);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG(" fail (0x%02x)\n", ret);
  }
  else
  {
    APP_DBG_MSG(" success\n");
  }
  
  /* The Synchronized state may be entered only from the Configuring state or  
     the Updating state. The ESL transitions to the Synchronized state when the  
     ESL completes the Periodic Advertising Bluetooth SIG Sync Transfer procedure 
     and synchronizes to a periodic advertising train transmitted by the AP. */
  if (ESL_APP_ConfiguringOrUpdatingState())
  {  
    APP_DBG_MSG("*** Synchronized State\n");
    ESL_APP_Context.state = ESL_STATE_SYNCHRONIZED;
    /* If the ESL has not received a valid ESL message in a synchronization message from
       the AP for 60 minutes then the ESL shall transition to the Unsynchronized state. */  
    HAL_RADIO_TIMER_StartVirtualTimer(&ESL_APP_Context.Synchronized_timer_Id, SYNC_TIMEOUT_MS);
  }

  ret = aci_gap_terminate(ESL_APP_Context.ConnectionHandle, BLE_ERROR_TERMINATED_REMOTE_USER);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("aci_gap_terminate failure: reason=0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("==>> aci_gap_terminate : Success (BLE_ERROR_TERMINATED_REMOTE_USER)\n");
  }
    
}

void ESL_APP_AdvPayloadReceived(uint16_t pa_event, uint8_t *p_adv_data, uint8_t size)
{
  tBleStatus ret;
  uint32_t decrypted_data[MAX_ESL_PAYLOAD_SIZE/4 + 1];
  uint8_t encrypted_data_length;
  uint8_t *esl_payload_p;
  uint8_t esl_payload_length;
   
  /* The ESL must be in Synchronized state */
  if(ESL_APP_Context.state != ESL_STATE_SYNCHRONIZED)
  {
    /* Ignore data in Updating state. */
    return;
  }
  
  if(size > MAX_ESL_PAYLOAD_SIZE + EAD_MIC_SIZE + EAD_RANDOMZER_SIZE + 4) /* 4 is the overhead for AD types and lengths. */
  {
    return;
  }  
  
  if(p_adv_data[1] != AD_TYPE_ENCRYPTED_ADVERTISING_DATA || p_adv_data[0] + 1 != size)
  {
    /* Accept only (well formatted) encrypted advertising data. */
    return;
  }

  /* If the ESL has received a valid ESL message in a synchronization message from
     the AP then the ESL stop the Synchronized Timer. */  
  HAL_RADIO_TIMER_StopVirtualTimer(&ESL_APP_Context.Synchronized_timer_Id);
  
  /* p_adv_data[0] contains the ADV packet length (see Fig 5.1 ESL profile spec) */
  encrypted_data_length = p_adv_data[0] - 1;
  
  ret = aci_gap_decrypt_adv_data(ESL_APP_Context.ap_sync_key_material.Session_Key,
                                 ESL_APP_Context.ap_sync_key_material.IV,
                                 encrypted_data_length,
                                 &p_adv_data[2], //Encrypted data on ADV packet 
                                 decrypted_data);
  
  if(ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("Decryption fail.\n");
    return;
  }
  
  esl_payload_p = (uint8_t *)decrypted_data;
  esl_payload_length = esl_payload_p[0];
  
  if(esl_payload_length != encrypted_data_length - EAD_RANDOMZER_SIZE - EAD_MIC_SIZE - 1 ||
     esl_payload_p[1] != AD_TYPE_ELECTRONIC_SHELF_LABEL)
  {
    return;
  }
  
  synch_packet_received(pa_event, &esl_payload_p[2], esl_payload_length); 
  
  APP_DBG_MSG("End of ESL_APP_AdvPayloadReceived\n");
}

/* To handle both periodic advertising TLV management and control point characteristic write TLV management */
static uint8_t TLV_OpCode_handling(uint8_t *p_cmd, uint8_t opcode, uint8_t esl_cmd_id, uint8_t * esl_payload_resp, uint8_t param_length, uint8_t resp_idx)
{
    uint8_t ret;
    
    switch(opcode)
    {
    case ESL_CMD_PING:
      {          
        if(esl_cmd_id != BRC_ESL_ID)
        {
          //TBR: to check if response exceeds ESL payload size.
          APP_DBG_MSG("PING command [opcode: 0x%02x] \n", opcode);
          esl_payload_resp[resp_idx] = ESL_RESP_BASIC_STATE;
          //Basic State response bitmap parmam (16 bits)
          HOST_TO_LE_16(&esl_payload_resp[resp_idx + 1], ESL_APP_Context.basic_state);
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;
    case ESL_CMD_LED_CONTROL:
      {
        uint8_t led_index;
        uint8_t led_RGB_Brigthness;
        uint8_t led_flash_pattern[5];
        uint8_t led_off_period, led_on_period;
        uint16_t led_repeat; /* Repeat type and duration. */
        
        led_index = p_cmd[2];
        led_RGB_Brigthness = p_cmd[3]; 
        memcpy(led_flash_pattern, &p_cmd[4], 5);
        led_off_period = p_cmd[9];
        led_on_period = p_cmd[10];
        led_repeat = LE_TO_HOST_16(&p_cmd[11]);
        
        APP_DBG_MSG("LED CONTROL command [opcode: 0x%02x] \n", opcode);
        ret = ESL_DEVICE_LEDControlCmdCB(led_index, led_RGB_Brigthness, led_flash_pattern, led_off_period, led_on_period, led_repeat);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          /* Error */
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret; /* ERROR_INVALID_PARAMETERS */
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {            
          //TBR: to check if response exceeds ESL payload size.
          esl_payload_resp[resp_idx] = ESL_RESP_LED_STATE;
          esl_payload_resp[resp_idx + 1] = led_index;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;
    case ESL_CMD_READ_SENSOR_DATA:
      {
        uint8_t sensor_index;
        uint8_t sensor_data_length;
        
        APP_DBG_MSG("READ SENSOR DATA command [opcode: 0x%02x] \n", opcode);
        
        if(esl_cmd_id == BRC_ESL_ID)
        {
          /* This command cannot have a broadcast address. Do not invoke callback. */
          break;
        }
                
        sensor_index = p_cmd[2];
        //PTS configuration with "One Sensor"
        if (sensor_index > (MAX_NUM_SENSOR-1))
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ERROR_INVALID_PARAMETERS;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
          break;
        }
        /* TBR: Need to use asynchronous response, since data from sensor may not 
           be available immediately.Check also for available space in response.  */
        ret = ESL_DEVICE_SensorDataCmdCB(sensor_index, &esl_payload_resp[resp_idx + 2], &sensor_data_length);
        
        if(ret != 0)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0)
        {
          esl_payload_resp[resp_idx] = SET_LENGTH_TO_OPCODE(ESL_RESP_SENSOR_VALUE_TAG_NIBBLE, sensor_data_length + 1);            
          esl_payload_resp[resp_idx + 1] = sensor_index;            
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;
    case ESL_CMD_VS_TXT:
      {
        APP_DBG_MSG("TXT VENDOR command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_TxtVsCmdCB(param_length - 1, (char *)&p_cmd[2]);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_VS_OK;
          esl_payload_resp[resp_idx + 1] = 0; /* Not used. */
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);            
        }
        
      }
      break;
    case ESL_CMD_VS_PRICE:
      {        
        APP_DBG_MSG("PRICE VENDOR command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_PriceVsCmdCB(LE_TO_HOST_16(&p_cmd[2]), p_cmd[4]);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_VS_OK;
          esl_payload_resp[resp_idx + 1] = 0; /* Not used. */
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
      }
      break;
    /* When the Update Complete command is received by an ESL, the ESL shall 
       synchronize with the AP and then disconnect the link with the AP. */
    case ESL_CMD_UPDATE_COMPLETE:
      {        
        APP_DBG_MSG("UPDATE COMPLETE command [opcode: 0x%02x] \n", opcode);
        /* If an ESL receives the Update Complete command and it is synchronized, 
           the ESL shall immediately terminate the ACL connection and transition 
           to the Synchronized state. */
        if (ESL_synchronized)    
        {         
          /* If group_id is changed then the ESL have to resynchronise with AP */
          if (b_group_id_changed)
          {
            APP_DBG_MSG("hci_le_set_periodic_sync_subevent:");    
            ret = hci_le_set_periodic_sync_subevent(ESL_APP_Context.sync_handle,
                                                    0, /* Periodic_Advertising_Properties */
                                                    1, /* Num_Subevents */
                                                    &ESL_APP_Context.group_id);
            if (ret != BLE_STATUS_SUCCESS)
            {
              APP_DBG_MSG(" fail (0x%02x)\n", ret);
            }
            else
            {
              APP_DBG_MSG(" success\n");
            }
          }  
          
          APP_DBG_MSG("*** Synchronized State\n");
          ESL_APP_Context.state = ESL_STATE_SYNCHRONIZED;
          /* Delay for disconnection */
          HAL_RADIO_TIMER_StartVirtualTimer(&Disconnection_timer_Id, 100);          
        }
        /* If an ESL receives the Update Complete command and it is not synchronized, 
           the ESL shall wait for synchronization to be established and then terminate 
           the ACL connection and transition to the Synchronized state. */
        
        /* The Update Complete Command has NO RESPONSE */
      }
      break;
  
    case ESL_CMD_UNASSOCIATE_FROM_AP:
      {          
        APP_DBG_MSG("UNASSOCIATE FROM AP command [opcode: 0x%02x] \n", opcode);
        
        if(esl_cmd_id != BRC_ESL_ID)
        {  
          esl_payload_resp[resp_idx] = ESL_RESP_BASIC_STATE;
          //Basic State response bitmap parmam (16 bits)
          HOST_TO_LE_16(&esl_payload_resp[resp_idx + 1], ESL_APP_Context.basic_state);
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break; 
    case ESL_CMD_SERVICE_RESET:
      {          
        APP_DBG_MSG("SERVICE RESET command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_ServiceResetCmdCB();
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_BASIC_STATE;
          //Basic State response bitmap parmam (16 bits)
          HOST_TO_LE_16(&esl_payload_resp[resp_idx + 1], ESL_APP_Context.basic_state);
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }      
      }
      break;
    case ESL_CMD_FACTORY_RESET:
      {          
        APP_DBG_MSG("FACTORY RESET command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_FactoryResetCmdCB();
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;         //Invalid State
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }       
      }
      break;
      
#if DISPLAY      
    case ESL_CMD_DISPLAY_IMG:
      {        
        APP_DBG_MSG("DISPLAY IMAGE command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_DisplayImageCmdCB(p_cmd[2], p_cmd[3]);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_DISPLAY_STATE;
          esl_payload_resp[resp_idx + 1] = p_cmd[2]; /* Display index */
          esl_payload_resp[resp_idx + 2] = p_cmd[3]; /* Image index */            
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;      
    case ESL_CMD_REFRESH_DISPLAY:
      {        
        APP_DBG_MSG("REFRESH DISPLAY command [opcode: 0x%02x] \n", opcode);
        
        ret = ESL_DEVICE_RefreshDisplayCmdCB(p_cmd[2]);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_DISPLAY_STATE;
          esl_payload_resp[resp_idx + 1] = p_cmd[2]; /* Display index */
          esl_payload_resp[resp_idx + 2] = returnIndexCurrImage(); /* Image index */            
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;
    case ESL_CMD_DISPLAY_TIMED_IMG:
      {    
        uint32_t img_abs_time; 
        
        APP_DBG_MSG("DISPLAY TIMED IMAGE command [opcode: 0x%02x] \n", opcode);
        
        img_abs_time = LE_TO_HOST_32(&p_cmd[4]);
        
        ret = ESL_DEVICE_DisplayTimedImageCmdCB(p_cmd[2], p_cmd[3], img_abs_time);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {
          esl_payload_resp[resp_idx] = ESL_RESP_DISPLAY_STATE;
          esl_payload_resp[resp_idx + 1] = p_cmd[2]; /* Display index */
          esl_payload_resp[resp_idx + 2] = p_cmd[3]; /* Image index */            
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;          
#endif    
      
    case ESL_CMD_LED_TIMED_CONTROL:
      {                
        uint8_t led_index;
        uint8_t led_RGB_Brigthness;
        uint16_t led_repeat; /* Repeat type and duration. */
        uint8_t led_flash_pattern[5];
        uint8_t led_off_period, led_on_period;
        uint32_t led_abs_time; 
        
        APP_DBG_MSG("LED TIMED CONTROL command [opcode: 0x%02x] \n", opcode);
        
        led_index = p_cmd[2];
        led_RGB_Brigthness = p_cmd[3]; 
        memcpy(led_flash_pattern, &p_cmd[4], 5);
        led_off_period = p_cmd[9];
        led_on_period = p_cmd[10];
        led_repeat = LE_TO_HOST_16(&p_cmd[11]);
        led_abs_time = LE_TO_HOST_32(&p_cmd[13]);
        
        ret = ESL_DEVICE_LEDTimedControlCmdCB(led_index,  led_RGB_Brigthness, led_flash_pattern, led_off_period, led_on_period, led_repeat, led_abs_time);
        
        if(ret != 0 && esl_cmd_id != BRC_ESL_ID)
        {
          /* Error */
          esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
          esl_payload_resp[resp_idx + 1] = ret; /* ERROR_INVALID_PARAMETERS or ERROR_IMPLAUSIBLE_ABSOLUTE_TIME or ERROR_QUEUE_FULL */
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
        
        if(ret == 0 && esl_cmd_id != BRC_ESL_ID)
        {            
          //TBR: to check if response exceeds ESL payload size.
          esl_payload_resp[resp_idx] = ESL_RESP_LED_STATE;
          esl_payload_resp[resp_idx + 1] = led_index;
          resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
        }
      }
      break;           

    default:
      esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
      esl_payload_resp[resp_idx + 1] = ERROR_INVALID_OPCODE;
      resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
      break;
    }
  return resp_idx;
}

static void disconnection_delay(void *arg)
{
  HAL_RADIO_TIMER_StopVirtualTimer(&Disconnection_timer_Id);
  APP_BLE_Procedure_Gap_General(PROC_GAP_GEN_CONN_TERMINATE);
}

static void synch_packet_received(uint16_t pa_event, uint8_t *p_esl_data, uint8_t size)
{
  uint8_t opcode;
  uint8_t group_id;
  uint8_t param_length;
  uint8_t esl_cmd_id;
  int8_t tlv_num = -1, relevant_cmd_tlv_num = -1;
  uint8_t *p_cmd;
  uint8_t resp_idx = 2; /* Reserve 2 extra bytes for AD header */
  ALIGN(4) uint8_t esl_payload_resp[MAX_ESL_PAYLOAD_SIZE + 2]; /* 2 extra bytes needed for ESL AD type and length */
  
  group_id = p_esl_data[0] & 0x7F;
  
  if(group_id != ESL_APP_Context.group_id)
    return;
  
  p_cmd = &p_esl_data[1];
  
  while(p_cmd < p_esl_data + size - 1) /* Shortest command is 2 bytes. */
  {
    param_length = ((p_cmd[0] & 0xF0) >> 4) + 1;
    esl_cmd_id = p_cmd[1]; /* First cmd parameter is always the ESL_ID */
    tlv_num += 1;
    
    if(esl_cmd_id == ESL_APP_Context.esl_id || esl_cmd_id == BRC_ESL_ID)
    {
      /* Identify the relevant command to choose response slot. */
      /* Broadcast messages shall be disregarded because do not elicit a response. */
      if( esl_cmd_id != BRC_ESL_ID)
      {
        relevant_cmd_tlv_num = tlv_num;
      }
      opcode = p_cmd[0];
      resp_idx = TLV_OpCode_handling(p_cmd, opcode, esl_cmd_id, esl_payload_resp, param_length, resp_idx);
    }
    p_cmd += (param_length + 1); /* increment the iterator to go on next element*/
  }
  
  if(relevant_cmd_tlv_num >= 0)
  {
    /* Send the response */
    esl_payload_resp[0] = resp_idx - 1;
    esl_payload_resp[1] = AD_TYPE_ELECTRONIC_SHELF_LABEL;
    
    send_resp(pa_event, relevant_cmd_tlv_num, (uint32_t *)esl_payload_resp, resp_idx);
    
    /* When the Unassociate from AP command is received, and the response has  
       been sent, the ESL shall remove all information with the AP */
    if (opcode == ESL_CMD_UNASSOCIATE_FROM_AP)
    {
      ESL_APP_UnassociatedFromAPCmd();
    }  
  }
  
}

static void send_resp(uint16_t pa_event, uint8_t resp_slot, uint32_t *p_esl_resp, uint8_t resp_size)
{
  tBleStatus ret;
  
  ESL_APP_Context.a_resp[0] = resp_size + EAD_RANDOMZER_SIZE + EAD_MIC_SIZE + 1;
  ESL_APP_Context.a_resp[1] = AD_TYPE_ENCRYPTED_ADVERTISING_DATA;
  
  ret = aci_gap_encrypt_adv_data(ESL_APP_Context.esl_resp_key_material.Session_Key,
                                 ESL_APP_Context.esl_resp_key_material.IV,
                                 resp_size,
                                 p_esl_resp,
                                 &ESL_APP_Context.a_resp[2]);
  
  ret = ll_set_periodic_advertising_response_data_ptr(ESL_APP_Context.sync_handle,
                                                      pa_event,
                                                      ESL_APP_Context.group_id,
                                                      ESL_APP_Context.group_id,
                                                      resp_slot,
                                                      ESL_APP_Context.a_resp[0] + 1,
                                                      ESL_APP_Context.a_resp);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("ll_set_periodic_advertising_response_data_ptr failure: reason=0x%02X, Sync_Handle 0x%04X, Request_Event %d, Subevent %d, Response_Slot %d\n", ret,
                ESL_APP_Context.sync_handle,
                pa_event,
                ESL_APP_Context.group_id,
                resp_slot);
  }
  else
  {
    APP_DBG_MSG("==>> ll_set_periodic_advertising_response_data_ptr : Success\n");
  }
}

void ESL_ControlPoint_received(uint8_t *p_cmd, uint8_t size)
{
  uint8_t opcode;
  uint8_t param_length;
  uint8_t esl_cmd_id;
  
  uint8_t resp_idx = 0;
  uint8_t esl_payload_resp[MAX_ESL_PAYLOAD_SIZE]; 
  
  param_length = ((p_cmd[0] & 0xF0) >> 4) + 1;
  esl_cmd_id = p_cmd[1]; /* First cmd parameter is always the ESL_ID */
  
  /* ESL service specification 3.9.2 Command behavior: If an opcode,  ESL_ID  
     value does not match the ESL_ID of the ESL or matches the Broadcast Address,  
     then the ESL shall reject the command by responding with the Error response: 
     Invalid Parameter(s). */
  if(esl_cmd_id != ESL_APP_Context.esl_id || esl_cmd_id == BRC_ESL_ID)
  {
    esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
    esl_payload_resp[resp_idx + 1] = ERROR_INVALID_PARAMETERS;
    resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
  }
  else
  {
    /* If, after receipt of the Factory Reset cmd and prior to disconnection from
       the AP, the ESL receives any other cmd from the AP written to the ECP char,
       then the other cmd shall be rejected with the error code: ”Unspecified Error” */
    if ((bFactoryReset) && (ESL_connected))
    {
      esl_payload_resp[resp_idx] = ESL_RESP_ERROR;
      esl_payload_resp[resp_idx + 1] = ERROR_UNSPECIFIED;
      resp_idx += GET_LENGTH_FROM_OPCODE(esl_payload_resp[resp_idx]);
    }
    else
    {  
      opcode = p_cmd[0]; /* TLV OpCode */
      /* To handle control point characteristic write TLV management */
      resp_idx = TLV_OpCode_handling(p_cmd, opcode, esl_cmd_id, esl_payload_resp, param_length, resp_idx); 
    }
  } /* end else */
  
  /* Send response through notification */
    
  if (resp_idx > 0)
  {  
    ESL_SERVICE_Data_t eslResp;
    
    eslResp.p_Payload = esl_payload_resp;
    eslResp.Length = resp_idx;
    
    APP_DBG_MSG("RESPONSE opcode: 0x%02x \n", eslResp.p_Payload[0]);
    
    ESL_SERVICE_NotifyValue(ESL_SERVICE_CONTROL_POINT, &eslResp, ESL_APP_Context.ConnectionHandle);
    
    /* When the Unassociate from AP command is received, and the response has  
       been sent, the ESL shall remove all information with the AP */
    if (opcode == ESL_CMD_UNASSOCIATE_FROM_AP)
    {
      ESL_APP_UnassociatedFromAPCmd();
    }  
  }
}

uint8_t ESL_APP_ConfiguringOrUpdatingState(void)
{
  if((ESL_APP_Context.state == ESL_STATE_CONFIGURING) || 
     (ESL_APP_Context.state == ESL_STATE_UPDATING))
    return 1;
  else 
    return 0;
}

static void ESL_APP_Unsynchronized_State(void)
{
  ESL_APP_Context.state = ESL_STATE_UNSYNCHRONIZED;
  /* In the Unsynchronized state, the ESL shall enter a GAP connectable mode. 
     If a connection is formed with a Client, then the ESL transitions to the 
     Updating */
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
  /* If the ESL is not moved to the Updating state for 60 minutes, then the 
     ESL shall transition to the Unassociated state, and shall remove all 
     bonding information with the AP and delete the value of the AP Sync 
     Key Material in internal storage. */
  HAL_RADIO_TIMER_StartVirtualTimer(&ESL_APP_Context.Unsynchronized_timer_Id, UNSYNC_TIMEOUT_MS);
}  

// Synchronized_timer_Id callback
/* If the ESL has not received a valid ESL message in a synchronization message from
   the AP for 60 minutes then the ESL shall transition to the Unsynchronized state. */
static void ESL_APP_Unsynchronized_State_Transition(void *arg)
{
  if(ESL_APP_Context.state == ESL_STATE_SYNCHRONIZED)
  {  
    APP_DBG_MSG("Unsynchronized state for timeout\n");
    hci_le_periodic_advertising_terminate_sync(ESL_APP_Context.sync_handle);
    ESL_APP_Unsynchronized_State();
  }
}

// Unsynchronized_timer_Id callback
/* If the ESL is not moved to the Updating state for 60 minutes, then the 
   ESL shall transition to the Unassociated state */
static void ESL_APP_Unsassociated_State_Transition(void *arg)
{
  if(ESL_APP_Context.state == ESL_STATE_UNSYNCHRONIZED)
  { 
    ESL_APP_Context.state = ESL_STATE_UNASSOCIATED;
    APP_DBG_MSG("*** Unssociated state for timeout\n");
    /* ESL shall remove all bonding information with the AP and delete the value 
       of the AP Sync Key Material in internal storage. */
    aci_gap_clear_security_db();       
    memset(ESL_APP_Context.ap_sync_key_material.Session_Key, 0, 16);
    memset(ESL_APP_Context.ap_sync_key_material.IV, 0, 8);
  }
}


void ESL_APP_Updating_State_Transition(uint16_t sync_handle)
{ 
  if (sync_handle != 0xFFFF)
  {
    ESL_APP_Context.sync_handle = sync_handle;
    /* To transition an ESL from the Synchronized state to the Updating state, the 
       AP shall use the Periodic Advertising Connection procedure. When the AP 
       connects with the ESL, the ESL transitions to the Updating state. */      
    if(ESL_APP_Context.state == ESL_STATE_SYNCHRONIZED)
    {  
      /* The ESL shall transition to the Updating state only if the peer device 
         is the Client with which the ESL is already associated. */  
      if(aci_gap_is_device_bonded(AP_bonded_Peer_Address_Type, AP_bonded_Peer_Address) == BLE_STATUS_SUCCESS)
      {
        ESL_APP_Context.state = ESL_STATE_UPDATING;
        APP_DBG_MSG("*** Updating state transition from Synchronized state\n"); 
        //Stop the timer
        HAL_RADIO_TIMER_StopVirtualTimer(&ESL_APP_Context.Synchronized_timer_Id);
      }  
    }  
  }  
  else
  {
    /* In the Unsynchronized state, if a connection is formed with a Client, 
       then the ESL transitions to the Updating */
    if (ESL_APP_Context.state == ESL_STATE_UNSYNCHRONIZED)
    {
      ESL_APP_Context.state = ESL_STATE_UPDATING;
      APP_DBG_MSG("*** Updating state transition from Unsynchronized state\n"); 
      //Stop the timer
      HAL_RADIO_TIMER_StopVirtualTimer(&ESL_APP_Context.Unsynchronized_timer_Id);    
    }  
  }  
}

uint8_t ESL_APP_Get_ESL_State(void)
{
  return ESL_APP_Context.state;
}

int ESL_APP_GetAddress(uint8_t *group_id_p, uint8_t *esl_id_p)
{
  if(ESL_APP_Context.state < ESL_STATE_SYNCHRONIZED)
  {
    return -1;
  }
  
  *group_id_p = ESL_APP_Context.group_id;
  *esl_id_p = ESL_APP_Context.esl_id;
  
  return 0;
}

void ESL_APP_pairing_request(uint16_t connHandle)
{
  Bonded_Device_Entry_t bonded_devices;
  uint8_t num_devices = 0;
  
  /* The Server shall reject any pairing requests that are received 
     while the Server is in the Updating state */
  if (ESL_APP_Get_ESL_State() == ESL_STATE_UPDATING)
  {
    //reject any pairing request
    aci_gap_pairing_resp(connHandle, 0);
    return;
  }   
  /* An ESL shall be bonded with a maximum of one AP at any one time. */
  aci_gap_get_bonded_devices(0, 1, &num_devices, &bonded_devices);
  
  if (num_devices == 0)
  {
    /* Accept pairing request */
    aci_gap_pairing_resp(connHandle, 1);
  }
  else
  {
    /* Reject pairing */
    aci_gap_pairing_resp(connHandle, 0);    
  }
}

uint16_t ESL_APP_Get_Basic_State_Bitmap(void)
{
  return ESL_APP_Context.basic_state;
}    
    

/* Return 1 if the basic_resp_bit is already set, else set the basic_resp_bit
   on Basic State bitmap and return 0 */
uint8_t ESL_APP_Set_Basic_State_Bitmap(uint8_t basic_resp_bit)
{
  if(ESL_APP_Context.basic_state & basic_resp_bit) 
  {
    return 1;
  }  
  else
  {  
    ESL_APP_Context.basic_state |= basic_resp_bit;
    return 0;
  }
}

void ESL_APP_Unset_Basic_State_Bitmap(uint8_t basic_resp_bit)
{
  ESL_APP_Context.basic_state &= (~basic_resp_bit);
}

void ESL_APP_UnassociatedFromAPCmd(void)
{
  /* The ESL shall remove all bonding information with the AP, delete
     the value of the AP Sync Key Material in internal storage, the 
     ESL ID and delete all stored commands. */          
  aci_gap_clear_security_db();       
  memset(ESL_APP_Context.ap_sync_key_material.Session_Key, 0, 16);
  memset(ESL_APP_Context.ap_sync_key_material.IV, 0, 8);
  ESL_APP_Context.esl_id = -1;
  //TODO: delete pending commands
  
  /* The ESL shall enter the Unassociated state. */
  ESL_APP_Context.state = ESL_STATE_UNASSOCIATED;
  APP_DBG_MSG("*** Unassociated state by commands (Unassociated from AP or Factory Reset) \n");
  // Enter GAP undirected connectable mode.
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
}

void ESL_APP_FactoryResetCmd(void)
{
  /* The ESL shall become unassociated from any AP and shall revert to its 
     original state before it was associated with an AP.
     The ESL shall remove all bonding information with the AP, delete
     the value of the AP Sync Key Material, ESL Response Key Material, 
     and ESL Address in internal storage; and delete any stored image 
     data that was written to the ESL. */                
  memset(ESL_APP_Context.esl_resp_key_material.Session_Key, 0, 16);
  memset(ESL_APP_Context.esl_resp_key_material.IV, 0, 8);
  ESL_APP_Context.group_id = -1;
  
  ESL_APP_Context.sync_handle = 0xFFFF;
  ESL_APP_Context.basic_state = 0;
  ESL_APP_Context.config_state_flags = 0;
  set_Service_Needed_State(false);
  
  ESL_APP_UnassociatedFromAPCmd();
}

bool get_Service_Needed_State(void)
{
  return ESL_APP_Context.service_needed_state;
}

// To set the Service Needed state 
void set_Service_Needed_State(bool bvalue)
{
  ESL_APP_Context.service_needed_state = bvalue;
  
  /* If a condition occurs that causes the ESL to set the Service Needed state to
     True, then the ESL shall set the value of the Service Needed bit to True. 
     This shall remain True until it is reset by the Client (Service Reset cmd) */
  if (bvalue)
  {
    ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_SERVICE_NEEDED_BIT);
  }  
}


void UartRxCpltCallback(uint8_t * pRxDataBuff, uint16_t nDataSize)
{
  /* nDataSize always 1 in current implementation. */
  
  /* Filling buffer and wait for '\r' char */
  if (indexReceiveChar < C_SIZE_CMD_STRING - 1)
  {
    putchar(*pRxDataBuff);

    if (*pRxDataBuff == '\r')
    {
      CommandString[indexReceiveChar] = '\0';
      ESL_APP_CMD_ProcessRequestCB();
    }
    else
    {
      CommandString[indexReceiveChar++] = *pRxDataBuff;
    }
  }
}

static int parse_cmd(void)
{
  if(strncasecmp((char *)CommandString, "HELP", 4) == 0)
  {
    APP_DBG_MSG("List of commands usefull for tests: \n");
    APP_DBG_MSG("  - ABSTIME: Get Current Absolute Time\n");
    APP_DBG_MSG("  - SRVNEEDED: Set Service Needed bit to True \n");
    APP_DBG_MSG("  - UNSYNC: Set The ESL state to Unsynchronized \n");
    APP_DBG_MSG("  - NODISPLAY: Each display is not displaying an image \n");
    return 0;
  } 
  else if(strncasecmp((char *)CommandString, "ABSTIME", 7) == 0)
  {
    APP_DBG_MSG("--> Get Current Absolute Time: %d\n", TIMEREF_GetCurrentAbsTime());
    return 0;
  } 
  else if(strncasecmp((char *)CommandString, "SRVNEEDED", 9) == 0)
  {
    APP_DBG_MSG("--> Set Service Needed bit to True\n");
    ESL_APP_Set_Basic_State_Bitmap(BASIC_STATE_SERVICE_NEEDED_BIT);
    return 0;
  } 
  else if(strncasecmp((char *)CommandString, "UNSYNC", 6) == 0)
  {
    APP_DBG_MSG("--> Set The ESL state to Unsynchronized\n");
    ESL_APP_Unsynchronized_State();
    return 0;
  }   
  else if(strncasecmp((char *)CommandString, "NODISPLAY", 9) == 0)
  {
    APP_DBG_MSG("--> Each display is not displaying an image \n");
    setImageDisplayed(false);
    return 0;
  }  
  return 1;
}

void ESL_APP_cmd_process(void)
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
}

