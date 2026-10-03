#ifndef ONDEVICE_FSBL_IMAGE_H
#define ONDEVICE_FSBL_IMAGE_H

#include <stdint.h>

/* UM3234 v2.3 base header, followed by the -nk padding extension.
 * Only the project's unencrypted, unauthenticated -align format is supported.
 * These checks and checksum are not cryptographic authentication. */
#define FSBL_IMAGE_HEADER_SIZE       0x400U
#define FSBL_IMAGE_PREFIX_SIZE       168U
#define FSBL_IMAGE_BASE_HEADER_SIZE  160U
#define FSBL_IMAGE_POST_HEADER_SIZE  416U
#define FSBL_IMAGE_PAYLOAD_OFFSET    (FSBL_IMAGE_BASE_HEADER_SIZE + FSBL_IMAGE_POST_HEADER_SIZE)
#define FSBL_IMAGE_HEAD_PADDING      (FSBL_IMAGE_HEADER_SIZE - FSBL_IMAGE_PAYLOAD_OFFSET)

typedef struct
{
  uint32_t copy_size;
  uint32_t checksum;
  uint32_t entry;
  uint32_t stack;
} FSBL_ImageInfo;

static inline uint32_t FSBL_ReadU32LE(const uint8_t *bytes)
{
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

/* capacity includes header space; subtract before adding to avoid overflow. */
static inline uint32_t FSBL_ParseImageHeader(const uint8_t *header,
                                            uint32_t capacity,
                                            FSBL_ImageInfo *image)
{
  uint32_t length = FSBL_ReadU32LE(header + 0x6CU);
  if ((header[0] != 'S') || (header[1] != 'T') ||
      (header[2] != 'M') || (header[3] != '2') ||
      (FSBL_ReadU32LE(header + 0x68U) != 0x00020300U) ||
      (FSBL_ReadU32LE(header + 0x84U) != 0x80000000U) ||
      (FSBL_ReadU32LE(header + 0x88U) != FSBL_IMAGE_POST_HEADER_SIZE) ||
      (FSBL_ReadU32LE(header + 0x8CU) != 0x10U) ||
      (FSBL_ReadU32LE(header + 0x98U) != 0U) ||
      (FSBL_ReadU32LE(header + 0xA0U) != 0xFFFF5453U) ||
      (FSBL_ReadU32LE(header + 0xA4U) != FSBL_IMAGE_POST_HEADER_SIZE) ||
      (capacity < FSBL_IMAGE_HEADER_SIZE + 8U) ||
      (length < FSBL_IMAGE_HEAD_PADDING + 8U) ||
      (length > capacity - FSBL_IMAGE_PAYLOAD_OFFSET))
  {
    return 0U;
  }
  image->copy_size = FSBL_IMAGE_PAYLOAD_OFFSET + length;
  image->checksum = FSBL_ReadU32LE(header + 0x64U);
  image->entry = FSBL_ReadU32LE(header + 0x70U);
  image->stack = 0U;
  return 1U;
}

static inline uint32_t FSBL_ValidateImageVectors(const uint8_t *vectors,
                                                uint32_t vector_address,
                                                uint32_t stack_min,
                                                uint32_t stack_top,
                                                FSBL_ImageInfo *image)
{
  uint32_t stack = FSBL_ReadU32LE(vectors);
  uint32_t reset = FSBL_ReadU32LE(vectors + 4U);
  uint32_t code_size = image->copy_size - FSBL_IMAGE_HEADER_SIZE;
  uint32_t code_address = reset & ~1U;
  if ((stack <= stack_min) || (stack > stack_top) || ((stack & 7U) != 0U) ||
      ((reset & 1U) == 0U) || (reset != image->entry) ||
      (code_address < vector_address) || (code_size < 2U) ||
      (code_address - vector_address > code_size - 2U))
  {
    return 0U;
  }
  image->stack = stack;
  return 1U;
}

#endif
