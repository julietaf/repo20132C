################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../commons/bitarray.c \
../commons/config.c \
../commons/deadlock_detection.c \
../commons/error.c \
../commons/geospatial.c \
../commons/log.c \
../commons/nivel-gui.c \
../commons/process.c \
../commons/sockets.c \
../commons/string.c \
../commons/tad_items.c \
../commons/temporal.c \
../commons/txt.c 

OBJS += \
./commons/bitarray.o \
./commons/config.o \
./commons/deadlock_detection.o \
./commons/error.o \
./commons/geospatial.o \
./commons/log.o \
./commons/nivel-gui.o \
./commons/process.o \
./commons/sockets.o \
./commons/string.o \
./commons/tad_items.o \
./commons/temporal.o \
./commons/txt.o 

C_DEPS += \
./commons/bitarray.d \
./commons/config.d \
./commons/deadlock_detection.d \
./commons/error.d \
./commons/geospatial.d \
./commons/log.d \
./commons/nivel-gui.d \
./commons/process.d \
./commons/sockets.d \
./commons/string.d \
./commons/tad_items.d \
./commons/temporal.d \
./commons/txt.d 


# Each subdirectory must supply rules for building sources it contributes
commons/%.o: ../commons/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


