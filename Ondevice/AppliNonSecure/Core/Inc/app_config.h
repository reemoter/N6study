#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Project-owned options: CubeMX does not generate this file.
 * Set to 1 only for a diagnostic build: it overwrites all 32MiB of PSRAM.
 * A compiler definition may override the default without editing this file. */
#ifndef APP_RAM_SELF_TEST
#define APP_RAM_SELF_TEST 0
#endif

#if APP_RAM_SELF_TEST != 0 && APP_RAM_SELF_TEST != 1
#error "APP_RAM_SELF_TEST must be 0 or 1"
#endif

#endif
