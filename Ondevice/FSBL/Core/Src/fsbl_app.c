#include "fsbl_app.h"
#include "nor_flash.h"

volatile FSBL_AppStatus fsbl_app_status = FSBL_APP_NOT_STARTED;
volatile uint32_t fsbl_secure_reset_vector = 0U;
volatile uint32_t fsbl_nonsecure_reset_vector = 0U;
volatile uint32_t fsbl_secure_copy_size = 0U;
volatile uint32_t fsbl_nonsecure_copy_size = 0U;

static uint32_t valid_image(uint32_t flash_offset, uint32_t secure,
                            FSBL_ImageInfo *image)
{
  uint8_t header[FSBL_IMAGE_PREFIX_SIZE];
  uint8_t vectors[8];
  uint32_t capacity = secure ? FSBL_SECURE_IMAGE_CAPACITY :
                               FSBL_NONSECURE_IMAGE_CAPACITY;

  if ((NOR_Read(flash_offset, header, sizeof(header)) != HAL_OK) ||
      (FSBL_ParseImageHeader(header, capacity, image) == 0U))
  {
    return 1U;
  }
  if (NOR_Read(flash_offset + FSBL_IMAGE_HEADER_SIZE,
               vectors, sizeof(vectors)) != HAL_OK)
  {
    return 2U;
  }

  if (secure != 0U)
  {
    fsbl_secure_reset_vector = FSBL_ReadU32LE(vectors + 4U);
    if (FSBL_ValidateImageVectors(vectors, 0x34000400U,
                                 0x34064000U, 0x34100000U, image) == 0U)
    {
      return 2U;
    }
  }
  else
  {
    fsbl_nonsecure_reset_vector = FSBL_ReadU32LE(vectors + 4U);
    if (FSBL_ValidateImageVectors(vectors, 0x24100400U,
                                 0x24180000U, 0x24200000U, image) == 0U)
    {
      return 2U;
    }
  }
  return 0U;
}

/* 0: verified, 1: read/copy error, 2: payload checksum mismatch. */
static uint32_t copy_image(uint32_t flash_offset, uint32_t ram_address,
                           const FSBL_ImageInfo *image)
{
  uint8_t buffer[256];
  uint32_t checksum = 0U;
  for (uint32_t offset = 0U; offset < image->copy_size;)
  {
    uint32_t count = image->copy_size - offset;
    if (count > sizeof(buffer))
    {
      count = sizeof(buffer);
    }
    if (NOR_Read(flash_offset + offset, buffer, count) != HAL_OK)
    {
      return 1U;
    }
    volatile uint8_t *destination = (volatile uint8_t *)(ram_address + offset);
    for (uint32_t i = 0U; i < count; ++i)
    {
      destination[i] = buffer[i];
    }
    __DSB();
    for (uint32_t i = 0U; i < count; ++i)
    {
      uint8_t value = destination[i];
      if (value != buffer[i])
      {
        return 1U;
      }
      if (offset + i >= FSBL_IMAGE_PAYLOAD_OFFSET)
      {
        checksum += value;
      }
    }
    offset += count;
  }
  if ((*(volatile uint32_t *)(ram_address + FSBL_IMAGE_HEADER_SIZE) != image->stack) ||
      (*(volatile uint32_t *)(ram_address + FSBL_IMAGE_HEADER_SIZE + 4U) != image->entry))
  {
    return 1U;
  }
  return (checksum == image->checksum) ? 0U : 2U;
}

FSBL_AppStatus FSBL_LoadApplications(void)
{
  uint32_t check;
  FSBL_ImageInfo secure_image;
  FSBL_ImageInfo nonsecure_image;

  fsbl_app_status = FSBL_APP_NOT_STARTED;
  fsbl_secure_copy_size = 0U;
  fsbl_nonsecure_copy_size = 0U;
  fsbl_secure_reset_vector = 0U;
  fsbl_nonsecure_reset_vector = 0U;
  check = valid_image(FSBL_SECURE_FLASH_OFFSET, 1U, &secure_image);
  if (check != 0U)
  {
    fsbl_app_status = (check == 1U) ? FSBL_APP_SECURE_HEADER_ERROR :
                                     FSBL_APP_SECURE_VECTOR_ERROR;
    return fsbl_app_status;
  }
  check = valid_image(FSBL_NONSECURE_FLASH_OFFSET, 0U, &nonsecure_image);
  if (check != 0U)
  {
    fsbl_app_status = (check == 1U) ? FSBL_APP_NONSECURE_HEADER_ERROR :
                                     FSBL_APP_NONSECURE_VECTOR_ERROR;
    return fsbl_app_status;
  }

  fsbl_secure_copy_size = secure_image.copy_size;
  fsbl_nonsecure_copy_size = nonsecure_image.copy_size;
  /* Verify physical SRAM rather than dirty cache lines before the handoff. */
  if ((SCB->CCR & SCB_CCR_DC_Msk) != 0U)
  {
    SCB_DisableDCache();
  }
  check = copy_image(FSBL_SECURE_FLASH_OFFSET, FSBL_SECURE_LOAD_ADDRESS, &secure_image);
  if (check != 0U)
  {
    fsbl_app_status = (check == 2U) ? FSBL_APP_SECURE_CHECKSUM_ERROR : FSBL_APP_SECURE_COPY_ERROR;
    return fsbl_app_status;
  }
  check = copy_image(FSBL_NONSECURE_FLASH_OFFSET, FSBL_NONSECURE_LOAD_ADDRESS, &nonsecure_image);
  if (check != 0U)
  {
    fsbl_app_status = (check == 2U) ? FSBL_APP_NONSECURE_CHECKSUM_ERROR : FSBL_APP_NONSECURE_COPY_ERROR;
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
