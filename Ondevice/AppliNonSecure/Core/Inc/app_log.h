#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdint.h>

/* Call Init after osKernelInitialize and before osKernelStart.
 * Write/Start/Poll are task-context APIs: messages are copied, never awaited.
 * A full queue drops the new message and increments dropped. */
int AppLog_Init(void);
int AppLog_Write(const char *text);
void AppLog_Start(void);
void AppLog_Poll(uint32_t tick, uint32_t loop_count);

extern volatile uint32_t app_log_tx_errors;
extern volatile uint32_t app_log_last_status;
extern volatile uint32_t app_log_dropped;
extern volatile uint32_t app_log_queue_peak;
extern volatile uint32_t app_log_stack_free;

#endif
