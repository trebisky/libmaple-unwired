# Standard things
sp              := $(sp).x
dirstack_$(sp)  := $(d)
d               := $(dir)
BUILDDIRS       += $(BUILD_PATH)/$(d)

# Add board directory and target-specific directory to
# BUILDDIRS. These are in subdirectories, but they're logically part
# of the Wirish submodule. That's a bit inconsistent with libmaple
# proper, and should be fixed.

# tjt - we need include files from here.
# Well, we used to.
###WIRISH_BOARD_PATH := boards/$(BOARD)
###BUILDDIRS += $(BUILD_PATH)/$(d)/$(WIRISH_BOARD_PATH)
BUILDDIRS += $(BUILD_PATH)/$(d)/$(TARGET_SERIES_MODULE)

# Safe includes for Wirish.
#WIRISH_INCLUDES := -I$(d)/include -I$(d)/$(WIRISH_BOARD_PATH)/include
#UNWIRED_INCLUDES := -I$(d)/include

# gives access to include files in this very directory
# Also sets up the appropriate BOARD directory to find board.h
UNWIRED_INCLUDES := -I$(d) -I$(d)/$(BOARD)

# gives access to "series" in its new location
###UNWIRED_INCLUDES += -I$(TJT_PATH)/libmaple

# Local flags. Add -I$(d) to allow for private includes.
#CFLAGS_$(d) := $(LIBMAPLE_INCLUDES) $(WIRISH_INCLUDES) -I$(d)
CFLAGS_$(d) := $(LIBMAPLE_INCLUDES) $(UNWIRED_INCLUDES)

# Local rules and targets
#sSRCS_$(d) := start.S
#cSRCS_$(d) := start_c.c

#cSRCS_$(d) += init.c
#cSRCS_$(d) += stm32f1_setup.c
cSRCS_$(d) += debug_f1.c
cSRCS_$(d) += digital_f1.c
cSRCS_$(d) += stm32f1_util_hooks.c
#cSRCS_$(d) += syscalls.c
cSRCS_$(d) += board.c
cSRCS_$(d) += time.c
cSRCS_$(d) += digital.c
cSRCS_$(d) += serial.c
cSRCS_$(d) += serial_usb.c

sFILES_$(d)   := $(sSRCS_$(d):%=$(d)/%)
cFILES_$(d)   := $(cSRCS_$(d):%=$(d)/%)
cppFILES_$(d) := $(cppSRCS_$(d):%=$(d)/%)

OBJS_$(d)     := $(sFILES_$(d):%.S=$(BUILD_PATH)/%.o) \
                 $(cFILES_$(d):%.c=$(BUILD_PATH)/%.o) \
                 $(cppFILES_$(d):%.cpp=$(BUILD_PATH)/%.o)
DEPS_$(d)     := $(OBJS_$(d):%.o=%.d)

$(OBJS_$(d)): TGT_CFLAGS := $(CFLAGS_$(d))

TGT_BIN += $(OBJS_$(d))

# Standard things
-include        $(DEPS_$(d))
d               := $(dirstack_$(sp))
sp              := $(basename $(sp))
