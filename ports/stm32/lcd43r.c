/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	lcd43g.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
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

#if (MICROPY_HW_LTDC_LCD && MICROPY_HW_LCD43R)
	
#include "modtftlcd.h"
#include "ltdc.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

#include "global.h" 

_lcd_dev lcddev;
//==============================================================================

//==============================================================================

static void LCD_WriteByteSPI(unsigned char byte)
{
	unsigned char n;
	for(n=0; n<8; n++)			
	{  
		if(byte&0x80) 
			mp_hal_pin_high(MICROPY_RGB_SPI_SDA);
		else 
			mp_hal_pin_low(MICROPY_RGB_SPI_SDA);
		byte<<= 1;
		mp_hal_pin_low(MICROPY_RGB_SPI_CLK);
		mp_hal_pin_high(MICROPY_RGB_SPI_CLK);
	}
}
static void SPI_WriteComm(uint16_t CMD)//3线9bit 串行接口
{			
	mp_hal_pin_low(MICROPY_RGB_SPI_CS);
	mp_hal_pin_low(MICROPY_RGB_SPI_SDA);
	mp_hal_pin_low(MICROPY_RGB_SPI_CLK);
	mp_hal_pin_high(MICROPY_RGB_SPI_CLK);
	LCD_WriteByteSPI(CMD);
	mp_hal_pin_high(MICROPY_RGB_SPI_CS);
}
static void SPI_WriteData(uint16_t tem_data)
{			
	mp_hal_pin_low(MICROPY_RGB_SPI_CS);
	mp_hal_pin_high(MICROPY_RGB_SPI_SDA);
	mp_hal_pin_low(MICROPY_RGB_SPI_CLK);
	mp_hal_pin_high(MICROPY_RGB_SPI_CLK);
	LCD_WriteByteSPI(tem_data);
	mp_hal_pin_high(MICROPY_RGB_SPI_CS);
}

static void LCD_init_code(void)//液晶屏初始化代码
{
	// LCD_Reset();
	mp_hal_pin_high(MICROPY_RGB_SPI_CS);	
	mp_hal_delay_ms(200);
	mp_hal_pin_low(MICROPY_RGB_SPI_CS);	

	SPI_WriteComm(0xc0);  
	SPI_WriteData(0x01);
	SPI_WriteData(0x11);

	SPI_WriteComm(0x20);  

	SPI_WriteComm(0x36);  
	//SPI_WriteData(0x88);//BRG
	SPI_WriteData(0x80);//RGB

	SPI_WriteComm(0x3a);  
	SPI_WriteData(0x77);//16/18/24bit
	//SPI_WriteData(0x55);//16bit

	SPI_WriteComm(0x35);  
	SPI_WriteData(0x00);

	SPI_WriteComm(0xb1);  
	SPI_WriteData(0x06);
	SPI_WriteData(0x03);
	SPI_WriteData(0x00);

	SPI_WriteComm(0xb2);            
	SPI_WriteData(0x00);
	SPI_WriteData(0xc8);

	SPI_WriteComm(0xb3);            
	SPI_WriteData(0x01);

	SPI_WriteComm(0xb4);            
	SPI_WriteData(0x04);

	SPI_WriteComm(0xb5);            
	SPI_WriteData(0x10);
	SPI_WriteData(0x30);
	SPI_WriteData(0x30);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);

	SPI_WriteComm(0xb6);  //
	SPI_WriteData(0x0b);  //0b
	SPI_WriteData(0x0f);
	SPI_WriteData(0x3c);
	SPI_WriteData(0x13);
	SPI_WriteData(0x13);
	SPI_WriteData(0xe8);

	SPI_WriteComm(0xb7);  
	SPI_WriteData(0x46);
	SPI_WriteData(0x06);
	SPI_WriteData(0x0c);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);

	SPI_WriteComm(0xc0); //Internal Oscillator Setting 
	SPI_WriteData(0x01);
	SPI_WriteData(0x15);

	SPI_WriteComm(0xc3); //Power Control 3 
	SPI_WriteData(0x07);
	SPI_WriteData(0x03);
	SPI_WriteData(0x04);
	SPI_WriteData(0x04);
	SPI_WriteData(0x04);
	mp_hal_delay_ms(40);

	SPI_WriteComm(0xc4); //Power Control 4 
	SPI_WriteData(0x12);//11
	SPI_WriteData(0x24);//23
	SPI_WriteData(0x12);//12   16 
	SPI_WriteData(0x12);//12   16
	SPI_WriteData(0x02);//05   
	SPI_WriteData(0x6b);//6d  49   //6A
	mp_hal_delay_ms(20);

	SPI_WriteComm(0xc5); //Power Control 5 
	SPI_WriteData(0x69);  //0x69
	mp_hal_delay_ms(10);
	
	SPI_WriteComm(0xc6); //Power Control 6  
	SPI_WriteData(0x41);//41 40
	SPI_WriteData(0x63);
	mp_hal_delay_ms(10);

	SPI_WriteComm(0xd0); //Positive Gamma Curve for Red 
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);
	
	SPI_WriteComm(0xd1); //Negative Gamma Curve for Red 
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);

	SPI_WriteComm(0xd2); //Positive Gamma Curve for Green 
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);

	SPI_WriteComm(0xd3); //Negative Gamma Curve for Green 
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);

	SPI_WriteComm(0xd4);//Positive Gamma Curve for Blue  
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);

	SPI_WriteComm(0xd5);//Negative Gamma Curve for Blue  
	SPI_WriteData(0x01);
	SPI_WriteData(0x26);
	SPI_WriteData(0x71);
	SPI_WriteData(0x16);
	SPI_WriteData(0x04);
	SPI_WriteData(0x03);
	SPI_WriteData(0x51);
	SPI_WriteData(0x15);
	SPI_WriteData(0x04);

	SPI_WriteComm(0x11); //Sleep Out 
	mp_hal_delay_ms(20);

	SPI_WriteComm(0x29);//Display On	
	mp_hal_delay_ms(10);
}
//==============================================================================

void lcd43g_init(void)
{
	STATIC bool init_flag = false;
	
	lcddev.type = 2;
	if(init_flag) return;

	mp_hal_pin_config(MICROPY_RGB_SPI_SDA, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(MICROPY_RGB_SPI_CLK, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(MICROPY_RGB_SPI_CS, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(MICROPY_HW_LTDC_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);

	LCD_init_code();

	ltdcdev.pwidth=480;	
	ltdcdev.pheight=800;

	ltdcdev.hsw=1;	
	ltdcdev.hbp=46;
	ltdcdev.hfp=10;
	
	ltdcdev.vsw=1;	
	ltdcdev.vbp=23;
	ltdcdev.vfp=22;
	
	ltdcdev.layer = 0;
	ltdcdev.pixsize=2;
	ltdcdev.ltdc_format = LTDC_PIXEL_FORMAT_RGB565;//LTDC_PIXEL_FORMAT_ARGB8888;//
	
	lcddev.x_pixel = ltdcdev.pwidth;
	lcddev.y_pixel = ltdcdev.pheight;
	
	ltdc_init();
	#if defined(STM32F4) || defined(STM32F7)
	ltdc_set_clk(5);
	#elif defined(STM32H7)
	ltdc_set_clk(17);
	#endif
	ltdc_conf();
	
	lcddev.backcolor = BLACK;

	 mp_hal_pin_high(MICROPY_HW_LTDC_BL);
	
	init_flag = true;

}

void lcd43g_full_cam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
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
		
	DMA2D->CR=0<<16;				//存储器到存储器模式
	DMA2D->FGPFCCR=ltdcdev.ltdc_format;//LTDC_PIXEL_FORMAT_RGB565;	//设置颜色格式
	DMA2D->FGOR=0;					//前景层行偏移为0
	DMA2D->OOR=offline;				//设置行偏移 
	DMA2D->CR&=~(1<<0);				//先停止DMA2D
	
	//DMA2D->FGMAR=(uint32_t)ltdc_framebuf[1];		//源地址
	DMA2D->FGMAR=(uint32_t)color;
	
	DMA2D->OMAR=ltdc_addr;				//输出存储器地址
	DMA2D->NLR=ltdc_whlen;	//设定行数寄存器 
	DMA2D->CR|=1<<0;				//启动DMA2D
	while((DMA2D->ISR&(1<<1))==0);
	DMA2D->IFCR|=1<<1;				//清除传输完成标志  	

#endif
	
}

#endif





