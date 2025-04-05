
set(IDF_TARGET esp32s3)
#boards/sdkconfig.usb
set(MICROPY_PORT_PICLIB y) #
set(MICROPY_PORT_NESEMU y) #

set(SDKCONFIG_DEFAULTS
	boards/sdkconfig.base
	boards/sdkconfig.usb
	boards/sdkconfig.ble
	boards/PYCONTROLLER/sdkconfig.board
)
if(NOT MICROPY_FROZEN_MANIFEST)
  set(MICROPY_FROZEN_MANIFEST ${MICROPY_BOARD_DIR}/../manifest.py)
endif()
