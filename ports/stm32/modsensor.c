
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://bbs.01studio.org/
	* Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
	* File Name 				 :	sensor.c
	* Author						 :	spring
	* Version 					 :	v1.0
	* date							 :	2020/12/02
	* Description 			 :	
	******************************************************************************
**/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/objlist.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/netutils/netutils.h"
#include "systick.h"
#include "pendsv.h"
#include "portmodules.h"

#if MICROPY_ENABLE_SENSOR

#if MICROPY_HW_OV2640
#include "ov2640.h"
#endif


STATIC const mp_rom_map_elem_t sensor_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sensor) },
        // Other
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQQVGA),              MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQQVGA)},   /* 80x60     */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QQVGA),               MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQVGA)},    /* 160x120   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_QVGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QVGA)},     /* 320x240   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_VGA),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_VGA)},      /* 640x480   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_WVGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WVGA)},     /* 720x480   */
    { MP_OBJ_NEW_QSTR(MP_QSTR_XGA),                 MP_OBJ_NEW_SMALL_INT(FRAMESIZE_XGA)},      /* 1024x768  */
    { MP_OBJ_NEW_QSTR(MP_QSTR_UXGA),                MP_OBJ_NEW_SMALL_INT(FRAMESIZE_UXGA)},     /* 1600x1200 */

	#if MICROPY_HW_OV2640
	{ MP_ROM_QSTR(MP_QSTR_OV2640), MP_ROM_PTR(&sensor_ov2640_type) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(sensor_module_globals, sensor_module_globals_table);

const mp_obj_module_t sensor_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&sensor_module_globals,
};

/*******************************************************************************/

#endif  // MICROPY_ENABLE_SENSOR
