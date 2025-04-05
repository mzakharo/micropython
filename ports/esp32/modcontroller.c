/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	modcontroller.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "extmod/vfs.h"
//#include "extmod/utime_mphal.h"

#include "modcontroller.h"

#if MICROPY_ENABLE_CONTROLLER

#if MICROPY_HW_GAMEPAD
#include "modgamepad.h"
#endif
#define STATIC static

STATIC const mp_rom_map_elem_t controller_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_controller) },

	#if MICROPY_HW_GAMEPAD
	{ MP_ROM_QSTR(MP_QSTR_CONTROLLER), MP_ROM_PTR(&controller_gamepad_type) },
	#endif
};

STATIC MP_DEFINE_CONST_DICT(controller_module_globals, controller_module_globals_table);

const mp_obj_module_t controller_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&controller_module_globals,
};

/*******************************************************************************/

#endif 





