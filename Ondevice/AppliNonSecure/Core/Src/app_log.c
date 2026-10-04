#include "app_log.h"
#include "usart.h"
#include <stdio.h>

/* Project-owned polling logger; independent of CubeMX-generated USART code. */
volatile uint32_t app_log_tx_errors = 0U;
volatile uint32_t app_log_last_status = HAL_OK;
static uint32_t last_log_tick;

static void WriteLog(const char *text, uint16_t length)
{
  HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (const uint8_t *)text,
                                             length, 100U);
  app_log_last_status = (uint32_t)status;
  if (status != HAL_OK)
  {
    app_log_tx_errors++;
  }
}

void AppLog_Start(void)
{
  static const char message[] = "\r\n[NS] boot OK; USART1 PE5/PE6; 115200 8N1\r\n";
  last_log_tick = HAL_GetTick();
  WriteLog(message, (uint16_t)(sizeof(message) - 1U));
}

void AppLog_Poll(uint32_t tick, uint32_t loop_count)
{
  if ((uint32_t)(tick - last_log_tick) < 1000U)
  {
    return;
  }
  last_log_tick = tick;
  char message[96];
  int length = snprintf(message, sizeof(message),
                        "[NS] tick=%lu ms loop=%lu tx_errors=%lu\r\n",
                        (unsigned long)tick, (unsigned long)loop_count,
                        (unsigned long)app_log_tx_errors);
  if (length > 0 && (size_t)length < sizeof(message))
  {
    WriteLog(message, (uint16_t)length);
  }
}
