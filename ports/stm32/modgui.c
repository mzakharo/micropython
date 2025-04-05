
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://bbs.01studio.org/
	* Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
	* File Name 				 :	modtouch.c
	* Author						 :	spring
	* Version 					 :	v1.0
	* date							 :	2020/12/02
	* Description 			 :	
	******************************************************************************
**/

#include <stdint.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"
#include "stm32_it.h"
#include "irq.h"
#include "timer.h"
#include "extint.h"
#include "usrsw.h"
#include "rng.h"
#include "rtc.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"
#include "storage.h"
#include "sdcard.h"
#include "usb.h"
#include "portmodules.h"
#include "modmachine.h"
#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"

#if MICROPY_GUI_BUTTON
#include "gui_button.h"
#endif

#if MICROPY_ENABLE_GUI
STATIC const mp_rom_map_elem_t gui_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gui) },
#if MICROPY_GUI_BUTTON
{ MP_ROM_QSTR(MP_QSTR_TouchButton), MP_ROM_PTR(&gui_button_type) },
{ MP_ROM_QSTR(MP_QSTR_task_handler), MP_ROM_PTR(&gui_button_task_handler_obj) },
#endif

};
STATIC MP_DEFINE_CONST_DICT(gui_module_globals, gui_module_globals_table);

const mp_obj_module_t gui_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&gui_module_globals,
};

/*******************************************************************************/

#endif  // MICROPY_ENABLE_GUI
