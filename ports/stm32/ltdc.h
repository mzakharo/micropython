/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	ltdc.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef MICROPY_INCLUDED_STM32_LTDC_H
#define MICROPY_INCLUDED_STM32_LTDC_H

#ifdef __cplusplus
 extern "C" {
#endif 
#include "py/obj.h"
#include "modtftlcd.h"

#if MICROPY_HW_LTDC_LCD


//LCD LTDC重要参数集
typedef struct  
{							 
	uint16_t pwidth;			//LCD面板的宽度,固定参数,不随显示方向改变,如果为0,说明没有任何RGB屏接入
	uint16_t pheight;		//LCD面板的高度,固定参数,不随显示方向改变
	uint16_t hsw;			//水平同步宽度
	uint16_t vsw;			//垂直同步宽度
	uint16_t hbp;			//水平后廊
	uint16_t vbp;			//垂直后廊
	uint16_t hfp;			//水平前廊
	uint16_t vfp;			//垂直前廊 
	uint8_t layer;		//当前层编号:0/1	
	uint8_t pixsize;		//每个像素所占字节数
	uint32_t LayerAdd;
	uint32_t ltdc_format;
}_ltdc_dev; 

//LCD参数
extern _ltdc_dev ltdcdev;	//管理LCD重要参数
extern volatile uint32_t *ltdc_framebuf[2];
extern Graphics_Display ltdc_glcd;

extern LTDC_HandleTypeDef  LTDC_Handler;	    //LTDC句柄
extern DMA2D_HandleTypeDef DMA2D_Handler; 	    //DMA2D句柄
extern void ltdc_conf(void);
extern void ltdc_init(void);
extern uint8_t ltdc_set_clk(uint32_t pll3r);


void ltdc_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
void set_lcd_dir(uint8_t dir);
void ltdc_fill_rgb565(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color);
void ltdc_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color);
void ltdc_full_rgb565(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color);
void ltdc_clear(uint16_t color);
#if MICROPY_HW_LCD7R
void lcd7r_full_cam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color);
extern const mp_obj_type_t tftlcd_lcd7r_type;
#endif

#if MICROPY_HW_LCD43R
void lcd43g_init(void);
void lcd43g_full_cam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color);
extern const mp_obj_type_t tftlcd_lcd43r_type;
#endif

#endif

/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __LTDC_H */

