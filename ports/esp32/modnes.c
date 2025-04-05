
/********************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modnes.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/8/05
	* Description			:	
******************************************************************************/

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

#if MICROPY_HW_NESEMU

#if MICROPY_HW_NESEMU
#include "app_nes_start.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#include "global.h"
//===================================================================================================================
typedef struct _nesemu_obj_t {
  mp_obj_base_t base;
}nesemu_obj_t;

//=====================================================================================================================
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t nesemu_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t nesemu_allowed_args[] = { 
    { MP_QSTR_file,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	{ MP_QSTR_reserve,  MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(nesemu_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, nesemu_allowed_args, args);

  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter error"));
    }else{
		mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

		const char *file_path = (const char *)bufinfo.buf;
		const char *ftype = mp_obj_str_get_str(file_type(file_path));

		if(strncmp(ftype,"nes",3) == 0 || strncmp(ftype,"NES",4) == 0){
			#if MICROPY_HW_NESEMU
			app_nes_start(file_path);
			#endif
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("input nes file type error"));
			return mp_const_none;
		}
    }
  }else{
      mp_raise_ValueError(MP_ERROR_TEXT("nes parameter is empty"));
  }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nesemu_start_obj, 1, nesemu_start);

//----------------------------------------------------------------------------------
STATIC mp_obj_t nesemu_deinit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

vTaskDelay(2000 / portTICK_RATE_MS);

return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nesemu_deinit_obj,0, nesemu_deinit);

//----------------------------------------------------------------------------------
STATIC mp_obj_t nesemu_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
	
	nesemu_obj_t *nesemu_obj;
	nesemu_obj = m_new_obj(nesemu_obj_t);
	nesemu_obj->base.type = type;

	return MP_OBJ_FROM_PTR(nesemu_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t nesemu_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_game_nes) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&nesemu_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&nesemu_start_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nesemu_locals_dict,nesemu_locals_dict_table);

const mp_obj_type_t game_nesemu_type = {
    { &mp_type_type },
    .name = MP_QSTR_NES,
    .make_new = nesemu_make_new,
    .locals_dict = (mp_obj_dict_t *)&nesemu_locals_dict,
};

#endif

