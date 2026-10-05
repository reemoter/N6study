#include "app_log.h"
#include "usart.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* Project-owned logger: only LogTask owns USART1 transmission. */
#define LOG_QUEUE_DEPTH 16U
#define LOG_TEXT_SIZE 96U

typedef struct {
  uint32_t is_status;
  union {
    char text[LOG_TEXT_SIZE];
    struct {
      uint32_t tick, os_tick, runs, heap, stack_words;
    } status;
  } payload;
} LogRecord;

volatile uint32_t app_log_tx_errors;
volatile uint32_t app_log_last_status = HAL_OK;
volatile uint32_t app_log_dropped;
volatile uint32_t app_log_queue_peak;
volatile uint32_t app_log_stack_free;
static osMessageQueueId_t log_queue;
static osThreadId_t log_thread;
static uint32_t last_log_tick;

static int Enqueue(const LogRecord *record)
{
  if (log_queue == NULL || osMessageQueuePut(log_queue, record, 0U, 0U) != osOK)
  {
    taskENTER_CRITICAL();
    app_log_dropped++;
    taskEXIT_CRITICAL();
    return 0;
  }
  uint32_t count = osMessageQueueGetCount(log_queue);
  taskENTER_CRITICAL();
  if (count > app_log_queue_peak) { app_log_queue_peak = count; }
  taskEXIT_CRITICAL();
  return 1;
}

static void LogTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    LogRecord record;
    if (osMessageQueueGet(log_queue, &record, NULL, osWaitForever) != osOK)
      continue;
    char message[224];
    int length;
    if (record.is_status)
    {
      length = snprintf(message, sizeof(message),
          "[NS] tick=%lu os_tick=%lu task_runs=%lu heap=%lu stack_words=%lu log_stack_words=%lu tx_errors=%lu dropped=%lu queue_peak=%lu\r\n",
          (unsigned long)record.payload.status.tick,
          (unsigned long)record.payload.status.os_tick,
          (unsigned long)record.payload.status.runs,
          (unsigned long)record.payload.status.heap,
          (unsigned long)record.payload.status.stack_words,
          (unsigned long)app_log_stack_free,
          (unsigned long)app_log_tx_errors,
          (unsigned long)app_log_dropped,
          (unsigned long)app_log_queue_peak);
    }
    else
    {
      length = snprintf(message, sizeof(message), "%s", record.payload.text);
    }
    if (length > 0 && (size_t)length < sizeof(message))
    {
      HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1,
          (const uint8_t *)message, (uint16_t)length, 100U);
      app_log_last_status = (uint32_t)status;
      if (status != HAL_OK) { app_log_tx_errors++; }
    }
    app_log_stack_free = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
  }
}

int AppLog_Init(void)
{
  static const osThreadAttr_t attributes = {
    .name = "uartLog",
    .priority = osPriorityBelowNormal,
    .stack_size = 2048U
  };
  if (log_queue != NULL) { return log_thread != NULL; }
  log_queue = osMessageQueueNew(LOG_QUEUE_DEPTH, sizeof(LogRecord), NULL);
  if (log_queue == NULL) { return 0; }
  log_thread = osThreadNew(LogTask, NULL, &attributes);
  if (log_thread == NULL)
  {
    osMessageQueueDelete(log_queue);
    log_queue = NULL;
    return 0;
  }
  return 1;
}

int AppLog_Write(const char *text)
{
  if (text == NULL) { return 0; }
  LogRecord record = {0};
  size_t length = 0U;
  while (length < LOG_TEXT_SIZE - 1U && text[length] != '\0') { length++; }
  memcpy(record.payload.text, text, length);
  return Enqueue(&record);
}

void AppLog_Start(void)
{
  last_log_tick = HAL_GetTick();
  (void)AppLog_Write("\r\n[NS] FreeRTOS queued logger; USART1 PE5/PE6; 115200 8N1\r\n");
}

void AppLog_Poll(uint32_t tick, uint32_t loop_count)
{
  if ((uint32_t)(tick - last_log_tick) < 1000U) { return; }
  last_log_tick = tick;
  LogRecord record = { .is_status = 1U };
  record.payload.status.tick = tick;
  record.payload.status.os_tick = osKernelGetTickCount();
  record.payload.status.runs = loop_count;
  record.payload.status.heap = (uint32_t)xPortGetFreeHeapSize();
  record.payload.status.stack_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
  (void)Enqueue(&record);
}
