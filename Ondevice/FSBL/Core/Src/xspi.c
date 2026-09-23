#include "xspi.h"

XSPI_HandleTypeDef hxspi2;

void MX_XSPI2_Init(void)
{
  XSPIM_CfgTypeDef manager = {0};

  hxspi2.Instance = XSPI2;
  hxspi2.Init.FifoThresholdByte = 4;
  hxspi2.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi2.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi2.Init.MemorySize = HAL_XSPI_SIZE_1GB;
  hxspi2.Init.ChipSelectHighTimeCycle = 2;
  hxspi2.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi2.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi2.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  /* IC3 is configured to 50 MHz, as in ST's STM32N6570-DK FSBL template. */
  hxspi2.Init.ClockPrescaler = 0;
  hxspi2.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi2.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  hxspi2.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi2.Init.MaxTran = 0;
  hxspi2.Init.Refresh = 0;
  hxspi2.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi2) != HAL_OK)
  {
    Error_Handler();
  }

  manager.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  manager.IOPort = HAL_XSPIM_IOPORT_2;
  manager.Req2AckTime = 1;
  if (HAL_XSPIM_Config(&hxspi2, &manager, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_XSPI_MspInit(XSPI_HandleTypeDef *handle)
{
  GPIO_InitTypeDef gpio = {0};
  RCC_PeriphCLKInitTypeDef clock = {0};

  if (handle->Instance != XSPI2)
  {
    return;
  }

  clock.PeriphClockSelection = RCC_PERIPHCLK_XSPI2;
  /* PLL1 output is 1600 MHz in this project: 1600 / 32 = 50 MHz. */
  clock.ICSelection[RCC_IC3].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  clock.ICSelection[RCC_IC3].ClockDivider = 32;
  clock.Xspi2ClockSelection = RCC_XSPI2CLKSOURCE_IC3;
  if (HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_RCC_XSPIM_CLK_ENABLE();
  __HAL_RCC_XSPI2_CLK_ENABLE();
  __HAL_RCC_GPION_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
             GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8 |
             GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF9_XSPIM_P2;
  HAL_GPIO_Init(GPION, &gpio);
}
