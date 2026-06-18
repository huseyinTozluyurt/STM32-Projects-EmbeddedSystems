/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "hcsr04.h"
#include <stdio.h>
#include <string.h>
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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */



#define SENSOR_COUNT 4

#define OBSTACLE_THRESHOLD_CM 20



#define BIN_LED0_PORT GPIOA
#define BIN_LED0_PIN  GPIO_PIN_5   // Least significant bit

#define BIN_LED1_PORT GPIOA
#define BIN_LED1_PIN  GPIO_PIN_6  // Most significant bit



HCSR04_Sensor hcsr04_sensors[SENSOR_COUNT] =
{
    /*
     * Front sensor
     * TRIG: PB0
     * ECHO: PA0
     */
    {GPIOB, GPIO_PIN_0,  GPIOA, GPIO_PIN_0,  0, HCSR04_TIMEOUT_ERROR},

    /*
     * Left sensor
     * TRIG: PB1
     * ECHO: PA1
     */
    {GPIOB, GPIO_PIN_1,  GPIOA, GPIO_PIN_1,  0, HCSR04_TIMEOUT_ERROR},

    /*
     * Right sensor
     * TRIG: PB10
     * ECHO: PA2
     */
    {GPIOB, GPIO_PIN_10, GPIOA, GPIO_PIN_2,  0, HCSR04_TIMEOUT_ERROR},

    /*
     * Back sensor
     * TRIG: PB11
     * ECHO: PA3
     */
    {GPIOB, GPIO_PIN_11, GPIOA, GPIO_PIN_3,  0, HCSR04_TIMEOUT_ERROR}
};




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Set_Binary_LEDs(uint8_t value)
{
    /*
     * value: 0, 1, 2, 3
     *
     * PB7 PB6
     *  0   0  = 0
     *  0   1  = 1
     *  1   0  = 2
     *  1   1  = 3
     */

    if (value & 0x01)
    {
        HAL_GPIO_WritePin(BIN_LED0_PORT, BIN_LED0_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(BIN_LED0_PORT, BIN_LED0_PIN, GPIO_PIN_RESET);
    }

    if (value & 0x02)
    {
        HAL_GPIO_WritePin(BIN_LED1_PORT, BIN_LED1_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(BIN_LED1_PORT, BIN_LED1_PIN, GPIO_PIN_RESET);
    }
}


void Print_Sensor_Readings(void)
{
    char msg[200];

    snprintf(msg, sizeof(msg),
             "Front: %lu cm [%s] | Left: %lu cm [%s] | Right: %lu cm [%s] | Back: %lu cm [%s]\r\n",
             hcsr04_sensors[0].distance_cm,
             hcsr04_sensors[0].status == HCSR04_OK ? "OK" : "ERR",
             hcsr04_sensors[1].distance_cm,
             hcsr04_sensors[1].status == HCSR04_OK ? "OK" : "ERR",
             hcsr04_sensors[2].distance_cm,
             hcsr04_sensors[2].status == HCSR04_OK ? "OK" : "ERR",
             hcsr04_sensors[3].distance_cm,
             hcsr04_sensors[3].status == HCSR04_OK ? "OK" : "ERR");

    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HCSR04_Init(&htim2);
  Set_Binary_LEDs(0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */





	  HCSR04_Read_All(hcsr04_sensors, SENSOR_COUNT);

	  uint8_t front_back_obstacle = 0;
	  uint8_t left_right_obstacle = 0;
	  uint8_t sensor_error = 0;

	  /*
	   * Sensor index mapping:
	   * 0 = Front
	   * 1 = Left
	   * 2 = Right
	   * 3 = Back
	   */

	  for (uint8_t i = 0; i < SENSOR_COUNT; i++)
	  {
	      if (hcsr04_sensors[i].status != HCSR04_OK)
	      {
	          sensor_error = 1;
	      }
	  }

	  if (hcsr04_sensors[0].status == HCSR04_OK &&
	      hcsr04_sensors[0].distance_cm <= OBSTACLE_THRESHOLD_CM)
	  {
	      front_back_obstacle = 1;
	  }

	  if (hcsr04_sensors[3].status == HCSR04_OK &&
	      hcsr04_sensors[3].distance_cm <= OBSTACLE_THRESHOLD_CM)
	  {
	      front_back_obstacle = 1;
	  }

	  if (hcsr04_sensors[1].status == HCSR04_OK &&
	      hcsr04_sensors[1].distance_cm <= OBSTACLE_THRESHOLD_CM)
	  {
	      left_right_obstacle = 1;
	  }

	  if (hcsr04_sensors[2].status == HCSR04_OK &&
	      hcsr04_sensors[2].distance_cm <= OBSTACLE_THRESHOLD_CM)
	  {
	      left_right_obstacle = 1;
	  }

	  /*
	   * Binary LED status:
	   *
	   * PA6 PA5
	   *  0   0  = 00 = no obstacle
	   *  0   1  = 01 = front/back obstacle
	   *  1   0  = 10 = left/right obstacle
	   *  1   1  = 11 = sensor error or obstacle on both groups
	   */

	  if (sensor_error)
	  {
	      Set_Binary_LEDs(3);   // 11
	  }
	  else if (front_back_obstacle && left_right_obstacle)
	  {
	      Set_Binary_LEDs(3);   // 11
	  }
	  else if (left_right_obstacle)
	  {
	      Set_Binary_LEDs(2);   // 10
	  }
	  else if (front_back_obstacle)
	  {
	      Set_Binary_LEDs(1);   // 01
	  }
	  else
	  {
	      Set_Binary_LEDs(0);   // 00
	  }

	  Print_Sensor_Readings();

	  HAL_Delay(300);






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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

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
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
