
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modespusb.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/8/05
	* Description			:	
******************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "modmachine.h"
#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"

#if MICROPY_HW_USB_CAM
#include "modusbcam.h" 
#endif

#if MICROPY_ENABLE_ESP_USB

STATIC const mp_rom_map_elem_t esp_usb_module_globals_table[] = {
  { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_esp_usb) },
	
	#if MICROPY_HW_USB_CAM
	{ MP_ROM_QSTR(MP_QSTR_CAM), MP_ROM_PTR(&usbcam_type) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(esp_usb_module_globals, esp_usb_module_globals_table);

const mp_obj_module_t esp_usb_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&esp_usb_module_globals,
};

/*******************************************************************************/

#endif 
