
/********************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modsensor.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/22
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

#if MICROPY_HW_OV2640
#include "modov2640.h"
#endif

#if MICROPY_ENABLE_SENSOR

#include "esp_camera.h"

STATIC const mp_rom_map_elem_t sensor_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sensor) },
	
	//{ MP_OBJ_NEW_QSTR(MP_QSTR_QQQVGA), MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQQVGA)},   /* 80x60     */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_QQVGA),  MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QQVGA)},		/* 160x120   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_LCD),		MP_OBJ_NEW_SMALL_INT(FRAMESIZE_LCD)},	/* 240x240   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_QVGA),   MP_OBJ_NEW_SMALL_INT(FRAMESIZE_QVGA)},		/* 320x240   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_VGA),    MP_OBJ_NEW_SMALL_INT(FRAMESIZE_VGA)},		/* 640x480   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_WVGA),   MP_OBJ_NEW_SMALL_INT(FRAMESIZE_WVGA)},		/* 720x480   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_XGA),    MP_OBJ_NEW_SMALL_INT(FRAMESIZE_XGA)},		/* 1024x768  */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_UXGA),   MP_OBJ_NEW_SMALL_INT(FRAMESIZE_UXGA)},		/* 1600x1200 */
	
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

#endif 
