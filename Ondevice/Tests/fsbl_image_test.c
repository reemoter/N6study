#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsbl_image.h"

static void put32(uint8_t *bytes, uint32_t value)
{
  for (unsigned int i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (8 * i));
}

static void check_file(const char *path, uint32_t capacity, uint32_t base,
                       uint32_t stack_min, uint32_t stack_top)
{
  FILE *file = fopen(path, "rb");
  assert(file != NULL);
  assert(fseek(file, 0, SEEK_END) == 0);
  long size = ftell(file);
  assert(size >= FSBL_IMAGE_HEADER_SIZE + 8);
  rewind(file);
  uint8_t *bytes = malloc((size_t)size);
  assert(bytes != NULL);
  assert(fread(bytes, 1, (size_t)size, file) == (size_t)size);
  fclose(file);
  FSBL_ImageInfo image;
  assert(FSBL_ParseImageHeader(bytes, capacity, &image));
  assert(image.copy_size == (uint32_t)size);
  assert(FSBL_ValidateImageVectors(bytes + FSBL_IMAGE_HEADER_SIZE,
                                 base, stack_min, stack_top, &image));
  uint32_t checksum = 0;
  for (uint32_t i = FSBL_IMAGE_PAYLOAD_OFFSET; i < image.copy_size; ++i)
    checksum += bytes[i];
  assert(checksum == image.checksum);

  uint8_t header[FSBL_IMAGE_PREFIX_SIZE];
  memcpy(header, bytes, sizeof(header));
  /* Exact destination limit accepted; one byte beyond rejected. */
  put32(header + 0x6C, capacity - FSBL_IMAGE_PAYLOAD_OFFSET);
  assert(FSBL_ParseImageHeader(header, capacity, &image));
  put32(header + 0x6C, capacity - FSBL_IMAGE_PAYLOAD_OFFSET + 1);
  assert(!FSBL_ParseImageHeader(header, capacity, &image));
  put32(header + 0x6C, UINT32_MAX);
  assert(!FSBL_ParseImageHeader(header, capacity, &image));
  put32(header + 0x6C, FSBL_IMAGE_HEAD_PADDING + 7);
  assert(!FSBL_ParseImageHeader(header, capacity, &image));
  put32(header + 0x6C, 0x10001 - FSBL_IMAGE_PAYLOAD_OFFSET);
  assert(FSBL_ParseImageHeader(header, capacity, &image));
  assert(image.copy_size == 0x10001); /* Above old 64 KiB limit, partial tail. */
  assert(!FSBL_ParseImageHeader(header, 8, &image));
  const unsigned int fields[] = {0, 0x68, 0x84, 0x88, 0x8C, 0x98, 0xA0, 0xA4};
  for (unsigned int i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
  {
    memcpy(header, bytes, sizeof(header));
    header[fields[i]] ^= 1;
    assert(!FSBL_ParseImageHeader(header, capacity, &image));
  }
  assert(FSBL_ParseImageHeader(bytes, capacity, &image));
  uint8_t vectors[8];
  memcpy(vectors, bytes + FSBL_IMAGE_HEADER_SIZE, sizeof(vectors));
  put32(vectors, stack_top - 1);
  assert(!FSBL_ValidateImageVectors(vectors, base, stack_min, stack_top, &image));
  put32(vectors, stack_min);
  assert(!FSBL_ValidateImageVectors(vectors, base, stack_min, stack_top, &image));
  put32(vectors, stack_top);
  put32(vectors + 4, base + image.copy_size - FSBL_IMAGE_HEADER_SIZE + 1);
  image.entry = FSBL_ReadU32LE(vectors + 4);
  assert(!FSBL_ValidateImageVectors(vectors, base, stack_min, stack_top, &image));
  put32(vectors + 4, base + 1);
  image.entry = base + 3;
  assert(!FSBL_ValidateImageVectors(vectors, base, stack_min, stack_top, &image));
  put32(vectors + 4, base);
  image.entry = base;
  assert(!FSBL_ValidateImageVectors(vectors, base, stack_min, stack_top, &image));
  printf("PASS %s: %ld bytes, header, checksum, bounds, malformed images\n", path, size);
  free(bytes);
}

int main(int argc, char **argv)
{
  assert(argc == 3);
  check_file(argv[1], 0x64000, 0x34000400, 0x34064000, 0x34100000);
  check_file(argv[2], 0x80000, 0x24100400, 0x24180000, 0x24200000);
  return 0;
}
