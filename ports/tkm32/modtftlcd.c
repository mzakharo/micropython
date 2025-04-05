/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modtftlcd.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/3/25
	* Description			:	
******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "storage.h"
#include "sdcard.h"

#include "modmachine.h"
#include "extmod/utime_mphal.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "extmod/vfs_lfs.h"
#include "bufhelper.h"

#include <math.h>

#include "py/builtin.h"

#if MICROPY_ENABLE_TFTLCD
#include "global.h" 
#include "modtftlcd.h"
#include "font.h"

#if MICROPY_HW_LCD7R
#include "lcd7r.h"
#endif

#if MICROPY_HW_LCD43G
#include "lcd43g.h"
#endif

//-------------------------------------------------------------
_lcd_dev lcddev;
Graphics_Display * draw_global = NULL;

//-------------------------------------------------------------

uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color)
{
    r_color = ((r_color & 0xF8));
    g_color = ((g_color & 0xFC));
    b_color = ((b_color & 0xF8)>>3);
    return (((uint16_t)r_color << 8) + ((uint16_t)g_color << 3) + b_color );
}

uint16_t get_bgr565(uint8_t r_color, uint8_t g_color , uint8_t b_color)
{
    r_color = (r_color & 0xF8);
    g_color = (g_color & 0xFC);
    b_color = (b_color & 0xF8);
    return (((uint16_t)r_color >> 3) + ((uint16_t)g_color << 3) + ((uint16_t)b_color<<8) );
}

uint32_t get_rgb888(uint8_t r_color, uint8_t g_color , uint8_t b_color)
{
	uint32_t color = (((uint32_t)r_color << 16) | ((uint32_t)g_color << 8) | b_color ) ;
	
	return (uint32_t)(color & 0xFFFFFF);
}

uint32_t rgb888tobgr888(uint32_t color)
{
	uint32_t color_r=0,color_b = 0;
	color_r = ((color & 0xFF0000U) >> 16);
	color_b = ((color & 0xFFU) << 16);
	color &= 0xFF00U;
	color = (color | color_r | color_b);
	return color;
}

uint32_t rgb565torgb888(uint16_t color)
{
	uint32_t color_r=0,color_g=0,color_b = 0;
	color_r = ((uint32_t)(color & RED) << 8);
	color_g = ((uint32_t)(color & GREEN) << 5);
	color_b = ((uint32_t)(color & BLUE) << 3);

	return (uint32_t)(color_r | color_g | color_b);
}

uint32_t bgr2rgb(uint32_t color)
{
	uint32_t color_r=0,color_b = 0;
	color_b = ((color & 0xFF0000U) >> 16);
	color_r = ((color & 0xFFU) << 16);
	color &= 0xFF00U;
	color = (color | color_r | color_b);
	return color;
}

uint16_t rgb565tobgr565(uint16_t color)
{
	uint16_t color_r=0,color_b = 0;
	color_r = ((color & 0xF800) >> 11);
	color_b = ((color & 0x001F) << 11);
	color &= 0x07E0;
	color = (color | color_r | color_b);
	return color;
}
uint16_t bgr565torgb565(uint16_t color)
{
	uint16_t color_r=0,color_b = 0;
	color_b = ((color & 0xF800) >> 11);
	color_r = ((color & 0x001F) << 11);
	color &= 0x07E0;
	color = (color | color_r | color_b);
	return color;
}
static uint32_t lcd_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}		

void grap_drawPoint(uint16_t x, uint16_t y,uint16_t color)
{
	if(draw_global == NULL) return;
	draw_global->callDrawPoint(x, y, color);
}

uint16_t grap_ReadPoint(uint16_t x, uint16_t y)
{
	if(draw_global == NULL) return 0;
	return draw_global->callReadPoint(x,y);
}

void grap_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	if(draw_global == NULL) return;
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; 
	else if(delta_x==0)incx=0;
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )
	{  
		draw_global->callDrawPoint(uRow, uCol, color);
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	} 
}

void grap_drawHline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if(draw_global == NULL) return;
	if((len==0)||(x0>draw_global->width)||(y0>draw_global->height))return;
	draw_global->callDrawFill(x0,y0,x0+len-1,y0,color);	
}

void grap_drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t border, uint16_t color)
{
	if(draw_global == NULL) return;
	for(uint16_t i=0 ; i < border; i++ )
	{
		grap_drawLine(x,   y+i, x+width, y+i,  color);
		grap_drawLine(x+i, y, x+i, y+height, color);
		grap_drawLine(x, y+height-i ,x+width, y+height-i, color);
		grap_drawLine(x+width-i, y, x+width-i, y+height, color);
	}
}

void grap_drawColorCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
	if(draw_global == NULL) return;
	int16_t a = 0, b = r;
	uint16_t net_r = 0;

	net_r = r;
	if((x - r) < 0){
		net_r = x;
	}else if((x+r) > draw_global->width){
		net_r = draw_global->width - x;
	}
	
	if((y - net_r) < 0){
		net_r = y;
	}else if((y+net_r) > draw_global->height){
		net_r = draw_global->height - y;
	}	
	
	/* 如果圆在屏幕可见区域外，直接退出 */
	if (x - net_r < 0 || x + net_r > draw_global->width || y - net_r < 0 || y + net_r > draw_global->height) 
	{
		return;
	}
	int16_t d = 3 - (net_r << 1);

	/* 开始画圆 */
	while(a<=b){
		draw_global->callDrawPoint(x+a, y-b, color); //1
		draw_global->callDrawPoint(x+b, y-a, color);
		draw_global->callDrawPoint(x+b, y+a, color); //2
		draw_global->callDrawPoint(x+a, y+b, color);
		draw_global->callDrawPoint(x-a, y+b, color); //3
		draw_global->callDrawPoint(x-b, y+a, color);

		draw_global->callDrawPoint(x-a, y-b, color); //4
		draw_global->callDrawPoint(x-b, y-a, color);
		a++;
		//使用Bresenham算法画圆
		if(d<0){
			d += (a<<2) + 6;
		}else{
			d += 10 + ((a - b)<<2);
			b--;
		}
	}
}

#define BUF_LEN 1160
static uint16_t str_buf[BUF_LEN]={0};//申请最大字体内存
void grap_drawChar(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint16_t color,uint16_t backcolor)
{
	if(draw_global == NULL) return;
	uint8_t temp,t1,t;
	uint16_t y0=0,x0=0;;
	uint16_t str_h,str_w;
	//得到字体一个字符对应点阵集所占的字节数
	uint16_t csize=((size>>3)+((size%8)?1:0))*(size>>1);		
 	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）

	str_h = size; 
	str_w = size>>1;

	for(t=0;t<csize;t++)
	{
		
		if(0){
		}else if(size==24){
			temp=asc2_2412[num][t];
		}else if(size==32){
			temp=asc2_3216[num][t];	
		}else if(size==48){
			temp=asc2_4824[num][t];
		}else{
			temp=asc2_1608[num][t];
		}
		for(t1=0;t1<8;t1++)
		{
			if(temp&0x80){
				str_buf[str_w * y0 + x0] = color;
			}else{
				str_buf[str_w * y0 + x0] = backcolor;
			}
			y0++;
			if(y0 >= size){
				y0 = 0;
				x0++;
			}
			temp<<=1;
		}  	 
	}
	draw_global->callDrawFlush(x,y,str_w,str_h,str_buf);
}

void grap_drawStr(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size, char *p , uint16_t color,uint16_t backcolor)
{         
	uint8_t x0=x;
	width+=x;
	height+=y;
	while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
	{       
		if(x>=width)
		{
			x=x0;
			y+=size;
		}
		if(y>=height)break;//退出

		grap_drawChar(x,y,*p,size,color, backcolor);
		x+=(size>>1);
		p++;
	}  
}

void grap_drawNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint16_t color,uint16_t backcolor)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/lcd_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				grap_drawChar(x+(size/2)*t,y,' ',size,color, backcolor);
				continue;
			}else enshow=1; 
		}
		grap_drawChar(x+(size/2)*t,y,temp+'0',size,color, backcolor);
	}
} 
//---------------------------------------------------------------------------

mp_obj_t tftlcd_printStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	STATIC const mp_arg_t tft_allowed_args[] = {
		{ MP_QSTR_text,     		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_x,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_backcolor,		MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
		{ MP_QSTR_size,      		MP_ARG_INT, {.u_int = 2} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);

	uint16_t text_size = args[5].u_int;
	uint32_t color = 0;
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
	if(args[0].u_obj !=MP_OBJ_NULL) {
		mp_buffer_info_t bufinfo;
		if (mp_obj_is_int(args[0].u_obj)) {
			mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter error"));
		}else{
			mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);
			char *str = bufinfo.buf;

			if(text_size == 1)  text_size = 16;
			else if(text_size == 2) text_size = 24;
			else if(text_size == 3) text_size = 32;
			else if(text_size == 4) text_size = 48;
			else mp_raise_ValueError(MP_ERROR_TEXT("lcd size parameter error"));
			grap_drawStr(args[1].u_int, args[2].u_int, text_size* bufinfo.len, text_size , text_size,str ,color,lcddev.backcolor);
		}
	}else{
		mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
	}
	return mp_const_none;
}

mp_obj_t tftlcd_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	STATIC const mp_arg_t tft_allowed_args[] = {
		{ MP_QSTR_x,			MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y,			MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_radius,		MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_color,		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_border,		MP_ARG_INT, {.u_int = 1} }, 
		{ MP_QSTR_fillcolor,	MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL} }, //7
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);

	uint16_t color=0;

	//Circlecolor
	if(args[3].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[3].u_obj, &len, &params);
		if(len == 3){
			color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			for(uint16_t i=0; i < args[4].u_int ;i++) {
				grap_drawColorCircle(args[0].u_int, args[1].u_int, args[2].u_int-i, color);
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
			grap_drawColorCircle(args[0].u_int,args[1].u_int,args[2].u_int-args[4].u_int-i,color);
		}
	}
	return mp_const_none;
}

mp_obj_t tftlcd_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	STATIC const mp_arg_t Rect_args[] = {
		{ MP_QSTR_x,			MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y,			MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_width,		MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_height,		MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_color,		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_border,		MP_ARG_INT, {.u_int = 1} }, 
		{ MP_QSTR_fillcolor,	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(Rect_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(Rect_args), Rect_args, args);

	if(args[4].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[4].u_obj, &len, &params);
		if(len == 3){
			grap_drawRect(args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,
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
		uint32_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
		for(uint16_t i=0 ; i <= (args[3].u_int-(args[5].u_int*2)); i++ ) 
			grap_drawLine(args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
	}
  return mp_const_none;
}

mp_obj_t tftlcd_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
	static const mp_arg_t clear_args[] = {
	{ 
		MP_QSTR_fillcolor,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
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
			draw_global->callDrawClear(lcddev.backcolor);
			lcddev.clercolor = lcddev.backcolor;
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}
	return mp_const_none;
}

mp_obj_t tftlcd_drawL(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
			grap_drawLine(args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,
			 get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
		}
	}
	return mp_const_none;
}

mp_obj_t tftlcd_drawPixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t drawp_args[] = {
		{ MP_QSTR_x,		MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y,		MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_color,	MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(drawp_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawp_args), drawp_args, args);

	if(args[2].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[2].u_obj, &len, &params);
		if(len == 3){
			draw_global->callDrawPoint(args[0].u_int,args[1].u_int ,
			get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
		}
	}

  return mp_const_none;
}

mp_obj_t tftlcd_write_buf(size_t n_args, const mp_obj_t *args) {
	if(6 != n_args || draw_global == NULL) {
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

	draw_global->callDrawFlush(start_x, start_y,width,height,(uint16_t *)lcd_write_data.buf);

    return mp_obj_new_int(1);
}

void grap_drawFull(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{
	if(draw_global == NULL){
		printf("draw_global is null\r\n");
		return;
	}
	draw_global->callDrawFlush(x,y,width,height,color);
}

void grap_drawFill(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color)
{
	if(draw_global == NULL){
		printf("draw_global is null\r\n");
		return;
	}
	draw_global->callDrawFill(x,y,width,height,color);
}


#if MICROPY_PY_PICLIB

#include "piclib.h" 

uint8_t grap_drawCached(FATFS *fs, uint16_t x, uint16_t y, const char *filename)
{
	if(draw_global == NULL) return 0;
	uint32_t readlen = 0;
	uint8_t *databuf;    		//数据读取存 
	uint8_t *hardbuf;    		//数据读取存 
	UINT br;
	IMAGE2LCD *image2lcd;
	uint16_t display_w,display_h;
	uint16_t *d_color;
	
	FIL* f_file;
	f_file=(FIL *)m_malloc(sizeof(FIL));
	hardbuf=(uint8_t*)m_malloc(8);

	uint8_t res = f_open(fs,f_file,filename,FA_READ);
	res = f_read(f_file,hardbuf,8,&br); //读取头信息
	
	if(res == 0){
		image2lcd = (IMAGE2LCD *)hardbuf;
		display_w = image2lcd->w;
		display_h = image2lcd->h;
		readlen = display_w * 2;
		databuf=(uint8_t*)m_malloc(readlen);		//开辟readlen字节的内存区域
		
		if(databuf == NULL)
		{
			m_free(databuf);
			res = 1;
			goto error;
		}else
		{
			for(uint16_t i=0; i < display_h; i++)
			{
				f_read(f_file,(uint8_t *)databuf,readlen,&br);
				d_color = (uint16_t *)&databuf[0];
				draw_global->callDrawFlush(x, y+i, display_w, 1, d_color);
			}
		}

	}
	
error:
	f_close(f_file);
	
	m_free(f_file);
	m_free(hardbuf);
	
	return res;
}

// new cached file
uint8_t grap_newCached(FATFS *fs, const char *filename, uint16_t width, uint16_t height)
{
	if(draw_global == NULL) return  0;
	uint16_t display_w,display_h;
	uint16_t i=0,j = 0;
	uint8_t bar = 0;
	uint8_t last_bar = 0;
	UINT bw;
	ssize_t res = 0;
	{
		uint8_t hard_buf[8] = {0x00,0X10,0x00,0x00,0x00,0x00,0X01,0X1B};
		hard_buf[2] = (uint8_t)width;
		hard_buf[3] = (uint8_t)(width >> 8);
		hard_buf[4] = (uint8_t)height;
		hard_buf[5] = (uint8_t)(height >> 8);

		printf("start loading:0%%\r\n");

		display_w = width;
		display_h = height;

		uint16_t *r_buf;	
		r_buf = m_malloc(display_w*2);

		if(r_buf == NULL){
			mp_raise_ValueError(MP_ERROR_TEXT("malloc r_buf error"));
		}

		FIL* f_file;
		f_file=(FIL *)m_malloc(sizeof(FIL));

		res = f_open(fs,f_file,filename,FA_WRITE|FA_CREATE_ALWAYS);
		if(res != FR_OK){
			mp_raise_ValueError(MP_ERROR_TEXT("path_buf open file error"));
		}

		res=f_write(f_file,hard_buf,8,&bw);
		if(res != FR_OK){
			mp_raise_ValueError(MP_ERROR_TEXT("file write hard error"));
		}

		for(i =0; i < display_h; i++)
		{
			for(j =0; j<display_w; j++){
				r_buf[j] = (uint16_t)draw_global->callReadPoint(j , i);
			}
			res=f_write(f_file,(uint8_t *)r_buf,display_w*2,&bw);
			mp_hal_delay_ms(2);
			if(res != FR_OK){
				grap_drawStr( 0,0,12*17,25,24,"Cache Error!     ",RED, WHITE);
				mp_raise_ValueError(MP_ERROR_TEXT("file write hard error"));
			}
			bar = (i*100)/display_h;
			if((bar != last_bar) && !(bar%10)) printf("loading:%d%%\r\n",bar);

			if(i==25){
				grap_drawStr( 0,0,12*17,25,24,"Image Caching:00%",RED, WHITE);
			}
			if((i >= 25) && (bar != last_bar)){
				grap_drawNum(168,0,bar,2,24,RED,WHITE);
			}
			last_bar = bar;
		}
		grap_drawStr(0, 0, 12*17, 25, 24,"Cache Done!      ",RED, WHITE);
		printf("cache done!\r\n");

		f_close(f_file);
		m_free(f_file);
		m_free(r_buf);
	}

  return 0;
}

mp_obj_t tftlcd_Picture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	STATIC const mp_arg_t tft_allowed_args[] = { 
		{ MP_QSTR_x,		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_y,		MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_file,		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_cached, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
	};

	uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
	mp_arg_val_t args[arg_num];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);

	uint8_t res=0;
	if(args[2].u_obj !=MP_OBJ_NULL) 
	{
		mp_buffer_info_t bufinfo;
		if (mp_obj_is_int(args[2].u_obj)) {
			mp_raise_ValueError(MP_ERROR_TEXT("picture parameter error"));
		}else {
			mp_get_buffer_raise(args[2].u_obj, &bufinfo, MP_BUFFER_READ);
			mp_obj_t tuple[2];
			const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
			const char *ftype = mp_obj_str_get_str(file_type(file_path));
			mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
			if(res == 1){
				vfs = vfs->next;
			}
			fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
			//---------------------------------------------------------------
			if(args[3].u_bool == true){
				uint8_t file_len = strlen(file_path);
				char *file_buf = (char *)m_malloc(file_len+7);	
				memset(file_buf, '\0', file_len+7);
				sprintf(file_buf,"%s%s",file_path,".cache");
				res = grap_drawCached(&vfs_fat->fatfs, args[0].u_int, args[1].u_int, (const char *)file_buf);
				m_free(file_buf);
				if(!res) return mp_const_none;
			}
			//---------------------------------------------------------------
			piclib_init();
			if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0){
				jpg_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int ,1);
			}else if(strncmp(ftype , "bmp" , 3) == 0){
				stdbmp_decode(&vfs_fat->fatfs ,file_path, args[0].u_int, args[1].u_int) ;
			}else{
				mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
				return mp_const_none;
			}
			tuple[0] = mp_obj_new_int(picinfo.S_Height);
			tuple[1] = mp_obj_new_int(picinfo.S_Width);
			return mp_obj_new_tuple(2, tuple);
		}
	}else{
	mp_raise_ValueError(MP_ERROR_TEXT("picture parameter is empty"));
	}
    return mp_const_none;
}

mp_obj_t tftlcd_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	STATIC const mp_arg_t tft_allowed_args[] = { 
		{ MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
	};

	uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
	mp_arg_val_t args[arg_num];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);

	uint8_t res=0;

	char *path_buf = (char *)m_malloc(50);  //最大支持50字符
	memset(path_buf, '\0', 50);

	if(args[0].u_obj !=MP_OBJ_NULL) {
		mp_buffer_info_t bufinfo;
		if (mp_obj_is_int(args[0].u_obj)) {
			mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter error,should is .cache"));
		} else{
			mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

			const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
			const char *ftype = mp_obj_str_get_str(file_type(file_path));
			mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);

			if(res == 1){
				vfs = vfs->next;
			}
			fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);

			//test file
			FIL	*f_file;
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
			if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0){
				res = jpg_decode(&vfs_fat->fatfs,file_path, 0, 0 ,1);
				if(res){
					printf("jpg_decode err:%d\r\n",res);
					return mp_const_none;
				}
			}else if(strncmp(ftype , "bmp" , 3) == 0){
				res = stdbmp_decode(&vfs_fat->fatfs ,file_path, 0, 0) ;
				printf("bmp_decode err:%d\r\n",res);
				if(res)return mp_const_none;
			}else{
				mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
				return mp_const_none;
			}
			//-----------------------------------------------------------
			if(args[1].u_obj !=MP_OBJ_NULL){
				mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_READ);
				const char *path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
				const char *path_ftype = mp_obj_str_get_str(file_type(path));

				if(strncmp(path_ftype , "cache" , 5)){
					mp_raise_ValueError(MP_ERROR_TEXT("CachePicture path file type error"));
					return mp_const_none;
				}
				sprintf(path_buf,"%s",path);
			}else{
				sprintf(path_buf,"%s%s",file_path,".cache");
			}
			//------------------------------------------------
			res = f_open(&vfs_fat->fatfs,f_file,path_buf,FA_READ);
			f_close(f_file);
			f_sync(f_file);

			if(args[2].u_bool == true || res != 0){
				grap_newCached(&vfs_fat->fatfs, path_buf,picinfo.S_Width, picinfo.S_Height);	
			}

			f_sync(f_file);
			m_free(f_file);
		}
	}else{
		mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter is empty"));
	}
	m_free(path_buf);
	return mp_const_none;
}


#endif


//--------------------------------------------------------------
STATIC const mp_rom_map_elem_t tftlcd_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_tftlcd) },
	
	#if MICROPY_HW_LCD43G
	{ MP_ROM_QSTR(MP_QSTR_LCD43R), MP_ROM_PTR(&tftlcd_lcd43g_type) },
	#endif
	
	#if MICROPY_HW_LCD7R
	{ MP_ROM_QSTR(MP_QSTR_LCD7R), MP_ROM_PTR(&tftlcd_lcd7r_type) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(tftlcd_module_globals, tftlcd_module_globals_table);

const mp_obj_module_t tftlcd_module = {
		.base = { &mp_type_module },
		.globals = (mp_obj_dict_t *)&tftlcd_module_globals,
};
#endif
