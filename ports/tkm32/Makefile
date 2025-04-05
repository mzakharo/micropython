# Select the board to build for: if not given on the command line,
# then default to PYBV10.
BOARD ?= TKM32
LVGL ?= 0

ifeq ($(LVGL),1)
CFLAGS += -DMICROPY_PY_LVGL
CFLAGS += -DMICROPY_PY_LODEPNG
CFLAGS += -DMICROPY_PY_LCDFB
LVGL_PORT_DIR = lvgl_port
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)

BOARD_DIR ?= boards/$(BOARD)
ifeq ($(wildcard $(BOARD_DIR)/.),)
$(error Invalid BOARD specified: $(BOARD_DIR))
endif

include ../../py/mkenv.mk
-include mpconfigport.mk
include $(BOARD_DIR)/mpconfigboard.mk

# Files that are generated and needed before the QSTR build.
QSTR_GENERATED_HEADERS = $(BUILD)/pins_qstr.h $(BUILD)/modstm_qstr.h

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(QSTR_GENERATED_HEADERS)
QSTR_GLOBAL_DEPENDENCIES = mpconfigboard_common.h $(BOARD_DIR)/mpconfigboard.h $(QSTR_GENERATED_HEADERS)

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# File containing description of content to be frozen into firmware.
FROZEN_MANIFEST ?= boards/manifest.py

# include py core make definitions
include $(TOP)/py/py.mk

TKM_LIB_DIR=tkm32lib

GIT_SUBMODULES = lib/libhydrogen lib/lwip lib/mbedtls lib/mynewt-nimble

MCU_SERIES_UPPER = $(shell echo $(MCU_SERIES) | tr '[:lower:]' '[:upper:]')
CMSIS_MCU_LOWER = $(shell echo $(CMSIS_MCU) | tr '[:upper:]' '[:lower:]')

LD_DIR=boards

CMSIS_DIR=$(TKM_LIB_DIR)/Hal_lib/inc
HAL_DIR=$(TKM_LIB_DIR)/Hal_lib

USB_LIB_DIR=usb_lib
USB_APP_DIR=usb_app
USB_CDC_DIR=usbcdc

USBDEV_DIR=usbdev

STARTUP_FILE ?= $(TKM_LIB_DIR)/startup/gcc/startup_tk499xx.o
SYSTEM_FILE ?= $(TKM_LIB_DIR)/sys/system_tkm32_startup.o

# Select the cross compile prefix
CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(CMSIS_DIR)/
INC += -I$(TOP)/$(HAL_DIR)/inc
INC += -I$(USBDEV_DIR)/core/inc -I$(USBDEV_DIR)/class/inc
#INC += -I$(USBHOST_DIR)
#INC += -Ilwip_inc
INC += -I$(USB_LIB_DIR)/inc -I$(USB_APP_DIR)/inc
INC += -I$(USB_CDC_DIR)/inc

INC += -I$(TKM_LIB_DIR)/sys
INC += -I$(TKM_LIB_DIR)/Hal_lib/inc

# Basic Cortex-M flags
CFLAGS_CORTEX_M = -mthumb

# Select hardware floating-point support
SUPPORTS_HARDWARE_FP_SINGLE = 0
SUPPORTS_HARDWARE_FP_DOUBLE = 0

CFLAGS_CORTEX_M += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
SUPPORTS_HARDWARE_FP_SINGLE = 1

# Options for particular MCU series
CFLAGS_MCU_f4 = $(CFLAGS_CORTEX_M) -mtune=cortex-m4 -mcpu=cortex-m4

#CFLAGS += $(INC) -Wall -Wpointer-arith -Wdouble-promotion -Wfloat-conversion -std=gnu99 -nostdlib $(CFLAGS_MOD) $(CFLAGS_EXTRA)
CFLAGS += $(INC) -Wall -Wpointer-arith -Wdouble-promotion -Wfloat-conversion -std=gnu99 $(CFLAGS_MOD) $(CFLAGS_EXTRA)

CFLAGS += -D$(CMSIS_MCU)
CFLAGS += $(CFLAGS_MCU_$(MCU_SERIES))
CFLAGS += $(COPT)
CFLAGS += -I$(BOARD_DIR)
CFLAGS += -DTKM32_HAL_H='<$(HAL_DIR)/inc/HAL_conf.h>'
CFLAGS += -DMICROPY_HW_VTOR=$(TEXT0_ADDR)

# Configure for nan-boxing object model if requested
ifeq ($(NANBOX),1)
CFLAGS += -DMP_CONFIGFILE='"mpconfigport_nanbox.h"'
ifneq ($(MICROPY_FLOAT_IMPL),none)
MICROPY_FLOAT_IMPL = double
endif
endif

# Configure floating point support
ifeq ($(MICROPY_FLOAT_IMPL),double)
CFLAGS += -DMICROPY_FLOAT_IMPL=MICROPY_FLOAT_IMPL_DOUBLE
else
ifeq ($(MICROPY_FLOAT_IMPL),none)
CFLAGS += -DMICROPY_FLOAT_IMPL=MICROPY_FLOAT_IMPL_NONE
else
CFLAGS += -DMICROPY_FLOAT_IMPL=MICROPY_FLOAT_IMPL_FLOAT
CFLAGS += -fsingle-precision-constant
endif
endif

LDFLAGS = -nostdlib -L $(LD_DIR) $(addprefix -T,$(LD_FILES)) -Map=$(@:.elf=.map) --cref
LDFLAGS += --defsym=_estack_reserve=8
LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

# Remove uncalled code from the final image.
CFLAGS += -fdata-sections -ffunction-sections
LDFLAGS += --gc-sections

DEBUG = 1
# Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -g -DPENDSV_DEBUG
COPT = -Og
# Disable text compression in debug builds
MICROPY_ROM_TEXT_COMPRESSION = 0
else
COPT += -Os -DNDEBUG
endif

# Flags for optional C++ source code

CXXFLAGS += $(filter-out -Wmissing-prototypes -Wold-style-definition -std=gnu99,$(CFLAGS))
CXXFLAGS += $(CXXFLAGS_MOD)
ifneq ($(SRC_CXX)$(SRC_MOD_CXX),)
LDFLAGS += -L$(dir $(shell $(CXX) $(CXXFLAGS) -print-file-name=libstdc++.a))
endif

# Flags for optional C++ source code
# CXXFLAGS += $(filter-out -Wmissing-prototypes -Wold-style-definition -std=gnu99,$(CFLAGS))
# CXXFLAGS += $(CXXFLAGS_MOD)
# ifneq ($(SRC_CXX)$(SRC_MOD_CXX),)
# LIBSTDCPP_FILE_NAME = "$(shell $(CXX) $(CXXFLAGS) -print-file-name=libstdc++.a)"
# LDFLAGS += -L"$(shell dirname $(LIBSTDCPP_FILE_NAME))"
# endif


# Options for mpy-cross
MPY_CROSS_FLAGS += -march=armv7m

SHARED_SRC_C += $(addprefix shared/,\
	libc/string0.c \
	netutils/dhcpserver.c \
	netutils/netutils.c \
	netutils/trace.c \
	readline/readline.c \
	runtime/gchelper_native.c \
	runtime/interrupt_char.c \
	runtime/mpirq.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	runtime/sys_stdio_mphal.c \
	timeutils/timeutils.c \
	)

ifeq ($(MICROPY_FLOAT_IMPL),double)
LIBM_SRC_C += $(addprefix lib/libm_dbl/,\
	__cos.c \
	__expo2.c \
	__fpclassify.c \
	__rem_pio2.c \
	__rem_pio2_large.c \
	__signbit.c \
	__sin.c \
	__tan.c \
	acos.c \
	acosh.c \
	asin.c \
	asinh.c \
	atan.c \
	atan2.c \
	atanh.c \
	ceil.c \
	cos.c \
	cosh.c \
	copysign.c \
	erf.c \
	exp.c \
	expm1.c \
	floor.c \
	fmod.c \
	frexp.c \
	ldexp.c \
	lgamma.c \
	log.c \
	log10.c \
	log1p.c \
	modf.c \
	nearbyint.c \
	pow.c \
	rint.c \
	round.c \
	scalbn.c \
	sin.c \
	sinh.c \
	tan.c \
	tanh.c \
	tgamma.c \
	trunc.c \
	)
ifeq ($(SUPPORTS_HARDWARE_FP_DOUBLE),1)
LIBM_SRC_C += lib/libm_dbl/thumb_vfp_sqrt.c
else
LIBM_SRC_C += lib/libm_dbl/sqrt.c
endif
else
LIBM_SRC_C += $(addprefix lib/libm/,\
	math.c \
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	nearbyintf.c \
	roundf.c \
	sf_cos.c \
	sf_erf.c \
	sf_frexp.c \
	sf_ldexp.c \
	sf_modf.c \
	sf_sin.c \
	sf_tan.c \
	wf_lgamma.c \
	wf_tgamma.c \
	)
ifeq ($(SUPPORTS_HARDWARE_FP_SINGLE),1)
LIBM_SRC_C += lib/libm/thumb_vfp_sqrtf.c
else
LIBM_SRC_C += lib/libm/ef_sqrt.c
endif
endif

LIBM_O = $(addprefix $(BUILD)/, $(LIBM_SRC_C:.c=.o))

# Too many warnings in libm_dbl, disable for now.
ifeq ($(MICROPY_FLOAT_IMPL),double)
$(LIBM_O): CFLAGS := $(filter-out -Wdouble-promotion -Wfloat-conversion, $(CFLAGS))
endif

EXTMOD_SRC_C += $(addprefix extmod/,\
        )
#modonewire.c \

DRIVERS_SRC_C += $(addprefix drivers/,\
	bus/softspi.c \
	bus/softqspi.c \
	memory/spiflash.c \
	)
#dht/dht.c \

SRC_C += \
	boardctrl.c \
	main.c \
	tkm32_it.c \
	mphalport.c \
	irq.c \
	pendsv.c \
	pybthread.c \
	mpthreadport.c \
	systick.c  \
	softtimer.c  \
	powerctrl.c \
	factoryreset.c \
	timer.c \
	led.c \
	pin.c \
	extint.c \
	usrsw.c \
	i2c.c \
	pin_defs_tkm32.c \
	pin_named_pins.c \
	bufhelper.c \
	gccollect.c \
	help.c \
	modmachine.c \
	machine_adc.c \
	machine_uart.c \
	machine_timer.c \
	machine_i2c.c \
	modutime.c \
	wdt.c \
	rtc.c \
	rng.c \
	uart.c \
	moduos.c \
	spibdev.c \
	storage.c \
	fatfs_port.c \
	sdio_sdcard.c \
	sdcard.c \
	qspi_fun.c \
	ft54x6.c \
	gt911.c \
	tp_touch.c \
	modtouch.c \
	lcd43g.c \
	lcd7r.c \
	modtftlcd.c \
	gui_button.c \
	modgui.c \
	modnetwork.c \
	modusocket.c \
	esp8285.c \
	modesp8285.c \
	buffer.c \
	my_sprintf.c \
	spi.c \
	modspi.c \
	usb_cdc_port.c \
	machine_Soft_i2c.c \
	machine_neopixel.c \
	dht.c \
	modtftlcd.c \
	$(wildcard $(BOARD_DIR)/*.c)
	
#pic
ifeq ($(MICROPY_PY_PICLIB),1)
SRC_C += $(addprefix $(BOARD_DIR)/picture/,\
	piclib.c \
	tjpgd.c \
	bmp.c \
	)
INC += -I$(BOARD_DIR)/picture
endif

SRC_C += $(TKM_LIB_DIR)/sys/sys.c

SRC_C += modonewire.c

SRC_CXX += \
	$(SRC_MOD_CXX)

SRC_O += \
	$(STARTUP_FILE) \
	$(SYSTEM_FILE)

SRC_O += \
	resethandler.o \
	shared/runtime/gchelper_m3.o
	
HAL_SRC_C += $(addprefix $(HAL_DIR)/src/HAL_,\
	dma.c \
	exti.c \
	gpio.c \
	i2c.c \
	misc.c \
	pwr.c \
	rcc.c \
	spi.c \
	syscfg.c \
	tim.c \
	uart.c \
	)
	
	
USBDEV_SRC_C += \
	$(wildcard $(USB_LIB_DIR)/src/*.c)
USBDEV_SRC_C += \
	$(wildcard $(USB_APP_DIR)/src/*.c)
USBCDC_SRC_C += \
	$(wildcard $(USB_CDC_DIR)/src/*.c)
	
ifeq ($(LVGL),1)
INC += -I$(LVGL_PORT_DIR)
LIB_SRC_C += $(LVGL_PORT_DIR)/modlvport.c
endif

OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))
OBJ += $(LIBM_O)
OBJ += $(addprefix $(BUILD)/, $(SHARED_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(EXTMOD_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(DRIVERS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(HAL_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(USBDEV_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(USBCDC_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))
OBJ += $(BUILD)/pins_$(BOARD).o

# This file contains performance critical functions so turn up the optimisation
# level.  It doesn't add much to the code size and improves performance a bit.
# Don't use -O3 with this file because gcc tries to optimise memset in terms of itself.
$(BUILD)/shared/libc/string0.o: COPT += -O2

# We put several files into the first 16K section with the ISRs.
# If we compile these using -O0 then it won't fit. So if you really want these
# to be compiled with -O0, then edit boards/common.ld (in the .isr_vector section)
# and comment out the following lines.
$(BUILD)/$(OOFATFS_DIR)/ff.o: COPT += -Os
$(filter $(PY_BUILD)/../extmod/vfs_fat_%.o, $(PY_O)): COPT += -Os
$(PY_BUILD)/formatfloat.o: COPT += -Os
$(PY_BUILD)/parsenum.o: COPT += -Os
$(PY_BUILD)/mpprint.o: COPT += -Os

all: $(TOP)/lib/stm32lib/README.md $(BUILD)/firmware.hex $(BUILD)/firmware.bin


# For convenience, automatically fetch required submodules if they don't exist
$(TOP)/lib/stm32lib/README.md:
	$(ECHO) "stm32lib submodule not found, fetching it now..."
	(cd $(TOP) && git submodule update --init lib/stm32lib)

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
CFLAGS += -DMICROPY_MODULE_FROZEN_STR
endif

define GENERATE_ELF
	$(ECHO) "LINK $(1)"
	$(Q)$(LD) $(LDFLAGS) -o $(1) $(2) $(LDFLAGS_MOD) $(LIBS)
	$(Q)$(SIZE) $(1)
	$(if $(filter-out $(TEXT0_ADDR),0x70008000), \
		$(ECHO) "INFO: this build requires mboot to be installed first")
	$(if $(filter $(TEXT1_ADDR),0x90000000), \
		$(ECHO) "INFO: this build places firmware in external QSPI flash")
endef

define GENERATEmake_BIN
	$(ECHO) "GEN $(1)"
	$(Q)$(OBJCOPY) -O binary $(addprefix -j ,$(3)) $(2) $(1)
endef

define GENERATE_BIN
	$(ECHO) "GEN $(1)"
	$(Q)$(OBJCOPY) -O binary $(addprefix -j ,$(3)) $(2) $(1)
endef

define GENERATE_HEX
	$(ECHO) "GEN $(1)"
	$(Q)$(OBJCOPY) -O ihex $(2) $(1)
endef


# A board should specify TEXT0_ADDR if to use a different location than the
# default for the firmware memory location.  A board can also optionally define
# TEXT1_ADDR to split the firmware into two sections; see below for details.
TEXT0_ADDR ?= 0x70008000

# No TEXT1_ADDR given so put all firmware at TEXT0_ADDR location
TEXT0_SECTIONS ?= .isr_vector .text .data
$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(call GENERATE_BIN,$@,$^,$(TEXT0_SECTIONS))

$(BUILD)/firmware.hex: $(BUILD)/firmware.elf
	$(call GENERATE_HEX,$@,$^)

$(BUILD)/firmware.elf: $(OBJ)
	$(call GENERATE_ELF,$@,$^)

PLLVALUES = boards/pllvalues.py
MAKE_PINS = boards/make-pins.py
BOARD_PINS = $(BOARD_DIR)/pins.csv
PREFIX_FILE = boards/tkm32f499_prefix.c
GEN_PINS_SRC = $(BUILD)/pins_$(BOARD).c
GEN_PINS_HDR = $(HEADER_BUILD)/pins.h
GEN_PINS_QSTR = $(BUILD)/pins_qstr.h
GEN_PINS_AF_CONST = $(HEADER_BUILD)/pins_af_const.h
GEN_PINS_AF_DEFS = $(HEADER_BUILD)/pins_af_defs.h
GEN_PINS_AF_PY = $(BUILD)/pins_af.py

INSERT_USB_IDS = $(TOP)/tools/insert-usb-ids.py
FILE2H = $(TOP)/tools/file2h.py

USB_IDS_FILE = usb.h
CDCINF_TEMPLATE = pybcdc.inf_template
GEN_CDCINF_FILE = $(HEADER_BUILD)/pybcdc.inf
GEN_CDCINF_HEADER = $(HEADER_BUILD)/pybcdc_inf.h


# List of sources for qstr extraction
# SRC_QSTR += $(SRC_C) $(SRC_CXX) $(SRC_MOD) $(LIB_SRC_C) $(EXTMOD_SRC_C)
SRC_QSTR += $(SRC_C) $(SRC_CXX) $(SRC_MOD) $(SHARED_SRC_C) $(EXTMOD_SRC_C)
# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
#SRC_QSTR_AUTO_DEPS += $(GEN_CDCINF_HEADER)

# Making OBJ use an order-only depenedency on the generated pins.h file
# has the side effect of making the pins.h file before we actually compile
# any of the objects. The normal dependency generation will deal with the
# case when pins.h is modified. But when it doesn't exist, we don't know
# which source files might need it.
$(OBJ): | $(GEN_PINS_HDR)

# With conditional pins, we may need to regenerate qstrdefs.h when config
# options change.
$(HEADER_BUILD)/qstrdefs.generated.h: $(BOARD_DIR)/mpconfigboard.h

# main.c can't be even preprocessed without $(GEN_CDCINF_HEADER)
#main.c: $(GEN_CDCINF_HEADER)

# Use a pattern rule here so that make will only call make-pins.py once to make
# both pins_$(BOARD).c and pins.h
$(BUILD)/%_$(BOARD).c $(HEADER_BUILD)/%.h $(HEADER_BUILD)/%_af_const.h $(HEADER_BUILD)/%_af_defs.h $(BUILD)/%_qstr.h: $(BOARD_DIR)/%.csv $(MAKE_PINS) $(AF_FILE) $(PREFIX_FILE) | $(HEADER_BUILD)
	$(ECHO) "GEN1 $@"
	$(Q)$(PYTHON) $(MAKE_PINS) --board $(BOARD_PINS) --af $(AF_FILE) \
		--prefix $(PREFIX_FILE) --hdr $(GEN_PINS_HDR) --hdr-obj-decls \
		--qstr $(GEN_PINS_QSTR) --af-const $(GEN_PINS_AF_CONST) \
		--af-defs $(GEN_PINS_AF_DEFS) --af-defs-cmp-strings \
		--af-py $(GEN_PINS_AF_PY) > $(GEN_PINS_SRC)

$(BUILD)/pins_$(BOARD).o: $(BUILD)/pins_$(BOARD).c
	$(ECHO) "GEN->10 $@"
	$(Q)$(call compile_c)

GEN_PLLFREQTABLE_HDR = $(HEADER_BUILD)/pllfreqtable.h
GEN_STMCONST_HDR = $(HEADER_BUILD)/modstm_const.h
GEN_STMCONST_QSTR = $(BUILD)/modstm_qstr.h
GEN_STMCONST_MPZ = $(HEADER_BUILD)/modstm_mpz.h
CMSIS_MCU_HDR = $(CMSIS_DIR)/HAL_conf.h

modmachine.c: $(GEN_PLLFREQTABLE_HDR)
$(GEN_PLLFREQTABLE_HDR): $(PLLVALUES) | $(HEADER_BUILD)
	$(ECHO) "GEN2 $@"
	$(Q)$(PYTHON) $(PLLVALUES) -c -m $(MCU_SERIES) file:boards/TKM32/tkm32f499_hal_conf.h > $@
	#end gen2
	

$(BUILD)/modstm.o: $(GEN_STMCONST_HDR)
# Use a pattern rule here so that make will only call make-tkmconst.py once to
# make both modstm_const.h and modstm_qstr.h
$(HEADER_BUILD)/%_const.h $(BUILD)/%_qstr.h: $(CMSIS_MCU_HDR) make-tkmconst.py | $(HEADER_BUILD)
	$(ECHO) "GEN3 stmconst $@"
	$(Q)$(PYTHON) make-tkmconst.py --qstr $(GEN_STMCONST_QSTR) --mpz $(GEN_STMCONST_MPZ) $(CMSIS_MCU_HDR) > $(GEN_STMCONST_HDR)


include $(TOP)/py/mkrules.mk
