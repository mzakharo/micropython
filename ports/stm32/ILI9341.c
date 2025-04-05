/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	ili9341.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/7/1
	* Description 			 :	
	******************************************************************************
**/

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"

#if (MICROPY_HW_LCD32 & MICROPY_ENABLE_TFTLCD & MICROPY_ENABLE_SPILCD)
	
#include "lcd_spibus.h"
#include "modtftlcd.h"
#include "ILI9341.h"

#include "global.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

STATIC lcd_spibus_t *p_ili9341 = NULL;

Graphics_Display ili_glcd;

STATIC const lcd_init_cmd_t ili_init_cmds[]={
		{0xCF, {0x00, 0x83, 0X30}, 3},
		{0xED, {0x64, 0x03, 0X12, 0X81}, 4},
		{0xE8, {0x85, 0x01, 0x79}, 3},
		{0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
		{0xF7, {0x20}, 1},
		{0xEA, {0x00, 0x00}, 2},
		{0xC0, {0x26}, 1},			/*Power control*/
		{0xC1, {0x11}, 1},			/*Power control */
		{0xC5, {0x35, 0x3E}, 2},	/*VCOM control*/
		{0xC7, {0xBE}, 1},			/*VCOM control*/
		{0x36, {0x48}, 1},			/*Memory Access Control*/
		{0x3A, {0x55}, 1},			/*Pixel Format Set*/
		{0xB1, {0x00, 0x1B}, 2},
		{0xF2, {0x08}, 1},
		{0x26, {0x01}, 1},
		{0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
		{0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
		{0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
		{0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
		{0x2C, {0}, 0},
		{0xB7, {0x07}, 1},
		{0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
		{0x11, {0}, 0x80},
		{0x29, {0}, 0x80},
		{0, {0}, 0xff},
	};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void  mp_init_ILI9341(void)
{
	lcd_bus_init();
	p_ili9341 = lcd_spibus;
	//Send all the commands
	uint16_t cmd = 0;
	while (ili_init_cmds[cmd].databytes!=0xff) {
		lcd_spibus_send_cmd(p_ili9341, ili_init_cmds[cmd].cmd);
		lcd_spibus_send_data(p_ili9341, ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
		if (ili_init_cmds[cmd].databytes & 0x80) {
			mp_hal_delay_ms(100);
		}
		cmd++;
	} 
}

//设置LCD显示方向
void ili9341_set_dir(uint8_t dir)
{
	uint8_t dir_data = 0;
	lcddev.dir=dir;		//竖屏
	
	switch (dir)
		{
		case 2:
		dir_data = 0x28;
		lcddev.width=320;
		lcddev.height=240;
		break;
		case 3:
		dir_data = 0x88;
		lcddev.width=240;
		lcddev.height=320;
		break;
		case 4:
		dir_data = 0xE8;
		lcddev.width=320;
		lcddev.height=240;
		break;
		default:
		dir_data = 0x48;
		lcddev.width=240;
		lcddev.height=320;
		break;
		}

	uint8_t data[4] = {0};
	
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = (lcddev.width-1) >> 8;
	data[3] = (lcddev.width-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);
	
	lcd_spibus_send_cmd(p_ili9341, 0x2B); //2A
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = (lcddev.height-1) >> 8;
	data[3] = (lcddev.height-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	lcd_spibus_send_cmd(p_ili9341, 0x36);
	data[0] = dir_data;
	lcd_spibus_send_data(p_ili9341, data, 1);
	
	ili_glcd.width = lcddev.width;
	ili_glcd.height = lcddev.height;

}	 

//画点
void ili9341_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
	uint8_t data[2];

	/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (x >> 8);
	data[1] = (x & 0xFF);
	lcd_spibus_send_data(p_ili9341, data, 2);
	
		/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (y >> 8);
	data[1] = (y & 0xFF);
	lcd_spibus_send_data(p_ili9341, data, 2);
	
		/*Memory write*/
	lcd_spibus_send_cmd(p_ili9341, 0x2C);
	data[0] = (color >> 8);
	data[1] = (color & 0xFF);
	lcd_spibus_send_data(p_ili9341, data, 2);
}

//读点
uint16_t ili9341_readPoint(uint16_t x, uint16_t y)
{
	uint8_t r[2],b[2];
	uint16_t r1=0,g1=0,b1=0;
	uint8_t data[2]={0};

	/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (x >> 8) & 0xFF;
	data[1] = x & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 2);

	/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (y >> 8) & 0xFF;
	data[1] = y & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 2);
	
	lcd_spibus_send_cmd(p_ili9341, 0x2E);
	
	
	lcd_spibus_read(p_ili9341,r,2);								//dummy Read	   
	mp_hal_delay_ms(2); 
 	lcd_spibus_read(p_ili9341,r,2);  		  						//实际坐标颜色

	mp_hal_delay_ms(2);	  
	lcd_spibus_read(p_ili9341,b,2);
	g1=r[1]&0XFF;		
	g1<<=8; 
	b1 = (b[1]<<8) | b[0];
	return (((r1>>11)<<11)|((g1>>10)<<5)|(b1>>11));	

}

//填充指定颜色
void ili9341_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
	uint8_t data[4];

	uint32_t size = ((ex - sx)) * ((ey - sy));
	uint32_t size_max = 0;

	/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (sx >> 8) & 0xFF;
	data[1] = sx & 0xFF;
	data[2] = ((ex-1) >> 8) & 0xFF;
	data[3] = (ex-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (sy >> 8) & 0xFF;
	data[1] = sy & 0xFF;
	data[2] = ((ey-1) >> 8) & 0xFF;
	data[3] = (ey-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Memory write*/
	lcd_spibus_send_cmd(p_ili9341, 0x2C);
#if 1
	if(size >= 320*120){
		size_max = size>>2;
		lcd_spibus_fill(p_ili9341, color, size_max);
		lcd_spibus_fill(p_ili9341, color, size_max);
		lcd_spibus_fill(p_ili9341, color, size_max);
		lcd_spibus_fill(p_ili9341, color, size-(size_max*3));
	}else{
		lcd_spibus_fill(p_ili9341, color, size);
	}
#else
	size_max = (size/lcddev.width);
	uint32_t remainder = size - (size_max*lcddev.width);
	if(size_max){
		for(uint16_t i=0; i < size_max; i++){
			lcd_spibus_fill(p_ili9341, color, lcddev.width);
		}
		if(remainder){
			lcd_spibus_fill(p_ili9341, color, remainder);
		}
	}else{
		lcd_spibus_fill(p_ili9341, color, size);
	}
#endif

	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (0 >> 8) & 0xFF;
	data[1] = 0 & 0xFF;
	data[2] = (lcddev.width >> 8) & 0xFF;
	data[3] = lcddev.width & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (0 >> 8) & 0xFF;
	data[1] = 0 & 0xFF;
	data[2] = (lcddev.height >> 8) & 0xFF;
	data[3] = lcddev.height & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);
	
}

//填充指定区域块颜色
//开始位置填充多少个
void ili9341_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
	uint8_t data[4];

	/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (sx >> 8) & 0xFF;
	data[1] = sx & 0xFF;
	data[2] = ((sx+ex-1) >> 8) & 0xFF;
	data[3] = (sx+ex-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (sy >> 8) & 0xFF;
	data[1] = sy & 0xFF;
	data[2] = ((sy+ey-1) >> 8) & 0xFF;
	data[3] = (sy+ey-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Memory write*/
	lcd_spibus_send_cmd(p_ili9341, 0x2C);

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
	
	if(size >= 320*120){
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size);
		color += (size>>1);
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size);
	}else{
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size * 2);
	}
	
		/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (0 >> 8) & 0xFF;
	data[1] = 0 & 0xFF;
	data[2] = (lcddev.width >> 8) & 0xFF;
	data[3] = lcddev.width & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (0 >> 8) & 0xFF;
	data[1] = 0 & 0xFF;
	data[2] = (lcddev.height >> 8) & 0xFF;
	data[3] = lcddev.height & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);
}

//填充指定区域块颜色
//开始位置填充多少个
void ili9341_cam_full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{
	uint8_t data[4];

	/*Column addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2A);
	data[0] = (sx >> 8) & 0xFF;
	data[1] = sx & 0xFF;
	data[2] = ((sx+ex-1) >> 8) & 0xFF;
	data[3] = (sx+ex-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Page addresses*/
	lcd_spibus_send_cmd(p_ili9341, 0x2B);
	data[0] = (sy >> 8) & 0xFF;
	data[1] = sy & 0xFF;
	data[2] = ((sy+ey-1) >> 8) & 0xFF;
	data[3] = (sy+ey-1) & 0xFF;
	lcd_spibus_send_data(p_ili9341, data, 4);

	/*Memory write*/
	lcd_spibus_send_cmd(p_ili9341, 0x2C);

	uint32_t size = ex * ey;
	
	if(size > 320*120){
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size);
		color += (size>>1);
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size);
	}else{
		lcd_spibus_send_data(p_ili9341, (uint8_t*)color, size * 2);
	}
	
}

//绘制横线函数
void ili9341_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	ili9341_Fill(x0, y0,x0+len, y0, color);
}
//
void ili9341_draw_vline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	ili9341_Fill(x0, y0,x0, y0+len, color);
}
//------------------------------------------------
Graphics_Display ili_glcd =
{
	16,
	240,
	320,
	ili9341_DrawPoint,
	ili9341_readPoint,
	ili9341_draw_hline,
	ili9341_draw_vline,
	ili9341_Fill,
	ili9341_Full
};
//==============================================================================================
//mpy
STATIC mp_obj_t ILI9341_drawpPixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
			ili9341_DrawPoint(args[0].u_int,args[1].u_int ,
			get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawpPixel_obj, 1, ILI9341_drawpPixel);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawpFull(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
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
			
			ili9341_Fill(0,0,lcddev.width, lcddev.height, lcddev.backcolor);
			lcddev.clercolor = lcddev.backcolor;
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawpFull_obj, 1, ILI9341_drawpFull);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawLin(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
						grap_drawLine(&ili_glcd,args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,
             get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
          }else{
            mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
          }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawLin_obj, 4, ILI9341_drawLin);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
			grap_drawRect(&ili_glcd,args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,
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
     grap_drawLine(&ili_glcd,args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,
					args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
		}
     
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawRect_obj, 1, ILI9341_drawRect);
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  STATIC const mp_arg_t ILI9341_allowed_args[] = {
	{ MP_QSTR_x, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_y, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_radius, 	 		MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_color,	  		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	{ MP_QSTR_border, 	 		MP_ARG_INT, {.u_int = 1} }, 
	{ MP_QSTR_fillcolor,	 	MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL} }, //7

  };

  mp_arg_val_t args[MP_ARRAY_SIZE(ILI9341_allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(ILI9341_allowed_args), ILI9341_allowed_args, args);
  
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
          grap_drawColorCircle(&ili_glcd,
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
      grap_drawColorCircle(&ili_glcd,
						args[0].u_int, args[1].u_int, args[2].u_int-args[4].u_int-i, color);
    }
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawCircle_obj, 1, ILI9341_drawCircle);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_write_buf(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ILI9341_write_buf_obj, 1, 6, ILI9341_write_buf);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t ILI9341_allowed_args[] = {
		{ MP_QSTR_text,     		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_x,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_backcolor,   	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
    { MP_QSTR_size,      		MP_ARG_INT, {.u_int = 2} },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(ILI9341_allowed_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(ILI9341_allowed_args), ILI9341_allowed_args, args);

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
				
        grap_drawStr(&ili_glcd, args[1].u_int, args[2].u_int, 
									text_size* bufinfo.len, text_size , text_size,str ,color, lcddev.backcolor);
													
    }
  }
	else{
     mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawStr_obj, 1, ILI9341_drawStr);
//---------------------------华丽的分割线-------------------------------------------------------------------

#if MICROPY_PY_PICLIB

mp_obj_t ILI9341_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t ILI9341_allowed_args[] = { 
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(ILI9341_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, ILI9341_allowed_args, args);
	
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_CachePicture_obj, 1, ILI9341_CachePicture);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_drawPicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t ILI9341_allowed_args[] = { 
    { MP_QSTR_x,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_file,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_cached,  MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
  };
  uint8_t arg_num = MP_ARRAY_SIZE(ILI9341_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, ILI9341_allowed_args, args);

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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ILI9341_drawPicture_obj, 1, ILI9341_drawPicture);

#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t ILI9341_deinit(mp_obj_t self_in) {
	lcd_spibus_deinit();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ILI9341_deinit_obj, ILI9341_deinit);
//---------------------------华丽的分割线-------------------------------------------------------------------

STATIC mp_obj_t ILI9341_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcd_spibus_t *self = m_new_obj(lcd_spibus_t);
	
	mp_init_ILI9341();
	
	self = p_ili9341;
	self->base.type = type;
	
	ili9341_set_dir(args[ARG_portrait].u_int);
	
	lcddev.type = 4;
	lcddev.backcolor = 0x0000;

	ili9341_Fill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);

	lcddev.clercolor = lcddev.backcolor;
	draw_global = &ili_glcd;
	return MP_OBJ_FROM_PTR(self);
}
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC const mp_rom_map_elem_t ILI9341_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&ILI9341_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ILI9341_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&ILI9341_drawpFull_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&ILI9341_drawpPixel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&ILI9341_drawLin_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&ILI9341_drawRect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&ILI9341_drawCircle_obj) },
	{ MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&ILI9341_drawStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&ILI9341_write_buf_obj) },
	
	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&ILI9341_drawPicture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&ILI9341_CachePicture_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(ILI9341_locals_dict, ILI9341_locals_dict_table);
//---------------------------华丽的分割线-------------------------------------------------------------------
const mp_obj_type_t ILI9341_type = {
    { &mp_type_type },
    .name = MP_QSTR_ILI9341,
    .make_new = ILI9341_make_new,
    .locals_dict = (mp_obj_dict_t*)&ILI9341_locals_dict,
};

//-------------------------------------------------------------------------------------------
#endif
