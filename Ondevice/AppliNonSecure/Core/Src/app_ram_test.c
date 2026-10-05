#include "main.h"
#include "app_ram_test.h"
#include "app_config.h"
#include "app_log.h"
#include "cmsis_os2.h"
#include <stdio.h>

volatile uint32_t ram_test_status;
volatile uint32_t ram_test_fail_address;
volatile uint32_t ram_test_expected;
volatile uint32_t ram_test_actual;
volatile uint32_t ram_test_bytes;

/* Destructive startup test: no frame buffers/heap may use PSRAM yet. */
void AppRamTest_Run(void)
{
  char message[96];
  uint32_t status = SECURE_ExtRamStatus();
  uint32_t id = SECURE_ExtRamId();
  snprintf(message, sizeof(message), "[RAM] init=%lu id=0x%04lx base=0x90000000 size=32MiB\r\n",
           (unsigned long)status, (unsigned long)id);
  (void)AppLog_Write(message);
  if (status != 1U) { ram_test_status = 3U; return; }
#if APP_RAM_SELF_TEST == 0
  ram_test_status = 5U; /* Initialized; destructive self-test deliberately skipped. */
  (void)AppLog_Write("[RAM] ready; full self-test disabled\r\n");
#else
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
  {
    ram_test_status = 4U;
    (void)AppLog_Write("[RAM] SKIP: disable NS DCache before physical RAM validation\r\n");
    return;
  }
  ram_test_status = 1U;
  volatile uint32_t *memory = (volatile uint32_t *)0x90000000U;
  const uint32_t words = 0x02000000U / sizeof(uint32_t);
  uint32_t start = HAL_GetTick();
  for (uint32_t pass = 0U; pass < 2U; pass++)
  {
    uint32_t mask = pass == 0U ? 0xA5A55A5AU : 0x5A5AA5A5U;
    for (uint32_t i = 0U; i < words; i++)
    {
      memory[i] = (i * 4U) ^ mask;
      if ((i & 8191U) == 8191U) { __DSB(); osDelay(1U); }
    }
    __DSB();
    for (uint32_t i = 0U; i < words; i++)
    {
      uint32_t expected = (i * 4U) ^ mask;
      uint32_t actual = memory[i];
      if (actual != expected)
      {
        ram_test_fail_address = 0x90000000U + i * 4U;
        ram_test_expected = expected;
        ram_test_actual = actual;
        ram_test_status = 3U;
        snprintf(message, sizeof(message), "[RAM] FAIL addr=0x%08lx expected=0x%08lx actual=0x%08lx\r\n",
                 (unsigned long)ram_test_fail_address, (unsigned long)expected,
                 (unsigned long)actual);
        (void)AppLog_Write(message);
        return;
      }
      if ((i & 8191U) == 8191U) { osDelay(1U); }
    }
    snprintf(message, sizeof(message), "[RAM] pass=%lu full 32MiB write/read verified\r\n",
             (unsigned long)(pass + 1U));
    (void)AppLog_Write(message);
  }
  ram_test_bytes = 0x02000000U;
  ram_test_status = 2U;
  snprintf(message, sizeof(message), "[RAM] PASS NS access; 32MiB; two patterns; elapsed_ms=%lu\r\n",
           (unsigned long)(HAL_GetTick() - start));
  (void)AppLog_Write(message);
#endif
}
