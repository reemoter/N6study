#ifndef ONDEVICE_FSBL_NOR_FLASH_H
#define ONDEVICE_FSBL_NOR_FLASH_H

#include "stm32n6xx_hal.h"

/* Read-only debug snapshots; inspect these after NOR_Read() returns. */
extern volatile HAL_StatusTypeDef nor_command_status;
extern volatile HAL_StatusTypeDef nor_receive_status;
extern volatile uint32_t nor_xspi_error;
extern volatile uint32_t nor_xspi_sr;
extern volatile uint32_t nor_xfer_count;

HAL_StatusTypeDef NOR_Read(uint32_t offset, uint8_t *data, uint32_t length);

#endif
