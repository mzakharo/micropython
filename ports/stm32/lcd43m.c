
/**
  ******************************************************************************
  *	This file is part of the MicroPython project, http://bbs.01studio.org/
  * Copyright (C), 2020 -2021, 01studio Tech. Co., Ltd.
  * File Name          :	lcd43m.c
  * Author             :	spring
  * Version            :	v1.0
  * date               :	2020/11/27
  * Description        :	lcd dev
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

#if (MICROPY_ENABLE_TFTLCD && MICROPY_HW_LCD43M)

#include "lcd43m.h"
#include "modtftlcd.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

#include "global.h" 

Graphics_Display lcd43_glcd;

_lcd_dev lcddev;

void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	 
	LCD43M_REG = (SETXCMD); 		LCD43M_RAM = (Xpos>>8); 		
	LCD43M_REG = (SETXCMD+1); 	LCD43M_RAM = (Xpos&0XFF); 		 
	LCD43M_REG = (SETYCMD); 		LCD43M_RAM = (Ypos>>8); 		
	LCD43M_REG = (SETYCMD+1); 	LCD43M_RAM = (Ypos&0XFF); 	
}
		 
//设置LCD的自动扫描方向
void LCD_Scan_Dir(uint8_t dir)
{
	uint16_t regval=0;
	uint16_t temp;  
	switch(dir)
	{
		case L2R_U2D:
			regval|=(0<<7)|(0<<6)|(0<<5); 
			break;
		case L2R_D2U:
			regval|=(1<<7)|(0<<6)|(0<<5); 
			break;
		case R2L_U2D:
			regval|=(0<<7)|(1<<6)|(0<<5); 
			break;
		case R2L_D2U:
			regval|=(1<<7)|(1<<6)|(0<<5); 
			break;	 
		case U2D_L2R:
			regval|=(0<<7)|(0<<6)|(1<<5); 
			break;
		case U2D_R2L:
			regval|=(0<<7)|(1<<6)|(1<<5); 
			break;
		case D2U_L2R:
			regval|=(1<<7)|(0<<6)|(1<<5); 
			break;
		case D2U_R2L:
			regval|=(1<<7)|(1<<6)|(1<<5); 
			break;	 
	}

	LCD43M_REG = 0X3600;
	LCD43M_RAM = regval;

	if(regval&0X20)
	{
		if(lcddev.width<lcddev.height)//交换X,Y
		{
			temp=lcddev.width;
			lcddev.width=lcddev.height;
			lcddev.height=temp;
		}
	}else  
	{
		if(lcddev.width>lcddev.height)//交换X,Y
		{
			temp=lcddev.width;
			lcddev.width=lcddev.height;
			lcddev.height=temp;
		}
	}  
	
	LCD43M_REG = (SETXCMD);		LCD43M_RAM = (0); 
	LCD43M_REG = (SETXCMD+1);	LCD43M_RAM = (0); 
	LCD43M_REG = (SETXCMD+2);	LCD43M_RAM = ((lcddev.width-1)>>8); 
	LCD43M_REG = (SETXCMD+3);	LCD43M_RAM = ((lcddev.width-1)&0XFF); 
	LCD43M_REG = (SETYCMD);		LCD43M_RAM = (0); 
	LCD43M_REG = (SETYCMD+1);	LCD43M_RAM = (0); 
	LCD43M_REG = (SETYCMD+2);	LCD43M_RAM = ((lcddev.height-1)>>8); 
	LCD43M_REG = (SETYCMD+3);	LCD43M_RAM = ((lcddev.height-1)&0XFF); 
		
}    

void LCD43M_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
if(x >= lcddev.width || y >= lcddev.height) return;
	LCD43M_REG = SETXCMD;			LCD43M_RAM = (x>>8);
	LCD43M_REG = (SETXCMD+1);	LCD43M_RAM = (x&0XFF);
	LCD43M_REG = SETYCMD;			LCD43M_RAM = (y>>8);
	LCD43M_REG = (SETYCMD+1);	LCD43M_RAM = (y&0XFF);
	LCD43M_REG = WRAMCMD;			LCD43M_RAM=color; 
}	 

uint16_t LCD43M_ReadPoint(uint16_t x , uint16_t y)
{
  uint16_t r = 0, g = 0, b=0;
  if(x >= lcddev.width || y >= lcddev.height) return 0;
  LCD_SetCursor(x, y);
  LCD43M_REG = 0x2E00;
  r = LCD43M_RAM;
  mp_hal_delay_us(2);
  r = LCD43M_RAM;
  mp_hal_delay_us(2);
  b = LCD43M_RAM;
  g = r & 0xFF;
  g <<= 8;
  return (uint16_t)((r & 0xF800)|(g>>5)|((b&0xF800)>>11));
}

void LCD43M_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
  uint16_t i,j;
  uint16_t xlen=0;
  xlen = ex - sx + 1;
  for(i = sy; i< ey; i++)
  {
    LCD_SetCursor(sx, i);
    LCD43M_REG = WRAMCMD;
    for(j=0; j<xlen; j++) LCD43M_RAM = color;
  }
}

void lcd43m_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if((len==0)||(x0>lcddev.width)||(y0>lcddev.height))return;
	LCD43M_Fill(x0,y0,x0+len-1,y0,color);	
}

void LCD43M_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
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
		LCD43M_DrawPoint(uRow,uCol ,color);
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

void LCD_Display_Dir(uint8_t dir)
{
	lcddev.dir=dir;		//竖屏
	switch (dir)
		{
		case 2:
		lcddev.width=800;
		lcddev.height=480;
		LCD_Scan_Dir(D2U_L2R); 
		break;
		case 3:
		lcddev.width=480;
		lcddev.height=800;
		LCD_Scan_Dir(R2L_D2U); 
		break;
		case 4:
		lcddev.width=800;
		lcddev.height=480;
		LCD_Scan_Dir(U2D_R2L); 
		break;
		default:lcddev.dir = 1;
		lcddev.width=480;
		lcddev.height=800;
		LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
		break;
		}
	lcd43_glcd.width = lcddev.width;
	lcd43_glcd.height = lcddev.height;
}	 

void LCD43M_Full(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height,uint16_t *color)
{  
	uint16_t i,j; 
	if(width > lcddev.width || height > lcddev.height) return;

	for(i=0;i<height;i++)
	{
		LCD43M_REG = (SETXCMD);		LCD43M_RAM = (sx>>8); 		
		LCD43M_REG = (SETXCMD+1);	LCD43M_RAM = (sx&0XFF);			 
		LCD43M_REG = (SETYCMD);		LCD43M_RAM = ((sy+i)>>8);  		
		LCD43M_REG = (SETYCMD+1);	LCD43M_RAM = ((sy+i)&0XFF);
		LCD43M_REG = WRAMCMD;
		
		for(j=0;j<width;j++)	{
			LCD43M_RAM=color[i*width+j];//写入数据 
		}

	} 
} 

void LCD_Clear(uint16_t color)
{
	uint32_t index=0;      
	uint32_t totalpoint=lcddev.width; 
	totalpoint*=lcddev.height;
	LCD_SetCursor(0x00,0x0000);			//设置光标位置 
	LCD43M_REG=WRAMCMD;     		//开始写入GRAM	 	  
	for(index=0;index<totalpoint;index++)
	{
		LCD43M_RAM=color;	
	} 

}  

void LCD_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{  
	uint16_t twidth,theight;
	twidth=sx+width-1;
	theight=sy+height-1;

	LCD43M_REG = (SETXCMD);		LCD43M_RAM = (sx>>8);  
	LCD43M_REG = (SETXCMD+1);	LCD43M_RAM = (sx&0XFF);	  
	LCD43M_REG = (SETXCMD+2);	LCD43M_RAM = (twidth>>8);   
	LCD43M_REG = (SETXCMD+3);	LCD43M_RAM = (twidth&0XFF);   
	LCD43M_REG = (SETYCMD);		LCD43M_RAM = (sy>>8);   
	LCD43M_REG = (SETYCMD+1);	LCD43M_RAM = (sy&0XFF);  
	LCD43M_REG = (SETYCMD+2);	LCD43M_RAM = (theight>>8);   
	LCD43M_REG = (SETYCMD+3);	LCD43M_RAM = (theight&0XFF);  
}

Graphics_Display lcd43_glcd =
{
	16,
	480,
	800,
	LCD43M_DrawPoint,
	LCD43M_ReadPoint,
	lcd43m_hline,
	NULL,
	LCD43M_Fill,
	LCD43M_Full
};

#if MICROPY_HW_BOARD_MAGELLAM

#if defined(STM32H7) || defined(STM32F7)
//LCD MPU保护参数
#define LCD_REGION_NUMBER		MPU_REGION_NUMBER7		//LCD使用region0
#define LCD_ADDRESS_START		(0X68000000)			//LCD区的首地址
#define LCD_REGION_SIZE			MPU_REGION_SIZE_16MB//MPU_REGION_SIZE_256MB   //LCD区大小

//配置MPU的region(SRAM区域为透写模式)
static void lcd43m_mpu_config(void)
{	
	MPU_Region_InitTypeDef MPU_Initure;

	HAL_MPU_Disable();							//配置MPU之前先关闭MPU,配置完成以后在使能MPU	
	//外部SRAM为region0，大小为2MB，此区域可读写
	MPU_Initure.Enable=MPU_REGION_ENABLE;	    //使能region
	MPU_Initure.Number=LCD_REGION_NUMBER;		//设置region，外部SRAM使用的region0
	MPU_Initure.BaseAddress=LCD_ADDRESS_START;	//region基地址
	MPU_Initure.Size=LCD_REGION_SIZE;			//region大小
	MPU_Initure.SubRegionDisable=0X00;
	MPU_Initure.TypeExtField=MPU_TEX_LEVEL0;
	MPU_Initure.AccessPermission=MPU_REGION_FULL_ACCESS;	//此region可读写
	MPU_Initure.DisableExec=MPU_INSTRUCTION_ACCESS_ENABLE;	//允许读取此区域中的指令
	MPU_Initure.IsShareable=MPU_ACCESS_NOT_SHAREABLE;
	MPU_Initure.IsCacheable=MPU_ACCESS_NOT_CACHEABLE;
	MPU_Initure.IsBufferable=MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_Initure);
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);     //开启MPU
}
#endif

#endif

void lcd43m_init()
{
	STATIC bool init_flag = false;
	if(init_flag) return;

	mp_hal_pin_config(MICROPY_HW_LCD43M_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	
	#if MICROPY_HW_BOARD_COLUMBUS
	__HAL_RCC_FSMC_CLK_ENABLE(); 

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NE4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_NE4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_A0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_A0);

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NOE, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_NOE);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NWE, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_NWE);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D0);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D1, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D1);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D2, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D2);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D3);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D5, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D5);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D6, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D6);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D7, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D7);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D8, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D8);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D9, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D9);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D10, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D10);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D11, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D11);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D12, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D12);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D13, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D13);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D14, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D14);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D15, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FSMC_D15);

	FSMC_Bank1->BTCR[6]=0X00000000;
	FSMC_Bank1->BTCR[7]=0X00000000;
	FSMC_Bank1E->BWTR[6]=0X00000000;
	FSMC_Bank1->BTCR[6]|=1<<12; 	//存储器写使能
	FSMC_Bank1->BTCR[6]|=1<<14; 	//读写使用不同的时序
	FSMC_Bank1->BTCR[6]|=1<<4;		//存储器数据宽度为16bit 			

	FSMC_Bank1->BTCR[7]|=0<<28; 	//模式A 
	
	FSMC_Bank1->BTCR[7]|=0XF<<0;	//地址建立时间(ADDSET)为15个HCLK 1/168M=6ns*15=90ns 
	FSMC_Bank1->BTCR[7]|=60<<8; 	//数据保存时间(DATAST)为60个HCLK	=6*60=360ns

	FSMC_Bank1E->BWTR[6]|=0<<28;	//模式A 										
	FSMC_Bank1E->BWTR[6]|=9<<0; 	//地址建立时间(ADDSET)为9个HCLK=54ns
		 
	FSMC_Bank1E->BWTR[6]|=8<<8; 	//数据保存时间(DATAST)为6ns*9个HCLK=54ns
	FSMC_Bank1->BTCR[6]|=1<<0;		//使能BANK1，区域1 	 
	#endif

	#if MICROPY_HW_BOARD_MAGELLAM
	#if defined(STM32H7)  || defined(STM32F7)
	lcd43m_mpu_config();
	#endif
  __HAL_RCC_FMC_CLK_ENABLE();

	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NE3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_NE3);
	
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NOE, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_NOE);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_NWE, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_NWE);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_A18, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_A18);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D0, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D0);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D1, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D1);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D2, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D2);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D3, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D3);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D4, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D4);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D5, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D5);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D6, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D6);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D7, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D7);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D8, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D8);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D9, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D9);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D10, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D10);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D11, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D11);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D12, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D12);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D13, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D13);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D14, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D14);
	mp_hal_pin_config_alt_static_speed(MICROPY_HW_LCD_D15, MP_HAL_PIN_MODE_ALT, MP_HAL_PIN_PULL_NONE, MP_HAL_PIN_SPEED_VERY_HIGH, STATIC_AF_FMC_D15);

	#if defined(STM32H7)
	FMC_Bank1_R->BTCR[4]=0X00000000;
	
	FMC_Bank1_R->BTCR[5]=0X00000000;
	FMC_Bank1E_R->BWTR[4]=0X00000000;
	
	FMC_Bank1_R->BTCR[4]|=1<<12; 	//存储器写使能
	FMC_Bank1_R->BTCR[4]|=1<<14; 	//读写使用不同的时序
	FMC_Bank1_R->BTCR[4]|=1<<4;		//存储器数据宽度为16bit 			

	FMC_Bank1_R->BTCR[5]|=0<<28; 	//模式A 
	
	FMC_Bank1_R->BTCR[5]|=0XF<<0;	//地址建立时间(ADDSET)为15个HCLK 1/168M=6ns*15=90ns 
	FMC_Bank1_R->BTCR[5]|=82<<8; 	//数据保存时间(DATAST)为 82 个 fmc_ker_ck=4.3*82=352.6ns

	FMC_Bank1E_R->BWTR[4]|=0<<28;	//模式A 										
	FMC_Bank1E_R->BWTR[4]|=15<<0; 	//地址建立时间(ADDSET)为 15 个 fmc_ker_ck=64.5ns
	 
	FMC_Bank1E_R->BWTR[4]|=15<<8; 	//数据保存时间(DATAST)为 15 个 fmc_ker_ck=64.5ns
	
	FMC_Bank1_R->BTCR[4]|=1<<0; //使能 BANK1，区域 1
	FMC_Bank1_R->BTCR[4]|=(uint32_t)1<<31; //使能 FMC
	#elif defined(STM32F4)	|| defined(STM32F7)
	FMC_Bank1->BTCR[4]=0X00000000;
	FMC_Bank1->BTCR[5]=0X00000000;
	FMC_Bank1E->BWTR[4]=0X00000000;
	
	FMC_Bank1->BTCR[4]|=1<<12; 	//存储器写使能
	FMC_Bank1->BTCR[4]|=1<<14; 	//读写使用不同的时序
	FMC_Bank1->BTCR[4]|=1<<4;		//存储器数据宽度为16bit 			

	FMC_Bank1->BTCR[5]|=0<<28; 	//模式A 
	
	FMC_Bank1->BTCR[5]|=0XF<<0;	//地址建立时间(ADDSET)为15个HCLK 1/168M=6ns*15=90ns 
	FMC_Bank1->BTCR[5]|=60<<8; 	//数据保存时间(DATAST)为 82 个 fmc_ker_ck=4.3*82=352.6ns

	FMC_Bank1E->BWTR[4]|=0<<28;	//模式A 										
	FMC_Bank1E->BWTR[4]|=9<<0; 	//地址建立时间(ADDSET)为 15 个 fmc_ker_ck=64.5ns
	 
	FMC_Bank1E->BWTR[4]|=8<<8; 	//数据保存时间(DATAST)为 15 个 fmc_ker_ck=64.5ns
	FMC_Bank1->BTCR[4]|=1<<0; //使能 BANK1，区域 1

	#endif
	
	#endif

	mp_hal_delay_ms(50);
	LCD43M_REG = 0XD4;  
	lcddev.id = LCD43M_RAM;
	lcddev.id = LCD43M_RAM; 
	lcddev.id = LCD43M_RAM;
	lcddev.id<<=8;	 
	lcddev.id |= LCD43M_RAM; 

	if(lcddev.id!=0X5310)	
	{
		LCD43M_REG = (0XDA00); 
		lcddev.id = LCD43M_RAM;	 
		LCD43M_REG = (0XDB00); 
		lcddev.id = LCD43M_RAM;	
		lcddev.id<<=8;	
		LCD43M_REG = (0XDC00); 
		lcddev.id |= LCD43M_RAM;	
		if(lcddev.id==0x8000)lcddev.id=0x5510;
	}	

if(lcddev.id==0x5510)
	{
		LCD43M_REG = 0xF000;	LCD43M_RAM = 0x55;
		LCD43M_REG = 0xF001;	LCD43M_RAM = 0xAA;
		LCD43M_REG = 0xF002;	LCD43M_RAM = 0x52;
		LCD43M_REG = 0xF003;	LCD43M_RAM = 0x08;
		LCD43M_REG = 0xF004;	LCD43M_RAM = 0x01;
		//AVDD Set AVDD 5.2V
		LCD43M_REG = 0xB000;	LCD43M_RAM = 0x0D;
		LCD43M_REG = 0xB001;	LCD43M_RAM = 0x0D;
		LCD43M_REG = 0xB002;	LCD43M_RAM = 0x0D;
		//AVDD ratio
		LCD43M_REG = 0xB600;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB601;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB602;	LCD43M_RAM = 0x34;
		//AVEE -5.2V
		LCD43M_REG = 0xB100;	LCD43M_RAM = 0x0D;
		LCD43M_REG = 0xB101;	LCD43M_RAM = 0x0D;
		LCD43M_REG = 0xB102;	LCD43M_RAM = 0x0D;
		//AVEE ratio
		LCD43M_REG = 0xB700;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB701;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB702;	LCD43M_RAM = 0x34;
		//VCL -2.5V
		LCD43M_REG = 0xB200;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xB201;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xB202;	LCD43M_RAM = 0x00;
		//VCL ratio
		LCD43M_REG = 0xB800;	LCD43M_RAM = 0x24;
		LCD43M_REG = 0xB801;	LCD43M_RAM = 0x24;
		LCD43M_REG = 0xB802;	LCD43M_RAM = 0x24;
		//VGH 15V (Free pump)
		LCD43M_REG = 0xBF00;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xB300;	LCD43M_RAM = 0x0F;
		LCD43M_REG = 0xB301;	LCD43M_RAM = 0x0F;
		LCD43M_REG = 0xB302;	LCD43M_RAM = 0x0F;
		//VGH ratio
		LCD43M_REG = 0xB900;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB901;	LCD43M_RAM = 0x34;
		LCD43M_REG = 0xB902;	LCD43M_RAM = 0x34;
		//VGL_REG -10V
		LCD43M_REG = 0xB500;	LCD43M_RAM = 0x08;
		LCD43M_REG = 0xB501;	LCD43M_RAM = 0x08;
		LCD43M_REG = 0xB502;	LCD43M_RAM = 0x08;
		LCD43M_REG = 0xC200;	LCD43M_RAM = 0x03;
		//VGLX ratio
		LCD43M_REG = 0xBA00;	LCD43M_RAM = 0x24;
		LCD43M_REG = 0xBA01;	LCD43M_RAM = 0x24;
		LCD43M_REG = 0xBA02;	LCD43M_RAM = 0x24;
		//VGMP/VGSP 4.5V/0V
		LCD43M_REG = 0xBC00;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xBC01;	LCD43M_RAM = 0x78;
		LCD43M_REG = 0xBC02;	LCD43M_RAM = 0x00;
		//VGMN/VGSN -4.5V/0V
		LCD43M_REG = 0xBD00;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xBD01;	LCD43M_RAM = 0x78;
		LCD43M_REG = 0xBD02;	LCD43M_RAM = 0x00;
		//VCOM
		LCD43M_REG = 0xBE00;	LCD43M_RAM = 0x00;
		//

		LCD43M_REG = 0xBE01;	LCD43M_RAM = 0x67;
		//************Gamma Setting******************//
		LCD43M_REG = 0xD100;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD101;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD102;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD103;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD104;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD105;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD106;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD107;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD108;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD109;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD10A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD10B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD10C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD10D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD10E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD10F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD110;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD111;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD112;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD113;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD114;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD115;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD116;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD117;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD118;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD119;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD11A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD11B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD11C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD11D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD11E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD11F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD120;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD121;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD122;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD123;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD124;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD125;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD126;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD127;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD128;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD129;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD12A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD12B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD12C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD12D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD12E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD12F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD130;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD131;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD132;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD133;	LCD43M_RAM = 0x8E;

		LCD43M_REG = 0xD200;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD201;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD202;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD203;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD204;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD205;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD206;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD207;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD208;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD209;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD20A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD20B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD20C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD20D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD20E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD20F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD210;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD211;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD212;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD213;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD214;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD215;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD216;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD217;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD218;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD219;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD21A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD21B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD21C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD21D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD21E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD21F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD220;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD221;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD222;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD223;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD224;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD225;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD226;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD227;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD228;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD229;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD22A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD22B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD22C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD22D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD22E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD22F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD230;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD231;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD232;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD233;	LCD43M_RAM = 0x8E;	

		LCD43M_REG = 0xD300;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD301;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD302;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD303;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD304;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD305;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD306;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD307;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD308;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD309;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD30A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD30B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD30C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD30D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD30E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD30F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD310;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD311;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD312;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD313;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD314;	LCD43M_RAM = 0x01; 
		LCD43M_REG = 0xD315;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD316;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD317;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD318;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD319;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD31A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD31B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD31C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD31D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD31E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD31F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD320;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD321;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD322;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD323;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD324;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD325;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD326;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD327;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD328;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD329;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD32A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD32B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD32C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD32D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD32E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD32F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD330;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD331;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD332;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD333;	LCD43M_RAM = 0x8E;

		LCD43M_REG = 0xD400;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD401;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD402;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD403;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD404;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD405;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD406;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD407;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD408;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD409;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD40A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD40B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD40C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD40D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD40E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD40F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD410;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD411;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD412;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD413;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD414;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD415;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD416;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD417;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD418;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD419;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD41A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD41B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD41C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD41D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD41E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD41F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD420;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD421;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD422;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD423;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD424;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD425;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD426;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD427;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD428;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD429;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD42A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD42B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD42C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD42D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD42E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD42F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD430;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD431;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD432;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD433;	LCD43M_RAM = 0x8E;

		LCD43M_REG = 0xD500;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD501;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD502;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD503;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD504;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD505;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD506;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD507;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD508;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD509;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD50A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD50B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD50C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD50D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD50E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD50F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD510;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD511;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD512;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD513;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD514;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD515;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD516;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD517;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD518;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD519;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD51A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD51B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD51C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD51D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD51E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD51F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD520;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD521;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD522;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD523;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD524;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD525;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD526;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD527;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD528;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD529;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD52A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD52B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD52C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD52D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD52E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD52F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD530;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD531;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD532;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD533;	LCD43M_RAM = 0x8E;

		LCD43M_REG = 0xD600;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD601;	LCD43M_RAM = 0x32;
		LCD43M_REG = 0xD602;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD603;	LCD43M_RAM = 0x33;
		LCD43M_REG = 0xD604;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD605;	LCD43M_RAM = 0x41;
		LCD43M_REG = 0xD606;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD607;	LCD43M_RAM = 0x5A;
		LCD43M_REG = 0xD608;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD609;	LCD43M_RAM = 0x76;
		LCD43M_REG = 0xD60A;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD60B;	LCD43M_RAM = 0xA7;
		LCD43M_REG = 0xD60C;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xD60D;	LCD43M_RAM = 0xCF;
		LCD43M_REG = 0xD60E;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD60F;	LCD43M_RAM = 0x09;
		LCD43M_REG = 0xD610;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD611;	LCD43M_RAM = 0x36;
		LCD43M_REG = 0xD612;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD613;	LCD43M_RAM = 0x73;
		LCD43M_REG = 0xD614;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD615;	LCD43M_RAM = 0x9F;
		LCD43M_REG = 0xD616;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD617;	LCD43M_RAM = 0xDF;
		LCD43M_REG = 0xD618;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD619;	LCD43M_RAM = 0x10;
		LCD43M_REG = 0xD61A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD61B;	LCD43M_RAM = 0x11;
		LCD43M_REG = 0xD61C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD61D;	LCD43M_RAM = 0x3D;
		LCD43M_REG = 0xD61E;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD61F;	LCD43M_RAM = 0x69;
		LCD43M_REG = 0xD620;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD621;	LCD43M_RAM = 0x81;
		LCD43M_REG = 0xD622;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD623;	LCD43M_RAM = 0x9D;
		LCD43M_REG = 0xD624;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD625;	LCD43M_RAM = 0xAD;
		LCD43M_REG = 0xD626;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD627;	LCD43M_RAM = 0xC3;
		LCD43M_REG = 0xD628;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD629;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xD62A;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD62B;	LCD43M_RAM = 0xE2;
		LCD43M_REG = 0xD62C;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xD62D;	LCD43M_RAM = 0xEE;
		LCD43M_REG = 0xD62E;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD62F;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xD630;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD631;	LCD43M_RAM = 0x26;
		LCD43M_REG = 0xD632;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xD633;	LCD43M_RAM = 0x8E;


		//LV2 Page 0 enable
		LCD43M_REG = 0xF000;	LCD43M_RAM = 0x55;
		LCD43M_REG = 0xF001;	LCD43M_RAM = 0xAA;
		LCD43M_REG = 0xF002;	LCD43M_RAM = 0x52;
		LCD43M_REG = 0xF003;	LCD43M_RAM = 0x08;
		LCD43M_REG = 0xF004;	LCD43M_RAM = 0x00;
		//Display control
		LCD43M_REG = 0xB100;	LCD43M_RAM = 0xCC;
		LCD43M_REG = 0xB101;	LCD43M_RAM = 0x00;
		//Source hold time
		LCD43M_REG = 0xB600;	LCD43M_RAM = 0x05;
		//Gate EQ control
		LCD43M_REG = 0xB700;	LCD43M_RAM = 0x70;
		LCD43M_REG = 0xB701;	LCD43M_RAM = 0x70;
		//Source EQ control (Mode 2)
		LCD43M_REG = 0xB800;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xB801;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xB802;	LCD43M_RAM = 0x03;
		LCD43M_REG = 0xB803;	LCD43M_RAM = 0x03;
		//Inversion mode (2-dot)
		LCD43M_REG = 0xBC00;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xBC01;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0xBC02;	LCD43M_RAM = 0x00;
#if 1 
		//***************Frame rate***************//      
		LCD43M_REG = 0xBD00;	LCD43M_RAM = 0x01;
		LCD43M_REG = 0xBD01;	LCD43M_RAM = 0x84;
		LCD43M_REG = 0xBD02;	LCD43M_RAM = 0x1C;  //0X1C
		LCD43M_REG = 0xBD03;	LCD43M_RAM = 0x1C;
		LCD43M_REG = 0xBD04;	LCD43M_RAM = 0x00;
#endif
		//Timing control 4H w/ 4-delay
		LCD43M_REG = 0xC900;	LCD43M_RAM = 0xD0;
		LCD43M_REG = 0xC901;	LCD43M_RAM = 0x02;
		LCD43M_REG = 0xC902;	LCD43M_RAM = 0x50;
		LCD43M_REG = 0xC903;	LCD43M_RAM = 0x50;
		LCD43M_REG = 0xC904;	LCD43M_RAM = 0x50;
		
		LCD43M_REG = 0x3500;	LCD43M_RAM = 0x00;
		LCD43M_REG = 0x3600;	LCD43M_RAM = 0x00;//01 studio
		LCD43M_REG = 0x3A00;	LCD43M_RAM = 0x05;	//01 studio
	
		LCD43M_REG = (0x1100);
		mp_hal_delay_us(120);
		LCD43M_REG = 0x2900 ;//lcd_wr_reg(0x2900);   //开显示
		//LCD43M_REG = 0X2800; //关显示
		mp_hal_delay_us(50);
		LCD43M_REG = (0x2C00); //01 studio
	}

#if MICROPY_HW_BOARD_COLUMBUS
	FSMC_Bank1E->BWTR[6]&=~(0XF<<0);
	FSMC_Bank1E->BWTR[6]&=~(0XF<<8);
	FSMC_Bank1E->BWTR[6]|=3<<0;			 
	FSMC_Bank1E->BWTR[6]|=2<<8; 
#endif
#if MICROPY_HW_BOARD_MAGELLAM
	#if defined(STM32F4) || defined(STM32F7)
	FMC_Bank1E->BWTR[4]&=~(0XF<<0);
	FMC_Bank1E->BWTR[4]&=~(0XF<<8);
	FMC_Bank1E->BWTR[4]|=3<<0;			 
	FMC_Bank1E->BWTR[4]|=2<<8;
	#elif defined(STM32H7)
	FMC_Bank1E_R->BWTR[4]&=~(0XF<<0);
	FMC_Bank1E_R->BWTR[4]&=~(0XF<<8);
	FMC_Bank1E_R->BWTR[4]|=4<<0;			 
	FMC_Bank1E_R->BWTR[4]|=4<<8; 
	#endif
#endif
	LCD_Display_Dir(lcddev.dir);		//默认为竖屏0
	LCD_Clear(lcddev.backcolor);
	lcddev.clercolor = lcddev.backcolor;
	mp_hal_pin_high(MICROPY_HW_LCD43M_BL);
	init_flag = true;

}

//==============================================================================================================
typedef struct _tftlcd_lcd43m_obj_t {
    mp_obj_base_t base;
    int16_t buf[2];
} tftlcd_lcd43m_obj_t;

STATIC tftlcd_lcd43m_obj_t tftlcd_lcd43m_obj;
//------------------------------------------------------------------------------------------------------

STATIC void tftlcd_lcd43m_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
		mp_printf(print, "LCD(portrait=%d)\n",lcddev.dir);
    mp_printf(print,"LCD ID:%X\n",lcddev.id);
}

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd43m_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	// check arguments
	//mp_arg_check_num(n_args, n_kw, 1, 2, true);

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcddev.dir = args[ARG_portrait].u_int;
	lcddev.backcolor = BLACK;
	lcddev.type = 1;
	
	lcd43m_init();
	LCD_Display_Dir(lcddev.dir);
	LCD_Clear(lcddev.backcolor);

	tftlcd_lcd43m_obj.base.type = &tftlcd_lcd43m_type;

	draw_global = &lcd43_glcd;

	return MP_OBJ_FROM_PTR(&tftlcd_lcd43m_obj);
}


//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd43m_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
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
			LCD_Clear(lcddev.backcolor);
			lcddev.clercolor = lcddev.backcolor;
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_clear_obj, 1, tftlcd_lcd43m_clear);
//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd43m_drawp(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
            LCD43M_DrawPoint(args[0].u_int,args[1].u_int ,
            get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
          }else{
            mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
          }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_drawp_obj, 1, tftlcd_lcd43m_drawp);
//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd43m_drawL(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
				grap_drawLine(&lcd43_glcd,args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,
				 get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
				 
			}else{
				mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
			}
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_drawL_obj, 4, tftlcd_lcd43m_drawL);
//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd43m_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
			 grap_drawRect(&lcd43_glcd,args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,
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
    for(uint16_t i=0 ; i <= (args[3].u_int-(args[5].u_int*2)); i++ ) 
     grap_drawLine(&lcd43_glcd,args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,
					args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_drawRect_obj, 1, tftlcd_lcd43m_drawRect);

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd43m_write_buf(size_t n_args, const mp_obj_t *args) {
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
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tftlcd_lcd43m_write_buf_obj, 1, 6, tftlcd_lcd43m_write_buf);

//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd43m_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
          grap_drawColorCircle(&lcd43_glcd,
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
			grap_drawColorCircle(&lcd43_glcd,
						args[0].u_int, args[1].u_int, args[2].u_int-args[4].u_int-i, color);
    }
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_drawCircle_obj, 1, tftlcd_lcd43m_drawCircle);
//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd43m_printStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
        grap_drawStr(&lcd43_glcd, args[1].u_int, args[2].u_int, 
									text_size* bufinfo.len, text_size , text_size,str ,color, lcddev.backcolor);
    }
  }
	else{
     mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_printStr_obj, 1, tftlcd_lcd43m_printStr);
//------------------------------------------------------------------------------------------------------
#if MICROPY_PY_PICLIB

STATIC mp_obj_t tftlcd_lcd43m_Picture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t tft_allowed_args[] = { 
    { MP_QSTR_x,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_cached, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);

  if(args[2].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[2].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter error"));
    }else {
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
			res = grap_drawCached(&lcd43_glcd,&vfs_fat->fatfs, args[0].u_int, args[1].u_int, (const char *)file_buf);
			m_free(file_buf);
			if(!res) return mp_const_none;
		}
		//---------------------------------------------------------------
		piclib_init();
		if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0){
			#if MICROPY_PY_HJPEG_DECODE
			if(hjpgd_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int)){
				printf("hjpgd_decode error\r\n");
			}
			#else
			jpg_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int ,1);
			#endif
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43m_Picture_obj, 1, tftlcd_lcd43m_Picture);

#if defined(STM32F4)
extern volatile uint8_t Is_FileReadOk;
#endif
// cached file
STATIC mp_obj_t tftlcd_lcd43g_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	STATIC const mp_arg_t tft_allowed_args[] = { 
		{ MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
	};

	uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
	mp_arg_val_t args[arg_num];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);

	uint8_t res=0;
	#if defined(STM32F4)
	while(Is_FileReadOk){
		Is_FileReadOk = 0;
		mp_hal_delay_ms(1000);
	}
	#else
	mp_hal_delay_ms(1000);
	#endif

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
				is_sdcard = 1;
			}else{
				is_sdcard = 0;
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
				grap_newCached(&lcd43_glcd, is_sdcard,&vfs_fat->fatfs, path_buf,picinfo.S_Width, picinfo.S_Height);	
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_CachePicture_obj, 1, tftlcd_lcd43g_CachePicture);

#endif

//=======================================================================================================
STATIC const mp_rom_map_elem_t tftlcd_lcd43m_locals_dict_table[] = {
    // instance methods
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&tftlcd_lcd43m_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&tftlcd_lcd43m_drawp_obj) },
    { MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&tftlcd_lcd43m_drawL_obj) },
    { MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&tftlcd_lcd43m_drawRect_obj) },
    { MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&tftlcd_lcd43m_drawCircle_obj) },
    { MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&tftlcd_lcd43m_printStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&tftlcd_lcd43m_write_buf_obj) },
	
    #if MICROPY_PY_PICLIB
    { MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&tftlcd_lcd43m_Picture_obj) },
    { MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&tftlcd_lcd43g_CachePicture_obj) },
    #endif

};

STATIC MP_DEFINE_CONST_DICT(tftlcd_lcd43m_locals_dict, tftlcd_lcd43m_locals_dict_table);

const mp_obj_type_t tftlcd_lcd43m_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD43M,
    .print = tftlcd_lcd43m_print,
    .make_new = tftlcd_lcd43m_make_new,
    .locals_dict = (mp_obj_dict_t *)&tftlcd_lcd43m_locals_dict,
};

#endif



