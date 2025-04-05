/**
  ******************************************************************************
  *	This file is part of the MicroPython project, http://bbs.01studio.org/
  * Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
  * File Name          :	modmagellan.c
  * Author             :	spring
  * Version            :	v1.0
  * date               :	2020/11/27
  * Description        :	lcd dev
  ******************************************************************************
**/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"
#include "drivers/dht/dht.h"
#include "stm32_it.h"
#include "storage.h"

#include "portmodules.h"
#include "modmachine.h"
#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"

#if MICROPY_ENABLE_MAGELLAN_MODULE

STATIC const mp_rom_map_elem_t magellan_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_magellan) },
		
	#if MICROPY_HW_ENABLE_MAGELLAN_CAN
    { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&pyb_can_type) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(magellan_module_globals, magellan_module_globals_table);

const mp_obj_module_t magellan_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&magellan_module_globals,
};
#endif
