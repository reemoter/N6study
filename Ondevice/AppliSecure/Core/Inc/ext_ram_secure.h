#ifndef EXT_RAM_SECURE_H
#define EXT_RAM_SECURE_H
#include <stdint.h>
void ExtRam_SecureInit(void);
extern volatile uint32_t psram_init_status;
extern volatile uint32_t psram_id;
extern volatile uint32_t psram_clock_hz;
#endif
