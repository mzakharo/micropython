/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name				:	ST7789.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/9/18
	* Description			:	
	******************************************************************************
**/
#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD & MICROPY_ENABLE_SPILCD)
	
#include "lcd_spibus.h"
#include "modtftlcd.h"

#include "ST7789.h"
#include "global.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[15];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

Graphics_Display st7789_glcd;

STATIC lcd_spibus_t *p_st7789 = NULL;

STATIC const lcd_init_cmd_t st7789_init_cmds[]={
	
		{0x3A, {0x05}, 1},  /*Pixel Format Set*/
		{0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5},
		{0xB7, {0x35}, 1},
		{0xBB, {0x32}, 1},	//Vcom=1.35V
		{0xC2, {0x01}, 1},
		{0xC3, {0x15}, 1}, 	//GVDD=4.8V  颜色深度
		{0xC4, {0x20}, 1},  //VDV, 0x20:0v
		{0xC6, {0x0F}, 1}, 	//0x0F:60Hz  
		{0xD0, {0xA4, 0xA1}, 2},
		{0xE0, {0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34}, 14},
		{0XE1, {0xD0, 0x08, 0x0E, 0x09, 0x09, 0x15, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34}, 14},
		{0x21, {0}, 0x80},
		{0x11, {0}, 0x80},
		{0x29, {0}, 0x80},
		{0, {0}, 0xff},
	};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void  mp_init_ST7789(void)
{
	lcd_bus_init();
	p_st7789 = lcd_spibus;
	//Send all the commands
	uint16_t cmd = 0;
	while (st7789_init_cmds[cmd].databytes!=0xff) {
		lcd_spibus_send_cmd(p_st7789, st7789_init_cmds[cmd].cmd);
		lcd_spibus_send_data(p_st7789, st7789_init_cmds[cmd].data, st7789_init_cmds[cmd].databytes & 0x1F);
		if (st7789_init_cmds[cmd].databytes & 0x80) {
			mp_hal_delay_ms(100);
		}
		cmd++;
	} 
}
static void st7789_set_addr(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey)
{
	uint8_t data[4] = {0};
	uint16_t x1=0, x2=0, y1=0, y2=0;

	switch (lcddev.dir)
	{
		case 1:
		case 2:
			x1 = sx;
			x2 = ex;
			y1 = sy;
			y2 = ey;
		break;
		case 3:
			x1 = sx;
			x2 = ex;
			y1 = sy + 80;
			y2 = ey + 80;
		break;
		case 4:
			x1 = sx + 80;
			x2 = ex + 80;
			y1 = sy;
			y2 = ey;
		break;
		default:

		break;
	}

	lcd_spibus_send_cmd(p_st7789, 0x2A);
	data[0] = (x1 >> 8) & 0xFF;
	data[1] = x1 & 0xFF;
	data[2] = ((x2) >> 8) & 0xFF;
	data[3] = (x2) & 0xFF;
	lcd_spibus_send_data(p_st7789, data, 4);
	
	lcd_spibus_send_cmd(p_st7789, 0x2B); //2A
	data[0] = (y1 >> 8) & 0xFF;
	data[1] = y1 & 0xFF;
	data[2] = ((y2) >> 8) & 0xFF;
	data[3] = (y2) & 0xFF;
	lcd_spibus_send_data(p_st7789, data, 4);
}
//设置LCD显示方向
void st7789_set_dir(uint8_t dir)
{
	uint8_t dir_data = 0;
	
	lcddev.dir=dir;		//竖屏

	switch (dir)
		{
		case 2:
		dir_data = 0x70;
		break;
		case 3:
		dir_data = 0xC0;
		break;
		case 4:
		dir_data = 0xA0;
		break;
		default:
		dir_data = 0x00;
		break;
		}

	uint8_t data[2] = {0};

	lcd_spibus_send_cmd(p_st7789, 0x36);
	data[0] = dir_data;
	lcd_spibus_send_data(p_st7789, data, 1);

	st7789_set_addr(0, 0, lcddev.width-1, lcddev.height-1);

}	 

//画点
void st7789_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
	uint8_t data[2];

	st7789_set_addr(x,y,x,y);

	/*Memory write*/
	lcd_spibus_send_cmd(p_st7789, 0x2C);
	data[0] = (color >> 8);
	data[1] = (color & 0xFF);
	lcd_spibus_send_data(p_st7789, data, 2);

}
//读点
uint16_t st7789_readPoint(uint16_t x, uint16_t y)
{
	return 0;
}
//填充指定颜色
void st7789_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
	uint32_t size = ((ex - sx)) * ((ey - sy));
	uint32_t size_max = 0;

	st7789_set_addr(sx, sy, ex-1, ey-1);

	/*Memory write*/
	lcd_spibus_send_cmd(p_st7789, 0x2C);


#if 0
	if(size >= 240*120){
		size_max = size>>2;
		lcd_spibus_fill(p_st7789, color, size_max);
		lcd_spibus_fill(p_st7789, color, size-size_max);
	}else{
		lcd_spibus_fill(p_st7789, color, size);
	}
#else
	size_max = (size/lcddev.width);
	uint32_t remainder = size - (size_max*lcddev.width);
	if(size_max){
		for(uint16_t i=0; i < size_max; i++){
			lcd_spibus_fill(p_st7789, color, lcddev.width);
		}
		if(remainder){
			lcd_spibus_fill(p_st7789, color, remainder);
		}
	}else{
		lcd_spibus_fill(p_st7789, color, size);
	}
#endif


}

//填充指定区域块颜色
//开始位置填充多少个
void st7789_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
	st7789_set_addr(sx, sy, sx+ex-1, sy+ey-1);

	/*Memory write*/
	lcd_spibus_send_cmd(p_st7789, 0x2C);

	uint32_t size = ex * ey;

	/*Byte swapping is required*/
	uint32_t i;
	uint8_t * color_u8 = (uint8_t *) color;
	uint8_t color_tmp;

	for(i = 0; i < size * 2; i += 2) {
		color_tmp = color_u8[i + 1];
		color_u8[i + 1] = color_u8[i];
		color_u8[i] = color_tmp;
	}
	
	if(size >= 240*120){
		lcd_spibus_send_data(p_st7789, (uint8_t*)color, size);
		color += (size>>1);
		lcd_spibus_send_data(p_st7789, (uint8_t*)color, size);
	}else{
		lcd_spibus_send_data(p_st7789, (uint8_t*)color, size * 2);
	}
	
}

//绘制横线函数
void st7789_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	st7789_Fill(x0, y0,x0+len, y0, color);
}
//
void st7789_draw_vline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	st7789_Fill(x0, y0,x0, y0+len, color);
}
//------------------------------------------------
Graphics_Display st7789_glcd =
{
	16,
	240,
	240,
	st7789_DrawPoint,
	st7789_readPoint,
	st7789_draw_hline,
	st7789_draw_vline,
	st7789_Fill,
	st7789_Full
};
//==============================================================================================
//mpy
STATIC mp_obj_t ST7789_drawpPixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t drawp_args[] = {
			{ MP_QSTR_x,       MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_y,       MP_ARG_INT, {.u_int = 0} },
			{ MP_QSTR_color,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(drawp_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawp_args), drawp_args, args);

	if(args[2].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[2].u_obj, &len, &params);
		if(len == 3){
			st7789_DrawPoint(args[0].u_int,args[1].u_int ,
			get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawpPixel_obj, 1, ST7789_drawpPixel);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawpFull(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
	static const mp_arg_t clear_args[] = {
			{ MP_QSTR_fillcolor,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(clear_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(clear_args), clear_args, args);

	if(args[0].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[0].u_obj, &len, &params);
		if(len == 3){
			lcddev.backcolor = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			
			st7789_Fill(0,0,lcddev.width, lcddev.height, lcddev.backcolor);
			lcddev.clercolor = lcddev.backcolor;
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawpFull_obj, 1, ST7789_drawpFull);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawLin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drawL_args[] = {
				{ MP_QSTR_x0,        	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y0,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_x1,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y1,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_color,   		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drawL_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawL_args), drawL_args, args);

    if(args[4].u_obj !=MP_OBJ_NULL) 
    {
          size_t len;
          mp_obj_t *params;
          mp_obj_get_array(args[4].u_obj, &len, &params);
          if(len == 3){
						grap_drawLine(&st7789_glcd,args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,
             get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
          }else{
            mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
          }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawLin_obj, 4, ST7789_drawLin);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t Rect_args[] = {
		{ MP_QSTR_x,        		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,        		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_width,     		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_height,    		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_border,    		MP_ARG_INT, {.u_int = 1} }, 
    { MP_QSTR_fillcolor,   	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(Rect_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(Rect_args), Rect_args, args);
  
  if(args[4].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[4].u_obj, &len, &params);
    if(len == 3){
			grap_drawRect(&st7789_glcd,args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,
          get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("lcd drawRect parameter error \n"));
    }
  }
    //MP_OBJ_NULL
  if(args[6].u_obj != mp_const_none && args[6].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[6].u_obj, &len, &params);

    if (len != 3) { // Check params len
       mp_raise_ValueError(MP_ERROR_TEXT("lcd fillcolor parameter error"));
    }
    uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
    for(uint16_t i=0 ; i <= (args[3].u_int-(args[5].u_int*2)); i++ ){ 
     grap_drawLine(&st7789_glcd,args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,
					args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
		}
     
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawRect_obj, 1, ST7789_drawRect);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  STATIC const mp_arg_t tft_allowed_args[] = {
	{ MP_QSTR_x, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_y, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_radius, 	 		MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_color,	  		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	{ MP_QSTR_border, 	 		MP_ARG_INT, {.u_int = 1} }, 
	{ MP_QSTR_fillcolor,	 	MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL} }, //7

  };

  mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);
  
  uint16_t color;
//Circlecolor
  if(args[3].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[3].u_obj, &len, &params);
    if(len == 3){
      color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			
			
        for(uint16_t i=0; i < args[4].u_int ;i++) {
          grap_drawColorCircle(&st7789_glcd,
														args[0].u_int,args[1].u_int,args[2].u_int-i,color);
        }
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("lcd color parameter error \n"));
    }
  }
//fillcolor
  if(args[5].u_obj != mp_const_none && args[5].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[5].u_obj, &len, &params);

    if (len != 3) { // Check params len
       mp_raise_ValueError(MP_ERROR_TEXT("lcd fillcolor parameter error"));
    }
    color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));

    for(uint16_t i=0 ; i <= (args[2].u_int-args[4].u_int); i++ ) {
      grap_drawColorCircle(&st7789_glcd,
						args[0].u_int, args[1].u_int, args[2].u_int-args[4].u_int-i, color);
    }
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawCircle_obj, 1, ST7789_drawCircle);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t tft_allowed_args[] = {
		{ MP_QSTR_text,     		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_x,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_backcolor,   	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
    { MP_QSTR_size,      		MP_ARG_INT, {.u_int = 2} },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);

  uint16_t text_size = args[5].u_int;
  uint16_t color = 0;
  //color
  if(args[3].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[3].u_obj, &len, &params);
    if(len == 3){
      color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("printStr color parameter error \n"));
    }
  }

  if(args[4].u_obj != mp_const_none && args[4].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[4].u_obj, &len, &params);
  
    if (len != 3) { 
       mp_raise_ValueError(MP_ERROR_TEXT("lcd backolor parameter error"));
    }
    lcddev.backcolor = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])); 
  }
//
  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter error"));

    } else {
        mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);
        char *str = bufinfo.buf;

        if(text_size == 1)  text_size = 16;
        else if(text_size == 2) text_size = 24;
        else if(text_size == 3) text_size = 32;
        else if(text_size == 4) text_size = 48;
        else mp_raise_ValueError(MP_ERROR_TEXT("lcd size parameter error"));
				
        grap_drawStr(&st7789_glcd, args[1].u_int, args[2].u_int, 
									text_size* bufinfo.len, text_size , text_size,str ,color, lcddev.backcolor);
													
    }
  }
	else{
     mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawStr_obj, 1, ST7789_drawStr);


//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_write_buf(size_t n_args, const mp_obj_t *args) {
	if(6 != n_args) {
		mp_raise_ValueError(MP_ERROR_TEXT("lcd write_buf parameter error \n"));
	}
	unsigned short start_x = mp_obj_get_int(args[2]);
	unsigned short start_y = mp_obj_get_int(args[3]);
	unsigned short width = mp_obj_get_int(args[4]);
	unsigned short height = mp_obj_get_int(args[5]);

	mp_buffer_info_t lcd_write_data = {0};

	
	mp_get_buffer_raise(args[1], &lcd_write_data, MP_BUFFER_READ);
	
	if(lcd_write_data.buf == NULL || lcd_write_data.len == 0) {
		return mp_obj_new_int(-3);
	}

	grap_drawFull(start_x, start_y,width,height,(uint16_t *)lcd_write_data.buf);

    return mp_obj_new_int(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ST7789_write_buf_obj, 1, 6, ST7789_write_buf);


//---------------------------华丽的分割线-------------------------------------------------------------------
#if MICROPY_PY_PICLIB

mp_obj_t ST7789_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t ST7789_allowed_args[] = { 
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(ST7789_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, ST7789_allowed_args, args);
	
	uint8_t res=0;
	
	char *path_buf = (char *)m_malloc(50);  //最大支持50字符
	memset(path_buf, '\0', 50);
	
  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter error,should is .cache"));
    } 
		else 
		{
			mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

			const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
			const char *ftype = mp_obj_str_get_str(file_type(file_path));
			 mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
			 
			 if(res == 1){
				 vfs = vfs->next;
				 is_sdcard = 1;
			 }else{
			 	is_sdcard = 0;
			 }
			fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);

			//test file
			FIL		*f_file;
			f_file=(FIL *)m_malloc(sizeof(FIL));
			if(f_file == NULL){
				mp_raise_ValueError(MP_ERROR_TEXT("malloc f_file error"));
			}
			sprintf(path_buf,"%s%s",file_path,".cache");
			res = f_open(&vfs_fat->fatfs,f_file,path_buf,FA_READ);
			f_close(f_file);
			f_sync(f_file);
			if(res == FR_OK && args[2].u_bool == false){
				return mp_const_none;
			}else{
				args[2].u_bool = true;
			}

			memset(path_buf, '\0', 50);
printf("start loading->%s\r\n",file_path);

			piclib_init();
			if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0)
			{
				res = jpg_decode(&vfs_fat->fatfs,file_path, 0, 0 ,1);
				if(res){
					printf("jpg_decode err:%d\r\n",res);
					return mp_const_none;
				}
			}
			else if(strncmp(ftype , "bmp" , 3) == 0)
			{
				res = stdbmp_decode(&vfs_fat->fatfs ,file_path, 0, 0) ;
				printf("bmp_decode err:%d\r\n",res);
				if(res)return mp_const_none;
			}
			else
			{
				mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
				return mp_const_none;
			}
//-----------------------------------------------------------
			if(args[1].u_obj !=MP_OBJ_NULL)
			{
				mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_READ);
				const char *path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
				const char *path_ftype = mp_obj_str_get_str(file_type(path));

				if(strncmp(path_ftype , "cache" , 5))
				{
					mp_raise_ValueError(MP_ERROR_TEXT("CachePicture path file type error"));
					return mp_const_none;
				}
				sprintf(path_buf,"%s",path);
			}else
			{
				sprintf(path_buf,"%s%s",file_path,".cache");
			}
//------------------------------------------------
			res = f_open(&vfs_fat->fatfs,f_file,path_buf,FA_READ);
			f_close(f_file);
			f_sync(f_file);
			
			if(args[2].u_bool == true || res != 0)
			{
				grap_newCached(&ili_glcd, is_sdcard,&vfs_fat->fatfs, path_buf,picinfo.S_Width, picinfo.S_Height);	
			}
			
			f_sync(f_file);
			m_free(f_file);
    }
  }
	else{
      mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter is empty"));
  }
	m_free(path_buf);
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_CachePicture_obj, 1, ST7789_CachePicture);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_drawPicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t ST7789_allowed_args[] = { 
    { MP_QSTR_x,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_file,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_cached,  MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
  };
  uint8_t arg_num = MP_ARRAY_SIZE(ST7789_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, ST7789_allowed_args, args);

  if(args[2].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[2].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter error"));
    } 
		else 
		{
        mp_get_buffer_raise(args[2].u_obj, &bufinfo, MP_BUFFER_READ);

				uint8_t res=0;
				
				mp_obj_t tuple[2];
				
				const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
				const char *ftype = mp_obj_str_get_str(file_type(file_path));
				 
				 mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
				 if(res == 1){
					 vfs = vfs->next;
					 is_sdcard = 1;
				 }else{
					 is_sdcard = 0;
				 }
				 fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
				 //---------------------------------------------------------------
					 if(args[3].u_bool == true){
						 uint8_t file_len = strlen(file_path);
						 char *file_buf = (char *)m_malloc(file_len+7);  
		 
						 memset(file_buf, '\0', file_len+7);
						 sprintf(file_buf,"%s%s",file_path,".cache");
						 res = grap_drawCached(&ili_glcd,&vfs_fat->fatfs, args[0].u_int, args[1].u_int, (const char *)file_buf);
						 m_free(file_buf);
						 if(!res) return mp_const_none;
					 }
				 //---------------------------------------------------------------

				 piclib_init();
				 
				if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0)
					{
						jpg_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int ,1);
					}
				else if(strncmp(ftype , "bmp" , 3) == 0)
					{
						stdbmp_decode(&vfs_fat->fatfs ,file_path, args[0].u_int, args[1].u_int) ;
					}
				else
					{
						mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
						return mp_const_none;
					}

				tuple[0] = mp_obj_new_int(picinfo.S_Height);
				tuple[1] = mp_obj_new_int(picinfo.S_Width);
				return mp_obj_new_tuple(2, tuple);
    }
  }
	else{
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7789_drawPicture_obj, 1, ST7789_drawPicture);

#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_deinit(mp_obj_t self_in) {
	lcd_spibus_deinit();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ST7789_deinit_obj, ST7789_deinit);
//---------------------------华丽的分割线-------------------------------------------------------------------

STATIC mp_obj_t ST7789_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcd_spibus_t *self = m_new_obj(lcd_spibus_t);
	
	lcddev.width=240;
	lcddev.height=240;
	st7789_glcd.width = lcddev.width;
	st7789_glcd.height = lcddev.height;
	
	mp_init_ST7789();
	
	self = p_st7789;
	self->base.type = type;
	
	st7789_set_dir(args[ARG_portrait].u_int);
	
	lcddev.type = 5;
	lcddev.backcolor = 0x0000;

	st7789_Fill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);

	lcddev.clercolor = lcddev.backcolor;
	draw_global = &st7789_glcd;
	return MP_OBJ_FROM_PTR(self);
}
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC const mp_rom_map_elem_t ST7789_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&ST7789_deinit_obj) },
  { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ST7789_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&ST7789_drawpFull_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&ST7789_drawpPixel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&ST7789_drawLin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&ST7789_drawRect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&ST7789_drawCircle_obj) },
	{ MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&ST7789_drawStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&ST7789_write_buf_obj) },
	
	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&ST7789_drawPicture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&ST7789_CachePicture_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(ST7789_locals_dict, ST7789_locals_dict_table);
//---------------------------华丽的分割线-------------------------------------------------------------------
const mp_obj_type_t ST7789_type = {
    { &mp_type_type },
    .name = MP_QSTR_ST7789,
    .make_new = ST7789_make_new,
    .locals_dict = (mp_obj_dict_t*)&ST7789_locals_dict,
};

//-------------------------------------------------------------------------------------------
#endif
