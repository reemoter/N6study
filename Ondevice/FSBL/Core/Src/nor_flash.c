#include "xspi.h"
#include "nor_flash.h"

volatile HAL_StatusTypeDef nor_command_status = HAL_ERROR;
volatile HAL_StatusTypeDef nor_receive_status = HAL_ERROR;
volatile uint32_t nor_xspi_error = 0U;
volatile uint32_t nor_xspi_sr = 0U;
volatile uint32_t nor_xfer_count = 0U;

HAL_StatusTypeDef NOR_Read(uint32_t offset, uint8_t *data, uint32_t length)
{
  XSPI_RegularCmdTypeDef command = {0};
  HAL_StatusTypeDef status;

  if ((data == NULL) || (length == 0U) || (offset > 0x00FFFFFFU) ||
      ((length - 1U) > (0x00FFFFFFU - offset)))
  {
    return HAL_ERROR;
  }

  /* Read-only SPI 1-1-1 fast-read command. No erase or write is performed. */
  command.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
  command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
  command.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
  command.Instruction = 0x0BU;
  command.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
  command.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
  command.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
  command.Address = offset;
  command.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
  command.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  command.DataMode = HAL_XSPI_DATA_1_LINE;
  command.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
  command.DataLength = length;
  command.DummyCycles = 8;
  command.DQSMode = HAL_XSPI_DQS_DISABLE;

  nor_command_status = HAL_XSPI_Command(&hxspi2, &command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
  if (nor_command_status != HAL_OK)
  {
    status = nor_command_status;
  }
  else
  {
    nor_receive_status = HAL_XSPI_Receive(&hxspi2, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
    status = nor_receive_status;
  }

  nor_xspi_error = HAL_XSPI_GetError(&hxspi2);
  nor_xspi_sr = hxspi2.Instance->SR;
  nor_xfer_count = hxspi2.XferCount;
  return status;
}
