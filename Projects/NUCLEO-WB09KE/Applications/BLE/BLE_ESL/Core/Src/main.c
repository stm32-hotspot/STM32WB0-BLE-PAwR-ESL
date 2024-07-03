/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

PKA_HandleTypeDef hpka;
ADC_HandleTypeDef hadc;
IWDG_HandleTypeDef hiwdg;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RADIO_Init(void);
static void MX_RADIO_TIMER_Init(void);
static void MX_PKA_Init(void);
void MX_ADC_Init(void);
#ifndef DISABLE_WATCHDOG
static void MX_IWDG_Init(void);
#endif

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RADIO_Init();
  MX_RADIO_TIMER_Init();
  MX_PKA_Init();
  MX_ADC_Init();
#ifndef DISABLE_WATCHDOG
  MX_IWDG_Init();
#endif
  
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init code for STM32_BLE */
  MX_APPE_Init(NULL);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_APPE_Process();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the SYSCLKSource and SYSCLKDivider
  */
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_RC64MPLL;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_RC64MPLL_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV4;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief PKA Initialization Function
  * @param None
  * @retval None
  */
static void MX_PKA_Init(void)
{

  /* USER CODE BEGIN PKA_Init 0 */

  /* USER CODE END PKA_Init 0 */

  /* USER CODE BEGIN PKA_Init 1 */

  /* USER CODE END PKA_Init 1 */
  hpka.Instance = PKA;
  if (HAL_PKA_Init(&hpka) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PKA_Init 2 */

  /* USER CODE END PKA_Init 2 */

}

/**
  * @brief RADIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_Init(void)
{

  /* USER CODE BEGIN RADIO_Init 0 */

  /* USER CODE END RADIO_Init 0 */

  RADIO_HandleTypeDef hradio = {0};

  /* USER CODE BEGIN RADIO_Init 1 */

  /* USER CODE END RADIO_Init 1 */

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
  {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  hradio.Instance = RADIO;
  HAL_RADIO_Init(&hradio);
  /* USER CODE BEGIN RADIO_Init 2 */

  /* USER CODE END RADIO_Init 2 */

}

/**
  * @brief RADIO_TIMER Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_TIMER_Init(void)
{

  /* USER CODE BEGIN RADIO_TIMER_Init 0 */

  /* USER CODE END RADIO_TIMER_Init 0 */

  RADIO_TIMER_InitTypeDef RADIO_TIMER_InitStruct = {0};

  /* USER CODE BEGIN RADIO_TIMER_Init 1 */

  /* USER CODE END RADIO_TIMER_Init 1 */

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
  {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  /* Wait to be sure that the Radio Timer is active */
  while(LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP) < 0x10);
  RADIO_TIMER_InitStruct.XTAL_StartupTime = 320;
  RADIO_TIMER_InitStruct.enableInitialCalibration = FALSE;
  RADIO_TIMER_InitStruct.periodicCalibrationInterval = 0;
  HAL_RADIO_TIMER_Init(&RADIO_TIMER_InitStruct);
  /* USER CODE BEGIN RADIO_TIMER_Init 2 */

  /* USER CODE END RADIO_TIMER_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*RT DEBUG GPIO_Init */
  RT_DEBUG_GPIO_Init();

/* USER CODE BEGIN MX_GPIO_Init_2 */
  
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  (void)GPIO_InitStruct;  
  
#ifdef ENABLE_TXRX_SIGNAL
  
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
#endif
/* USER CODE END MX_GPIO_Init_2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC_Init(void)
{
/* USER CODE BEGIN MX_ADC_Init_1 */
/* USER CODE END MX_ADC_Init_1 */  
  
  ADC_ChannelConfTypeDef xChannel;
  
  /* Parameters for ADC initialization */
  __HAL_RCC_ADCDIG_CLK_ENABLE();
  __HAL_RCC_ADCANA_CLK_ENABLE();

    /* Enable the ADC peripheral */
  hadc.Instance = ADC1;
  hadc.Init.DownSamplerConfig.DataRatio = ADC_DS_RATIO_128;
  hadc.Init.DownSamplerConfig.DataWidth = ADC_DS_DATA_WIDTH_16_BIT;
  hadc.Init.InvertOutputMode = ADC_DATA_INVERT_SING;
  hadc.Init.Overrun = ADC_NEW_DATA_IS_KEPT;
  hadc.Init.SampleRate = ADC_SAMPLE_RATE_64;
  hadc.Init.SamplingMode = ADC_SAMPLING_AT_END;
  hadc.Init.SequenceLength = 1;

  if (HAL_ADC_Init(&hadc) != HAL_OK) {
    Error_Handler();
  }

  /* Set the input channel */
  xChannel.Channel = ADC_CHANNEL_VBAT;
  xChannel.Rank = ADC_RANK_1;
  xChannel.VoltRange = ADC_VIN_RANGE_3V6;
  xChannel.CalibrationPoint.Number = ADC_CALIB_POINT_1;
  xChannel.CalibrationPoint.Offset = 0;
  xChannel.CalibrationPoint.Gain = LL_ADC_DEFAULT_RANGE_VALUE_3V6;
  
  if(LL_ADC_GET_CALIB_GAIN_FOR_VINMX_3V6() != 0xFFF)
  {
    xChannel.CalibrationPoint.Offset = LL_ADC_GET_CALIB_OFFSET_FOR_VINMX_3V6();
    xChannel.CalibrationPoint.Gain = LL_ADC_GET_CALIB_GAIN_FOR_VINMX_3V6();
  }
  
  HAL_ADC_ConfigChannel(&hadc, &xChannel);
  
/* USER CODE BEGIN MX_ADC_Init_2 */
   
/* USER CODE END MX_ADC_Init_2 */
}

#ifndef DISABLE_WATCHDOG
/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */


  /* USER CODE BEGIN IWDG_Init 1 */

  /* WDG clock enable */
  __HAL_RCC_WDG_CLK_ENABLE();

  /* Force WDG peripheral reset */
  __HAL_RCC_WDG_FORCE_RESET();
  __HAL_RCC_WDG_RELEASE_RESET();

  /* Check if WDG Reset Release flag interrupt occurred or not */
  while(__HAL_RCC_GET_IT(RCC_IT_WDGRSTRELRDY) == 0)
  {
  }

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_128;
  hiwdg.Init.Window = IWDG_WINDOW_DISABLE;
  hiwdg.Init.Reload = 0xFFF;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END IWDG_Init 1 */


  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */
}
#endif

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  NVIC_SystemReset();
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  Error_Handler();
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
