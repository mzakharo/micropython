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
#include <string.h>
#include <math.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"
#include "mpconfigboard.h"
#include "modmachine.h"
#include "extmod/vfs.h"
#include "extmod/utime_mphal.h"
#include "py/obj.h"
#include "py/builtin.h"
#include "py/stream.h"

#if MICROPY_HW_LCD32
#include "ILI9341.h"
#endif

#if MICROPY_HW_LCD15
#include "ST7789.h"
#endif

#if MICROPY_HW_LCD18
#include "ST7735.h"
#endif

#if MICROPY_ENABLE_TFTLCD

#include "modtftlcd.h"
#include "font.h"

_lcd_dev lcddev;

Graphics_Display * draw_global = NULL;

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
#define BUF_LEN 1160
static uint16_t str_buf[BUF_LEN]={0};//申请最大字体内存
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

	for(t=0;t<csize;t++)
	{
		
		if(0){
		#if MICROPY_STRING_SIZE_24
		}else if(size==24){
			temp=asc2_2412[num][t];
		#endif
		#if MICROPY_STRING_SIZE_32
		}else if(size==32){
			temp=asc2_3216[num][t];	
		#endif
		#if MICROPY_STRING_SIZE_48
		}else if(size==48){
			temp=asc2_4824[num][t];
		#endif
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
	display->callDrawFlush(x,y,str_w,str_h,str_buf);

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

void grap_drawCam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{
	if(draw_global == NULL){
		printf("draw_global is null\r\n");
		return;
	}
	draw_global->callDrawCam(x,y,width,height,color);
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
	// if(draw_global == NULL){
		// printf("draw_global is null\r\n");
		// return;
	// }
	draw_global->callDrawFill(x,y,width,height,color);
}
//------------------------------------------------------------------------------------------------------
#if MICROPY_PY_PICLIB

//显示成功返回0，其他失败
uint8_t grap_drawCached(const Graphics_Display *display,
								void *fs, uint16_t x, uint16_t y, const char *filename)
{
	uint16_t i=0;
	uint32_t readlen = 0;
	uint8_t *databuf;    		//数据读取存 
	uint8_t *hardbuf;    		//数据读取存 

	uint16_t rowcnt = 10;				//一次读取的行数
	uint16_t vp_line;	  		 	//垂直整数倍
	uint16_t vp_dot;	  		 	//垂直余数倍
	
	IMAGE2LCD *image2lcd;
	uint16_t display_w,display_h;
	uint16_t *d_color;
	
	hardbuf=(uint8_t*)m_malloc(8);

	int errcode = 0;
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_rb),
	};
	mp_obj_t open_file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

	mp_stream_posix_read(open_file, hardbuf, 8);

	if(errcode == 0){
		image2lcd = (IMAGE2LCD *)hardbuf;
		display_w = image2lcd->w;
		display_h = image2lcd->h;

		vp_line = display_h / rowcnt;
		vp_dot = display_h % rowcnt; //最后剩余的行数
		if(vp_line == 0){
			rowcnt = vp_dot;
		} 
		
		readlen = display_w * 2 * rowcnt; //
		databuf=(uint8_t*)m_malloc(readlen);		//开辟readlen字节的内存区域
		
		if(databuf == NULL)
		{
			m_free(databuf);
			errcode = 1;
			goto error;
		}else
		{
			i=0;
			for(i=0; i < (display_h/rowcnt); i++)
			{
				mp_stream_posix_read(open_file, (uint8_t *)databuf, readlen);
				d_color = (uint16_t *)&databuf[0];
				display->callDrawFlush(x, y+i*rowcnt, x+display_w, rowcnt, d_color);
			}
			if(vp_dot)
			{
				readlen = display_w * 2 * vp_dot; //
				mp_stream_posix_read(open_file, (uint8_t *)databuf, readlen);
				d_color = (uint16_t *)&databuf[0];
				display->callDrawFlush(x, y+i*rowcnt, x+display_w, vp_dot, d_color);
			}
		}
		m_free(databuf);
	}
	
error:
	mp_stream_close(open_file);
	m_free(hardbuf);
	return errcode;
}

// new cached file
uint8_t grap_newCached(const Graphics_Display *display, 
								mp_obj_t stream ,const char *filename, uint16_t width, uint16_t height)
{
static mp_obj_t f_new;
//------------------------------------------------
	uint16_t display_w,display_h;
	uint16_t *r_buf;    		//数据读取存 
	uint16_t i=0,j = 0;
	uint8_t bar = 0;
	uint8_t last_bar = 0;

	ssize_t res = 0;

	uint8_t hard_buf[8] = {0x00,0X10,0x00,0x00,0x00,0x00,0X01,0X1B};

	hard_buf[2] = (uint8_t)width;
	hard_buf[3] = (uint8_t)(width >> 8);
	hard_buf[4] = (uint8_t)height;
	hard_buf[5] = (uint8_t)(height >> 8);

	printf("start loading:0%%\r\n");
	
	display_w = width;
	display_h = height;

	r_buf = (uint16_t *)m_malloc(display_w);
	if(r_buf == NULL){
		mp_raise_ValueError(MP_ERROR_TEXT("malloc r_buf error"));
	}
	
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_wb),
	};
	
	printf("start open:%s\r\n",filename);
	
	f_new = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
	
	res = mp_stream_posix_write(f_new, (uint8_t*)hard_buf, 8);
	
	if(res != 8){
		mp_stream_close(f_new);
		mp_raise_ValueError(MP_ERROR_TEXT("file write hard error"));
	}
	//mp_stream_close(f_new);
	printf("newCached\r\n");
	//return 0;
	for(i =0; i < display_h; i++)
	{
		for(j =0; j<display_w; j++){
			r_buf[j] = display->callReadPoint(j , i);
		}
		res = mp_stream_posix_write(f_new, (uint8_t*)r_buf, display_w*2);
		if(res != display_w){
			grap_drawStr(display, 0,0,12*17,25,24,"Cache Error!     ",RED, WHITE);
			mp_stream_close(f_new);
			mp_raise_ValueError(MP_ERROR_TEXT("file write hard error"));
		}
		bar = (i*100)/display_h;
		if((bar != last_bar) && !(bar%10)) printf("loading:%d%%\r\n",bar);

		if(i==25){
			grap_drawStr(display, 0,0,12*17,25,24,"Image Caching:00%",RED, WHITE);
		}
		if((i >= 25) && (bar != last_bar)){
			//grap_drawChar(display,168,0,bar,2,24,RED, WHITE);
		}
		last_bar = bar;
	}
	grap_drawStr(display, 0, 0, 12*17, 25, 24,"Cache Done!      ",RED, WHITE);
	
	printf("cache done!\r\n");

	mp_stream_close(f_new);
	m_free(r_buf);

  return 0;
}

#endif

STATIC const mp_rom_map_elem_t tftlcd_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_tftlcd) },
	
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
