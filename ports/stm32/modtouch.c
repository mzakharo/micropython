/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://bbs.01studio.org/
	* Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
	* File Name					:	modtouch.c
	* Author					:	spring
	* Version					:	v1.0
	* date						:	2020/12/01
	* Description				:	
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

#if MICROPY_ENABLE_TOUCH

#include "modtouch.h"

#if MICROPY_HW_GT1151
#include "gt1151.h"
#endif
#if MICROPY_HW_FT54X6
#include "ft54x6.h"
#endif
#if MICROPY_HW_GT911
#include "gt911.h"
#endif
TP_DEV tp_dev;

bool touch_is_init = 0;

static uint16_t pre_x[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_y[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_w[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint8_t s_tp_dowm[TOUCH_MAX_TOUCH] ={0};

void tp_touch_up(int8_t id)
{
    if(s_tp_dowm[id] == 1) //EVENT_UP
    {
        s_tp_dowm[id] = 1;
        tp_dev.sta = TP_CATH_UP;
				tp_dev.x[id]=pre_x[id];
				tp_dev.y[id]=pre_y[id];
				//printf("UP->%d,%d\n",tp_dev.x[id],tp_dev.y[id]);
    }
    else //EVENT_NON
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
    	if(x != tp_dev.x[id]||y != tp_dev.y[id]) tp_dev.sta = TP_PRES_MOVE;
			else{
				tp_dev.sta = TP_PRES_DOWN;
				//printf("DOWN->%d,%d\n",tp_dev.x[id],tp_dev.y[id]);
			}
    }
    else{
       	tp_dev.sta = TP_PRES_DOWN;
        s_tp_dowm[id] = 1;
    }
		tp_dev.x[id]=x;
		tp_dev.y[id]=y;
		
    pre_x[id] = x; /* save last point */
    pre_y[id] = y;
    pre_w[id] = w;
}

void gui_read_points(void)
{
	//1:FT5416,2:GT911	,3 XPT2046, 4:gt1151
		switch (tp_dev.type)
	{
		case 1:
		#if MICROPY_HW_FT54X6
		ft54x6_read_point();
		#endif
		break;
		case 2:
		#if MICROPY_HW_GT911
		gt911_read_point();
		#endif
		break;
		case 3:
		#if MICROPY_HW_XPT2046
		xpt_read_point();
		#endif
		break;
		case 4:
		#if MICROPY_HW_GT1151
		gtxx_read_point();
		#endif
		break;
	}
	
}
STATIC const mp_rom_map_elem_t touch_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_touch) },
    #if MICROPY_HW_GT1151
		{ MP_ROM_QSTR(MP_QSTR_GT1151), MP_ROM_PTR(&touch_gt1151_type) },
		#endif
		#if MICROPY_HW_XPT2046
		{ MP_ROM_QSTR(MP_QSTR_XPT2046), MP_ROM_PTR(&touch_xpt2046_type) },
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

#endif  // MICROPY_PY_NETWORK
