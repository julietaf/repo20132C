################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Fuse/FuseProc.c \
../Fuse/bitarray.c \
../Fuse/t_disco.c \
../Fuse/t_grasa_fs.c 

OBJS += \
./Fuse/FuseProc.o \
./Fuse/bitarray.o \
./Fuse/t_disco.o \
./Fuse/t_grasa_fs.o 

C_DEPS += \
./Fuse/FuseProc.d \
./Fuse/bitarray.d \
./Fuse/t_disco.d \
./Fuse/t_grasa_fs.d 


# Each subdirectory must supply rules for building sources it contributes
Fuse/%.o: ../Fuse/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


