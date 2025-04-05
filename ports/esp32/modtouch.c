
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modtouch.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/3/18
	* Description			:	
******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/objlist.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/netutils/netutils.h"

#include "modtouch.h"

#if MICROPY_HW_GT1151

#endif

#if MICROPY_HW_FT54X6

#endif


#if MICROPY_ENABLE_TOUCH

TP_DEV tp_dev;

static uint16_t pre_x[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_y[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_w[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint8_t s_tp_dowm[TOUCH_MAX_TOUCH] ={0};

void tp_touch_up(int8_t id)
{
	if(s_tp_dowm[id] == 1) //EVENT_UP
	{
		s_tp_dowm[id] = 0;
		tp_dev.sta = TP_CATH_UP;
		tp_dev.x[id]=pre_x[id];
		tp_dev.y[id]=pre_y[id];
	}else //EVENT_NON
	{
		tp_dev.sta = TP_INACTIVE;
		pre_x[id] = 0;  /* last point is none */
		pre_y[id] = 0;
		pre_w[id] = 0;
	}
}

void tp_touch_down(int8_t id, uint16_t x, uint16_t y, uint8_t w)
{
	if (s_tp_dowm[id] == 1){
		if((x != (tp_dev.x[id]-1)||x != (tp_dev.x[id])||x != (tp_dev.x[id]+1)) || 
			(y != (tp_dev.y[id]-1)||y != (tp_dev.y[id])||y != (tp_dev.y[id]+1))){
		//if(x != tp_dev.x[id] || y != tp_dev.y[id]){
			tp_dev.sta = TP_PRES_MOVE;
		}
		else{
			tp_dev.sta = TP_PRES_DOWN;
		}
	}else{
		tp_dev.sta = TP_PRES_DOWN;
		s_tp_dowm[id] = 1;
	}
	tp_dev.x[id]=x;
	tp_dev.y[id]=y;
	
	pre_x[id] = x; /* save last point */
	pre_y[id] = y;
	pre_w[id] = w;
}
//---------------------------------------------------------

STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_touch) },

		#if MICROPY_HW_XPT2046
		{ MP_ROM_QSTR(MP_QSTR_XPT2046), MP_ROM_PTR(&touch_xpt2046_type) },
		#endif
		
};
STATIC MP_DEFINE_CONST_DICT(touch_module_globals, touch_module_globals_table);

const mp_obj_module_t touch_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&touch_module_globals,
};

/*******************************************************************************/

#endif  // 
