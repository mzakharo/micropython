set(IDF_TARGET esp32s3)

set(MICROPY_PORT_DRONE y) #CAM

set(MICROPY_PORT_PICLIB y) #
set(MICROPY_PORT_CAMLIB y) #CAM
set(MICROPY_PORT_WEB_STREAM y) #WEB stream
# boards/sdkconfig.usb
set(SDKCONFIG_DEFAULTS
	boards/sdkconfig.ble
	boards/sdkconfig.usb
    boards/PYDRONE/sdkconfig.board
	boards/sdkconfig.cam
)

if(NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/boards/manifest.py)
endif()
