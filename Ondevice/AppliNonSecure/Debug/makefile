################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32N6xx_HAL_Driver/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Ondevice_AppliNonSecure
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Ondevice_AppliNonSecure.elf \

MAP_FILES += \
Ondevice_AppliNonSecure.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Ondevice_AppliNonSecure.list \

OBJCOPY_BIN += \
Ondevice_AppliNonSecure.bin \


# All Target
all: main-build

# Main-build Target
main-build: Ondevice_AppliNonSecure.elf secondary-outputs

dependents:
	-cd C:/Users/wbeet/Desktop/Project/ST/Ondevice/Ondevice/AppliSecure/Debug && $(MAKE) all

# Tool invocations
Ondevice_AppliNonSecure.elf Ondevice_AppliNonSecure.map: $(OBJS) $(USER_OBJS) C:/Users/wbeet/Desktop/Project/ST/Ondevice/Ondevice/AppliSecure/Debug/Ondevice_AppliSecure.elf C:\Users\wbeet\Desktop\Project\ST\Ondevice\Ondevice\AppliNonSecure\STM32N657X0HXQ_LRUN.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Ondevice_AppliNonSecure.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m55 -T"C:\Users\wbeet\Desktop\Project\ST\Ondevice\Ondevice\AppliNonSecure\STM32N657X0HXQ_LRUN.ld" --specs=nosys.specs -Wl,-Map="Ondevice_AppliNonSecure.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Ondevice_AppliNonSecure.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Ondevice_AppliNonSecure.list"
	@echo 'Finished building: $@'
	@echo ' '

Ondevice_AppliNonSecure.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "Ondevice_AppliNonSecure.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Ondevice_AppliNonSecure.bin Ondevice_AppliNonSecure.elf Ondevice_AppliNonSecure.list Ondevice_AppliNonSecure.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified
C:/Users/wbeet/Desktop/Project/ST/Ondevice/Ondevice/AppliSecure/Debug/Ondevice_AppliSecure.elf:

-include ../makefile.targets
