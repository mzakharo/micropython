
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	tp_touch.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/5/11
	* Description 			 :	
	******************************************************************************
**/
#include "py/runtime.h"
#include "py/mphal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "py/obj.h"
#include "tp_touch.h"

TP_DEV tp_dev;

#if MICROPY_ENABLE_TOUCH

static uint16_t pre_x[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_y[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint16_t pre_w[TOUCH_MAX_TOUCH] = {0, 0, 0, 0, 0};
static uint8_t s_tp_dowm[TOUCH_MAX_TOUCH] ={0};

void gtxx_touch_up(int8_t id)
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

void gtxx_touch_down(int8_t id, uint16_t x, uint16_t y, uint8_t w)
{
    if (s_tp_dowm[id] == 1){
    	if(x != tp_dev.x[id] || y != tp_dev.y[id]){
				tp_dev.sta = TP_PRES_MOVE;
			}
			else{
				tp_dev.sta = TP_PRES_DOWN;
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
#endif
/**********************************************************************************************************/
