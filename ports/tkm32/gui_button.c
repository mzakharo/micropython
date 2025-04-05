
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	gui_button.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/3/25
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
#include "pin.h"
#include "pin_static_af.h"
#include "systick.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"


#include "py/mperrno.h" // used by mp_is_nonblocking_error
#include "py/nlr.h"
#include "py/gc.h"


#if MICROPY_GUI_BUTTON

#include "tp_touch.h"

#include "modtftlcd.h"

#include "global.h" 
#include "gui_button.h"

#if MICROPY_HW_FT54X6
#include "ft54x6.h"
#endif

#if MICROPY_HW_GT1151
#include "gt1151.h"
#endif

//===================================================================================================================
typedef struct _gui_button_obj_t {
	mp_obj_base_t base;
	mp_obj_t callback;
	_btn_obj *gui_btn;  //按钮指针
	bool is_enabled : 1;
	mp_uint_t btn_id : 8;
} gui_button_obj_t;

STATIC uint8_t btn_num = 0;

//====================================================================================================================

STATIC void gui_show_strmid(uint16_t x,uint16_t y,uint16_t width,uint16_t height,color_t color,uint8_t size,char *str)
{
	uint16_t xoff=0,yoff=0;
	uint16_t strlenth;
	uint16_t strwidth;

	if(str==NULL)return;
	strlenth=strlen((const char*)str);	
	strwidth=strlenth*(size>>1);	
	if(strwidth > width-x){
		strlenth = ((width-x)/(size>>1)-1);
		strwidth=strlenth*(size>>1);
	}
	char *upda_str = (char *)m_malloc(strlenth);
	if(upda_str == NULL) printf("malloc err\n");
	memset(upda_str, '\0', strlenth);
	strncpy(upda_str,(const char*)str, strlenth);
	if(height>size)	yoff=(height-y-size)/2;
	if(strwidth<=width-x){
		xoff=(width-x-strwidth)/2;	  
	}
	grap_drawStr(x+xoff,y+yoff,width-1,height-1,size,upda_str,color,lcddev.backcolor);

	m_free(upda_str);
}

void btn_draw(_btn_obj * btnx,uint8_t sta)
{  
	switch(sta)
	{
		case BTN_RELEASE://正常(松开)
			grap_drawFill(btnx->x,btnx->y,btnx->width,btnx->height,btnx->upcolor);
			lcddev.backcolor = btnx->upcolor;
			gui_show_strmid(btnx->x,btnx->y,btnx->width,btnx->height,btnx->fcolor,btnx->font,btnx->label);
		break;
		case BTN_PRES://按下   
			grap_drawFill(btnx->x,btnx->y,btnx->width,btnx->height,btnx->downcolor);
			lcddev.backcolor = btnx->downcolor;
			gui_show_strmid(btnx->x,btnx->y,btnx->width,btnx->height,btnx->fcolor,btnx->font,btnx->label);
		break;
		case BTN_INACTIVE:
		break;
	} 
}
//----------------------------------------------------------------------------------
//画8点(Bresenham算法)		  
void draw_circle8(uint16_t sx,uint16_t sy,int a,int b,color_t color)
{
	grap_drawPoint(sx+a,sy+b,color);
	grap_drawPoint(sx-a,sy+b,color);
	grap_drawPoint(sx+a,sy-b,color);
	grap_drawPoint(sx-a,sy-b,color);
	
	grap_drawPoint(sx+b,sy+a,color);
	grap_drawPoint(sx-b,sy+a,color);
	grap_drawPoint(sx+b,sy-a,color);
	grap_drawPoint(sx-b,sy-a,color);                
}	 
//Bresenham's circle algorithm
void draw_circle(uint16_t xc, uint16_t yc, int r, uint8_t fill, color_t color) {
    if (xc + r < 0 || xc - r >= lcddev.width ||
            yc + r < 0 || yc - r >= lcddev.height) return;
    int x = 0, y = r, yi, d;
    d = 3 - 2 * r;
 
    if (fill) {
        // 如果填充（画实心圆）
        while (x <= y) {
            for (yi = x; yi <= y; yi ++) draw_circle8(xc, yc, x, yi, color);
            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y --;
            }
            x++;
        }
    } else {
        // 如果不填充（画空心圆）
        while (x <= y) {
            draw_circle8( xc, yc, x, y, color);
 
            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y --;
            }
            x ++;
        }
    }
}

//---------------------------------------------------------------------------------
//画圆角按钮
//btnx:按钮
void btn_draw_arcbtn(_btn_obj * btnx,uint8_t sta)
{
	int rlen = 8;//半径固定10
	int r = 0;  
	uint16_t x=0,y=0;
	color_t color = 0;
	color_t backcolor = 0;
	backcolor = lcddev.backcolor;
	if(sta == BTN_PRES || sta==BTN_RELEASE)
	{
		switch(sta)
		{
			case BTN_RELEASE://正常(松开)
				color = btnx->upcolor;
				lcddev.backcolor = btnx->upcolor;
			break;
			case BTN_PRES://按下   
				lcddev.backcolor = btnx->downcolor;
				color = btnx->downcolor;
			break;
		} 
		r = rlen;
		x=btnx->x+rlen;
		y=btnx->y+rlen;
		draw_circle(x,y,r-1,1,color);  //左上角
		x=btnx->width-rlen;
		draw_circle(x,y,r-1,1,color);  //右上角
		x=btnx->x+rlen;
		y=btnx->height-rlen;
		draw_circle(x,y,r-1,1,color);  //左下角
		x=btnx->width-rlen;
		y=btnx->height-rlen;
		draw_circle(x,y,r-1,1,color);  //右下角

		grap_drawFill(btnx->x+rlen,btnx->y,btnx->width-rlen,btnx->y+rlen,color);
		grap_drawFill(btnx->x,btnx->y+rlen,btnx->width,btnx->height-rlen,color); 
		grap_drawFill(btnx->x+rlen,btnx->height-rlen,btnx->width-rlen,btnx->height,color);
		gui_show_strmid(btnx->x,btnx->y,btnx->width,btnx->height,btnx->fcolor,btnx->font,btnx->label);
		lcddev.backcolor = backcolor;
	}
}
//---------------------------------------------------------------------------------
STATIC void btn_deinit_arcbtn(_btn_obj * btnx){
	grap_drawFill(btnx->x,btnx->y,btnx->width,btnx->height,lcddev.clercolor);
}

//---------------------------------------------------------------------------------
uint8_t find_font(uint16_t w,uint16_t h,char *str)
{
	uint8_t font_arg[4]={16,24,32,48};

	uint8_t font_len = strlen(str);
	int i=0;
	for(i=0;i<4;i++){
		if(((font_arg[i]>>1) * font_len) >= w) break;
	}
	for(;i>=0;--i){
		if(font_arg[i] <= h) break;
	}
	if(i!=0) i--;
	return font_arg[i];
}
//==============================================================================	
STATIC int btn_indev(uint16_t sx,uint16_t sy)
{
	int event_id = -1;
	uint8_t i = 0;
	gui_button_obj_t *self;
	for(i = 0; i < btn_num; i++){
		self = MP_STATE_PORT(gui_btn_obj_all)[i];
		if((sx >= self->gui_btn->x && sx <= self->gui_btn->width) && 
		(sy >= self->gui_btn->y &&  sy <= self->gui_btn->height)){
			event_id = self->gui_btn->id;
			break;
		}
	}
	//printf("-----------------------------------------------------------\n");
	return event_id;
}
//--------------------------------------------------------------------------------------
STATIC void btn_callback(gui_button_obj_t *self) {
    if (self->callback != mp_const_none) {
      mp_sched_lock();
        gc_lock();
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_1(self->callback, self);
            nlr_pop();
        } else {
            self->callback = mp_const_none;
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
        mp_sched_unlock();
    }
}
//-------------------------------------------------------------------------------

void button_task(void)
{
	STATIC bool btn_flag = false;
	STATIC uint16_t lastX = 0;
	STATIC uint16_t lastY = 0;
	STATIC int get_id = -1;
	STATIC int last_id = -1;
	gui_button_obj_t *self;

	if (tp_dev.sta==TP_PRES_DOWN) {
		lastX = tp_dev.x[0];
		lastY = tp_dev.y[0];
		if(btn_flag==0){
			get_id = btn_indev(lastX,lastY);
			if(get_id >= 0){
				btn_flag = true;
				self = MP_STATE_PORT(gui_btn_obj_all)[get_id];
				btn_draw_arcbtn(self->gui_btn,BTN_PRES);
			}
		}
	} else if (tp_dev.sta==TP_CATH_UP) {
		if(btn_flag){
			btn_flag = false;
			lastX = tp_dev.x[0];
			lastY = tp_dev.y[0];
			last_id = btn_indev(lastX,lastY);
			self = MP_STATE_PORT(gui_btn_obj_all)[get_id];
			btn_draw_arcbtn(self->gui_btn,BTN_RELEASE);
			if(get_id == last_id && get_id >= 0){
				btn_callback(self);
			}
		}
		tp_dev.sta=TP_INACTIVE;
	} 
}

STATIC mp_obj_t gui_button_task_handler(size_t n_args, const mp_obj_t *args) {
	button_task();
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gui_button_task_handler_obj, 0, 1, gui_button_task_handler);

//-------------------------------------------------------------------------------------
STATIC mp_obj_t gui_button_info(mp_obj_t self_in) {
	gui_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
	printf("<id:%d,name:%s,x:%d,y:%d,width:%d,height:%d>\n",self->btn_id,self->gui_btn->label,self->gui_btn->x,self->gui_btn->y,
		self->gui_btn->width-self->gui_btn->x,self->gui_btn->height-self->gui_btn->y);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(gui_button_info_obj, gui_button_info);

//-------------------------------------------------------------------------------------
STATIC mp_obj_t gui_button_id(mp_obj_t self_in) {
	gui_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
	return mp_obj_new_int(self->btn_id);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(gui_button_id_obj, gui_button_id);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STATIC void gui_button_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	gui_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
	mp_printf(print, "<ID:%d label:%s>", self->btn_id, self->gui_btn->label);
}
//----------------------------------------------------------------------------------
STATIC mp_obj_t gui_button_deinit(mp_obj_t self_in) {
	uint8_t i = 0;
	gui_button_obj_t *self;
	if(btn_num){
		for(i = 0; i < btn_num; i++){
			self = MP_STATE_PORT(gui_btn_obj_all)[i];
			self->callback = mp_const_none;
			btn_deinit_arcbtn(self->gui_btn);
			m_free(self);
		}
		btn_num = 0;
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(gui_button_deinit_obj, gui_button_deinit);
//----------------------------------------------------------------------------------
STATIC mp_obj_t gui_button_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	STATIC const mp_arg_t allowed_args[] = {
		{ MP_QSTR_x0,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y0,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_width,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_height,   MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_btcolor,   	MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_text, 		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_label_color,   	MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
		
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(btn_num >= GUI_BTN_NUM_MAX){
		mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Button(%d) doesn't exist"), btn_num + 1);
	}
	color_t bt_color=0,label_color=0;
	size_t len;
	mp_obj_t *params;
	mp_buffer_info_t bufinfo;
	if(args[4].u_obj !=MP_OBJ_NULL) 
	{
		mp_obj_get_array(args[4].u_obj, &len, &params);
		if(len == 3){
			bt_color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("Button color parameter error \n"));
		}
	}
	if(args[6].u_obj !=MP_OBJ_NULL) 
	{
		mp_obj_get_array(args[6].u_obj, &len, &params);
		if(len == 3){
			label_color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("Button label color parameter error \n"));
		}
	}

	if(args[5].u_obj !=MP_OBJ_NULL) 
	{
		if (mp_obj_is_int(args[5].u_obj)) {
			mp_raise_ValueError(MP_ERROR_TEXT("button text parameter error"));
		} else {
			mp_get_buffer_raise(args[5].u_obj, &bufinfo, MP_BUFFER_READ);
		}
	}else{
		mp_raise_ValueError(MP_ERROR_TEXT("button text parameter is empty"));
	}
	
	char *label = bufinfo.buf;
	uint8_t font_size = find_font(args[2].u_int,args[3].u_int,label);
	gui_button_obj_t *btn;

	if (MP_STATE_PORT(gui_btn_obj_all)[btn_num] == NULL) {
		btn = m_new_obj(gui_button_obj_t);
		btn->base.type = &gui_button_type;
		btn->btn_id = btn_num;
		btn->is_enabled = true;
		btn->gui_btn = (_btn_obj*)m_malloc(sizeof(_btn_obj));
		btn->gui_btn->x = args[0].u_int;
		btn->gui_btn->y = args[1].u_int;
		btn->gui_btn->width = args[2].u_int+args[0].u_int;
		btn->gui_btn->height = args[3].u_int+args[1].u_int;
		btn->gui_btn->id = btn_num;
		btn->gui_btn->font = font_size;
		btn->gui_btn->fcolor = label_color;
		btn->gui_btn->upcolor = bt_color;
		btn->gui_btn->downcolor = bt_color>>1;
		btn->gui_btn->label =(char *)m_malloc(strlen(label));
		btn->gui_btn->label =label;

		if (args[7].u_obj == mp_const_none || args[7].u_obj ==MP_OBJ_NULL) {
			btn->callback = mp_const_none;
		} else if (mp_obj_is_callable(args[7].u_obj)) {
			btn->callback = args[7].u_obj;
		} else {
			nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,MP_ERROR_TEXT("callback must be None or a callable object")));
		}
		btn_draw_arcbtn(btn->gui_btn,BTN_RELEASE);
		MP_STATE_PORT(gui_btn_obj_all)[btn_num] = btn;
	}else {
		btn = MP_STATE_PORT(gui_btn_obj_all)[btn_num];
	}
	btn_num++;

   return MP_OBJ_FROM_PTR(btn);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t gui_button_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_TouchButton) },
	{ MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&gui_button_info_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&gui_button_deinit_obj) },

	{ MP_ROM_QSTR(MP_QSTR_ID), MP_ROM_PTR(&gui_button_id_obj) },

};
STATIC MP_DEFINE_CONST_DICT(gui_button_locals_dict,gui_button_locals_dict_table);

const mp_obj_type_t gui_button_type = {
    { &mp_type_type },
    .name = MP_QSTR_TouchButton,
    .print = gui_button_print,
    .make_new = gui_button_make_new,
    .locals_dict = (mp_obj_dict_t *)&gui_button_locals_dict,
};

#endif

