/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modtftlcd.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/3/25
	* Description			:	
******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODTFTLCD_H
#define __MODTFTLCD_H

#ifdef __cplusplus
 extern "C" {
#endif 
#include "py/obj.h"
#include "py/stream.h"

/* Includes ------------------------------------------------------------------*/  

#define WHITE         	 	0xFFFF
#define BLACK         	 	0x0000	  
#define BLUE				0x001F  
#define RED           	 	0xF800
#define GREEN         	 	0x07E0
#define BRED             	0XF81F
#define GRED				0XFFE0
#define GBLUE				0X07FF
#define MAGENTA       	 	0xF81F
#define CYAN          	 	0x7FFF
#define YELLOW        	 	0xFFE0
#define BROWN				0XBC40 //棕色
#define BRRED				0XFC07 //棕红色
#define GRAY				0X8430 //灰色

typedef struct  
{		 	 
	volatile uint16_t width;			//LCD 宽度
	volatile uint16_t height;			//LCD 高度
	volatile uint16_t id;					//LCD ID
	volatile uint8_t  dir;				//	
	// 1->MCU LCD,
	// 2->4.3RGB
	// 3->7RGB
	// 4->3.2电阻
	// 5->1.5cld
	// 6->1.77
	volatile uint8_t  type;				
	volatile uint16_t backcolor; 	//默认字体背景色
	volatile uint16_t clercolor; 	//清屏颜色
	
	volatile uint16_t x_pixel;
	volatile uint16_t y_pixel;
	
}_lcd_dev;

extern _lcd_dev lcddev;
//-----------------------------------------------------
//解码image2lcd数据
typedef struct
{
	uint8_t scan;
	uint8_t gray;
	uint16_t w;
	uint16_t h;
	uint8_t is565;
	uint8_t rgb;
}__attribute__((packed)) IMAGE2LCD; 

typedef struct
{
	uint8_t color_depth; //8,16,32
	volatile uint16_t width;
	volatile uint16_t height;
	void (*callDrawPoint)(uint16_t x, uint16_t y,uint16_t color);
	uint16_t (*callReadPoint)(uint16_t x, uint16_t y);
	void (*callDrawhline)(uint16_t x,uint16_t y,uint16_t len,uint16_t color);
	void (*callDrawvline)(uint16_t x,uint16_t y,uint16_t len,uint16_t color);
	void (*callDrawFill)(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color);
	void (*callDrawFlush)(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
	void (*callDrawCam)(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
	void (*callDrawClear)(uint16_t color);
} Graphics_Display;

extern void grap_drawPoint(uint16_t x, uint16_t y,uint16_t color);
extern uint16_t grap_ReadPoint(uint16_t x, uint16_t y);
extern void grap_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
extern void grap_drawHline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color);
extern void grap_drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t border, uint16_t color);
extern void grap_drawColorCircle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
extern void grap_drawChar(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint16_t color,uint16_t backcolor);
extern void grap_drawStr(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size, char *p , uint16_t color,uint16_t backcolor);
extern void grap_drawNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint16_t color,uint16_t backcolor);
extern void grap_drawFull(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color);
extern void grap_drawFill(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color);

uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color);
uint16_t get_bgr565(uint8_t r_color, uint8_t g_color , uint8_t b_color);
uint32_t get_rgb888(uint8_t r_color, uint8_t g_color , uint8_t b_color);
uint32_t rgb888tobgr888(uint32_t color);
uint32_t rgb565torgb888(uint16_t color);
uint32_t bgr2rgb(uint32_t color);
uint16_t rgb565tobgr565(uint16_t color);
uint16_t bgr565torgb565(uint16_t color);

extern Graphics_Display * draw_global;


mp_obj_t tftlcd_printStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
mp_obj_t tftlcd_write_buf(size_t n_args, const mp_obj_t *args);
mp_obj_t tftlcd_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
mp_obj_t tftlcd_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
mp_obj_t tftlcd_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) ;
mp_obj_t tftlcd_drawL(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
mp_obj_t tftlcd_drawPixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);

#define X_PIXEL 480
#define Y_PIXEL 800

__attribute__ ((aligned (256))) uint16_t LTDC_Buf[X_PIXEL*Y_PIXEL+10];


#if MICROPY_PY_PICLIB

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

extern uint8_t grap_drawCached(FATFS *fs, uint16_t x, uint16_t y, const char *filename);
extern uint8_t grap_newCached(FATFS *fs, const char *filename, uint16_t width, uint16_t height);

extern mp_obj_t tftlcd_Picture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
extern mp_obj_t tftlcd_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);

#endif









/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __MODTFTLCD_H */

