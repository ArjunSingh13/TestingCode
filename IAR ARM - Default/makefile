################################################################################
# Automatically-generated file. Do not edit!
################################################################################

export WINEDEBUG=-all

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include zcl-framework-core/subdir.mk
-include strong-random/subdir.mk
-include serial/EFR32/subdir.mk
-include serial/subdir.mk
-include scenes/subdir.mk
-include reporting/subdir.mk
-include ota-storage-simple-eeprom/subdir.mk
-include ota-storage-simple/subdir.mk
-include ota-storage-common/subdir.mk
-include ota-common/subdir.mk
-include ota-client/subdir.mk
-include ota-bootload/subdir.mk
-include osramds-time-server/subdir.mk
-include osramds-reset2factory/subdir.mk
-include osramds-ota-client-policy/subdir.mk
-include osramds-identify-server/subdir.mk
-include osramds-basic-server/subdir.mk
-include osramds-alarm-server/subdir.mk
-include osramds-UserInterface/subdir.mk
-include on-off/subdir.mk
-include nvm3/EFR32/subdir.mk
-include nvm3/subdir.mk
-include mbedtls/EFR32/subdir.mk
-include hal-library/EFR32_xx/subdir.mk
-include hal-library/EFR32_2/subdir.mk
-include hal-library/EFR32/subdir.mk
-include groups-server/subdir.mk
-include external-generated-files/subdir.mk
-include encelium-sw-timer/subdir.mk
-include encelium-restart-message/subdir.mk
-include encelium-radio-control/subdir.mk
-include encelium-main/subdir.mk
-include encelium-interpan/subdir.mk
-include encelium-interface/subdir.mk
-include encelium-diagnostics/subdir.mk
-include encelium-coroutine/subdir.mk
-include encelium-commissioning/subdir.mk
-include emlib/subdir.mk
-include emdrv/subdir.mk
-include ember-minimal-printf/subdir.mk
-include efr32/subdir.mk
-include eeprom/subdir.mk
-include debug-jtag/EFR32/subdir.mk
-include debug-basic-library/EFR32/subdir.mk
-include counters/subdir.mk
-include concentrator/subdir.mk
-include coexistence-stub/EFR32/subdir.mk
-include ccm-encryption/subdir.mk
-include antenna-stub/subdir.mk
-include adc/EFR32/subdir.mk
-include ZigbeeServices/Zigbee2Dali/DaliUart/DaliUartHAL/subdir.mk
-include ZigbeeServices/Zigbee2Dali/DaliUart/subdir.mk
-include ZigbeeServices/Zigbee2Dali/subdir.mk
-include ZigbeeServices/subdir.mk
-include ZigbeeCallbacks/ClusterServerCallbacks/subdir.mk
-include ZigbeeCallbacks/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_UPPER_DEPS)),)
-include $(MSA_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S79_DEPS)),)
-include $(S79_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_DEPS)),)
-include $(MSA_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S79_UPPER_DEPS)),)
-include $(S79_UPPER_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := ZigbeeHaLightEndDevice
BUILD_ARTIFACT_EXTENSION := out
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory pre-build && $(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: ZigbeeHaLightEndDevice.out

# Tool invocations
ZigbeeHaLightEndDevice.out: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: IAR Linker for ARM'
	ilinkarm $(LIBS) $(OBJS) $(USER_OBJS) -o "ZigbeeHaLightEndDevice.out" --config "C:/SiliconLabs/SimplicityStudio/v5/developer/sdks/gecko_sdk_suite/v3.2//platform/base/hal/micro/cortexm3/efm32/iar-cfg.icf" --config_def APP_BTL=1 --config_def FLASH_SIZE=786432 --config_def RAM_SIZE=65536 --config_def SIMEEPROM_SIZE=32768 --config_def LOCKBITS_IN_MAINFLASH_SIZE=8192 --config_def EMBER_MALLOC_HEAP_SIZE=2048 --config_def HEADER_SIZE=512 --config_def PSSTORE_SIZE=0 --config_def LONGTOKEN_SIZE=0 --config_def BTL_SIZE=16384 --no_wrap_diagnostics --entry __iar_program_start --map "C:/Users/arjun/SimplicityStudio/v5_workspace/ZigbeeHaLightEndDevice/IAR ARM - Default/ZigbeeHaLightEndDevice.map" --log_file "C:/Users/arjun/SimplicityStudio/v5_workspace/ZigbeeHaLightEndDevice/IAR ARM - Default/ZigbeeHaLightEndDevice.log" --log initialization,modules,sections,veneers --redirect _Printf=_PrintfSmallNoMb --redirect _Scanf=_ScanfFullNoMb  
	ielftool --srec --verbose "ZigbeeHaLightEndDevice.out" "ZigbeeHaLightEndDevice.s37"  
	ielftool --ihex --verbose "ZigbeeHaLightEndDevice.out" "ZigbeeHaLightEndDevice.hex"  
	ielftool --bin --verbose "ZigbeeHaLightEndDevice.out" "ZigbeeHaLightEndDevice.bin"
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(ASM_UPPER_DEPS)$(MSA_UPPER_DEPS)$(EXECUTABLES)$(OBJS)$(ASM_DEPS)$(S79_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(MSA_DEPS)$(C_DEPS)$(S79_UPPER_DEPS) ZigbeeHaLightEndDevice.out
	-@echo ' '

pre-build:
	-cmd //c 'C:\Users\arjun\SIMPLI~1\V5_WOR~1\ZIGBEE~1/ZigbeeHaLightEndDevice-prebuild.bat' 'ZigbeeHaLightEndDevice' 'C:/Users/arjun/SimplicityStudio/v5_workspace/ZigbeeHaLightEndDevice' 'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.4/arm' 'C:\SiliconLabs\SimplicityStudio\v5'
	-@echo ' '

post-build:
	-cmd //c 'C:\Users\arjun\SIMPLI~1\V5_WOR~1\ZIGBEE~1/ZigbeeHaLightEndDevice-postbuild.bat' 'ZigbeeHaLightEndDevice' 'C:/Users/arjun/SimplicityStudio/v5_workspace/ZigbeeHaLightEndDevice' 'C:/Program Files (x86)/IAR Systems/Embedded Workbench 8.4/arm' 'C:\SiliconLabs\SimplicityStudio\v5'
	-@echo ' '

.PHONY: all clean dependents main-build pre-build post-build

-include ../makefile.targets
