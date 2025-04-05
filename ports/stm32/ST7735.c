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

#if (MICROPY_HW_LCD18 & MICROPY_ENABLE_TFTLCD & MICROPY_ENABLE_SPILCD)
	
#include "lcd_spibus.h"
#include "modtftlcd.h"
#include "ST7735.h"

#include "global.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[17];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

Graphics_Display st7735_glcd;

STATIC lcd_spibus_t *p_st7735 = NULL;

STATIC const lcd_init_cmd_t st7735_init_cmds[]={
		{0x11, {0}, 0x80},
		{0xB1, {0x05, 0x3A, 0x3A}, 3},
		{0xB2, {0x05, 0x3A, 0x3A}, 3},
		{0xB3, {0x05, 0x3A, 0x3A, 0x05, 0x3A, 0x3A}, 6},
		{0xB4, {0x03,}, 1},
		{0xC0, {0x62, 0x02, 0x04}, 3},
		{0xC1, {0xC0}, 1},
		{0xC2, {0x0D, 0x00}, 2},
		{0xC3, {0x8D, 0xEA}, 2},
		{0xC4, {0x8D, 0xEE}, 2},
		{0xC5, {0x0D}, 1},
		{0x36, {0xC0}, 1},
		{0xE0, {0x0A, 0x1F, 0x0E, 0x17, 0x37, 0x31, 0x2B, 0x2E, 0x2C, 0x29, 0x31, 0x3C, 0x00, 0x05, 0x03, 0x0D}, 16},
		{0xE0, {0x0B, 0x1F, 0x0E, 0x12, 0x28, 0x24, 0x1F, 0x25, 0x25, 0x26, 0x30, 0x3C, 0x00, 0x05, 0x03, 0x0D}, 16},
		{0x3A, {0x55}, 1},  /*Pixel Format Set*/ //65k mode
		{0x29, {0}, 0x80}, //Display on
		{0, {0}, 0xff},
	};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void  mp_init_ST7735(void)
{
	lcd_bus_init();
	p_st7735 = lcd_spibus;
	//Send all the commands
	uint16_t cmd = 0;
	while (st7735_init_cmds[cmd].databytes!=0xff) {
		lcd_spibus_send_cmd(p_st7735, st7735_init_cmds[cmd].cmd);
		lcd_spibus_send_data(p_st7735, st7735_init_cmds[cmd].data, st7735_init_cmds[cmd].databytes & 0x1F);
		if (st7735_init_cmds[cmd].databytes & 0x80) {
			mp_hal_delay_ms(100);
		}
		cmd++;
	} 
}
static void st7735_set_addr(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey)
{
	uint8_t data[4] = {0};

	lcd_spibus_send_cmd(p_st7735, 0x2A);
	data[0] = (sx >> 8) & 0xFF;
	data[1] = sx & 0xFF;
	data[2] = ((ex) >> 8) & 0xFF;
	data[3] = (ex) & 0xFF;
	lcd_spibus_send_data(p_st7735, data, 4);
	
	lcd_spibus_send_cmd(p_st7735, 0x2B); //2A
	data[0] = (sy >> 8) & 0xFF;
	data[1] = sy & 0xFF;
	data[2] = ((ey) >> 8) & 0xFF;
	data[3] = (ey) & 0xFF;
	lcd_spibus_send_data(p_st7735, data, 4);
}
//设置LCD显示方向
void st7735_set_dir(uint8_t dir)
{
	uint8_t dir_data = 0;
	
	lcddev.dir=dir;		//竖屏

	switch (dir)
		{
		case 2:
		lcddev.width = 160;
		lcddev.height = 128;
		dir_data = 0xA0;
		break;
		case 3:
		lcddev.width = 128;
		lcddev.height = 160;
		dir_data = 0x00;
		break;
		case 4:
		lcddev.width = 160;
		lcddev.height = 128;
		dir_data = 0x70;
		break;
		default:
		lcddev.width = 128;
		lcddev.height = 160;
		dir_data = 0xC0;
		break;
		}

	uint8_t data[2] = {0};

	lcd_spibus_send_cmd(p_st7735, 0x36);
	data[0] = dir_data;
	lcd_spibus_send_data(p_st7735, data, 1);

	st7735_set_addr(0, 0, lcddev.width-1, lcddev.height-1);
	
	st7735_glcd.width = lcddev.width;
	st7735_glcd.height = lcddev.height;
}	 

//画点
void st7735_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
	uint8_t data[2];

	/*Column addresses*/
	lcd_spibus_send_cmd(p_st7735, 0x2A);
	data[0] = (x >> 8);
	data[1] = (x & 0xFF);
	lcd_spibus_send_data(p_st7735, data, 2);
	
		/*Page addresses*/
	lcd_spibus_send_cmd(p_st7735, 0x2B);
	data[0] = (y >> 8);
	data[1] = (y & 0xFF);
	lcd_spibus_send_data(p_st7735, data, 2);
	
		/*Memory write*/
	lcd_spibus_send_cmd(p_st7735, 0x2C);
	data[0] = (color >> 8);
	data[1] = (color & 0xFF);
	lcd_spibus_send_data(p_st7735, data, 2);

}
//读点
uint16_t st7735_readPoint(uint16_t x, uint16_t y)
{
	return 0;
}
//填充指定颜色
void st7735_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
	uint32_t size = ((ex - sx)) * ((ey - sy));

	uint8_t data[4] = {0};

	lcd_spibus_send_cmd(p_st7735, 0x2A);
	data[0] = (sx >> 8) & 0xFF;
	data[1] = sx & 0xFF;
	data[2] = ((ex-1) >> 8) & 0xFF;
	data[3] = (ex-1) & 0xFF;
	lcd_spibus_send_data(p_st7735, data, 4);
	
	lcd_spibus_send_cmd(p_st7735, 0x2B); //2A
	data[0] = (sy >> 8) & 0xFF;
	data[1] = sy & 0xFF;
	data[2] = ((ey-1) >> 8) & 0xFF;
	data[3] = (ey-1) & 0xFF;
	lcd_spibus_send_data(p_st7735, data, 4);
	
	/*Memory write*/
	lcd_spibus_send_cmd(p_st7735, 0x2C);

#if 0
	lcd_spibus_fill(p_st7735, color, size);
#else
	uint32_t size_max = 0;
	size_max = (size/lcddev.width);
	uint32_t remainder = size - (size_max*lcddev.width);
	if(size_max){
		for(uint16_t i=0; i < size_max; i++){
			lcd_spibus_fill(p_st7735, color, lcddev.width);
		}
		if(remainder){
			lcd_spibus_fill(p_st7735, color, remainder);
		}
	}else{
		lcd_spibus_fill(p_st7735, color, size);
	}
#endif
}

//填充指定区域块颜色
//开始位置填充多少个
void st7735_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
	st7735_set_addr(sx, sy, sx+ex-1, sy+ey-1);

	/*Memory write*/
	lcd_spibus_send_cmd(p_st7735, 0x2C);

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
	
 	if(size >= 128*80){
		lcd_spibus_send_data(p_st7735, (uint8_t*)color, size);
		color += (size>>1);
		lcd_spibus_send_data(p_st7735, (uint8_t*)color, size);
	}else{
		lcd_spibus_send_data(p_st7735, (uint8_t*)color, size * 2);
	} 
}

//绘制横线函数
void st7735_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	st7735_Fill(x0, y0,x0+len, y0, color);
}
//
void st7735_draw_vline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	st7735_Fill(x0, y0,x0, y0+len, color);
}
//------------------------------------------------
Graphics_Display st7735_glcd =
{
	16,
	128,
	160,
	st7735_DrawPoint,
	st7735_readPoint,
	st7735_draw_hline,
	st7735_draw_vline,
	st7735_Fill,
	st7735_Full
};
//==============================================================================================
//mpy
STATIC mp_obj_t ST7735_drawpPixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
			st7735_DrawPoint(args[0].u_int,args[1].u_int ,
			get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawpPixel_obj, 1, ST7735_drawpPixel);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawpFull(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
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
			
			st7735_Fill(0,0,lcddev.width, lcddev.height, lcddev.backcolor);
			lcddev.clercolor = lcddev.backcolor;
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawpFull_obj, 1, ST7735_drawpFull);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawLin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
						grap_drawLine(&st7735_glcd,args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,
             get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
          }else{
            mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
          }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawLin_obj, 4, ST7735_drawLin);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
			grap_drawRect(&st7735_glcd,args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,
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
     grap_drawLine(&st7735_glcd,args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,
					args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
		}
     
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawRect_obj, 1, ST7735_drawRect);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
          grap_drawColorCircle(&st7735_glcd,
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
      grap_drawColorCircle(&st7735_glcd,
						args[0].u_int, args[1].u_int, args[2].u_int-args[4].u_int-i, color);
    }
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawCircle_obj, 1, ST7735_drawCircle);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_write_buf(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ST7735_write_buf_obj, 1, 6, ST7735_write_buf);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
				
        grap_drawStr(&st7735_glcd, args[1].u_int, args[2].u_int, 
									text_size* bufinfo.len, text_size , text_size,str ,color, lcddev.backcolor);
													
    }
  }
	else{
     mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawStr_obj, 1, ST7735_drawStr);
//---------------------------华丽的分割线-------------------------------------------------------------------

#if MICROPY_PY_PICLIB

mp_obj_t ST7735_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t st7735_allowed_args[] = { 
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(st7735_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, st7735_allowed_args, args);
	
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
				grap_newCached(&st7735_glcd, is_sdcard,&vfs_fat->fatfs, path_buf,picinfo.S_Width, picinfo.S_Height);	
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_CachePicture_obj, 1, ST7735_CachePicture);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7735_drawPicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t st7735_allowed_args[] = { 
    { MP_QSTR_x,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_file,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_cached,  MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
  };
  uint8_t arg_num = MP_ARRAY_SIZE(st7735_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, st7735_allowed_args, args);

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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ST7735_drawPicture_obj, 1, ST7735_drawPicture);

#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ST7789_deinit(mp_obj_t self_in) {
	lcd_spibus_deinit();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ST7789_deinit_obj, ST7789_deinit);


//---------------------------华丽的分割线-------------------------------------------------------------------

STATIC mp_obj_t ST7735_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcd_spibus_t *self = m_new_obj(lcd_spibus_t);
	
	mp_init_ST7735();
	
	self = p_st7735;
	self->base.type = type;
	
	st7735_set_dir(args[ARG_portrait].u_int);
	
	lcddev.type = 6;
	lcddev.backcolor = 0x0000;

	st7735_Fill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);

	lcddev.clercolor = lcddev.backcolor;
	draw_global = &st7735_glcd;
	return MP_OBJ_FROM_PTR(self);
}
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC const mp_rom_map_elem_t ST7735_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&ST7789_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ST7789_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&ST7735_drawpFull_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&ST7735_drawpPixel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&ST7735_drawLin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&ST7735_drawRect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&ST7735_drawCircle_obj) },
	{ MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&ST7735_drawStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&ST7735_write_buf_obj) },
	
	
	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&ST7735_drawPicture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&ST7735_CachePicture_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(ST7735_locals_dict, ST7735_locals_dict_table);
//---------------------------华丽的分割线-------------------------------------------------------------------
const mp_obj_type_t ST7735_type = {
    { &mp_type_type },
    .name = MP_QSTR_ST7735,
    .make_new = ST7735_make_new,
    .locals_dict = (mp_obj_dict_t*)&ST7735_locals_dict,
};

//-------------------------------------------------------------------------------------------
#endif
