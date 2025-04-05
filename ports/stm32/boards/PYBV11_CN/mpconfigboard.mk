MCU_SERIES = f4
CMSIS_MCU = STM32F405xx
AF_FILE = boards/stm32f405_af.csv
ifeq ($(USE_MBOOT),1)
# When using Mboot all the text goes together after the filesystem
LD_FILES = boards/stm32f405.ld boards/common_blifs.ld
TEXT0_ADDR = 0x08020000
else
# When not using Mboot the ISR text goes first, then the rest after the filesystem
LD_FILES = boards/stm32f405.ld boards/common_ifs.ld
TEXT0_ADDR = 0x08000000
TEXT1_ADDR = 0x08020000
endif

# MicroPython settings
MICROPY_VFS_LFS2 = 1
MICROPY_PY_WIZNET5K = 5500
MICROPY_PY_LWIP = 1
MICROPY_PY_USSL = 1
MICROPY_SSL_MBEDTLS = 1

#01studio
MICROPY_PY_PICLIB = 1
