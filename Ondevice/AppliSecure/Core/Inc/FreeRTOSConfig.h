#ifndef SECURE_FREERTOS_CONFIG_H
#define SECURE_FREERTOS_CONFIG_H

/* Secure context support only; the kernel and application heap live in NS. */
#define configENABLE_MPU 0
#define configENABLE_FPU 1
#define configENABLE_MVE 0
#define secureconfigMAX_SECURE_CONTEXTS 8U
#define secureconfigTOTAL_HEAP_SIZE (10U * 1024U)

#endif
