#include "main.h"
#include "app_buffers.h"
#include "app_log.h"
#include <stdio.h>

#define RAM_BUFFER(name, bytes) \
  static uint8_t name[bytes] __attribute__((section(".external_ram." #name), aligned(APP_BUFFER_ALIGNMENT), used))
RAM_BUFFER(camera0, APP_CAMERA_FRAME_BYTES);
RAM_BUFFER(camera1, APP_CAMERA_FRAME_BYTES);
RAM_BUFFER(input, APP_INFERENCE_INPUT_BYTES);
RAM_BUFFER(output, APP_INFERENCE_OUTPUT_BYTES);
RAM_BUFFER(work, APP_INFERENCE_WORK_BYTES);
static const AppBuffer buffers[APP_BUFFER_COUNT] = {
  {camera0, sizeof(camera0)}, {camera1, sizeof(camera1)}, {input, sizeof(input)},
  {output, sizeof(output)}, {work, sizeof(work)}
};
static const char *const names[APP_BUFFER_COUNT] = {"camera0","camera1","input","output","work"};
extern volatile uint32_t ram_test_status;
volatile uint32_t app_buffers_ready;

const AppBuffer *AppBuffers_Get(AppBufferId id)
{
  if (!app_buffers_ready || (unsigned)id >= APP_BUFFER_COUNT) { return NULL; }
  return &buffers[id];
}

int AppBuffers_Init(void)
{
  if (app_buffers_ready) { return 1; }
  if (SECURE_ExtRamStatus() != 1U ||
      (ram_test_status != 2U && ram_test_status != 5U) ||
      (SCB->CCR & SCB_CCR_DC_Msk) != 0U)
  {
    (void)AppLog_Write("[BUF] unavailable: RAM init/test or cache policy\r\n");
    return 0;
  }
  /* Validate all ranges before accessing external memory. */
  for (unsigned i = 0U; i < APP_BUFFER_COUNT; i++)
  {
    uintptr_t start = (uintptr_t)buffers[i].data;
    size_t size = buffers[i].size;
    if (size < APP_BUFFER_ALIGNMENT || start < 0x90000000U ||
        start >= 0x92000000U || size > 0x92000000U - start ||
        (start % APP_BUFFER_ALIGNMENT) != 0U || (size % APP_BUFFER_ALIGNMENT) != 0U)
    { (void)AppLog_Write("[BUF] FAIL: range/alignment\r\n"); return 0; }
    for (unsigned j = 0U; j < i; j++)
    {
      uintptr_t other = (uintptr_t)buffers[j].data;
      if (start < other + buffers[j].size && other < start + size)
      { (void)AppLog_Write("[BUF] FAIL: overlap\r\n"); return 0; }
    }
  }
  uint32_t total = 0U;
  for (unsigned i = 0U; i < APP_BUFFER_COUNT; i++)
  {
    volatile uint32_t *first = (volatile uint32_t *)buffers[i].data;
    volatile uint32_t *last = (volatile uint32_t *)((uintptr_t)first + buffers[i].size - 4U);
    *first = 0x12345678U ^ i;
    *last = 0x87654321U ^ i;
    __DSB();
    if (*first != (0x12345678U ^ i) || *last != (0x87654321U ^ i))
    { (void)AppLog_Write("[BUF] FAIL: CPU boundary probe\r\n"); return 0; }
    *first = 0U; *last = 0U;
    __DSB();
    char message[96];
    snprintf(message, sizeof(message), "[BUF] %s addr=0x%08lx bytes=%lu align=64\r\n",
             names[i], (unsigned long)(uintptr_t)first, (unsigned long)buffers[i].size);
    (void)AppLog_Write(message);
    total += (uint32_t)buffers[i].size;
  }
  app_buffers_ready = 1U;
  char message[96];
  snprintf(message, sizeof(message), "[BUF] READY CPU probes passed; reserved=%lu free=%lu\r\n",
           (unsigned long)total, (unsigned long)(0x02000000U - total));
  (void)AppLog_Write(message);
  return 1;
}
