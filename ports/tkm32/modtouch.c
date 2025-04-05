/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	modtouch.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
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

#if MICROPY_HW_GT1151
#include "gt1151.h"
#endif
#if MICROPY_HW_FT54X6
#include "ft54x6.h"
#endif
#include "tp_touch.h"
#if MICROPY_ENABLE_TOUCH

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_touch) },
    #if MICROPY_HW_GT1151
		{ MP_ROM_QSTR(MP_QSTR_GT1151), MP_ROM_PTR(&touch_gt1151_type) },
		#endif
		#if MICROPY_HW_FT54X6
		{ MP_ROM_QSTR(MP_QSTR_FT5436), MP_ROM_PTR(&touch_ft_type) },
		#endif
		#if MICROPY_HW_GT911
		{ MP_ROM_QSTR(MP_QSTR_GT911), MP_ROM_PTR(&touch_gt911_type) },
		#endif
		
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&touch_module_globals,
};

/*******************************************************************************/

#endif  // 
