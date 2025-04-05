set(IDF_TARGET esp32s2)

# 配置定义支持图片解码库,y支持，否则不支持
set(MICROPY_PORT_PICLIB y) #
set(MICROPY_PORT_CAMLIB y) #CAM
set(MICROPY_PORT_USB_CAM y) #UVCCAM
set(MICROPY_PORT_WEB_STREAM y) #WEB stream

set(SDKCONFIG_DEFAULTS
    boards/PYWIFI_S2P/sdkconfig.board
)

if(NOT MICROPY_FROZEN_MANIFEST)
    set(MICROPY_FROZEN_MANIFEST ${MICROPY_PORT_DIR}/boards/manifest.py)
endif()
