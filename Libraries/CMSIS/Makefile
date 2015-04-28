##
# Makefile to build DSP_Lib into libraries
##

ARCH        := M4f
LIB         := libarm_cortex$(ARCH)_math.a


# Directories
DSPLIB      := DSP_Lib/Source
DSPDIRS     := BasicMathFunctions          \
               CommonTables                \
               ComplexMathFunctions        \
               ControllerFunctions         \
               FastMathFunctions           \
               FilteringFunctions          \
               MatrixFunctions             \
               StatisticsFunctions         \
               SupportFunctions            \
               TransformFunctions

# Targets to build
SRCS        := $(wildcard $(DSPLIB)/*/*.c)
ASMSRCS        := $(wildcard $(DSPLIB)/*/*.S)
OBJS        := $(patsubst %.c, %.o, $(SRCS))
OBJS += $(patsubst %.S, %.o, $(ASMSRCS))
SRCS += $(ASMSRCS)

# Tools
CC          := arm-none-eabi-gcc
CXX         := arm-none-eabi-g++
LD          := arm-none-eabi-ld
AR          := arm-none-eabi-ar
AS          := arm-none-eabi-gcc
CP          := arm-none-eabi-objcopy
OD          := arm-none-eabi-objdump
SIZE        := arm-none-eabi-size

# Flags
INCLUDES    := -I./Include
ARFLAGS     := 

CFLAGS  = -g -O2 -Wall 
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -DARM_MATH_CM4 -D__FPU_PRESENT

all: $(OBJS) $(LIB)
clean:
	rm -f $(OBJS) $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -lg -lm -c -o $@ $<

$(LIB): $(OBJS)
	$(AR) rcs $@ $?
