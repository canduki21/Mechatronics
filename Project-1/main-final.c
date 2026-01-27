/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * Stopwatch – STM32F103C8 - 3461BS-1 COMMON CATHODE - FINAL WITH DECIMAL
  ******************************************************************************
  */
#include "main.h"
#include <stdbool.h>

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
volatile uint32_t stopwatch_ticks = 0;
volatile bool running = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN 0 */

// 7-segment font (abcdefg) - PA0-PA6
const uint8_t seg_map[10] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

void set_segments(uint8_t value, bool show_decimal)
{
  // For COMMON CATHODE: Invert! LOW = ON, HIGH = OFF
  for (int i = 0; i < 7; i++)
  {
    HAL_GPIO_WritePin(GPIOA, (1 << i),
      (value & (1 << i)) ? GPIO_PIN_RESET : GPIO_PIN_SET);
  }
  // Decimal point: LOW = ON for common cathode
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, show_decimal ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void disable_digits(void)
{
  // For COMMON CATHODE: LOW to disable
  HAL_GPIO_WritePin(GPIOB,
    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11,
    GPIO_PIN_RESET);
}

void enable_digit(uint8_t d)
{
  disable_digits();
  // For COMMON CATHODE: HIGH to enable
  // Standard mapping: PB0=D1, PB1=D2, PB10=D3, PB11=D4
  switch(d)
  {
    case 0: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;  // D1
    case 1: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); break;  // D2
    case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET); break; // D3
    case 3: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET); break; // D4
  }
}

void display_time(uint32_t ticks)
{
  uint16_t ms = ticks % 100;
  uint16_t sec = (ticks / 100) % 60;

  uint8_t digits[4] = {
    sec / 10,
    sec % 10,
    ms / 10,
    ms % 10
  };

  for (uint8_t i = 0; i < 4; i++)
  {
    // Show decimal point after digit 1 (between seconds and centiseconds)
    set_segments(seg_map[digits[i]], i == 1);
    enable_digit(i);
    HAL_Delay(2);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2 && running)
  {
    stopwatch_ticks++;
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_12)
  {
    running = !running;
  }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();

  HAL_TIM_Base_Start_IT(&htim2);

  while (1)
  {
    display_time(stopwatch_ticks);
  }
}

/* CLOCK CONFIG --------------------------------------------------------------*/
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                               RCC_CLOCKTYPE_SYSCLK |
                               RCC_CLOCKTYPE_PCLK1 |
                               RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* TIM2 INIT ----------------------------------------------------------------*/
static void MX_TIM2_Init(void)
{
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim2);

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/* GPIO INIT ----------------------------------------------------------------*/
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = 0x00FF;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
