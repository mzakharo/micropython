/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	modgamepad.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/
#define STATIC static

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

#if MICROPY_HW_GAMEPAD

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "global.h"

#if MICROPY_ENABLE_PSXCONTROLLER
#include "psxcontroller.h"
#endif

//===================================================================================================================
typedef struct _gamepad_obj_t {
  mp_obj_base_t base;
}gamepad_obj_t;

//=====================================================================================================================
//---------------------------华丽的分割线-------------------------------------------------------------------
/****************************************************************
 ____________________________________________________________
|字节0|字节1  | 字节2 | 字节3 | 字节4 | 字节5 | 字节6 |字节7|
|-----|-------|-------|-------|-------|-------|-------|-----|
| ID  |左X轴值|左Y轴值|右X轴值|右Y轴值|Buttons|Buttons| MODE|
|-----|-------|-------|-------|-------|-------|-------|-----|

Byte5:
buttons - right cluster buttons
bit7	X
bit6	A
bit5	B
bit4	Y

VALUE	KEY
0	up
1	right&up
2	right
3	right&down
4	down
5	left&down
6	left
7	left&up
8	no action

Byte6:
button
BIT		KEY
bit7	AOK
bit6	BOK
bit5	START
bit4	BACK
bit3	RT
bit2	LT
bit1	RB
bit0	LB
****************************************************************/
#define COEFF	14
STATIC mp_obj_t gamepad_read(mp_obj_t self_in)
{

	uint16_t leftX=0,leftY=0,rightX =0,rightY=0;
	uint8_t keyByte5=0,keyByte6=0;
	
	mp_obj_t tuple[8];
	tuple[0] = mp_obj_new_int(0x01);  //0x01
#if MICROPY_ENABLE_PSXCONTROLLER
	gamepad_get_adc(&leftX,&leftY,&rightX,&rightY);
	gamepad_get_key(&keyByte5,&keyByte6);

	leftX /= COEFF;
	leftY  /= COEFF;
	rightX  /= COEFF;
	rightY /= COEFF;
	if(leftX >= 255) leftX = 255;
	if(leftY >= 255) leftY = 255;
	if(rightX >= 255) rightX = 255;
	if(rightY >= 255) rightY = 255;
	
	tuple[1] = mp_obj_new_int(leftX);
	tuple[2] = mp_obj_new_int(leftY);
	tuple[3] = mp_obj_new_int(rightX);
	tuple[4] = mp_obj_new_int(rightY);
	
	tuple[5] = mp_obj_new_int(keyByte5);
	tuple[6] = mp_obj_new_int(keyByte6);
#else
	tuple[1] = mp_obj_new_int(0);
	tuple[2] = mp_obj_new_int(0);
	tuple[3] = mp_obj_new_int(0);
	tuple[4] = mp_obj_new_int(0);
	tuple[5] = mp_obj_new_int(8);
	tuple[6] = mp_obj_new_int(0);
#endif
	tuple[7] = mp_obj_new_int(0x06);
	return mp_obj_new_tuple(8, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(gamepad_read_obj, gamepad_read);

//----------------------------------------------------------------------------------

STATIC mp_obj_t gamepad_deinit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	vTaskDelay(pdMS_TO_TICKS(2000));

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gamepad_deinit_obj,0, gamepad_deinit);

//----------------------------------------------------------------------------------
STATIC mp_obj_t gamepad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);
	#if MICROPY_ENABLE_PSXCONTROLLER
	psxcontrollerInit();
	#endif
	gamepad_obj_t *gamepad_obj;
	gamepad_obj = m_new_obj(gamepad_obj_t);
	gamepad_obj->base.type = type;

	return MP_OBJ_FROM_PTR(gamepad_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t gamepad_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_game_nes) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&gamepad_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&gamepad_read_obj) },
};
STATIC MP_DEFINE_CONST_DICT(gamepad_locals_dict,gamepad_locals_dict_table);

const mp_obj_type_t controller_gamepad_type;

MP_DEFINE_CONST_OBJ_TYPE(
    controller_gamepad_type,
    MP_QSTR_CONTROLLER,
    MP_TYPE_FLAG_NONE,
    make_new, gamepad_make_new,
    locals_dict, &gamepad_locals_dict
    );

#endif

