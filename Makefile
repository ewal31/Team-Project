# Projects files
#SRCS = main.c system_stm32f4xx.c

SRCS        = $(wildcard src/*.c)

# all the files will be generated with this name (main.elf, main.bin, main.hex, etc)
PROJ_NAME=main

# Location of the Libraries folder from the STM32F4xx Standard Peripheral Library
STD_PERIPH_LIB=Libraries/STM32F4xx_StdPeriph_Driver

#Location of the DSP lib
DSP_LIB=Libraries/CMSIS

# Location of FreeRTOS
FREERTOS_LOC=Libraries/FreeRTOSV8.2.0/FreeRTOS/Source

# Location of FAT SL
FATSL_LOC=Libraries/FreeRTOSV8.2.0/FreeRTOS-Plus/Source/FreeRTOS-Plus-FAT-SL

# Location of the linker scripts
LDSCRIPT_INC=Device

# that's it, no need to change anything below this line!

###################################################

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size

CFLAGS  = -Wall -pedantic -g -std=c99  -Wno-unused-variable -Wno-unused-function

notdebug: CFLAGS += -O2
notdebug: all

debug: CFLAGS += -O0
debug: all

CFLAGS += -mlittle-endian -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -DARM_MATH_CM4
CFLAGS += -Wl,--gc-sections -Wl,-Map=$(PROJ_NAME).map


###################################################

vpath %.c src
vpath %.a $(STD_PERIPH_LIB)

ROOT=$(shell pwd)

CFLAGS += -I inc -I $(STD_PERIPH_LIB) -I Libraries/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I Libraries/CMSIS/Include -I $(STD_PERIPH_LIB)/inc
CFLAGS += -include $(STD_PERIPH_LIB)/stm32f4xx_conf.h
CFLAGS += -I$(FREERTOS_LOC)/include -I$(FREERTOS_LOC)/portable/GCC/ARM_CM4F
CFLAGS += -I$(FATSL_LOC)/api -I $(FATSL_LOC)/fat_sl/common -I$(FATSL_LOC)/psp/include

SRCS += Device/startup_stm32f401xx.s # add startup file to build

# need if you want to build with -DUSE_CMSIS 
#SRCS += stm32f0_discovery.c
#SRCS += stm32f0_discovery.c stm32f0xx_it.c

FREERTOS_SRCS=$(FREERTOS_LOC)/tasks.c $(FREERTOS_LOC)/list.c $(FREERTOS_LOC)/queue.c $(FREERTOS_LOC)/timers.c  $(FREERTOS_LOC)/portable/MemMang/heap_1.c $(FREERTOS_LOC)/portable/GCC/ARM_CM4F/port.c

SRCS += $(FREERTOS_SRCS)

SRCS += $(wildcard $(FATSL_LOC)/fat_sl/common/*.c)
SRCS += $(FATSL_LOC)/psp/target/rtc/psp_rtc.c

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib proj

all: proj

lib:
	$(MAKE) -C $(STD_PERIPH_LIB)

proj: 	$(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@  -L$(STD_PERIPH_LIB) -lstm32f4 -L$(DSP_LIB) -larm_cortexM4f_math -mthumb -lg -lc -lm -L$(LDSCRIPT_INC) -Tstm32f401_flash.ld
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
	$(OBJDUMP) -dt $(PROJ_NAME).elf >$(PROJ_NAME).lst
	$(SIZE) $(PROJ_NAME).elf

prog:
	st-flash write main.bin 0x8000000
	
clean:
	find ./ -name '*~' | xargs rm -f	
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin
	rm -f $(PROJ_NAME).map
	rm -f $(PROJ_NAME).lst

reallyclean: clean
	$(MAKE) -C $(STD_PERIPH_LIB) clean


