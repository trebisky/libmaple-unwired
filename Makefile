# Try "make help" first

# I hate the word "sketch"
.DEFAULT_GOAL := vesuvius

##
## Useful paths, constants, etc.
##

ifeq ($(LIB_MAPLE_HOME),)
SRCROOT := .
else
SRCROOT := $(LIB_MAPLE_HOME)
endif
BUILD_PATH = build

# This lets me start moving stuff from libmaple/include/libmaple to libmaple
TJT_PATH := $(SRCROOT)

LIBMAPLE_PATH := $(SRCROOT)/libmaple
UNWIRED_PATH := $(SRCROOT)/unwired
SUPPORT_PATH := $(SRCROOT)/support
#tjt#LIBRARIES_PATH := $(SRCROOT)/libraries
# Support files for linker
LDDIR := $(SUPPORT_PATH)/ld
# Support files for this Makefile
MAKEDIR := $(SUPPORT_PATH)/make
BOARD_INCLUDE_DIR := $(MAKEDIR)/board-includes

##
## Target-specific configuration.  This determines some compiler and
## linker options/flags.
##

# Try "make help" for more information on BOARD and MEMORY_TARGET;
# these default to a Maple Flash build.
##BOARD ?= maple
##MEMORY_TARGET ?= flash
BOARD ?= maple_mini
MEMORY_TARGET ?= jtag

# Chooses the bootloader, available: maple and robotis
BOOTLOADER ?= maple

# This is the serial port used by robotis bootloader
ROBOTIS_PORT ?= /dev/ttyACM0

# $(BOARD)- and $(MEMORY_TARGET)-specific configuration
include $(MAKEDIR)/target-config.mk

##
## Build rules and useful templates
##

include $(MAKEDIR)/build-rules.mk
include $(MAKEDIR)/build-templates.mk

##
## Compilation flags
##

# FIXME: the following allows for deprecated include style, e.g.:
#     #include "libmaple.h"
# or
#     #include "unwired.h"
# It slows compilation noticeably; remove after 1 release.

#tjt#TARGET_FLAGS    += -I$(LIBMAPLE_PATH)/include/libmaple
#tjt#TARGET_FLAGS    += -I$(LIBMAPLE_PATH)/include/libmaple                       \
#tjt#                   -I$(UNWIRED_PATH)/include/unwired

# The order of these is important.
# someday include/unwired will go away.
TARGET_FLAGS    += -I$(UNWIRED_PATH)
TARGET_FLAGS    += -I$(TJT_PATH)
TARGET_FLAGS    += -I$(LIBMAPLE_PATH)/include/libmaple
TARGET_FLAGS    += -I$(UNWIRED_PATH)/include/unwired

#tjt#TARGET_FLAGS += -I$(LIBRARIES_PATH) # for internal lib. includes, e.g. <Wire/WireBase.h>

# tjt added no-builtin
GLOBAL_CFLAGS   := -Os -g3 -gdwarf-2 -nostdlib \
                   -ffunction-sections -fdata-sections \
		   -fno-builtin \
		   -Wl,--gc-sections $(TARGET_FLAGS) \
		   -DBOOTLOADER_$(BOOTLOADER)
GLOBAL_CXXFLAGS := -fno-rtti -fno-exceptions -Wall $(TARGET_FLAGS)
GLOBAL_ASFLAGS  := -x assembler-with-cpp $(TARGET_FLAGS)
LDFLAGS  = $(TARGET_LDFLAGS) $(TOOLCHAIN_LDFLAGS) -mcpu=cortex-m3 -mthumb \
           -Xlinker --gc-sections \
           -Xassembler --march=armv7-m -Wall
#          -Xlinker --print-gc-sections \

##
## Set all submodules here
##

LIBMAPLE_MODULES += $(SRCROOT)/libmaple
LIBMAPLE_MODULES += $(SRCROOT)/libmaple/usb   # The USB module is kept separate
# This was libmaple/stm32f1 - but I got rid of that 10-15-2020
##LIBMAPLE_MODULES += $(LIBMAPLE_MODULE_SERIES) # STM32 series submodule in libmaple
LIBMAPLE_MODULES += $(SRCROOT)/unwired

# Official libraries:
#tjt#LIBMAPLE_MODULES += $(SRCROOT)/libraries/Servo
#tjt#LIBMAPLE_MODULES += $(SRCROOT)/libraries/LiquidCrystal
#tjt#LIBMAPLE_MODULES += $(SRCROOT)/libraries/Wire

# User modules:
ifneq ($(USER_MODULES),)
LIBMAPLE_MODULES += $(USER_MODULES)
endif

# Call each module's rules.mk:
$(foreach m,$(LIBMAPLE_MODULES),$(eval $(call LIBMAPLE_MODULE_template,$(m))))

##
## Targets
##

# main target
include $(SRCROOT)/build-targets.mk

.PHONY: install vesuvius clean help cscope tags ctags ram flash jtag doxygen mrproper list-boards

# Target upload commands
# USB ID for DFU upload -- FIXME: do something smarter with this
BOARD_USB_VENDOR_ID  := 1EAF
BOARD_USB_PRODUCT_ID := 0003

ifeq ($(BOOTLOADER),maple)
UPLOAD_ram   := $(SUPPORT_PATH)/scripts/reset.py && \
                sleep 1                  && \
                $(DFU) -a0 -d $(BOARD_USB_VENDOR_ID):$(BOARD_USB_PRODUCT_ID) -D $(BUILD_PATH)/$(BOARD).bin -R
UPLOAD_flash := $(SUPPORT_PATH)/scripts/reset.py && \
                sleep 1                  && \
                $(DFU) -a1 -d $(BOARD_USB_VENDOR_ID):$(BOARD_USB_PRODUCT_ID) -D $(BUILD_PATH)/$(BOARD).bin -R
endif

ifeq ($(BOOTLOADER),robotis)
UPLOAD_flash := $(SUPPORT_PATH)/scripts/robotis-loader.py $(ROBOTIS_PORT) $(BUILD_PATH)/$(BOARD).bin
endif

# Conditionally upload to whatever the last build was
install: INSTALL_TARGET = $(shell cat $(BUILD_PATH)/build-type 2>/dev/null)
install: $(BUILD_PATH)/$(BOARD).bin
	@echo "Install target:" $(INSTALL_TARGET)
	$(UPLOAD_$(INSTALL_TARGET))

# Force a rebuild if the target changed
PREV_BUILD_TYPE = $(shell cat $(BUILD_PATH)/build-type 2>/dev/null)
build-check:
ifneq ($(PREV_BUILD_TYPE), $(MEMORY_TARGET))
	$(shell rm -rf $(BUILD_PATH))
endif

#sketch: build-check MSG_INFO $(BUILD_PATH)/$(BOARD).bin $(BUILD_PATH)/$(BOARD).hex
vesuvius: build-check $(BUILD_PATH)/$(BOARD).bin $(BUILD_PATH)/$(BOARD).hex

clean:
	rm -rf build

mrproper: clean
	rm -rf doxygen

help:
	@echo ""
	@echo "Basic usage (BOARD defaults to maple):"
	@echo "    $$ cp your-main.cpp main.cpp"
	@echo "    $$ make BOARD=your_board"
	@echo "    $$ make BOARD=your_board install"
	@echo ""
	@echo "(Multiple source files? Link with libmaple.a (\`$$ make library')"
	@echo "or hack build-targets.mk appropriately.)"
	@echo ""
	@echo "Important targets:"
	@echo "    vesuvius:     Compile for BOARD to MEMORY_TARGET (default)."
	@echo "    install:  Compile and upload over USB using Maple bootloader"
	@echo ""
	@echo "You *must* set BOARD if not compiling for Maple (e.g."
	@echo "use BOARD=maple_mini for mini, etc.), and MEMORY_TARGET"
	@echo "if not compiling to Flash. Run \`$$ make list-boards' for"
	@echo "a list of all boards."
	@echo ""
	@echo "Valid MEMORY_TARGETs (default=flash):"
	@echo "    ram:    Compile to RAM (doesn't touch Flash)"
	@echo "    flash:  Compile to Flash (for Maple bootloader)"
	@echo "    jtag:   Compile for JTAG/SWD upload (overwrites bootloader)"
	@echo ""
	@echo "Other targets:"
	@echo "    clean: Remove all build and object files"
	@echo "    doxygen: Build Doxygen HTML and XML documentation"
	@echo "    help: Show this message"
	@echo "    mrproper: Remove all generated files"
	@echo ""

# This is what we expect
#OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg
# Some newer chips are shipping with a idcode of 0x2ba01477, this will make them work.
#  without this, you will see:
# Warn : UNEXPECTED idcode: 0x2ba01477
# Error: expected 1 of 1: 0x1ba01477
#
OCDCFG = -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/cs32f1x.cfg

burn:	build/maple_mini.elf
	openocd $(OCDCFG) -c "program build/maple_mini.elf verify reset exit"

cscope:
	rm -rf cscope.*
	find . -name '*.[hcS]' -o -name '*.cpp' | xargs cscope -b

tags:
	etags `find . -name "*.c" -o -name "*.cpp" -o -name "*.h"`
	@echo "Made TAGS file for EMACS code browsing"

ctags:
	ctags-exuberant -R .
	@echo "Made tags file for VIM code browsing"

ram:
	@$(MAKE) MEMORY_TARGET=ram --no-print-directory vesuvius

flash:
	@$(MAKE) MEMORY_TARGET=flash --no-print-directory vesuvius

jtag:
	@$(MAKE) MEMORY_TARGET=jtag --no-print-directory vesuvius

doxygen:
	doxygen $(SUPPORT_PATH)/doxygen/Doxyfile

# This output is kind of ugly, but I don't understand make very well.
list-boards:
	@echo " $(addsuffix "\\n",$(basename $(notdir $(wildcard $(BOARD_INCLUDE_DIR)/*.mk))))"
