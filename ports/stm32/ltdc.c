/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	ltdc.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/10/14
	* Description 			 :	
	******************************************************************************
**/

#include "py/runtime.h"
#include "py/mphal.h"
#include "bufhelper.h"
#include <math.h>
#include "py/builtin.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "py/obj.h"
#include "pin.h"
#include "pin_static_af.h"
#include "mpu.h"
#include "systick.h"

#if (MICROPY_HW_LTDC_LCD)

#include "ltdc.h"
#include "modtftlcd.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

#include "global.h" 

LTDC_HandleTypeDef  LTDC_Handler;	    //LTDC句柄
DMA2D_HandleTypeDef DMA2D_Handler; 	    //DMA2D句柄

extern char _ltdcbuf;
static char *lcd_buf = &_ltdcbuf;

volatile uint32_t *ltdc_framebuf[2];
volatile uint32_t* ltdc_buf_t=NULL;

_ltdc_dev ltdcdev;

Graphics_Display ltdc_glcd;
//==============================================================================

uint8_t ltdc_set_clk(uint32_t pll3r)
{
	RCC_PeriphCLKInitTypeDef PeriphClkIniture;
	PeriphClkIniture.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
	#if defined(STM32F4)
	PeriphClkIniture.PLLSAI.PLLSAIN=350;   
	PeriphClkIniture.PLLSAI.PLLSAIR=pll3r;  
	PeriphClkIniture.PLLSAIDivR=RCC_PLLSAIDIVR_2;
	#elif defined(STM32F7)
	PeriphClkIniture.PLLSAI.PLLSAIN=175;  
	PeriphClkIniture.PLLSAI.PLLSAIR=pll3r;  
	PeriphClkIniture.PLLSAIDivR=RCC_PLLSAIDIVR_2;
	#elif defined(STM32H7)
	PeriphClkIniture.PLL3.PLL3M = MICROPY_HW_CLK_PLL3M;    
	PeriphClkIniture.PLL3.PLL3N = MICROPY_HW_CLK_PLL3N;
	PeriphClkIniture.PLL3.PLL3P = MICROPY_HW_CLK_PLL3P;
	PeriphClkIniture.PLL3.PLL3Q = MICROPY_HW_CLK_PLL3Q;  
	PeriphClkIniture.PLL3.PLL3R = pll3r;
	#endif
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkIniture)==HAL_OK){
        return 0;
    }
    else return 1;   
}

//LCD初始化函数
void ltdc_init(void)
{
	__HAL_RCC_LTDC_CLK_ENABLE();
	__HAL_RCC_DMA2D_CLK_ENABLE();

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R3);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R5, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R5);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R6, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R6);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R7, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R7);

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G2, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G2);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G3);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G5, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G5);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G6, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G6);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G7, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G7);
	
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B3);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B5, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B5);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B6, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B6);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B7, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B7);
	
	if(ltdcdev.ltdc_format == LTDC_PIXEL_FORMAT_RGB565){
		// printf("LTDC_PIXEL_FORMAT_RGB565\r\n");
		mp_hal_pin_config(MICROPY_HW_LCD_R0, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_R1, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_R2, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_G0, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_G1, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_B0, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_B1, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		mp_hal_pin_config(MICROPY_HW_LCD_B2, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN, 0);
		
		mp_hal_pin_low(MICROPY_HW_LCD_R0);
		mp_hal_pin_low(MICROPY_HW_LCD_R1);
		mp_hal_pin_low(MICROPY_HW_LCD_R2);
		mp_hal_pin_low(MICROPY_HW_LCD_G0);
		mp_hal_pin_low(MICROPY_HW_LCD_G1);
		mp_hal_pin_low(MICROPY_HW_LCD_B0);
		mp_hal_pin_low(MICROPY_HW_LCD_B1);
		mp_hal_pin_low(MICROPY_HW_LCD_B2);
	}else{
		// printf("LTDC_PIXEL_FORMAT_RGB888\r\n");
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R0);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R1, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R1);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_R2, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_R2);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G0);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_G1, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_G1);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B0);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B1, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B1);
		mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_B2, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_B2);
	}

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LTDC_DE, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_DE);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LTDC_PCLK, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_CLK);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LTDC_HSYNC, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_HSYNC);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LTDC_VSYNC, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_LCD_VSYNC);
}

void LTDC_Layer_Window_Config(uint8_t layerx,uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{
    HAL_LTDC_SetWindowPosition(&LTDC_Handler,sx,sy,layerx);  //设置窗口的位置
    HAL_LTDC_SetWindowSize(&LTDC_Handler,width,height,layerx);//设置窗口大小    	
}
  
void LTDC_Layer_Parameter_Config(uint8_t layerx,uint32_t bufaddr,uint8_t alpha,uint8_t alpha0,uint8_t bfac1,uint8_t bfac2,uint32_t bkcolor)
{
	LTDC_LayerCfgTypeDef pLayerCfg;
	
	pLayerCfg.WindowX0=0;
	pLayerCfg.WindowY0=0;
	pLayerCfg.WindowX1=ltdcdev.pwidth;
	pLayerCfg.WindowY1=ltdcdev.pheight; 
	
	pLayerCfg.PixelFormat=ltdcdev.ltdc_format;
	pLayerCfg.Alpha=alpha;
	pLayerCfg.Alpha0=alpha0;
	pLayerCfg.BlendingFactor1=(uint32_t)bfac1<<8;
	pLayerCfg.BlendingFactor2=(uint32_t)bfac2<<8;
	
// pLayerCfg.BlendingFactor1=LTDC_BLENDING_FACTOR1_CA;    //设置层混合系数
// pLayerCfg.BlendingFactor2=LTDC_BLENDING_FACTOR2_CA;	//设置层混合系数
	
	pLayerCfg.FBStartAdress=bufaddr;
	pLayerCfg.ImageWidth=ltdcdev.pwidth;  
	pLayerCfg.ImageHeight=ltdcdev.pheight;
	pLayerCfg.Backcolor.Red=(uint8_t)(bkcolor&0X00FF0000)>>16;
	pLayerCfg.Backcolor.Green=(uint8_t)(bkcolor&0X0000FF00)>>8;
	pLayerCfg.Backcolor.Blue=(uint8_t)bkcolor&0X000000FF;

	HAL_LTDC_ConfigLayer(&LTDC_Handler,&pLayerCfg,layerx);
	// HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);
	// HAL_NVIC_EnableIRQ(LTDC_IRQn);
	//HAL_LTDC_EnableDither(&LTDC_Handler); // 开启颜色抖动
}  

void ltdc_conf(void)
{
	//LTDC配置
	LTDC_Handler.Instance=LTDC;
	
	if(lcddev.type == 2){
		LTDC_Handler.Init.HSPolarity=LTDC_HSPOLARITY_AH;
		LTDC_Handler.Init.PCPolarity=LTDC_PCPOLARITY_IIPC;
	}else{
		LTDC_Handler.Init.HSPolarity=LTDC_HSPOLARITY_AL;
		LTDC_Handler.Init.PCPolarity=LTDC_PCPOLARITY_IPC;	
	}

	LTDC_Handler.Init.VSPolarity=LTDC_VSPOLARITY_AL;
	LTDC_Handler.Init.DEPolarity=LTDC_DEPOLARITY_AL;
	LTDC_Handler.Init.HorizontalSync=ltdcdev.hsw-1;
	LTDC_Handler.Init.VerticalSync=ltdcdev.vsw-1;
	LTDC_Handler.Init.AccumulatedHBP=ltdcdev.hsw+ltdcdev.hbp-1;
	LTDC_Handler.Init.AccumulatedVBP=ltdcdev.vsw+ltdcdev.vbp-1;
	LTDC_Handler.Init.AccumulatedActiveW=ltdcdev.hsw+ltdcdev.hbp+ltdcdev.pwidth-1;
	LTDC_Handler.Init.AccumulatedActiveH=ltdcdev.vsw+ltdcdev.vbp+ltdcdev.pheight-1;
	LTDC_Handler.Init.TotalWidth=ltdcdev.hsw+ltdcdev.hbp+ltdcdev.pwidth+ltdcdev.hfp-1; 
	LTDC_Handler.Init.TotalHeigh=ltdcdev.vsw+ltdcdev.vbp+ltdcdev.pheight+ltdcdev.vfp-1; 
	LTDC_Handler.Init.Backcolor.Red=0;
	LTDC_Handler.Init.Backcolor.Green=0; 
	LTDC_Handler.Init.Backcolor.Blue=0;
	HAL_LTDC_Init(&LTDC_Handler);
	
	ltdc_framebuf[0]=(uint32_t*)lcd_buf;
	ltdc_framebuf[1] = (uint32_t*)((uint32_t)lcd_buf + ltdcdev.pixsize*(lcddev.x_pixel*lcddev.y_pixel));

	LTDC_Layer_Parameter_Config(0,(uint32_t)ltdc_framebuf[0],255,0,0,0,0X000000);
	LTDC_Layer_Window_Config(0,0,0,ltdcdev.pwidth,ltdcdev.pheight);
}


//设置LCD显示方向
void set_lcd_dir(uint8_t dir)
{
	lcddev.dir=dir;		//竖屏

	switch (dir)
	{
		case 2:
		lcddev.width=480;
		lcddev.height=800;
		break;
		case 3:
		lcddev.width=800;
		lcddev.height=480;
		break;
		case 4:
		lcddev.width=480;
		lcddev.height=800;
		break;
		default:
		lcddev.width=800;
		lcddev.height=480;
		break;
	}
	
	ltdc_glcd.width = lcddev.width;
	ltdc_glcd.height = lcddev.height;
}	 
void ltdc_clear(uint16_t color)
{
	ltdc_fill_rgb565(0,0,lcddev.width-1,lcddev.height-1,color);
	lcddev.backcolor = color;
	lcddev.clercolor = lcddev.backcolor;
}
//快速画点
//x,y:坐标
//color:颜色

void ltdc_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
	if(x >= lcddev.width || y >= lcddev.height) return;
	
	if(lcddev.type == 2)
	{

		switch (lcddev.dir)
		{
			case 2:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*
					(((lcddev.x_pixel*lcddev.y_pixel)-(x+y*lcddev.x_pixel))-1)) = color;
			break;
			case 3:
				*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize *
			((lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y))) = color;
			break;
			case 4:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x)) = color;
			break;
			default:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(((lcddev.x_pixel*(x+1))-y) - 1)) = color;
			break;
		}
	}else if(lcddev.type == 3)  //7寸
	{
		color = rgb565tobgr565(color);
		switch (lcddev.dir)
		{
			case 2:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(((lcddev.x_pixel*(x+1))-y) - 1)) = color;
			break;
			case 3:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*
					(((lcddev.x_pixel*lcddev.y_pixel)-(x+y*lcddev.x_pixel))-1)) = color;
			break;
			case 4:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize *
					((lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y))) = color;
			break;
			default:
			*(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x)) = color;
			break;
		}
	}
}	 
uint16_t ltdc_ReadPoint(uint16_t x , uint16_t y)
{
	if(x >= lcddev.width || y >= lcddev.height) return 0;
	uint16_t re_color = 0;
	if(lcddev.type == 2)
	{
		switch (lcddev.dir)
		{
			case 2:
			re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*
					(((lcddev.x_pixel*lcddev.y_pixel)-(x+y*lcddev.x_pixel))-1));
			break;
			case 3:
			 re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize *
			((lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)));
			break;
			case 4:
			 re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			break;
			default:
			 re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(((lcddev.x_pixel*(x+1))-y) - 1));
			break;
		}
	}else if(lcddev.type == 3)  //7寸
	{

		switch (lcddev.dir)
		{
			case 2:
			re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(((lcddev.x_pixel*(x+1))-y) - 1));
			break;
			case 3:
			re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*
					(((lcddev.x_pixel*lcddev.y_pixel)-(x+y*lcddev.x_pixel))-1));
			break;
			case 4:
			re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize *
					((lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)));
			break;
			default:
			re_color = *(uint16_t*)((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			break;
		}
		
		re_color = rgb565tobgr565(re_color);
	}

	return re_color;
}

//在指定区域内填充指定颜色块			 
void ltdc_fill_rgb565(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color)
{  
	if(x >= lcddev.width || y >= lcddev.height) return;
	
	uint32_t offline =0 ;
	uint32_t ltdc_addr = 0;
	uint32_t ltdc_whlen = 0;

	if(lcddev.type == 2)
	{
		uint16_t *u32color = (uint16_t *)&ltdc_framebuf[1][0];
		*u32color = color;
		switch (lcddev.dir)
		{
			case 2:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-height-y)+(lcddev.x_pixel-width-x)));
			ltdc_whlen = (width<<16)|(height);
			break;
			case 3:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-width-x)+y));
			ltdc_whlen = (height<<16)|(width);
			break;
			case 4:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			ltdc_whlen = (width<<16)|(height);
			break;
			default:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = (height<<16)|(width);
			break;
		}
	}else if(lcddev.type == 3)  //7寸
	{
		uint16_t *u16color = (uint16_t *)&ltdc_framebuf[1][0];
		*u16color = rgb565tobgr565(color);
		
		switch (lcddev.dir)
		{
			case 2:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = (height<<16)|(width);
			break;
			case 3:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-height-y)+(lcddev.x_pixel-width-x)));
			ltdc_whlen = (width<<16)|(height);
			break;
			case 4:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-width-x)+y));
			ltdc_whlen = (height<<16)|(width);
			break;
			default:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			ltdc_whlen = (width<<16)|(height);
			break;
		}
	}

	DMA2D->CR	  &=	~(DMA2D_CR_START);
	DMA2D->CR		=	DMA2D_R2M;
	DMA2D->OPFCCR	=	ltdcdev.ltdc_format;//LTDC_PIXEL_FORMAT_RGB565;
	
	DMA2D->OCOLR	=	ltdc_framebuf[1][0];//color;
	DMA2D->OOR		=	offline;				//	设置行偏移
	DMA2D->OMAR		=	ltdc_addr;	// 地址;
	DMA2D->NLR		=	ltdc_whlen;			//	设定长度和宽度		

	DMA2D->CR	  |=	DMA2D_CR_START;					//	启动DMA2D
	while (DMA2D->CR & DMA2D_CR_START);			//	等待传输完成
	DMA2D->CR	  &=	~(DMA2D_CR_START);

} 

//在指定区域内填充指定颜色块			 
void ltdc_full_rgb565(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{  
if(x >= lcddev.width || y >= lcddev.height) return;
#if 1
	for(uint32_t i=0;i<height;i++){
		for(uint32_t j=0;j<width;j++){
			ltdc_DrawPoint(j+x,i+y,*color++);
		}
	}
#else
	uint32_t offline =0 ;
	uint32_t ltdc_addr = 0;
	uint32_t ltdc_whlen = 0;
	if(lcddev.type == 2) 
	{
		uint16_t* u16color = (uint16_t*)ltdc_framebuf[1];
		for(uint32_t i=0;i<height*width;i++){
			u16color[i] = color[i];
		}
		
		switch (lcddev.dir)
		{
			case 2:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = ((uint32_t)height<<16)|((uint32_t)width);
			break;
			case 3:
			offline = lcddev.x_pixel - height;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = ((uint32_t)height<<16)|((uint32_t)width);
			break;
			case 4:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			ltdc_whlen = ((uint32_t)width<<16)|((uint32_t)height);
			
			break;
			default:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = ((uint32_t)height<<16)|((uint32_t)width);
			break;
		}
	}else if(lcddev.type == 3)  //7寸
	{
		uint16_t* u16color = (uint16_t*)ltdc_framebuf[1];
		for(uint32_t i=0;i<height*width;i++){
			u16color[i] = rgb565tobgr565(color[i]);
		}
		switch (lcddev.dir)
		{
			case 2:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
			ltdc_whlen = ((uint32_t)height<<16)|((uint32_t)width);
			break;
			case 3:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-height-y)+(lcddev.x_pixel-width-x)));
			ltdc_whlen = ((uint32_t)width<<16)|((uint32_t)height);
			break;
			case 4:
			offline = lcddev.x_pixel - height; 
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-width-x)+y));
			ltdc_whlen = ((uint32_t)height<<16)|((uint32_t)width);
			break;
			default:
			offline = lcddev.x_pixel - width;
			ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
			ltdc_whlen = ((uint32_t)width<<16)|((uint32_t)height);
			break;
		}
	}

	DMA2D->CR=0<<16;				//存储器到存储器模式
	DMA2D->FGPFCCR=ltdcdev.ltdc_format;//LTDC_PIXEL_FORMAT_RGB565;	//设置颜色格式
	DMA2D->FGOR=0;					//前景层行偏移为0
	DMA2D->OOR=offline;				//设置行偏移 
	DMA2D->CR&=~(1<<0);				//先停止DMA2D
	DMA2D->FGMAR=(uint32_t)ltdc_framebuf[1];		//源地址
	DMA2D->OMAR=ltdc_addr;				//输出存储器地址
	DMA2D->NLR=ltdc_whlen;	//设定行数寄存器 
	DMA2D->CR|=1<<0;				//启动DMA2D
	while((DMA2D->ISR&(1<<1))==0);
	DMA2D->IFCR|=1<<1;				//清除传输完成标志  	
#endif
}

//绘制横线函数
void ltdc_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height)) return;
	ltdc_fill_rgb565(x0, y0,x0+len, y0, color);
} 

Graphics_Display ltdc_glcd =
{
	16,
	480,
	800,
	ltdc_DrawPoint,
	ltdc_ReadPoint,
	NULL,
	NULL,
	ltdc_fill_rgb565,
	ltdc_full_rgb565
};


#endif





