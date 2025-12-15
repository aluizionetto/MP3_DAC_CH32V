################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/buffer_fifo.c \
../User/ch32v30x_it.c \
../User/flash_spi.c \
../User/main.c \
../User/system_ch32v30x.c 

C_DEPS += \
./User/buffer_fifo.d \
./User/ch32v30x_it.d \
./User/flash_spi.d \
./User/main.d \
./User/system_ch32v30x.d 

OBJS += \
./User/buffer_fifo.o \
./User/ch32v30x_it.o \
./User/flash_spi.o \
./User/main.o \
./User/system_ch32v30x.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32imafcxw -mabi=ilp32f -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -fsingle-precision-constant -Wunused -Wuninitialized -g -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r5/Debug" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r5/Core" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r5/User" -I"c:/Users/aluiz/mounriver-studio-projects/MP3_dac_CH32V305FBP6_r5/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@

