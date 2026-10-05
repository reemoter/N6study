#include "main.h"
#include "ext_ram_secure.h"
#include "aps256xx.h"

/* STM32CubeN6 v1.4.1 BSP timing, with XSPI2/XSPIM reset deliberately avoided. */
static XSPI_HandleTypeDef ram;
volatile uint32_t psram_init_status;
volatile uint32_t psram_id;
volatile uint32_t psram_clock_hz;

CMSE_NS_ENTRY uint32_t SECURE_ExtRamStatus(void) { return psram_init_status; }
CMSE_NS_ENTRY uint32_t SECURE_ExtRamId(void) { return psram_id; }

void ExtRam_SecureInit(void)
{
  psram_init_status = 10U;
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  HAL_PWREx_ConfigVddIORange(PWR_VDDIO2, PWR_VDDIO_RANGE_1V8);
  RCC_PeriphCLKInitTypeDef clocks = {0};
  clocks.PeriphClockSelection = RCC_PERIPHCLK_XSPI1;
  clocks.Xspi1ClockSelection = RCC_XSPI1CLKSOURCE_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&clocks) != HAL_OK) { return; }
  psram_clock_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_XSPI1);
  if (psram_clock_hz < 10000000U || psram_clock_hz > 200000000U) { return; }
  __HAL_RCC_XSPI1_CLK_ENABLE();
  __HAL_RCC_XSPI1_FORCE_RESET();
  __HAL_RCC_XSPI1_RELEASE_RESET();
  __HAL_RCC_XSPIM_CLK_ENABLE();
  __HAL_RCC_GPIOP_CLK_ENABLE();
  __HAL_RCC_GPIOO_CLK_ENABLE();
  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_XSPI1,
                                       RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
  GPIO_InitTypeDef pins = {0};
  pins.Mode = GPIO_MODE_AF_PP;
  pins.Pull = GPIO_PULLUP;
  pins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  pins.Alternate = GPIO_AF9_XSPIM_P1;
  pins.Pin = GPIO_PIN_ALL;
  HAL_GPIO_ConfigPinAttributes(GPIOP, pins.Pin, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_Init(GPIOP, &pins);
  pins.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
  HAL_GPIO_ConfigPinAttributes(GPIOO, pins.Pin, GPIO_PIN_SEC | GPIO_PIN_NPRIV);
  HAL_GPIO_Init(GPIOO, &pins);

  ram.Instance = XSPI1;
  ram.Init.FifoThresholdByte = 8U;
  ram.Init.MemoryType = HAL_XSPI_MEMTYPE_APMEM_16BITS;
  ram.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  ram.Init.MemorySize = HAL_XSPI_SIZE_256MB;
  ram.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  ram.Init.ChipSelectHighTimeCycle = 5U;
  ram.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  ram.Init.ClockPrescaler = 3U;
  ram.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  ram.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  ram.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_16KB;
  ram.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  ram.Init.Refresh = (2U * psram_clock_hz / 1000000U) - 4U;
  ram.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  psram_init_status = 11U;
  if (HAL_XSPI_Init(&ram) != HAL_OK) { return; }
  XSPIM_CfgTypeDef manager = {0};
  manager.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  manager.IOPort = HAL_XSPIM_IOPORT_1;
  manager.Req2AckTime = 1U;
  if (HAL_XSPIM_Config(&ram, &manager, 100U) != HAL_OK) { return; }
  psram_init_status = 12U;
  if (APS256XX_WriteReg(&ram, 0U, 0x30U) != APS256XX_OK ||
      APS256XX_WriteReg(&ram, 4U, 0x20U) != APS256XX_OK) { return; }
  uint8_t id[2] = {0};
  if (APS256XX_ReadID(&ram, id, 7U) != APS256XX_OK) { return; }
  psram_id = (uint32_t)id[0] | ((uint32_t)id[1] << 8);
  if (psram_id == 0U || psram_id == 0xFFFFU) { return; }
  psram_init_status = 13U;
  if (APS256XX_WriteReg(&ram, 8U, 0x40U) != APS256XX_OK ||
      HAL_XSPI_SetClockPrescaler(&ram, 0U) != HAL_OK) { return; }
  if (APS256XX_EnableMemoryMappedMode(&ram, 7U, 7U, 1U, 0U) != APS256XX_OK) { return; }

  /* Only the mapped 32MiB RAM aperture is exposed to NS.
   * Controller, pins and shared XSPIM remain Secure-owned. */
  RISAF_BaseRegionConfig_t region = {0};
  region.StartAddress = 0U;
  region.EndAddress = 0x01FFFFFFU;
  region.Filtering = RISAF_FILTER_ENABLE;
  region.Secure = RIF_ATTRIBUTE_NSEC;
  region.ReadWhitelist = 0xFFU;
  region.WriteWhitelist = 0xFFU;
  region.PrivWhitelist = RIF_CID_NONE;
  HAL_RIF_RISAF_ConfigBaseRegion(RISAF11, RISAF_REGION_1, &region);
  SAU->RNR = 3U;
  SAU->RBAR = 0x90000000U;
  SAU->RLAR = (0x91FFFFFFU & SAU_RLAR_LADDR_Msk) | SAU_RLAR_ENABLE_Msk;
  __DSB();
  __ISB();
  psram_init_status = 1U;
}
