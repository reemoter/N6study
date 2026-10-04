#include "main.h"
#include "secure_boot.h"
#include "partition_stm32n657xx.h"

/* Fail compilation if a regenerated partition no longer supports this layout. */
_Static_assert(SAU_INIT_CTRL == 1 && SAU_INIT_CTRL_ENABLE == 1 && SAU_INIT_CTRL_ALLNS == 0,
               "SAU must be enabled for the SRAM boot layout");
_Static_assert(SAU_INIT_REGION0 == 1 && SAU_INIT_NSC0 == 1,
               "Secure Gateway veneers must be non-secure callable");
_Static_assert(SAU_INIT_REGION1 == 1 && SAU_INIT_START1 == 0x24100000U &&
               SAU_INIT_END1 == 0x241FFFFFU && SAU_INIT_NSC1 == 0,
               "SRAM2 must remain non-secure");
_Static_assert(SAU_INIT_REGION2 == 1 && SAU_INIT_START2 == 0x40000000U &&
               SAU_INIT_END2 == 0x4FFFFFFFU && SAU_INIT_NSC2 == 0,
               "Peripheral aliases must remain non-secure");

__attribute__((noreturn)) void Secure_BootEnterNonSecure(void)
{
  const uint32_t vector_address = SRAM2_AXI_BASE_NS + 0x400U;
  const uint32_t *vectors = (const uint32_t *)vector_address;

  HAL_SuspendTick();
  secure_boot_stage = 3U;
  SCB_NS->VTOR = vector_address;
  secure_boot_stage = 4U;
  __TZ_set_MSP_NS(vectors[0]);
  secure_boot_stage = 5U;
  funcptr_NS entry = (funcptr_NS)vectors[1];
  __DSB();
  __ISB();
  secure_boot_stage = 6U;
  entry();

  /* NonSecure Reset_Handler must not return to the Secure boot code. */
  Error_Handler();
  for (;;) {}
}
