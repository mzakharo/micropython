/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_espdrone.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/4/20
	* Description				:	
********************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"
#include "mod_espdrone.h"
#include "mod_wifllink.h"
#include "mod_drone.h"

#if MICROPY_ENABLE_ESP_DRONE

STATIC const mp_rom_map_elem_t espdrone_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_espdrone) },
	{ MP_ROM_QSTR(MP_QSTR_DRONE), MP_ROM_PTR(&drone_drone_type) },
};

STATIC MP_DEFINE_CONST_DICT(espdrone_module_globals, espdrone_module_globals_table);

const mp_obj_module_t espdrone_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&espdrone_module_globals,
};

/*******************************************************************************/

#endif 





