
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://bbs.01studio.org/
	* Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
	* File Name 				 :	modvideo.c
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

#if MICROPY_ENABLE_VIDEO

#include "video.h"


STATIC const mp_rom_map_elem_t video_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_video) },
		{ MP_ROM_QSTR(MP_QSTR_VIDEO), MP_ROM_PTR(&video_video_type) },
};
STATIC MP_DEFINE_CONST_DICT(video_module_globals, video_module_globals_table);

const mp_obj_module_t video_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&video_module_globals,
};

/*******************************************************************************/

#endif  // MICROPY_ENABLE_SENSOR
