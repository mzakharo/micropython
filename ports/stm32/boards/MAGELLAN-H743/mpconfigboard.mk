USE_MBOOT ?= 0

# MCU settings
MCU_SERIES = h7
CMSIS_MCU = STM32H743xx
MICROPY_FLOAT_IMPL = double
AF_FILE = boards/stm32h743_af_01.csv

ifeq ($(USE_MBOOT),1)
# When using Mboot all the text goes together after the filesystem
LD_FILES = boards/stm32h743.ld boards/common_blifs.ld
TEXT0_ADDR = 0x08040000
else
# When not using Mboot the ISR text goes first, then the rest after the filesystem
LD_FILES = boards/MAGELLAN-H743/magellam.ld boards/MAGELLAN-H743/common_ifs.ld
#LD_FILES = boards/MAGELLAN/stm32fxxx.lds
TEXT0_ADDR = 0x08000000
TEXT1_ADDR = 0x08040000
endif

#01studio
MICROPY_PY_PICLIB = 1
MICROPY_PY_MP3	  = 1
MICROPY_PY_VIDEO  = 1
# MicroPython settings
MICROPY_PY_LWIP = 1
MICROPY_PY_USSL = 1
MICROPY_SSL_MBEDTLS = 1
