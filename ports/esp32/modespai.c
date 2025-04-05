/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	modespai.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "modmachine.h"

#if MICROPY_HW_ESPAI

#include "modespai.h"

face_detection_t pFdetection;
color_detection_t pCdetection;

#if MICROPY_ENABLE_FACE_DETECTION || MICROPY_ENABLE_CAT_DETECTION || MICROPY_ENABLE_MOTION_DETECTION
#include "mod_face_detection.h"
#endif
#if MICROPY_ENABLE_CODE_RECOGNITION
#include "mod_code_recognition.h"
#endif

#if MICROPY_ENABLE_FACE_RECOGNITION
#include "mod_face_recognition.h"
#endif

#if MICROPY_ENABLE_COLOR_DETECTION
#include "mod_color_detection.h"
#endif
//-----------------------------------------------------------------------------------

STATIC const mp_rom_map_elem_t espai_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_espai) },
	#if MICROPY_ENABLE_FACE_DETECTION
	{ MP_ROM_QSTR(MP_QSTR_face_detection), MP_ROM_PTR(&face_detection_type) },
	#endif
	
	#if MICROPY_ENABLE_CAT_DETECTION
	{ MP_ROM_QSTR(MP_QSTR_cat_detection), MP_ROM_PTR(&cat_detection_type) },
	#endif
	#if MICROPY_ENABLE_CODE_RECOGNITION
	{ MP_ROM_QSTR(MP_QSTR_code_recognition), MP_ROM_PTR(&code_recognition_type) },
	#endif
	#if MICROPY_ENABLE_MOTION_DETECTION
	{ MP_ROM_QSTR(MP_QSTR_motion_recognition), MP_ROM_PTR(&motion_detection_type) },
	#endif
	#if MICROPY_ENABLE_FACE_RECOGNITION
	{ MP_ROM_QSTR(MP_QSTR_face_recognition), MP_ROM_PTR(&face_recognition_type) },
	#endif
	
	#if MICROPY_ENABLE_COLOR_DETECTION
	{ MP_ROM_QSTR(MP_QSTR_color_detection), MP_ROM_PTR(&color_detection_type) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(espai_module_globals, espai_module_globals_table);

const mp_obj_module_t espai_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&espai_module_globals,
};

#endif

