# Standard things
sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)
BUILDDIRS       += $(BUILD_PATH)/$(d)

LIBMAPLE_INCLUDES := -I$(LIBMAPLE_PATH)/include
#LIBMAPLE_INCLUDES += -I$(LIBMAPLE_MODULE_SERIES)/include
LIBMAPLE_INCLUDES += -I$(LIBMAPLE_PATH)

LIBMAPLE_PRIVATE_INCLUDES := -I$(LIBMAPLE_PATH)

# Local flags
#CFLAGS_$(d) = $(LIBMAPLE_PRIVATE_INCLUDES) $(LIBMAPLE_INCLUDES) -Wall -Werror
#CFLAGS_$(d) = $(LIBMAPLE_PRIVATE_INCLUDES) $(LIBMAPLE_INCLUDES) -Wall
CFLAGS_$(d) = $(LIBMAPLE_PRIVATE_INCLUDES) $(LIBMAPLE_INCLUDES)

sSRCS_$(d) := vector_table.S
sSRCS_$(d) += isrs.S
sSRCS_$(d) += exc.S
sSRCS_$(d) += start.S

# Local rules and targets
cSRCS_$(d) := start_c.c
cSRCS_$(d) += init.c
cSRCS_$(d) += setup_f1.c
cSRCS_$(d) += adc.c
cSRCS_$(d) += dac.c
cSRCS_$(d) += dma.c
cSRCS_$(d) += exti.c
cSRCS_$(d) += flash.c
cSRCS_$(d) += gpio.c
cSRCS_$(d) += iwdg.c
cSRCS_$(d) += nvic.c
cSRCS_$(d) += pwr.c
cSRCS_$(d) += rcc.c
cSRCS_$(d) += spi.c
cSRCS_$(d) += systick.c
cSRCS_$(d) += timer.c
cSRCS_$(d) += usart.c
cSRCS_$(d) += usart_private.c
cSRCS_$(d) += util.c
cSRCS_$(d) += i2c.c
cSRCS_$(d) += serial.c
cSRCS_$(d) += serial_usb.c
cSRCS_$(d) += time.c
cSRCS_$(d) += digital.c
cSRCS_$(d) += digital_f1.c
cSRCS_$(d) += util_hooks_f1.c
cSRCS_$(d) += debug_f1.c

# These all used to be in the stm32f1 directory
cSRCS_$(d) += usart_f1.c
cSRCS_$(d) += i2c_f1.c
cSRCS_$(d) += adc_f1.c
cSRCS_$(d) += bkp_f1.c
cSRCS_$(d) += dma_f1.c
cSRCS_$(d) += exti_f1.c
cSRCS_$(d) += fsmc_f1.c
cSRCS_$(d) += gpio_f1.c
cSRCS_$(d) += rcc_f1.c
cSRCS_$(d) += spi_f1.c
cSRCS_$(d) += timer_f1.c

## I2C support must be ported to F2:
#ifeq ($(MCU_SERIES),stm32f1)
#cSRCS_$(d) += i2c.c
#endif

cFILES_$(d) := $(cSRCS_$(d):%=$(d)/%)
sFILES_$(d) := $(sSRCS_$(d):%=$(d)/%)

OBJS_$(d) := $(cFILES_$(d):%.c=$(BUILD_PATH)/%.o) $(sFILES_$(d):%.S=$(BUILD_PATH)/%.o)
DEPS_$(d) := $(OBJS_$(d):%.o=%.d)

$(OBJS_$(d)): TGT_CFLAGS := $(CFLAGS_$(d))
$(OBJS_$(d)): TGT_ASFLAGS :=

TGT_BIN += $(OBJS_$(d))

# Standard things
-include        $(DEPS_$(d))
d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
