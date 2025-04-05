
/*
******************************************************************************
* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
* File Name				: modespusb.c
* Author				: Folktale
* Version				: v1.0
* date					: 2022/1/10
* Description			:	
******************************************************************************
*/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "modmachine.h"
#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"

#if MICROPY_HW_NESEMU
#include "modnes.h" 
#endif

#if MICROPY_ENABLE_GAME

STATIC const mp_rom_map_elem_t game_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_game) },

	#if MICROPY_HW_NESEMU
	{ MP_ROM_QSTR(MP_QSTR_NES), MP_ROM_PTR(&game_nesemu_type) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(game_module_globals, game_module_globals_table);

const mp_obj_module_t game_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&game_module_globals,
};

/*******************************************************************************/

#endif 
