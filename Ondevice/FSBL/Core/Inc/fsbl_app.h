#ifndef ONDEVICE_FSBL_APP_H
#define ONDEVICE_FSBL_APP_H

#include <stdint.h>
#include "fsbl_image.h"

/* Development LRUN layout from ST's STM32N6570-DK isolation template. */
#define FSBL_SECURE_FLASH_OFFSET       0x00100000U
#define FSBL_NONSECURE_FLASH_OFFSET    0x00180000U
#define FSBL_SECURE_LOAD_ADDRESS       0x34000000U
#define FSBL_NONSECURE_LOAD_ADDRESS    0x34100000U
/* Stop before each app's RAM/data region and the next Flash slot. */
#define FSBL_SECURE_IMAGE_CAPACITY     0x00064000U
#define FSBL_NONSECURE_IMAGE_CAPACITY  0x00080000U

typedef enum
{
  FSBL_APP_NOT_STARTED = 0,
  FSBL_APP_SECURE_HEADER_ERROR = 1,
  FSBL_APP_NONSECURE_HEADER_ERROR = 2,
  FSBL_APP_SECURE_VECTOR_ERROR = 3,
  FSBL_APP_NONSECURE_VECTOR_ERROR = 4,
  FSBL_APP_SECURE_COPY_ERROR = 5,
  FSBL_APP_NONSECURE_COPY_ERROR = 6,
  FSBL_APP_READY = 7,
  FSBL_APP_SECURE_CHECKSUM_ERROR = 8,
  FSBL_APP_NONSECURE_CHECKSUM_ERROR = 9
} FSBL_AppStatus;

extern volatile FSBL_AppStatus fsbl_app_status;
extern volatile uint32_t fsbl_secure_reset_vector;
extern volatile uint32_t fsbl_nonsecure_reset_vector;
extern volatile uint32_t fsbl_secure_copy_size;
extern volatile uint32_t fsbl_nonsecure_copy_size;

FSBL_AppStatus FSBL_LoadApplications(void);
void FSBL_JumpToSecure(void);

#endif
