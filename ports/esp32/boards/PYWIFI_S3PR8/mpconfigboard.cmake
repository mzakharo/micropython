set(IDF_TARGET esp32s3)
#    boards/sdkconfig.usb

set(MICROPY_PORT_PICLIB y) #
set(MICROPY_PORT_CAMLIB y) #CAM
set(MICROPY_PORT_USB_CAM y) #UVCCAM
set(MICROPY_PORT_WEB_STREAM y) #WEB stream

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
	boards/sdkconfig.ble
    boards/PYWIFI_S3PR8/sdkconfig.board
	boards/sdkconfig.cam
)

if(NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/boards/manifest.py)
endif()
