#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdint.h>

void AppLog_Start(void);
void AppLog_Poll(uint32_t tick, uint32_t loop_count);

extern volatile uint32_t app_log_tx_errors;
extern volatile uint32_t app_log_last_status;

#endif
