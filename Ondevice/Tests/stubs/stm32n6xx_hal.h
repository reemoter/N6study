#ifndef FSBL_TEST_HAL_H
#define FSBL_TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef struct { uint32_t CCR; uint32_t VTOR; } TestSCB;
extern TestSCB test_scb;
extern unsigned int test_cache_disabled;
#define SCB (&test_scb)
#define SCB_CCR_DC_Msk (1U << 16)
#define SCB_CCR_IC_Msk (1U << 17)
static inline void SCB_DisableDCache(void) { SCB->CCR &= ~SCB_CCR_DC_Msk; ++test_cache_disabled; }
static inline void SCB_DisableICache(void) { SCB->CCR &= ~SCB_CCR_IC_Msk; }
static inline void HAL_SuspendTick(void) {}
static inline void __DSB(void) {}
static inline void __ISB(void) {}
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) {}
static inline void __set_MSPLIM(uint32_t value) { (void)value; }
static inline void __set_MSP(uint32_t value) { (void)value; }
static inline void __set_PRIMASK(uint32_t value) { (void)value; }
#endif
