/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	modtftlcd.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/9/22
	* Description 			 :	
	******************************************************************************
**/

#include "py/runtime.h"
#include "py/mphal.h"
#include "softtimer.h"
#include <math.h>
#include "py/builtin.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "py/obj.h"

#include "systick.h"

#if MICROPY_ENABLE_TFTLCD

#include "font.h"
#include "modtftlcd.h"

#if MICROPY_HW_LCD43M
#include "lcd43m.h"
#endif

#if MICROPY_HW_LTDC_LCD
#include "ltdc.h"
#endif

#if MICROPY_HW_LCD32
#include "ILI9341.h"
#endif

#if MICROPY_HW_LCD15
#include "ST7789.h"
#endif

#if MICROPY_HW_LCD18
#include "ST7735.h"
#endif

_lcd_dev lcddev;
#if MICROPY_PY_PICLIB
uint8_t is_sdcard = 0;
#endif

Graphics_Display * draw_global = NULL;

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
void grap_drawLine(const Graphics_Display *display, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
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
		display->callDrawPoint(uRow, uCol, color);
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

void grap_drawRect(const Graphics_Display *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t border, uint16_t color)
{
	for(uint16_t i=0 ; i < border; i++ )
	{
		grap_drawLine(display, x,   y+i, x+width, y+i,  color);
		grap_drawLine(display, x+i, y, x+i, y+height, color);
		grap_drawLine(display, x, y+height-i ,x+width, y+height-i, color);
		grap_drawLine(display, x+width-i, y, x+width-i, y+height, color);
	}
}

/**
 * @breif	带颜色画圆函数
 * @param   x1,x2 —— 圆心坐标
 * @param	r —— 半径
 * @param	color —— 颜色
 * @retval	none
 *
 */

void grap_drawColorCircle(const Graphics_Display *display,
												uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{

  int16_t a = 0, b = r;
  //int16_t d = 3 - (r << 1);
	uint16_t net_r = 0;

	net_r = r;
	if((x - r) < 0){
		net_r = x;
	}else if((x+r) > display->width){
		net_r = display->width - x;
	}
	
	if((y - net_r) < 0){
		net_r = y;
	}else if((y+net_r) > display->height){
		net_r = display->height - y;
	}	
	
	/* 如果圆在屏幕可见区域外，直接退出 */
	if (x - net_r < 0 || x + net_r > display->width || y - net_r < 0 || y + net_r > display->height) 
	{
		return;
	}
	int16_t d = 3 - (net_r << 1);

	/* 开始画圆 */
	while(a<=b){
		display->callDrawPoint(x+a, y-b, color); //1
		display->callDrawPoint(x+b, y-a, color);
		display->callDrawPoint(x+b, y+a, color); //2
		display->callDrawPoint(x+a, y+b, color);
		display->callDrawPoint(x-a, y+b, color); //3
		display->callDrawPoint(x-b, y+a, color);

		display->callDrawPoint(x-a, y-b, color); //4
		display->callDrawPoint(x-b, y-a, color);
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

//------------------------------------------------------------------------------------------------------


#if MICROPY_HW_BOARD_MAGELLAM
extern char _grapbuf;
static char *str_buf_t = &_grapbuf;
uint16_t *str_buf = NULL;
#else

#define BUF_LEN 1160	
static uint16_t str_buf[BUF_LEN]={0};//申请最大字体内存
#endif

void grap_drawChar(const Graphics_Display *display, uint16_t x,uint16_t y,uint8_t num,
											uint8_t size,uint16_t color,uint16_t backcolor)
{
	uint8_t temp,t1,t;
	uint16_t y0=0,x0=0;;
	uint16_t str_h,str_w;
	//得到字体一个字符对应点阵集所占的字节数
	uint16_t csize=((size>>3)+((size%8)?1:0))*(size>>1);		
 	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）

	str_h = size; 
	str_w = size>>1;
	#if MICROPY_HW_BOARD_MAGELLAM
	str_buf = (uint16_t*)str_buf_t;
	#endif
	for(t=0;t<csize;t++)
	{
		if(size==24)
			temp=asc2_2412[num][t];
		else if(size==32)
			temp=asc2_3216[num][t];	
		else if(size==48)
			temp=asc2_4824[num][t];
		else 
			temp=asc2_1608[num][t];

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
	display->callDrawFlush(x,y,str_w,str_h,&str_buf[0]);
}
static uint32_t grap_lcd_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}			

//------------------------------------------------------------------------------------------------------
void grap_drawNum(const Graphics_Display *display,
										uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint16_t color,uint16_t backcolor)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/grap_lcd_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				grap_drawChar(display,x+(size/2)*t,y,' ',size, color, backcolor);
				continue;
			}else enshow=1; 
		}
		grap_drawChar(display,x+(size/2)*t,y,temp+'0',size, color, backcolor);
	}
} 

void grap_drawStr(const Graphics_Display *display, uint16_t x,uint16_t y,uint16_t width,uint16_t height,
									uint8_t size, char *p , uint16_t color,uint16_t backcolor)
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

		grap_drawChar(display,x,y,*p,size,color, backcolor);
		x+=(size>>1);
		p++;
	}  
}
// void grap_drawCam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
// {
	// if(draw_global == NULL){
		// printf("draw_global is null\r\n");
		// return;
	// }
	// draw_global->callDrawCam(x,y,width,height,color);
// }
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
//------------------------------------------------------------------------------------------------------
#if MICROPY_PY_PICLIB

//显示成功返回0，其他失败
uint8_t grap_drawCached(const Graphics_Display *display,
												FATFS *fs, uint16_t x, uint16_t y, const char *filename)
{
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
				display->callDrawFlush(x, y+i, display_w, 1, d_color);
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
uint8_t grap_newCached(const Graphics_Display *display, uint8_t is_sdcard,
								FATFS *fs, const char *filename, uint16_t width, uint16_t height)
{

//------------------------------------------------
	uint16_t display_w,display_h;
	// uint16_t *r_buf;    		//数据读取存 
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
		#if defined(STM32H7)
		uint16_t r_buf[800];
		// r_buf = (uint16_t*)cache_buf;
		// r_buf = m_malloc(display_w*2);
		#else
		uint16_t *r_buf;	
		r_buf = m_malloc(display_w*2);
		#endif

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
				r_buf[j] = (uint16_t)display->callReadPoint(j , i);
			}
			res=f_write(f_file,(uint8_t *)r_buf,display_w*2,&bw);
			mp_hal_delay_ms(2);
			if(res != FR_OK){
				grap_drawStr(display, 0,0,12*17,25,24,"Cache Error!     ",RED, WHITE);
				mp_raise_ValueError(MP_ERROR_TEXT("file write hard error"));
			}
			bar = (i*100)/display_h;
			if((bar != last_bar) && !(bar%10)) printf("loading:%d%%\r\n",bar);

			if(i==25){
				grap_drawStr(display, 0,0,12*17,25,24,"Image Caching:00%",RED, WHITE);
			}
			if((i >= 25) && (bar != last_bar)){
				grap_drawNum(display,168,0,bar,2,24,RED,WHITE);
			}
			last_bar = bar;
			if(is_sdcard)
			{
				mp_hal_delay_ms(2);
			}
		}
		grap_drawStr(display, 0, 0, 12*17, 25, 24,"Cache Done!      ",RED, WHITE);
		printf("cache done!\r\n");

		f_close(f_file);
		m_free(f_file);
		#if defined(STM32F4)
		m_free(r_buf);
		#endif
	}

  return 0;
}

#endif

STATIC const mp_rom_map_elem_t tftlcd_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_tftlcd) },
	
	#if MICROPY_HW_LCD43M
	{ MP_ROM_QSTR(MP_QSTR_LCD43M), MP_ROM_PTR(&tftlcd_lcd43m_type) },
	#endif
	
	#if MICROPY_HW_LCD43R
	{ MP_ROM_QSTR(MP_QSTR_LCD43R), MP_ROM_PTR(&tftlcd_lcd43r_type) },
	#endif
	
	#if MICROPY_HW_LCD7R
	{ MP_ROM_QSTR(MP_QSTR_LCD7R), MP_ROM_PTR(&tftlcd_lcd7r_type) },
	#endif
	
	#if (MICROPY_HW_LCD32)
	{ MP_ROM_QSTR(MP_QSTR_LCD32), MP_ROM_PTR(&ILI9341_type) },
	{ MP_ROM_QSTR(MP_QSTR_LCD24), MP_ROM_PTR(&ILI9341_type) },
	#endif
	#if (MICROPY_HW_LCD15)
	{ MP_ROM_QSTR(MP_QSTR_LCD15), MP_ROM_PTR(&ST7789_type) },
	#endif
	#if (MICROPY_HW_LCD18)
	{ MP_ROM_QSTR(MP_QSTR_LCD18), MP_ROM_PTR(&ST7735_type) },
	#endif

};
STATIC MP_DEFINE_CONST_DICT(tftlcd_module_globals, tftlcd_module_globals_table);

const mp_obj_module_t tftlcd_module = {
		.base = { &mp_type_module },
		.globals = (mp_obj_dict_t *)&tftlcd_module_globals,
};
#endif
