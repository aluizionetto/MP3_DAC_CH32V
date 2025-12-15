################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/pff/mmcbbp.c \
../User/pff/pff.c 

C_DEPS += \
./User/pff/mmcbbp.d \
./User/pff/pff.d 

OBJS += \
./User/pff/mmcbbp.o \
./User/pff/pff.o 

DIR_OBJS += \
./User/pff/*.o \

DIR_DEPS += \
./User/pff/*.d \

DIR_EXPANDS += \
./User/pff/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/pff/%.o: ../User/pff/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32imafcxw -mabi=ilp32f -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -fsingle-precision-constant -Wunused -Wuninitialized -g -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r6/Debug" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r6/Core" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r6/User" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r6/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@

