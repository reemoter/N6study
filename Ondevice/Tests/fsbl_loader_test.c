#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsbl_app.h"
#include "nor_flash.h"

TestSCB test_scb;
unsigned int test_cache_disabled;
static uint8_t secure_flash[0x64000];
static uint8_t ns_flash[0x80000];
static uint32_t secure_size, ns_size;
static uint32_t fail_offset;
static unsigned int read_calls;

HAL_StatusTypeDef NOR_Read(uint32_t offset, uint8_t *data, uint32_t length)
{
  ++read_calls;
  if (offset == fail_offset) return HAL_ERROR;
  uint32_t relative;
  const uint8_t *source;
  uint32_t size;
  if (offset >= FSBL_NONSECURE_FLASH_OFFSET)
  {
    relative = offset - FSBL_NONSECURE_FLASH_OFFSET;
    source = ns_flash; size = ns_size;
  }
  else
  {
    assert(offset >= FSBL_SECURE_FLASH_OFFSET);
    relative = offset - FSBL_SECURE_FLASH_OFFSET;
    source = secure_flash; size = secure_size;
  }
  assert(relative <= size && length <= size - relative);
  if (relative != 0 || length != FSBL_IMAGE_PREFIX_SIZE)
  {
    /* Vector validation may precede cache disable; bulk copying must not. */
    if (!(relative == FSBL_IMAGE_HEADER_SIZE && length == 8))
      assert((SCB->CCR & SCB_CCR_DC_Msk) == 0);
  }
  memcpy(data, source + relative, length);
  return HAL_OK;
}

static void put32(uint8_t *bytes, uint32_t value)
{
  for (unsigned int i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (8 * i));
}

static uint32_t load_file(const char *path, uint8_t *bytes, uint32_t capacity)
{
  FILE *file = fopen(path, "rb"); assert(file);
  size_t size = fread(bytes, 1, capacity, file);
  assert(size > FSBL_IMAGE_HEADER_SIZE && feof(file));
  fclose(file); return (uint32_t)size;
}

static void reset_test(void)
{
  memset((void *)(uintptr_t)FSBL_SECURE_LOAD_ADDRESS, 0xA5, sizeof(secure_flash));
  memset((void *)(uintptr_t)FSBL_NONSECURE_LOAD_ADDRESS, 0xA5, sizeof(ns_flash));
  test_scb.CCR = SCB_CCR_DC_Msk;
  test_cache_disabled = read_calls = 0;
  fail_offset = UINT32_MAX;
}

int main(int argc, char **argv)
{
  assert(argc == 3);
  assert(VirtualAlloc((void *)(uintptr_t)FSBL_SECURE_LOAD_ADDRESS, sizeof(secure_flash),
                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
  assert(VirtualAlloc((void *)(uintptr_t)FSBL_NONSECURE_LOAD_ADDRESS, sizeof(ns_flash),
                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
  secure_size = load_file(argv[1], secure_flash, sizeof(secure_flash));
  ns_size = load_file(argv[2], ns_flash, sizeof(ns_flash));
  reset_test();
  assert(FSBL_LoadApplications() == FSBL_APP_READY);
  assert(test_cache_disabled == 1);
  assert(fsbl_secure_copy_size == secure_size && fsbl_nonsecure_copy_size == ns_size);
  assert(memcmp((void *)(uintptr_t)FSBL_SECURE_LOAD_ADDRESS, secure_flash, secure_size) == 0);
  assert(memcmp((void *)(uintptr_t)FSBL_NONSECURE_LOAD_ADDRESS, ns_flash, ns_size) == 0);
  assert(*(uint8_t *)(uintptr_t)(FSBL_SECURE_LOAD_ADDRESS + secure_size) == 0xA5);
  assert(*(uint8_t *)(uintptr_t)(FSBL_NONSECURE_LOAD_ADDRESS + ns_size) == 0xA5);

  reset_test(); secure_flash[secure_size - 1] ^= 1;
  assert(FSBL_LoadApplications() == FSBL_APP_SECURE_CHECKSUM_ERROR);
  assert(*(uint8_t *)(uintptr_t)FSBL_NONSECURE_LOAD_ADDRESS == 0xA5);
  secure_flash[secure_size - 1] ^= 1;
  reset_test(); ns_flash[ns_size - 1] ^= 1;
  assert(FSBL_LoadApplications() == FSBL_APP_NONSECURE_CHECKSUM_ERROR);
  ns_flash[ns_size - 1] ^= 1;

  reset_test(); fail_offset = FSBL_SECURE_FLASH_OFFSET + 512;
  assert(FSBL_LoadApplications() == FSBL_APP_SECURE_COPY_ERROR);
  reset_test(); fail_offset = FSBL_NONSECURE_FLASH_OFFSET + 512;
  assert(FSBL_LoadApplications() == FSBL_APP_NONSECURE_COPY_ERROR);
  reset_test(); ns_flash[0] ^= 1;
  assert(FSBL_LoadApplications() == FSBL_APP_NONSECURE_HEADER_ERROR);
  assert(read_calls == 3); /* Reject both-image preflight before any writes. */
  assert(*(uint8_t *)(uintptr_t)FSBL_SECURE_LOAD_ADDRESS == 0xA5);
  ns_flash[0] ^= 1;

  /* Exercise a >64 KiB image with a one-byte final transfer. */
  uint32_t original_size = secure_size;
  secure_size = 0x10001;
  memset(secure_flash + original_size, 0x5A, secure_size - original_size);
  put32(secure_flash + 0x6C, secure_size - FSBL_IMAGE_PAYLOAD_OFFSET);
  uint32_t checksum = 0;
  for (uint32_t i = FSBL_IMAGE_PAYLOAD_OFFSET; i < secure_size; ++i) checksum += secure_flash[i];
  put32(secure_flash + 0x64, checksum);
  reset_test();
  assert(FSBL_LoadApplications() == FSBL_APP_READY);
  assert(memcmp((void *)(uintptr_t)FSBL_SECURE_LOAD_ADDRESS, secure_flash, secure_size) == 0);
  assert(*(uint8_t *)(uintptr_t)(FSBL_SECURE_LOAD_ADDRESS + secure_size) == 0xA5);
  puts("PASS production loader: exact copies, partial tails, >64KiB, preflight, read failures, checksum corruption");
  return 0;
}
