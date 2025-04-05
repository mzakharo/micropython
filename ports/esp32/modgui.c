
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modgui.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/20
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
