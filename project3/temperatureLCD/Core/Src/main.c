/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Minimal DHT11 -> OLED (Celsius only, centered + target)
  ******************************************************************************
  * @attention
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <math.h>      // <-- added for proper rounding
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_9

uint8_t RHI, RHD, TCI, TCD, SUM;
uint32_t pMillis, cMillis;

float tCelsius = 0.0f;
volatile int8_t temp_offset_c = 0;      // adjust via buttons
char strCopy[20];

// Tracks the latest measured real Celsius
static float g_lastRealC = 0.0f;

// Integer target setpoint and a flag indicating we've initialized it
static int16_t g_target_set_c = 0;
static bool    g_target_initialized = false;
/* USER CODE END PV */

/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);

/* USER CODE BEGIN PFP */
static void microDelay(uint16_t us);
static uint8_t DHT11_Start(void);
static uint8_t DHT11_Read(void);
static void HandleButtons(void);
static void CalculateCelsius(void);
static void ShowDisplay(float realC, float targetC);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static int16_t first_setpoint_from(float c)
{
  float frac = c - floorf(c);
  int16_t base = (int16_t)floorf(c);
  if (frac > 0.5f) base += 1;   // strictly greater than .5 => round up
  return base;                  // returns an integer °C
}


static void microDelay (uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}

static uint8_t DHT11_Start (void)
{
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Drive line
  GPIO_InitStruct.Pin   = DHT11_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
  microDelay(30);

  // Release line (input with pull-up)
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

  microDelay(40);
  if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
  {
    microDelay(80);
    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) Response = 1;
  }

  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (pMillis + 2 > cMillis))
    cMillis = HAL_GetTick();

  return Response;
}

static uint8_t DHT11_Read (void)
{
  uint8_t a, b = 0;  // IMPORTANT: init to 0
  for (a = 0; a < 8; a++)
  {
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (pMillis + 2 > cMillis))
      cMillis = HAL_GetTick();

    microDelay(40); // sample point ~40us

    if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
      b &= ~(1 << (7 - a));
    else
      b |=  (1 << (7 - a));

    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (pMillis + 2 > cMillis))
      cMillis = HAL_GetTick();
  }
  return b;
}
static void HandleButtons(void)
{
  static GPIO_PinState prev3 = GPIO_PIN_SET; // -
  static GPIO_PinState prev4 = GPIO_PIN_SET; // +
  static GPIO_PinState prev5 = GPIO_PIN_SET; // reset

  GPIO_PinState cur3 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3);
  GPIO_PinState cur4 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
  GPIO_PinState cur5 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);

  // --- Reset (PA5): forget initialization, next +/- will re-init
  if (prev5 == GPIO_PIN_SET && cur5 == GPIO_PIN_RESET) {
	  /*
    g_target_initialized = false;
    // keep the setpoint value or zero it; not required,
    // but you can uncomment the next line if you prefer:
    // g_target_set_c = 0;
    HAL_Delay(80);
  	  */
	    g_target_set_c      = first_setpoint_from(g_lastRealC); // 20.7 -> 21, 20.3 -> 20
	    g_target_initialized = true;                             // forces ".0" display
	    HAL_Delay(240);                                          // debounce

	    // Optional: immediate redraw so you don't wait for the 1 s DHT refresh:
	    ShowDisplay(g_lastRealC, (float)g_target_set_c);

  }

  // --- Decrement (PA3)
  if (prev3 == GPIO_PIN_SET && cur3 == GPIO_PIN_RESET) {
    if (!g_target_initialized) {
      g_target_set_c = first_setpoint_from(g_lastRealC);
      g_target_initialized = true;            // decimals flushed to .0 now
    } else {
      g_target_set_c -= 1;
    }
    HAL_Delay(120);
  }

  // --- Increment (PA4)
  if (prev4 == GPIO_PIN_SET && cur4 == GPIO_PIN_RESET) {
    if (!g_target_initialized) {
      g_target_set_c = first_setpoint_from(g_lastRealC);
      g_target_initialized = true;            // decimals flushed to .0 now
    } else {
      g_target_set_c += 1;
    }
    HAL_Delay(120);
  }

  prev3 = cur3;
  prev4 = cur4;
  prev5 = cur5;
}


/* -------- helper: format one-decimal with proper rounding ---------- */
static void format_temp_1dp(float c, int *ip, int *dp)
{
  // round to nearest 0.1 °C
  int n = (int)lroundf(c * 10.0f);   // e.g. 23.94 -> 239 -> 24.0
  *ip = n / 10;                      // integer part (handles negatives)
  int frac = n % 10;                 // signed remainder
  if (frac < 0) frac = -frac;
  *dp = frac;
}

static void CalculateCelsius(void)
{
  float realC = (float)TCI + (float)TCD / 10.0f;  // sensor value
  g_lastRealC = realC;

  // targetC shown: if initialized, use integer setpoint (.0)
  // otherwise show the real temp (no snap yet)
  float targetC = g_target_initialized ? (float)g_target_set_c : realC;

  // keep your existing tCelsius if you use it elsewhere
  tCelsius = targetC;

  ShowDisplay(realC, targetC);
}

/* Draw realC centered (big font) and targetC at bottom-left (small font) */


static void ShowDisplay(float realC, float targetC)
{
  #ifdef Font_6x8
    FontDef_t *Small = &Font_6x8;
  #else
    FontDef_t *Small = &Font_7x10;
  #endif
  FontDef_t *Big   = &Font_16x26;

  char realStr[20];
  char targetStr[24];

  int ri, rd, ti, td;
  format_temp_1dp(realC, &ri, &rd);

  // If initialized, we *always* show integer + ".0"
  if (g_target_initialized) {
    ti = (int)g_target_set_c;
    td = 0;
  } else {
    format_temp_1dp(targetC, &ti, &td);
  }

  snprintf(realStr,   sizeof(realStr),   "%d.%d C", ri, rd);
  snprintf(targetStr, sizeof(targetStr), "Target: %d.%d C", ti, td);

  uint16_t realW = (uint16_t)(strlen(realStr) * Big->FontWidth);
  uint16_t realH = Big->FontHeight;
  int16_t  rx = (int16_t)((128 - realW) / 2);
  int16_t  ry = (int16_t)((64  - realH) / 2);
  if (rx < 0) rx = 0;

  int16_t tx = 0;
  int16_t ty = (int16_t)(64 - Small->FontHeight - 2);
  if (ty < 0) ty = 0;

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(rx, ry);
  SSD1306_Puts(realStr, Big, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(tx, ty);
  SSD1306_Puts(targetStr, Small, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
}



/* USER CODE END 0 */


/* ============================== main ============================== */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();

  HAL_TIM_Base_Start(&htim1);
  SSD1306_Init();

  uint32_t lastDhtMs = 0;   // throttle DHT11 to ~1 Hz

  while (1)
  {
    HandleButtons();

    uint32_t now = HAL_GetTick();
    if (now - lastDhtMs >= 1000) {
      lastDhtMs = now;

      if (DHT11_Start())
      {
        RHI = DHT11_Read();
        RHD = DHT11_Read();
        TCI = DHT11_Read();
        TCD = DHT11_Read();
        SUM = DHT11_Read();

        if ((uint8_t)(RHI + RHD + TCI + TCD) == SUM)
        {
          CalculateCelsius();
        }
        // else: keep last good reading on screen
      }
    }

    HAL_Delay(20);
  }
}


/* --------- Boilerplate init code below (unchanged) --------- */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) { Error_Handler(); }
}

static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;                 // 72MHz/(71+1)=1MHz -> 1us tick
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK) { Error_Handler(); }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) { Error_Handler(); }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* DHT11 PB9 default level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);

  /* Buttons PA3, PA4, PA5: input with pull-up (pressed = LOW) */
  GPIO_InitStruct.Pin  = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* PB9 as output (reconfigured inside DHT11_Start as needed) */
  GPIO_InitStruct.Pin   = GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file; (void)line;
}
#endif
