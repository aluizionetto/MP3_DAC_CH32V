################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
MP3_dac_CH32V305FBP6_r5.hex \

SECONDARY_LIST += \
MP3_dac_CH32V305FBP6_r5.lst \

SECONDARY_SIZE += \
MP3_dac_CH32V305FBP6_r5.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: MP3_dac_CH32V305FBP6_r5.elf secondary-outputs

# Tool invocations
MP3_dac_CH32V305FBP6_r5.elf: $(OBJS) $(USER_OBJS_ESCAPE)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU RISC-V Cross C Linker'
	riscv-none-embed-gcc -march=rv32imafcxw -mabi=ilp32f -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -fsingle-precision-constant -Wunused -Wuninitialized -g -T "c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r5/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage -Wl,-Map,"MP3_dac_CH32V305FBP6_r5.map" --specs=nano.specs --specs=nosys.specs -o "MP3_dac_CH32V305FBP6_r5.elf" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@
MP3_dac_CH32V305FBP6_r5.hex: MP3_dac_CH32V305FBP6_r5.elf
	@echo 'Invoking: GNU RISC-V Cross Create Flash Image'
	@	riscv-none-embed-objcopy -O ihex "MP3_dac_CH32V305FBP6_r5.elf" "MP3_dac_CH32V305FBP6_r5.hex"
	@echo 'Finished building $@'
	@
MP3_dac_CH32V305FBP6_r5.lst: MP3_dac_CH32V305FBP6_r5.elf
	@echo 'Invoking: GNU RISC-V Cross Create Listing'
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "MP3_dac_CH32V305FBP6_r5.elf" > "MP3_dac_CH32V305FBP6_r5.lst"
	@echo 'Finished building $@'
	@
MP3_dac_CH32V305FBP6_r5.siz: MP3_dac_CH32V305FBP6_r5.elf
	@echo 'Invoking: GNU RISC-V Cross Print Size'
	riscv-none-embed-size --format=berkeley "MP3_dac_CH32V305FBP6_r5.elf"
	@echo 'Finished building $@'
	@

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) MP3_dac_CH32V305FBP6_r5.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets