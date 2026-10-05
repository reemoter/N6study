#include "main.h"
#include "ext_ram_secure.h"
#include "aps256xx.h"
#include "xspi.h"

/* CubeMX owns XSPI1 initialization; this driver owns PSRAM commands and mapping. */
volatile uint32_t psram_init_status;
volatile uint32_t psram_id;
volatile uint32_t psram_clock_hz;

CMSE_NS_ENTRY uint32_t SECURE_ExtRamStatus(void) { return psram_init_status; }
CMSE_NS_ENTRY uint32_t SECURE_ExtRamId(void) { return psram_id; }

void ExtRam_SecureInit(void)
{
  psram_init_status = 10U;
  /* MX_XSPI1_Init has already run once in generated Secure main. */
  psram_clock_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_XSPI1);
  if (psram_clock_hz != 100000000U ||
      hxspi1.Instance != XSPI1 || HAL_XSPI_GetState(&hxspi1) != HAL_XSPI_STATE_READY ||
      hxspi1.Init.MemoryType != HAL_XSPI_MEMTYPE_APMEM_16BITS ||
      hxspi1.Init.MemorySize != HAL_XSPI_SIZE_256MB ||
      hxspi1.Init.ClockPrescaler != 3U || hxspi1.Init.Refresh != 196U)
  {
    return;
  }
  psram_init_status = 12U;
  if (APS256XX_WriteReg(&hxspi1, 0U, 0x30U) != APS256XX_OK ||
      APS256XX_WriteReg(&hxspi1, 4U, 0x20U) != APS256XX_OK) { return; }
  uint8_t id[2] = {0};
  if (APS256XX_ReadID(&hxspi1, id, 7U) != APS256XX_OK) { return; }
  psram_id = (uint32_t)id[0] | ((uint32_t)id[1] << 8);
  if (psram_id == 0U || psram_id == 0xFFFFU) { return; }
  psram_init_status = 13U;
  if (APS256XX_WriteReg(&hxspi1, 8U, 0x40U) != APS256XX_OK ||
      HAL_XSPI_SetClockPrescaler(&hxspi1, 0U) != HAL_OK) { return; }
  if (APS256XX_EnableMemoryMappedMode(&hxspi1, 7U, 7U, 1U, 0U) != APS256XX_OK) { return; }

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

