#include "fsbl_app.h"
#include "nor_flash.h"

volatile FSBL_AppStatus fsbl_app_status = FSBL_APP_NOT_STARTED;
volatile uint32_t fsbl_secure_reset_vector = 0U;
volatile uint32_t fsbl_nonsecure_reset_vector = 0U;

static uint32_t read_u32_le(const uint8_t *bytes)
{
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint32_t valid_image(uint32_t flash_offset, uint32_t secure)
{
  uint8_t magic[4];
  uint8_t vectors[8];
  uint32_t stack;
  uint32_t reset;

  if ((NOR_Read(flash_offset, magic, sizeof(magic)) != HAL_OK) ||
      (magic[0] != 'S') || (magic[1] != 'T') ||
      (magic[2] != 'M') || (magic[3] != '2'))
  {
    return 1U;
  }
  if (NOR_Read(flash_offset + FSBL_IMAGE_HEADER_SIZE,
               vectors, sizeof(vectors)) != HAL_OK)
  {
    return 2U;
  }

  stack = read_u32_le(vectors);
  reset = read_u32_le(vectors + 4U);
  if (secure != 0U)
  {
    fsbl_secure_reset_vector = reset;
    if ((stack < 0x34064000U) || (stack > 0x34100000U) ||
        (reset < (0x34000000U + FSBL_IMAGE_HEADER_SIZE + 1U)) ||
        (reset >= 0x34064000U) || ((reset & 1U) == 0U))
    {
      return 2U;
    }
  }
  else
  {
    fsbl_nonsecure_reset_vector = reset;
    if ((stack < 0x24180000U) || (stack > 0x24200000U) ||
        (reset < (0x24100000U + FSBL_IMAGE_HEADER_SIZE + 1U)) ||
        (reset >= 0x24180000U) || ((reset & 1U) == 0U))
    {
      return 2U;
    }
  }
  return 0U;
}

static HAL_StatusTypeDef copy_image(uint32_t flash_offset, uint32_t ram_address)
{
  /* The initial LRUN test uses the same fixed 64-KiB copy size as ST's
     reference template. Increase it only after checking both signed images. */
  for (uint32_t offset = 0U; offset < FSBL_IMAGE_COPY_SIZE; offset += 256U)
  {
    if (NOR_Read(flash_offset + offset, (uint8_t *)(ram_address + offset), 256U) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }
  return HAL_OK;
}

FSBL_AppStatus FSBL_LoadApplications(void)
{
  uint32_t check;

  fsbl_app_status = FSBL_APP_NOT_STARTED;
  check = valid_image(FSBL_SECURE_FLASH_OFFSET, 1U);
  if (check != 0U)
  {
    fsbl_app_status = (check == 1U) ? FSBL_APP_SECURE_HEADER_ERROR :
                                     FSBL_APP_SECURE_VECTOR_ERROR;
    return fsbl_app_status;
  }
  check = valid_image(FSBL_NONSECURE_FLASH_OFFSET, 0U);
  if (check != 0U)
  {
    fsbl_app_status = (check == 1U) ? FSBL_APP_NONSECURE_HEADER_ERROR :
                                     FSBL_APP_NONSECURE_VECTOR_ERROR;
    return fsbl_app_status;
  }

  if (copy_image(FSBL_SECURE_FLASH_OFFSET, FSBL_SECURE_LOAD_ADDRESS) != HAL_OK)
  {
    fsbl_app_status = FSBL_APP_SECURE_COPY_ERROR;
    return fsbl_app_status;
  }
  if (copy_image(FSBL_NONSECURE_FLASH_OFFSET, FSBL_NONSECURE_LOAD_ADDRESS) != HAL_OK)
  {
    fsbl_app_status = FSBL_APP_NONSECURE_COPY_ERROR;
    return fsbl_app_status;
  }
  fsbl_app_status = FSBL_APP_READY;
  return fsbl_app_status;
}

void FSBL_JumpToSecure(void)
{
  uint32_t vector_table = FSBL_SECURE_LOAD_ADDRESS + FSBL_IMAGE_HEADER_SIZE;
  uint32_t stack = *(volatile uint32_t *)vector_table;
  uint32_t reset = *(volatile uint32_t *)(vector_table + 4U);
  uint32_t primask;
  void (*secure_reset)(void) = (void (*)(void))reset;

  if (fsbl_app_status != FSBL_APP_READY)
  {
    return;
  }

  /* The newly copied instructions and vectors must be visible to the CPU. */
  HAL_SuspendTick();
  if ((SCB->CCR & SCB_CCR_IC_Msk) != 0U)
  {
    SCB_DisableICache();
  }
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
  {
    SCB_DisableDCache();
  }

  primask = __get_PRIMASK();
  __disable_irq();
  SCB->VTOR = vector_table;
  __set_MSPLIM(0U);
  __set_MSP(stack);
  __DSB();
  __ISB();
  __set_PRIMASK(primask);
  secure_reset();

  /* A reset handler is not expected to return. */
  for (;;)
  {
  }
}
