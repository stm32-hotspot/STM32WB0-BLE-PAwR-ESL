/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_ble.c
  * @author  GPM WBL Application Team
  * @brief   BLE Application
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

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "stm32wb0x.h"
#include "ble.h"
#include "gatt_profile.h"
#include "gap_profile.h"
#include "app_ble.h"
#include "stm32wb0x_hal_radio_timer.h"
#include "bleplat.h"
#include "nvm_db.h"
#include "blenvm.h"
#include "pka_manager.h"
#include "stm32_seq.h"
#include "esl_profile.h"
#include "esl_app.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/**
 * Security parameters structure
 */
typedef struct
{
  /**
   * IO capability of the device
   */
  uint8_t ioCapability;

  /**
   * Authentication requirement of the device
   * Man In the Middle protection required?
   */
  uint8_t mitm_mode;

  /**
   * bonding mode of the device
   */
  uint8_t bonding_mode;

  /**
   * minimum encryption key size requirement
   */
  uint8_t encryptionKeySizeMin;

  /**
   * maximum encryption key size requirement
   */
  uint8_t encryptionKeySizeMax;

  /**
   * this flag indicates whether the host has to initiate
   * the security, wait for pairing or does not have any security
   * requirements.
   * 0x00 : no security required
   * 0x01 : host should initiate security by sending the slave security
   *        request command
   * 0x02 : host need not send the clave security request but it
   * has to wait for paiirng to complete before doing any other
   * processing
   */
  uint8_t initiateSecurity;
  /* USER CODE BEGIN tSecurityParams*/

  /* USER CODE END tSecurityParams */
}SecurityParams_t;

/**
 * Global context contains all BLE common variables.
 */
typedef struct
{
  /**
   * security requirements of the host
   */
  SecurityParams_t bleSecurityParam;

  /**
   * gap service handle
   */
  uint16_t gapServiceHandle;

  /**
   * device name characteristic handle
   */
  uint16_t devNameCharHandle;

  /**
   * appearance characteristic handle
   */
  uint16_t appearanceCharHandle;

  /**
   * connection handle of the current active connection
   * When not in connection, the handle is set to 0xFFFF
   */
  uint16_t connectionHandle;
  /* USER CODE BEGIN BleGlobalContext_t*/

  /* USER CODE END BleGlobalContext_t */
}BleGlobalContext_t;

typedef struct
{
  BleGlobalContext_t BleApplicationContext_legacy;
  APP_BLE_ConnStatus_t Device_Connection_Status;
  /* USER CODE BEGIN PTD_1*/
  
  /* Advertising timeout timerID*/
  VTIMER_HandleType Advertising_mgr_timer_Id;

  /* USER CODE END PTD_1 */
}BleApplicationContext_t;

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_ON_TIMEOUT_MS              (5)
#define ADV_TIMEOUT_MS                 (90 * 1000)
#define ADV_LP_TIMEOUT_MS              (15 * 60 * 1000)  // 15 minutes
/* USER CODE END PD */
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
extern IWDG_HandleTypeDef hiwdg;

NO_INIT(uint32_t dyn_alloc_a[BLE_DYN_ALLOC_SIZE>>2]);

static BleApplicationContext_t bleAppContext;

static char a_GapDeviceName[] = {  'E', 'S', 'L', '_', 'X', 'X'}; /* Gap Device Name */

int8_t offset_vinm0 = 0;

/**
 * Advertising Data
 */
uint8_t a_AdvData[] =
{
  2, AD_TYPE_FLAGS, FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE|FLAG_BIT_BR_EDR_NOT_SUPPORTED,
  7, AD_TYPE_COMPLETE_LOCAL_NAME, 'E', 'S', 'L', '_', 'X', 'X',  /* Complete name */
  3, AD_TYPE_16_BIT_SERV_UUID_CMPLT_LIST, 0x57, 0x18, /* Electronic Shelf Label Service UUID */
};

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout);
static void gap_cmd_resp_wait(void);
static void gap_cmd_resp_release(void);

/* USER CODE BEGIN PFP */
static void Adv_Cancel_Req(void *arg);
static void Adv_Cancel(void);
static void fill_advData(uint8_t *p_adv_data, uint8_t tab_size, const uint8_t*p_bd_addr);
static void fill_localName(char * name, const uint8_t* p_bd_addr);
void watchdog_enable(void);
void watchdog_disable(void);
void watchdog_refresh(void);
/* USER CODE END PFP */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private functions ---------------------------------------------------------*/

/* USER CODE BEGIN PF */

/* USER CODE END PF */

/* Functions Definition ------------------------------------------------------*/
void ModulesInit(void)
{
  BLENVM_Init();
  if (PKAMGR_Init() == PKAMGR_ERROR)
  {
    Error_Handler();
  }
}

void BLE_Init(void)
{
  uint8_t role;
  uint8_t privacy_type = 0;
  tBleStatus ret;
  uint16_t gatt_service_changed_handle;
  uint16_t gap_dev_name_char_handle;
  uint16_t gap_appearance_char_handle;
  uint16_t gap_periph_pref_conn_param_char_handle;
  uint8_t bd_address[6] = {0};
  uint8_t bd_address_len= 6;
  uint16_t appearance = CFG_GAP_APPEARANCE;

  BLE_STACK_InitTypeDef BLE_STACK_InitParams = {
    .BLEStartRamAddress = (uint8_t*)dyn_alloc_a,
    .TotalBufferSize = BLE_DYN_ALLOC_SIZE,
    .NumAttrRecords = CFG_BLE_NUM_GATT_ATTRIBUTES,
    .MaxNumOfClientProcs = CFG_BLE_NUM_OF_CONCURRENT_GATT_CLIENT_PROC,
    .NumOfRadioTasks = CFG_BLE_NUM_RADIO_TASKS,
    .NumOfEATTChannels = CFG_BLE_NUM_EATT_CHANNELS,
    .NumBlockCount = CFG_BLE_MBLOCKS_COUNT,
    .ATT_MTU = CFG_BLE_ATT_MTU_MAX,
    .MaxConnEventLength = CFG_BLE_CONN_EVENT_LENGTH_MAX,
    .SleepClockAccuracy = CFG_BLE_SLEEP_CLOCK_ACCURACY,
    .NumOfAdvDataSet = CFG_BLE_NUM_ADV_SETS,
    .NumOfSubeventsPAwR = CFG_BLE_NUM_PAWR_SUBEVENTS,
    .MaxPAwRSubeventDataCount = CFG_BLE_PAWR_SUBEVENT_DATA_COUNT_MAX,
    .NumOfAuxScanSlots = CFG_BLE_NUM_AUX_SCAN_SLOTS,
    .FilterAcceptListSizeLog2 = CFG_BLE_FILTER_ACCEPT_LIST_SIZE_LOG2,
    .L2CAP_MPS = CFG_BLE_COC_MPS_MAX,
    .L2CAP_NumChannels = CFG_BLE_COC_NBR_MAX,
    .NumOfSyncSlots = CFG_BLE_NUM_SYNC_SLOTS,
    .CTE_MaxNumAntennaIDs = CFG_BLE_NUM_CTE_ANTENNA_IDS_MAX,
    .CTE_MaxNumIQSamples = CFG_BLE_NUM_CTE_IQ_SAMPLES_MAX,
    .NumOfSyncBIG = CFG_BLE_NUM_SYNC_BIG_MAX,
    .NumOfBrcBIG = CFG_BLE_NUM_BRC_BIG_MAX,
    .NumOfSyncBIS = CFG_BLE_NUM_SYNC_BIS_MAX,
    .NumOfBrcBIS = CFG_BLE_NUM_BRC_BIS_MAX,
    .NumOfCIG = CFG_BLE_NUM_CIG_MAX,
    .NumOfCIS = CFG_BLE_NUM_CIS_MAX,
    .isr0_fifo_size = CFG_BLE_ISR0_FIFO_SIZE,
    .isr1_fifo_size = CFG_BLE_ISR1_FIFO_SIZE,
    .user_fifo_size = CFG_BLE_USER_FIFO_SIZE
  };

  /* Bluetooth LE stack init */
  ret = BLE_STACK_Init(&BLE_STACK_InitParams);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Error in BLE_STACK_Init() 0x%02x\r\n", ret);
    Error_Handler();
  }

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)
  
#ifdef ESL_ID
  bd_address[0] = (uint8_t)ESL_ID;
#else
  bd_address[0] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x0000000000FF));
#endif
#ifdef GROUP_ID
  bd_address[1] = (uint8_t)GROUP_ID;
#else
  bd_address[1] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x00000000FF00) >> 8);
#endif
  bd_address[2] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x000000FF0000) >> 16);
  bd_address[3] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x0000FF000000) >> 24);
  bd_address[4] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0x00FF00000000) >> 32);
  bd_address[5] = (uint8_t)((CFG_PUBLIC_BD_ADDRESS & 0xFF0000000000) >> 40);
  (void)bd_address_len;

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_address);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET\n");
  }
#endif

  /**
   * Set TX Power.
   */
  ret = aci_hal_set_tx_power_level(0, CFG_TX_POWER);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_set_tx_power_level command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_set_tx_power_level command\n");
  }

  /**
   * Initialize GATT interface
   */
  ret = aci_gatt_srv_profile_init(GATT_INIT_SERVICE_CHANGED_BIT, &gatt_service_changed_handle);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_srv_profile_init command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_srv_profile_init command\n");
  }

  /**
   * Initialize GAP interface
   */
  role = 0U;
  role |= GAP_PERIPHERAL_ROLE|GAP_OBSERVER_ROLE;

#if CFG_BLE_PRIVACY_ENABLED
  privacy_type = 0x02;
#endif

/* USER CODE BEGIN Role_Mngt*/

/* USER CODE END Role_Mngt */

  ret = aci_gap_init(privacy_type, CFG_BD_ADDRESS_TYPE);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_init command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_init command\n");
  }

  ret = aci_gap_profile_init(role, privacy_type,
                             &gap_dev_name_char_handle,
                             &gap_appearance_char_handle,
                             &gap_periph_pref_conn_param_char_handle);

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_STATIC_RANDOM_ADDR)
  ret = aci_hal_read_config_data(CONFIG_DATA_STORED_STATIC_RANDOM_ADDRESS,
                                 &bd_address_len, bd_address);
  APP_DBG_MSG("  Static Random Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",bd_address[5],bd_address[4],bd_address[3],bd_address[2],bd_address[1],bd_address[0]);
#elif (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)
  APP_DBG_MSG("  Public Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",bd_address[5],bd_address[4],bd_address[3],bd_address[2],bd_address[1],bd_address[0]);
#else
#error "Invalid CFG_BD_ADDRESS_TYPE"
#endif

  /* USER CODE BEGIN aci_gap_profile_init */  
  fill_localName(a_GapDeviceName, bd_address);
  fill_advData(&a_AdvData[0], sizeof(a_AdvData), bd_address);  
  /* USER CODE END aci_gap_profile_init */
  
  ret = Gap_profile_set_dev_name(0, sizeof(a_GapDeviceName), (uint8_t*)a_GapDeviceName);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : Gap_profile_set_dev_name - Device Name, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: Gap_profile_set_dev_name - Device Name\n");
  }

  ret = Gap_profile_set_appearance(0, sizeof(appearance), (uint8_t*)&appearance);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : Gap_profile_set_appearance - Appearance, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: Gap_profile_set_appearance - Appearance\n");
  }

#if CFG_BLE_CONTROLLER_2M_CODED_PHY_ENABLED
  /* Initialize Default PHY */
  ret = hci_le_set_default_phy(0x00, HCI_TX_PHYS_LE_2M_PREF, HCI_RX_PHYS_LE_2M_PREF);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : hci_le_set_default_phy command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: hci_le_set_default_phy command\n");
  }

#endif
  /**
   * Initialize IO capability
   */
  bleAppContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability = CFG_IO_CAPABILITY;
  ret = aci_gap_set_io_capability(bleAppContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_io_capability command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_io_capability command\n");
  }

  /**
   * Initialize authentication
   */
  bleAppContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode             = CFG_MITM_PROTECTION;
  bleAppContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin  = CFG_ENCRYPTION_KEY_SIZE_MIN;
  bleAppContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax  = CFG_ENCRYPTION_KEY_SIZE_MAX;
  bleAppContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode          = CFG_BONDING_MODE;
  /* USER CODE BEGIN Ble_Hci_Gap_Gatt_Init_1*/
  
  /* USER CODE END Ble_Hci_Gap_Gatt_Init_1*/

  ret = aci_gap_set_security_requirements(bleAppContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode,
                                               bleAppContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode,
                                               CFG_SC_SUPPORT,
                                               CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                               bleAppContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin,
                                               bleAppContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax,
                                               GAP_PAIRING_RESP_NONE);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gap_set_security_requirements command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gap_set_security_requirements command\n");
  }

  /**
   * Initialize Filter Accept List
   */
  if (bleAppContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode)
  {
    ret = aci_gap_configure_filter_accept_and_resolving_list(0x01);
    if (ret != BLE_STATUS_SUCCESS)
    {
      APP_DBG_MSG("  Fail   : aci_gap_configure_filter_accept_and_resolving_list command, result: 0x%02X\n", ret);
    }
    else
    {
      APP_DBG_MSG("  Success: aci_gap_configure_filter_accept_and_resolving_list command\n");
    }
  }

  uint8_t LE_Event_Mask[8] = {0xFF,
    HCI_LE_EVT_MASK_BYTE1_PERIODIC_ADVERTISING_SYNC_LOST,
    0xFF,0xFF,
    HCI_LE_EVT_MASK_BYTE4_PERIODIC_ADVERTISING_REPORT_V2|HCI_LE_EVT_MASK_BYTE4_PERIODIC_ADVERTISING_SYNC_TRANSFER_V2,
    0xFF,0xFF,0xFF};
  
  ret= aci_gap_set_le_event_mask(LE_Event_Mask);
  
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : hci_le_set_event_mask command, result: 0x%02X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: hci_le_set_event_mask command\n");
  }  

  APP_DBG_MSG("==>> End BLE_Init function\n"); 
}

void BLEStack_Process_Schedule(void)
{
  /* Keep BLE Stack Process priority low, since there are limited cases
     where stack wants to be rescheduled for busy waiting.  */
  UTIL_SEQ_SetTask( 1U << CFG_TASK_BLE_STACK, CFG_SEQ_PRIO_1);
}

static void BLEStack_Process(void)
{
  APP_DEBUG_SIGNAL_SET(APP_STACK_PROCESS);
  BLE_STACK_Tick();

  if(BLE_STACK_SleepCheck() == 0)
  {
    BLEStack_Process_Schedule();
  }
  
  if(bleAppContext.Device_Connection_Status == APP_BLE_ADV_FAST || bleAppContext.Device_Connection_Status == APP_BLE_ADV_LP)
  {
    watchdog_refresh();    
  }
  
  APP_DEBUG_SIGNAL_RESET(APP_STACK_PROCESS);
}

void VTimer_Process(void)
{
  HAL_RADIO_TIMER_Tick();
}

void VTimer_Process_Schedule(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_VTIMER, CFG_SEQ_PRIO_0);
}

void NVM_Process(void)
{
  NVMDB_Tick();
}

void NVM_Process_Schedule(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_NVM, CFG_SEQ_PRIO_1);
}

/* Function called from PKA_IRQHandler() context. */
void PKAMGR_IRQCallback(void)
{
  BLEStack_Process_Schedule();
}

/* Function called from RADIO_TIMER_TXRX_WKUP_IRQHandler() context. */
void HAL_RADIO_TIMER_TxRxWakeUpCallback(void)
{
  VTimer_Process_Schedule();
  BLEStack_Process_Schedule();
}

/* Function called from RADIO_TIMER_CPU_WKUP_IRQHandler() context. */
void HAL_RADIO_TIMER_CpuWakeUpCallback(void)
{
  VTimer_Process_Schedule();
  BLEStack_Process_Schedule();
}

/* Function called from RADIO_TXRX_IRQHandler() context. */
void HAL_RADIO_TxRxCallback(uint32_t flags)
{
  BLE_STACK_RadioHandler(flags);

  BLEStack_Process_Schedule();
  VTimer_Process_Schedule();
  NVM_Process_Schedule();
}

/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void)
{
  /* USER CODE BEGIN APP_BLE_Init_1 */
  tBleStatus ret;
  /* USER CODE END APP_BLE_Init_1 */

  UTIL_SEQ_RegTask(1U << CFG_TASK_BLE_STACK, UTIL_SEQ_RFU, BLEStack_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_VTIMER, UTIL_SEQ_RFU, VTimer_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_NVM, UTIL_SEQ_RFU, NVM_Process);

  ModulesInit();

  /* Initialization of HCI & GATT & GAP layer */
  BLE_Init();

  /* Need to call stack process at least once. */
  BLEStack_Process_Schedule();

  /**
  * Initialization of the BLE App Context
  */
  bleAppContext.Device_Connection_Status = APP_BLE_IDLE;
  bleAppContext.BleApplicationContext_legacy.connectionHandle = 0xFFFF;

  /* From here, all initialization are BLE application specific */

  /* USER CODE BEGIN APP_BLE_Init_4 */
  UTIL_SEQ_RegTask(1<<CFG_TASK_ADV_CANCEL_ID, UTIL_SEQ_RFU, Adv_Cancel);
  UTIL_SEQ_RegTask(1<<CFG_TASK_ESL_UPD, UTIL_SEQ_RFU, ESL_APP_Process);
  
  /* Create timer to handle the Advertising Stop */
  bleAppContext.Advertising_mgr_timer_Id.callback = Adv_Cancel_Req;    
  
  /* USER CODE END APP_BLE_Init_4 */
  
  /* USER CODE BEGIN APP_BLE_Init_3 */
  
  ret = aci_hal_set_radio_activity_mask(0x0006);
  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_hal_set_radio_activity_mask command, result: 0x%2X\n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_hal_set_radio_activity_mask command\n\r");
  }
  
  ESL_PROFILE_Init();
  
  /* Start to Advertise to accept a connection */
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
  
  /* USER CODE END APP_BLE_Init_3 */
  
  /* USER CODE BEGIN APP_BLE_Init_2 */
  
  /* USER CODE END APP_BLE_Init_2 */
  
  return;
}

void BLEEVT_App_Notification(const hci_pckt *hci_pckt)
{
  tBleStatus ret = BLE_STATUS_ERROR;
  hci_event_pckt    *p_event_pckt;
  hci_le_meta_event *p_meta_evt;
  void *event_data;

  UNUSED(ret);
  /* USER CODE BEGIN SVCCTL_App_Notification */

  /* USER CODE END SVCCTL_App_Notification */

  if(hci_pckt->type != HCI_EVENT_PKT_TYPE && hci_pckt->type != HCI_EVENT_EXT_PKT_TYPE)
  {
    /* Not an event */
    return;
  }

  p_event_pckt = (hci_event_pckt*)hci_pckt->data;

  if(hci_pckt->type == HCI_EVENT_PKT_TYPE){
    event_data = p_event_pckt->data;
  }
  else { /* hci_pckt->type == HCI_EVENT_EXT_PKT_TYPE */
    hci_event_ext_pckt *p_event_pckt = (hci_event_ext_pckt*)hci_pckt->data;
    event_data = p_event_pckt->data;
  }

  switch (p_event_pckt->evt) /* evt field is at same offset in hci_event_pckt and hci_event_ext_pckt */
  {
  case HCI_DISCONNECTION_COMPLETE_EVT_CODE:
    {
      hci_disconnection_complete_event_rp0 *p_disconnection_complete_event;
      p_disconnection_complete_event = (hci_disconnection_complete_event_rp0 *) p_event_pckt->data;

        /* USER CODE BEGIN EVT_DISCONN_COMPLETE_3 */

        /* USER CODE END EVT_DISCONN_COMPLETE_3 */

      if (p_disconnection_complete_event->Connection_Handle == bleAppContext.BleApplicationContext_legacy.connectionHandle)
      {
        bleAppContext.BleApplicationContext_legacy.connectionHandle = 0xFFFF;
        if(bleAppContext.Device_Connection_Status == APP_BLE_CONNECTED_SERVER)
        {
          /* Disconnected without being synchronized. */
          bleAppContext.Device_Connection_Status = APP_BLE_IDLE;
        }
        APP_DBG_MSG(">>== HCI_DISCONNECTION_COMPLETE_EVT_CODE\n");
        APP_DBG_MSG("     - Connection Handle:   0x%02X\n     - Reason:    0x%02X\n",
                    p_disconnection_complete_event->Connection_Handle,
                    p_disconnection_complete_event->Reason);

        if(bleAppContext.Device_Connection_Status != APP_BLE_SYNC)
        {
          APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
        }
        
        /* USER CODE BEGIN EVT_DISCONN_COMPLETE_2 */

        /* USER CODE END EVT_DISCONN_COMPLETE_2 */
      }

      gap_cmd_resp_release();

      /* USER CODE BEGIN EVT_DISCONN_COMPLETE_1 */

      /* USER CODE END EVT_DISCONN_COMPLETE_1 */
      
      /* USER CODE BEGIN EVT_DISCONN_COMPLETE */
      /* USER CODE END EVT_DISCONN_COMPLETE */
    }
    break;

  case HCI_LE_META_EVT_CODE:
    {
      p_meta_evt = (hci_le_meta_event*) p_event_pckt->data;
      /* USER CODE BEGIN EVT_LE_META_EVENT */

      /* USER CODE END EVT_LE_META_EVENT */
      switch (p_meta_evt->subevent)
      {
      case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE:
        {
          hci_le_connection_update_complete_event_rp0 *p_conn_update_complete;
          p_conn_update_complete = (hci_le_connection_update_complete_event_rp0 *) p_meta_evt->data;
          APP_DBG_MSG(">>== HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE\n");
          APP_DBG_MSG("     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout:   %d ms\n",
                      INT(p_conn_update_complete->Connection_Interval*1.25),
                      FRACTIONAL_2DIGITS(p_conn_update_complete->Connection_Interval*1.25),
                      p_conn_update_complete->Peripheral_Latency,
                      p_conn_update_complete->Supervision_Timeout*10);
          UNUSED(p_conn_update_complete);
          /* USER CODE BEGIN EVT_LE_CONN_UPDATE_COMPLETE */

          /* USER CODE END EVT_LE_CONN_UPDATE_COMPLETE */
        }
        break;
      case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE:
        {
          hci_le_phy_update_complete_event_rp0 *p_le_phy_update_complete;
          p_le_phy_update_complete = (hci_le_phy_update_complete_event_rp0*)p_meta_evt->data;
          UNUSED(p_le_phy_update_complete);

          gap_cmd_resp_release();

          /* USER CODE BEGIN EVT_LE_PHY_UPDATE_COMPLETE */

          /* USER CODE END EVT_LE_PHY_UPDATE_COMPLETE */
        }
        break;
      case HCI_LE_ENHANCED_CONNECTION_COMPLETE_V2_SUBEVT_CODE:
        {
          hci_le_enhanced_connection_complete_v2_event_rp0 *event;
          event = (hci_le_enhanced_connection_complete_v2_event_rp0 *) p_meta_evt->data;
          connection_complete_event(event->Status,
                                    event->Connection_Handle,
                                    event->Peer_Address_Type,
                                    event->Peer_Address,
                                    event->Connection_Interval,
                                    event->Peripheral_Latency,
                                    event->Supervision_Timeout);
        }
        break;
      case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE:
        {
          hci_le_connection_complete_event_rp0 *p_conn_complete;
          p_conn_complete = (hci_le_connection_complete_event_rp0 *) p_meta_evt->data;

          connection_complete_event(p_conn_complete->Status,
                                    p_conn_complete->Connection_Handle,
                                    p_conn_complete->Peer_Address_Type,
                                    p_conn_complete->Peer_Address,
                                    p_conn_complete->Connection_Interval,
                                    p_conn_complete->Peripheral_Latency,
                                    p_conn_complete->Supervision_Timeout);
        }
        break;
      /* USER CODE BEGIN EVT_LE_META_EVENT_1 */
        
      case HCI_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_V2_SUBEVT_CODE:
        {
          hci_le_periodic_advertising_sync_transfer_received_v2_event_rp0 *event;
          event = (hci_le_periodic_advertising_sync_transfer_received_v2_event_rp0 *) p_meta_evt->data;
          
          APP_DBG_MSG("HCI_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_V2\n");
          
          if(event->Status == 0)
          {
            bleAppContext.Device_Connection_Status = APP_BLE_SYNC;
            ESL_PROFILE_SyncInfoReceived(event->Sync_Handle);
          }
          else
          {
            APP_DBG_MSG("Status %02X\n", event->Status);
          }
        }
        break;
        
      case HCI_LE_PERIODIC_ADVERTISING_REPORT_V2_SUBEVT_CODE:
        {
          hci_le_periodic_advertising_report_v2_event_rp0 *event;
          event = (hci_le_periodic_advertising_report_v2_event_rp0 *)p_meta_evt->data;
          
          //APP_DBG_MSG("HCI_LE_PERIODIC_ADVERTISING_REPORT_V2\n");
          
          if(event->Data_Status == HCI_PADV_DATA_STATUS_COMPLETE && event->Data_Length != 0)
          {
            ESL_PROFILE_AdvPayloadReceived(event->Periodic_Event_Counter, event->Data, event->Data_Length);
          }          
          watchdog_refresh();
        }
        break;
        
      case HCI_LE_PERIODIC_ADVERTISING_SYNC_LOST_SUBEVT_CODE:
        {
          hci_le_periodic_advertising_sync_lost_event_rp0 *event;
          event = (hci_le_periodic_advertising_sync_lost_event_rp0 *)p_meta_evt->data;
          (void) event;
          
          APP_DBG_MSG("HCI_LE_PERIODIC_ADVERTISING_SYNC_LOST\n");
          
          bleAppContext.Device_Connection_Status = APP_BLE_IDLE;
          APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_LP);
        }
        break;

      /* USER CODE END EVT_LE_META_EVENT_1 */

      default:
        /* USER CODE BEGIN SUBEVENT_DEFAULT */
        APP_DBG_MSG("HCI_LE_META_EVT: 0x%02X\n", p_meta_evt->subevent);
        /* USER CODE END SUBEVENT_DEFAULT */
        break;
      }
    } /* HCI_LE_META_EVT_CODE */
    break;

  case HCI_VENDOR_EVT_CODE:
    {
      aci_blecore_event *p_blecore_evt = (aci_blecore_event*) event_data;
      /* USER CODE BEGIN EVT_VENDOR */

      /* USER CODE END EVT_VENDOR */
      switch (p_blecore_evt->ecode)
      {
        /* USER CODE BEGIN ecode */

        /* USER CODE END ecode */
      case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE:
        {
          aci_l2cap_connection_update_resp_event_rp0 *p_l2cap_conn_update_resp;
          p_l2cap_conn_update_resp = (aci_l2cap_connection_update_resp_event_rp0 *) p_blecore_evt->data;
          UNUSED(p_l2cap_conn_update_resp);
          /* USER CODE BEGIN EVT_L2CAP_CONNECTION_UPDATE_RESP */

          /* USER CODE END EVT_L2CAP_CONNECTION_UPDATE_RESP */
        }
        break;
      case ACI_GAP_PROC_COMPLETE_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PROC_COMPLETE_VSEVT_CODE\n");
          aci_gap_proc_complete_event_rp0 *p_gap_proc_complete;
          p_gap_proc_complete = (aci_gap_proc_complete_event_rp0*) p_blecore_evt->data;
          UNUSED(p_gap_proc_complete);

          /* USER CODE BEGIN EVT_GAP_PROCEDURE_COMPLETE */

          /* USER CODE END EVT_GAP_PROCEDURE_COMPLETE */
        }
        break;
      case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:
        /* USER CODE BEGIN RADIO_ACTIVITY_EVENT*/
        
        /* USER CODE END RADIO_ACTIVITY_EVENT*/
        break;
      case ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE\n");
          /* USER CODE BEGIN ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE*/

          /* USER CODE END ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE*/
        }
        break;
      case ACI_GAP_PASSKEY_REQ_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PASSKEY_REQ_VSEVT_CODE\n");

          ret = aci_gap_passkey_resp(bleAppContext.BleApplicationContext_legacy.connectionHandle, CFG_FIXED_PIN);
          if (ret != BLE_STATUS_SUCCESS)
          {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Fail, reason: 0x%02X\n", ret);
          }
          else
          {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Success\n");
          }
          /* USER CODE BEGIN ACI_GAP_PASSKEY_REQ_VSEVT_CODE*/

          /* USER CODE END ACI_GAP_PASSKEY_REQ_VSEVT_CODE*/
        }
        break;
      case ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE:
        {
          uint8_t confirm_value;
          APP_DBG_MSG(">>== ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE\n");
          APP_DBG_MSG("     - numeric_value = %d\n",
                      ((aci_gap_numeric_comparison_value_event_rp0 *)(p_blecore_evt->data))->Numeric_Value);
          APP_DBG_MSG("     - Hex_value = %x\n",
                      ((aci_gap_numeric_comparison_value_event_rp0 *)(p_blecore_evt->data))->Numeric_Value);

          /* Set confirm value to 1(YES) */
          confirm_value = 1;
          /* USER CODE BEGIN ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE_0*/

          /* USER CODE END ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE_0*/

          ret = aci_gap_numeric_comparison_value_confirm_yesno(bleAppContext.BleApplicationContext_legacy.connectionHandle, confirm_value);
          if (ret != BLE_STATUS_SUCCESS)
          {
            APP_DBG_MSG("==>> aci_gap_numeric_comparison_value_confirm_yesno : Fail, reason: 0x%02X\n", ret);
          }
          else
          {
            APP_DBG_MSG("==>> aci_gap_numeric_comparison_value_confirm_yesno : Success\n");
          }
          /* USER CODE BEGIN ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE*/

          /* USER CODE END ACI_GAP_NUMERIC_COMPARISON_VALUE_VSEVT_CODE*/
        }
        break;
      case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE:
        {
          APP_DBG_MSG(">>== ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE\n");
          aci_gap_pairing_complete_event_rp0 *p_pairing_complete;
          p_pairing_complete = (aci_gap_pairing_complete_event_rp0*)p_blecore_evt->data;

          if (p_pairing_complete->Status != 0)
          {
            APP_DBG_MSG("     - Pairing KO\n     - Status: 0x%02X\n     - Reason: 0x%02X\n",
                        p_pairing_complete->Status, p_pairing_complete->Reason);
          }
          else
          {
            APP_DBG_MSG("     - Pairing Success\n");
          }
          APP_DBG_MSG("\n");

          /* USER CODE BEGIN ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE*/

          /* USER CODE END ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE*/
        }
        break;
        /* USER CODE BEGIN EVT_VENDOR_1 */
      case ACI_HAL_FW_ERROR_VSEVT_CODE:
        {
          aci_hal_fw_error_event_rp0 *event;
          event = (aci_hal_fw_error_event_rp0 *)p_blecore_evt->data;
          
          APP_DBG_MSG("ACI_HAL_FW_ERROR_VSEVT 0x%02X\n", event->FW_Error_Type);
          NVIC_SystemReset();
          
        }
        break;        
        /* USER CODE END EVT_VENDOR_1 */
      default:
        /* USER CODE BEGIN EVT_VENDOR_DEFAULT */
        APP_DBG_MSG("HCI_VENDOR_EVT: 0x%04X\n", p_blecore_evt->ecode);
        /* USER CODE END EVT_VENDOR_DEFAULT */
        break;
      }
    } /* HCI_VENDOR_EVT_CODE */
    break;

  case HCI_HARDWARE_ERROR_EVT_CODE:
    {
      hci_hardware_error_event_rp0 *p_hci_hardware_error_event;
      p_hci_hardware_error_event = (hci_hardware_error_event_rp0*)p_event_pckt->data;

      APP_DBG_MSG("HCI_HARDWARE_ERROR_EVT 0x%02X\n", p_hci_hardware_error_event->Hardware_Code);
      
      if (p_hci_hardware_error_event->Hardware_Code <= 0x03)
      {
        NVIC_SystemReset();
      }
    }
    break;

    /* USER CODE BEGIN EVENT_PCKT */

    /* USER CODE END EVENT_PCKT */

  default:
    /* USER CODE BEGIN ECODE_DEFAULT*/
    APP_DBG_MSG("HCI_EVENT: 0x%02X\n", p_event_pckt->evt);
    /* USER CODE END ECODE_DEFAULT*/
    break;
  }
}

static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout)
{
  /* USER CODE BEGIN HCI_EVT_LE_CONN_COMPLETE_1 */

  /* USER CODE END HCI_EVT_LE_CONN_COMPLETE_1 */
  APP_DBG_MSG(">>== hci_le_connection_complete_event - Connection handle: 0x%04X\n", Connection_Handle);
  APP_DBG_MSG("     - Connection established with @:%02x:%02x:%02x:%02x:%02x:%02x\n",
              Peer_Address[5],
              Peer_Address[4],
              Peer_Address[3],
              Peer_Address[2],
              Peer_Address[1],
              Peer_Address[0]);
  APP_DBG_MSG("     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout: %d ms\n",
              INT(Connection_Interval*1.25),
              FRACTIONAL_2DIGITS(Connection_Interval*1.25),
              Peripheral_Latency,
              Supervision_Timeout * 10
              );

  if (bleAppContext.Device_Connection_Status == APP_BLE_LP_CONNECTING)
  {
    /* Connection as client */
    bleAppContext.Device_Connection_Status = APP_BLE_CONNECTED_CLIENT;
  }
  else
  {
    /* Connection as server */
    bleAppContext.Device_Connection_Status = APP_BLE_CONNECTED_SERVER;
  }
  bleAppContext.BleApplicationContext_legacy.connectionHandle = Connection_Handle;

  /* USER CODE BEGIN HCI_EVT_LE_CONN_COMPLETE */
  /* The connection is done, there is no need anymore to schedule the LP ADV */
  HAL_RADIO_TIMER_StopVirtualTimer(&(bleAppContext.Advertising_mgr_timer_Id));
  
  ESL_PROFILE_ConnectionComplete(Connection_Handle);
  
  /* USER CODE END HCI_EVT_LE_CONN_COMPLETE */
}/* end hci_le_connection_complete_event() */

/* USER CODE BEGIN EVT_VENDOR_3 */

/* USER CODE END EVT_VENDOR_3 */

APP_BLE_ConnStatus_t APP_BLE_Get_Server_Connection_Status(void)
{
  return bleAppContext.Device_Connection_Status;
}

void APP_BLE_Procedure_Gap_General(ProcGapGeneralId_t ProcGapGeneralId)
{
  tBleStatus status;

  switch(ProcGapGeneralId)
  {
#if (CFG_BLE_CONTROLLER_2M_CODED_PHY_ENABLED == 1)
    case PROC_GAP_GEN_PHY_TOGGLE:
    {
      uint8_t phy_tx, phy_rx;

      status = hci_le_read_phy(bleAppContext.BleApplicationContext_legacy.connectionHandle, &phy_tx, &phy_rx);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("hci_le_read_phy failure: reason=0x%02X\n",status);
      }
      else
      {
        APP_DBG_MSG("==>> hci_le_read_phy - Success\n");
        APP_DBG_MSG("==>> PHY Param  TX= %d, RX= %d\n", phy_tx, phy_rx);
        if ((phy_tx == HCI_TX_PHY_LE_2M) && (phy_rx == HCI_RX_PHY_LE_2M))
        {
          APP_DBG_MSG("==>> hci_le_set_phy PHY Param  TX= %d, RX= %d - ", HCI_TX_PHY_LE_1M, HCI_RX_PHY_LE_1M);
          status = hci_le_set_phy(bleAppContext.BleApplicationContext_legacy.connectionHandle, 0, HCI_TX_PHYS_LE_1M_PREF, HCI_RX_PHYS_LE_1M_PREF, 0);
          if (status != BLE_STATUS_SUCCESS)
          {
            APP_DBG_MSG("Fail\n");
          }
          else
          {
            APP_DBG_MSG("Success\n");
            gap_cmd_resp_wait();/* waiting for HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE */
          }
        }
        else
        {
          APP_DBG_MSG("==>> hci_le_set_phy PHY Param  TX= %d, RX= %d - ", HCI_TX_PHYS_LE_2M_PREF, HCI_RX_PHYS_LE_2M_PREF);
          status = hci_le_set_phy(bleAppContext.BleApplicationContext_legacy.connectionHandle, 0, HCI_TX_PHYS_LE_2M_PREF, HCI_RX_PHYS_LE_2M_PREF, 0);
          if (status != BLE_STATUS_SUCCESS)
          {
            APP_DBG_MSG("Fail\n");
          }
          else
          {
            APP_DBG_MSG("Success\n");
            gap_cmd_resp_wait();/* waiting for HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE */
          }
        }
      }
      break;
    }/* PROC_GAP_GEN_PHY_TOGGLE */
#endif /* (CFG_BLE_CONTROLLER_2M_CODED_PHY_ENABLED == 1) */
    case PROC_GAP_GEN_CONN_TERMINATE:
    {
      status = aci_gap_terminate(bleAppContext.BleApplicationContext_legacy.connectionHandle, BLE_ERROR_TERMINATED_REMOTE_USER);
      if (status != BLE_STATUS_SUCCESS)
      {
         APP_DBG_MSG("aci_gap_terminate failure: reason=0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> aci_gap_terminate : Success\n");
        gap_cmd_resp_wait();/* waiting for HCI_DISCONNECTION_COMPLETE_EVT_CODE */
      }
      break;
    }/* PROC_GAP_GEN_CONN_TERMINATE */
    case PROC_GATT_EXCHANGE_CONFIG:
    {
      status =aci_gatt_clt_exchange_config(bleAppContext.BleApplicationContext_legacy.connectionHandle);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("aci_gatt_clt_exchange_config failure: reason=0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> aci_gatt_clt_exchange_config : Success\n");
      }
      break;
    }
    /* USER CODE BEGIN GAP_GENERAL */

    /* USER CODE END GAP_GENERAL */
    default:
      break;
  }
  return;
}

void APP_BLE_Procedure_Gap_Peripheral(ProcGapPeripheralId_t ProcGapPeripheralId)
{
  tBleStatus status;
  uint32_t paramA = ADV_INTERVAL_MIN;
  uint32_t paramB = ADV_INTERVAL_MAX;
  uint32_t paramC, paramD;

  /* First set parameters before calling ACI APIs, only if needed */
  switch(ProcGapPeripheralId)
  {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST:
    {
      paramA = ADV_INTERVAL_MIN;
      paramB = ADV_INTERVAL_MAX;
      paramC = APP_BLE_ADV_FAST;

      /* USER CODE BEGIN PROC_GAP_PERIPH_ADVERTISE_START_FAST */

      /* USER CODE END PROC_GAP_PERIPH_ADVERTISE_START_FAST */
      break;
    }/* PROC_GAP_PERIPH_ADVERTISE_START_FAST */
    case PROC_GAP_PERIPH_ADVERTISE_START_LP:
    {
      paramA = ADV_LP_INTERVAL_MIN;
      paramB = ADV_LP_INTERVAL_MAX;
      paramC = APP_BLE_ADV_LP;

      /* USER CODE BEGIN PROC_GAP_PERIPH_ADVERTISE_START_LP */

      /* USER CODE END PROC_GAP_PERIPH_ADVERTISE_START_LP */
      break;
    }/* PROC_GAP_PERIPH_ADVERTISE_START_LP */
    case PROC_GAP_PERIPH_ADVERTISE_STOP:
    {
      paramC = APP_BLE_IDLE;

      /* USER CODE BEGIN PROC_GAP_PERIPH_ADVERTISE_STOP */

      /* USER CODE END PROC_GAP_PERIPH_ADVERTISE_STOP */
      break;
    }/* PROC_GAP_PERIPH_ADVERTISE_STOP */
    case PROC_GAP_PERIPH_CONN_PARAM_UPDATE:
    {
      paramA = CONN_INT_MS(1000);
      paramB = CONN_INT_MS(1000);
      paramC = 0x0000;
      paramD = 0x01F4;

      /* USER CODE BEGIN CONN_PARAM_UPDATE */
      
      /* USER CODE END CONN_PARAM_UPDATE */
      break;
    }/* PROC_GAP_PERIPH_CONN_PARAM_UPDATE */
    case PROC_GAP_PERIPH_CONN_TERMINATE:
    {
      status = aci_gap_terminate(bleAppContext.BleApplicationContext_legacy.connectionHandle, 0x13);
      if (status != BLE_STATUS_SUCCESS)
      {
         APP_DBG_MSG("aci_gap_terminate failure: reason=0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> aci_gap_terminate : Success\n");
        gap_cmd_resp_wait();/* waiting for HCI_DISCONNECTION_COMPLETE_EVT_CODE */
      }
      break;
    }
    /* PROC_GAP_PERIPH_CONN_TERMINATE */
    default:
      break;
  }

  /* Call ACI APIs */
  switch(ProcGapPeripheralId)
  {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST:
    case PROC_GAP_PERIPH_ADVERTISE_START_LP:
    {

      /* USER CODE BEGIN PROC_GAP_PERIPHERAL_ID */

      /* USER CODE END PROC_GAP_PERIPHERAL_ID */

      Advertising_Set_Parameters_t Advertising_Set_Parameters = {0};

      /* Start Fast or Low Power Advertising */

      /* Set advertising configuration for legacy advertising */
      status = aci_gap_set_advertising_configuration(0,
                                                     GAP_MODE_GENERAL_DISCOVERABLE,
                                                     ADV_TYPE,
                                                     paramA,
                                                     paramB,
                                                     HCI_ADV_CH_ALL,
                                                     0,
                                                     NULL, /* No peer address */
                                                     HCI_ADV_FILTER_NONE,
                                                     0, /* 0 dBm */
                                                     HCI_PHY_LE_1M, /* Primary advertising PHY */
                                                     0, /* 0 skips */
                                                     HCI_PHY_LE_1M, /* Secondary advertising PHY. Not used with legacy advertising. */
                                                     0, /* SID */
                                                     0 /* No scan request notifications */);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("==>> aci_gap_set_advertising_configuration - fail, result: 0x%02X\n", status);
      }
      else
      {
        bleAppContext.Device_Connection_Status = (APP_BLE_ConnStatus_t)paramC;
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_configuration\n");
      }

      status = aci_gap_set_advertising_data(0, ADV_COMPLETE_DATA, sizeof(a_AdvData), (uint8_t*) a_AdvData);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("==>> aci_gap_set_advertising_data Failed, result: 0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_data\n");
      }

      /* Enable advertising */
      status = aci_gap_set_advertising_enable(ENABLE, 1, &Advertising_Set_Parameters);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("==>> aci_gap_set_advertising_enable Failed, result: 0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_enable\n");
        
        HAL_RADIO_TIMER_StopVirtualTimer(&(bleAppContext.Advertising_mgr_timer_Id));
        
        /* Start a timer to stop advertising after a while */
        if(ProcGapPeripheralId == PROC_GAP_PERIPH_ADVERTISE_START_FAST)
        {
          HAL_RADIO_TIMER_StartVirtualTimer(&bleAppContext.Advertising_mgr_timer_Id, ADV_TIMEOUT_MS);
        }
        else if(ProcGapPeripheralId == PROC_GAP_PERIPH_ADVERTISE_START_LP)
        {
          HAL_RADIO_TIMER_StartVirtualTimer(&bleAppContext.Advertising_mgr_timer_Id, ADV_LP_TIMEOUT_MS);
        }
      }
      break;
    }
    case PROC_GAP_PERIPH_ADVERTISE_STOP:
    {
      status = aci_gap_set_advertising_enable(DISABLE, 0, NULL);
      if (status != BLE_STATUS_SUCCESS)
      {
        bleAppContext.Device_Connection_Status = (APP_BLE_ConnStatus_t)paramC;
        APP_DBG_MSG("Disable advertising - fail, result: 0x%02X\n",status);
      }
      else
      {
        APP_DBG_MSG("==>> Disable advertising - Success\n");
      }
      HAL_RADIO_TIMER_StopVirtualTimer(&(bleAppContext.Advertising_mgr_timer_Id));
      break;
    }/* PROC_GAP_PERIPH_ADVERTISE_STOP */
    case PROC_GAP_PERIPH_ADVERTISE_DATA_UPDATE:
    {
      Advertising_Set_Parameters_t Advertising_Set_Parameters = {0};

      /* Disable advertising */
      status = aci_gap_set_advertising_enable(DISABLE, 0, NULL);
      if (status != BLE_STATUS_SUCCESS)
      {
        bleAppContext.Device_Connection_Status = (APP_BLE_ConnStatus_t)paramC;
        APP_DBG_MSG("Disable advertising - fail, result: 0x%02X\n",status);
      }
      else
      {
        APP_DBG_MSG("==>> Disable advertising - Success\n");
      }
      /* Set advertising data */
      status = aci_gap_set_advertising_data(0, ADV_COMPLETE_DATA, sizeof(a_AdvData), (uint8_t*) a_AdvData);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("==>> aci_gap_set_advertising_data Failed, result: 0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_data\n");
      }
      /* Enable advertising */
      status = aci_gap_set_advertising_enable(ENABLE, 1, &Advertising_Set_Parameters);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("==>> aci_gap_set_advertising_enable Failed, result: 0x%02X\n", status);
      }
      else
      {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_enable\n");
      }
      break;
    }/* PROC_GAP_PERIPH_ADVERTISE_DATA_UPDATE */
    case PROC_GAP_PERIPH_CONN_PARAM_UPDATE:
    {
       status = aci_l2cap_connection_parameter_update_req(
                                                       bleAppContext.BleApplicationContext_legacy.connectionHandle,
                                                       paramA,
                                                       paramB,
                                                       paramC,
                                                       paramD);
      if (status != BLE_STATUS_SUCCESS)
      {
        APP_DBG_MSG("aci_l2cap_connection_parameter_update_req - fail, result: 0x%02X\n",status);
      }
      else
      {
        APP_DBG_MSG("==>> aci_l2cap_connection_parameter_update_req - Success\n");
      }

      break;
    }/* PROC_GAP_PERIPH_CONN_PARAM_UPDATE */

    case PROC_GAP_PERIPH_SET_BROADCAST_MODE:
    {

      break;
    }/* PROC_GAP_PERIPH_SET_BROADCAST_MODE */
    default:
      break;
  }
  return;
}

/* USER CODE BEGIN FD*/

/* USER CODE END FD*/

static void gap_cmd_resp_release(void)
{
  UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_PROC_GAP_COMPLETE);
  return;
}

static void gap_cmd_resp_wait(void)
{
  UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_PROC_GAP_COMPLETE);
  return;
}

/* USER CODE BEGIN FD_LOCAL_FUNCTION */

static void Adv_Cancel_Req(void *arg)
{
  APP_DBG_MSG("Adv_Cancel_Req\n");
  UTIL_SEQ_SetTask(1 << CFG_TASK_ADV_CANCEL_ID, CFG_SEQ_PRIO_0);
  return;
}

static void Adv_Cancel(void)
{
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
  bleAppContext.Device_Connection_Status = APP_BLE_IDLE;
  
  watchdog_disable();
  
  return;
}

static void convert_byte_to_ascii_hex(uint8_t byte, char *s)
{
  uint8_t nibble_1 = ((byte & 0xF0)>>4);
  uint8_t nibble_0 = byte & 0xF;
  
  if(nibble_1 >= 0x0A)
  {
    s[0] = nibble_1 - 0x0A + 'A';
  }
  else
  {
    s[0] = nibble_1 + '0';
  }
  
  if(nibble_0 >= 0x0A)
  {
    s[1] = nibble_0 - 0x0A + 'A';
  }
  else
  {
    s[1] = nibble_0 + '0';
  }  
}

static void fill_localName(char * name, const uint8_t* p_bd_addr)
{
  for(uint8_t i = 0; name[i] != '\0' ;i++)
  {
    if(i > 0 && name[i - 1] == 'X' && name[i] == 'X')
    {
      convert_byte_to_ascii_hex(p_bd_addr[0], &name[i - 1]);
      break;
    }
  }    
}

static void fill_advData(uint8_t *p_adv_data, uint8_t tab_size, const uint8_t* p_bd_addr)
{
  uint16_t i =0;
  uint8_t ad_length, ad_type;  
  
  while(i < tab_size)
  {
    ad_length = p_adv_data[i];
    ad_type = p_adv_data[i + 1];
      
    switch (ad_type)
    {
    case AD_TYPE_COMPLETE_LOCAL_NAME:
      {        
        if((p_adv_data[i + ad_length] == 'X') && (p_adv_data[i + ad_length - 1] == 'X'))
        {
          convert_byte_to_ascii_hex(p_bd_addr[0], (char *)&p_adv_data[i + ad_length - 1]);
        }
      }
      break;
    default:
      break;
    }
    i += ad_length + 1; /* increment the iterator to go on next element*/
  }
}
/* USER CODE END FD_LOCAL_FUNCTION */

/* USER CODE BEGIN FD_WRAP_FUNCTIONS */

void ESL_APP_ProcessRequest(void)
{
  UTIL_SEQ_SetTask( 1U << CFG_TASK_ESL_UPD, CFG_SEQ_PRIO_0);
}

void watchdog_enable(void)
{
#ifndef DISABLE_WATCHDOG
  __HAL_RCC_WDG_CLK_ENABLE();
  HAL_IWDG_Refresh(&hiwdg);
#endif
}

void watchdog_disable(void)
{
#ifndef DISABLE_WATCHDOG
  HAL_IWDG_Refresh(&hiwdg);
  __HAL_RCC_WDG_CLK_DISABLE();
#endif
}

void watchdog_refresh(void)
{
#ifndef DISABLE_WATCHDOG
  HAL_IWDG_Refresh(&hiwdg);
#endif
}

#if (CFG_BUTTON_SUPPORTED == 1)
void APPE_Button1Action(void)
{    
  if(bleAppContext.Device_Connection_Status == APP_BLE_IDLE || bleAppContext.Device_Connection_Status == APP_BLE_ADV_LP)
  {
     /* Relaunch fast advertising */
    
    if(bleAppContext.Device_Connection_Status == APP_BLE_IDLE)
    {
      watchdog_enable();
    }
    
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);    
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
  }

  return;
}
#endif /* (CFG_BUTTON_SUPPORTED == 1) */
/* USER CODE END FD_WRAP_FUNCTIONS */

/** \endcond
 */
