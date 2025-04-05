/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	lcd43g.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/3/25
	* Description			:	
******************************************************************************/


#include "py/runtime.h"
#include "py/mphal.h"
#include "softtimer.h"
#include "bufhelper.h"

#include <math.h>

#include "py/builtin.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "i2c.h"

#include "py/obj.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "extmod/vfs_lfs.h"

#include "pin.h"
#include "pin_static_af.h"
#include "systick.h"


#if (MICROPY_ENABLE_TFTLCD && MICROPY_HW_LCD43G)
	
#include "HAL_conf.h"

#include "modtftlcd.h"
#include "lcd43g.h"

#if MICROPY_PY_PICLIB
#include "piclib.h" 
#include "global.h" 
#endif

//==============================================================================
Graphics_Display lcd43r_glcd;
//==============================================================================
static void LCD_WriteByteSPI(unsigned char byte)
{
	unsigned char n;
	for(n=0; n<8; n++)			
	{  
		if(byte&0x80) 
			mp_hal_pin_high(MICROPY_HW_SPI_SDA);
		else 
			mp_hal_pin_low(MICROPY_HW_SPI_SDA);
		byte<<= 1;
		mp_hal_pin_low(MICROPY_HW_SPI_CLK);
		mp_hal_pin_high(MICROPY_HW_SPI_CLK);
	}
}
static void SPI_WriteComm(uint16_t CMD)//3线9bit 串行接口
{			
	mp_hal_pin_low(MICROPY_HW_SPI_CS);
	mp_hal_pin_low(MICROPY_HW_SPI_SDA);
	mp_hal_pin_low(MICROPY_HW_SPI_CLK);
	mp_hal_pin_high(MICROPY_HW_SPI_CLK);
	LCD_WriteByteSPI(CMD);
	mp_hal_pin_high(MICROPY_HW_SPI_CS);
}
static void SPI_WriteData(uint16_t tem_data)
{			
	mp_hal_pin_low(MICROPY_HW_SPI_CS);
	mp_hal_pin_high(MICROPY_HW_SPI_SDA);
	mp_hal_pin_low(MICROPY_HW_SPI_CLK);
	mp_hal_pin_high(MICROPY_HW_SPI_CLK);
	LCD_WriteByteSPI(tem_data);
	mp_hal_pin_high(MICROPY_HW_SPI_CS);
}
static void LCD_Reset(void)
{
	mp_hal_pin_low(MICROPY_HW_LTDC_RST);
	mp_hal_delay_ms(100);					   
	mp_hal_pin_high(MICROPY_HW_LTDC_RST);	 	 
	mp_hal_delay_ms(100);
}
static void LCD_init_code(void)//液晶屏初始化代码
{
	LCD_Reset();
	mp_hal_pin_high(MICROPY_HW_SPI_CS);	
	mp_hal_delay_ms(200);
	mp_hal_pin_low(MICROPY_HW_SPI_CS);	


	SPI_WriteComm(0xc0);  
	SPI_WriteData(0x01);
	SPI_WriteData(0x11);

	SPI_WriteComm(0x20);  

	SPI_WriteComm(0x36);  
	//SPI_WriteData(0x88);//BRG
	SPI_WriteData(0x80);//RGB

	SPI_WriteComm(0x3a);  
	SPI_WriteData(0x77);//16/18/24bit

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
	SPI_WriteData(0x69);  //69
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

static void Set_LTDC_REG(LCD_FORM_TypeDef* LCD_FORM)
{
	uint32_t aHorStart;
	uint32_t aHorEnd;
	uint32_t aVerStart;
	uint32_t aVerEnd;

	aHorStart = LCD_FORM->blkHorEnd + 1;
	aHorEnd = aHorStart + LCD_FORM->aHorLen;  
	aVerStart = LCD_FORM->blkVerEnd + 1 ;
	aVerEnd = aVerStart + LCD_FORM->aVerLen;

	LTDC->P_HOR = aHorEnd;//总宽度
	LTDC->HSYNC = (LCD_FORM->sHsyncStart <<16 )|LCD_FORM->sHsyncEnd;//水平同步信号起始和结束，位于背景色中间
	LTDC->A_HOR = (aHorStart<<16)|aHorEnd;//水平激活起始和结束
	LTDC->A_HOR_LEN = LCD_FORM->aHorLen ;//水平激活域宽度
	LTDC->BLK_HOR = (0<<16)|LCD_FORM->blkHorEnd;//背景开始和结束宽度0~激活地址	
	LTDC->P_VER =  aVerEnd;
	LTDC->VSYNC = (LCD_FORM->sVsyncStart<<16)|LCD_FORM->sVsyncEnd;
	LTDC->A_VER = (aVerStart<<16)|aVerEnd;
	LTDC->A_VER_LEN = LCD_FORM->aVerLen ;
	LTDC->BLK_VER = (0<<16)|LCD_FORM->blkVerEnd;
}

static void Set_LCD_Timing_to_LTDC(void)//设置LCD的时序到LTDC寄存器中
{
	LCD_FORM_TypeDef LCD_FORM;
	
	LTDC->OUT_EN = 0;
	LTDC->DP_ADDR0 = (uint32_t)LTDC_Buf;
	LTDC->BLK_DATA = 0xFFFF;//背景色
	
	LCD_FORM.sHsyncStart = 0x2;  //水平激活起始
	LCD_FORM.sHsyncEnd = 0x3;    //水平激活结束
	LCD_FORM.aHorLen = 480-1;  //水平分辨率
	LCD_FORM.blkHorEnd = 0xf;    //水平消隐

	LCD_FORM.sVsyncStart = 0x2;  //垂直激活起始
	LCD_FORM.sVsyncEnd = 0x8;    //垂直激活结束
	LCD_FORM.aVerLen= 800-1; 	 //垂直分辨率
	LCD_FORM.blkVerEnd = 0xf;   //垂直消隐

	Set_LTDC_REG(&LCD_FORM);
	LTDC->VI_FORMAT = 0x01;

	LTDC->POL_CTL = 0xA;
	LTDC->OUT_EN |= 0x107;
}

void lcd43g_init(void)
{
	STATIC bool init_flag = false;
	
	lcddev.type = 2;
	if(init_flag) return;

	GPIO_InitTypeDef GPIO_InitStructure;//定义GPIO初始化结构体变量
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin  =  GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOE,  GPIO_Pin_All , GPIO_AF_LTDC); //GPIOE所有的IO全部复用为LTDC的数据线

	mp_hal_pin_config(MICROPY_HW_LTDC_DE, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_NONE, 14);
	mp_hal_pin_config(MICROPY_HW_LTDC_PCLK, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_NONE, 14);
	mp_hal_pin_config(MICROPY_HW_LTDC_HSYNC, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_NONE, 14);
	mp_hal_pin_config(MICROPY_HW_LTDC_VSYNC, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_NONE, 14);

	mp_hal_pin_config(MICROPY_HW_SPI_SDA, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
	mp_hal_pin_config(MICROPY_HW_SPI_CLK, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
	mp_hal_pin_config(MICROPY_HW_SPI_CS, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
	mp_hal_pin_config(MICROPY_HW_LTDC_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
	mp_hal_pin_config(MICROPY_HW_LTDC_RST, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);

	LCD_init_code();

	RCC->AHB1ENR |= (1<<31);
	RCC->CR |= 1<<28;
	RCC->PLLDCKCFGR = 0x1<<16;  //分频系数 0~3 --> 2,4,6,8
	RCC->PLLLCDCFGR = 6<<6;   	//倍频系数

	Set_LCD_Timing_to_LTDC();//设置LCD的时序到LTDC寄存器中

	init_flag = true;
	
	lcddev.width 	= 480;
	lcddev.height = 800;
	lcddev.backcolor = BLACK;
	
	for(uint32_t i=0; i < lcddev.width*lcddev.height; i++){
		LTDC_Buf[i] = 0x0U;
	}
	mp_hal_pin_high(MICROPY_HW_LTDC_BL);

}

//快速画点
//x,y:坐标
//color:颜色
void lcd43r_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
	if(x >= lcddev.width || y >= lcddev.height) return;
	
	switch (lcddev.dir)
	{
		case 2:
		LTDC_Buf[((lcddev.x_pixel*lcddev.y_pixel)-(x+(y)*lcddev.x_pixel))-1] = color;
		break;
		case 3:
		LTDC_Buf[(lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)] = color;
		break;
		case 4:
		LTDC_Buf[lcddev.x_pixel*y+x] = color;
		break;
		default:
		LTDC_Buf[((lcddev.x_pixel*(x+1))-(y)) - 1] = color;
		break;
	}
}	 

uint16_t lcd43r_ReadPoint(uint16_t x , uint16_t y)
{
	if(x >= lcddev.width || y >= lcddev.height) return 0;

	switch (lcddev.dir)
	{
		case 2:
		return (LTDC_Buf[((lcddev.x_pixel*lcddev.y_pixel)-(x+(y)*lcddev.x_pixel))-1]);
		break;
		case 3:
		return (LTDC_Buf[(lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)]);
		break;
		case 4:
		return (LTDC_Buf[lcddev.x_pixel*y+x]);
		break;
		default:
		return (LTDC_Buf[((lcddev.x_pixel*(x+1))-(y)) - 1]);
		break;
	}

	return 0;
}

void lcd43r_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{  
	uint16_t x,y;
	for(y = sy; y < ey+sy; y++)
	{
		for(x=sx; x < ex+sx; x++)	
		{
			lcd43r_DrawPoint(x,y,*color++);
		}
	}
}

void lcd43r_fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
	uint16_t x,y;
	for(y = sy; y <= ey; y++)
	{
		for(x=sx; x <= ex; x++)	lcd43r_DrawPoint(x,y,color);
	}
}

//清屏函数
void lcd43r_Clear(uint16_t color)
{
	uint32_t i = lcddev.width*lcddev.height;
	while(i--){
		LTDC_Buf[i] = color;
	}
}

//设置LCD显示方向
void lcd43r_Set_Dir(uint8_t dir)
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
	lcd43r_glcd.width = lcddev.width;
	lcd43r_glcd.height = lcddev.height;
}	 

Graphics_Display lcd43r_glcd =
{
	16,
	480,
	800,
	lcd43r_DrawPoint,
	lcd43r_ReadPoint,
	NULL,
	NULL,
	lcd43r_fill,
	lcd43r_Full,
	NULL,
	lcd43r_Clear
};

//==============================================================================================================
typedef struct _tftlcd_lcd43g_obj_t {
    mp_obj_base_t base;
} tftlcd_lcd43g_obj_t;

STATIC tftlcd_lcd43g_obj_t lcd43g_obj;
//------------------------------------------------------------------------------------------------------

STATIC void tftlcd_lcd43g_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print, "LCD43R(portrait=%d),width:%u,height:%u,X_PIXEL:%u,Y_PIXEL:%u\n",lcddev.dir,lcddev.width,lcddev.height,
	lcddev.x_pixel,lcddev.y_pixel);
}

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd43g_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcddev.dir = args[ARG_portrait].u_int;
	lcddev.backcolor = BLACK;
	
	lcddev.x_pixel = 480;
	lcddev.y_pixel = 800;
	
	lcd43g_init();
	lcd43r_Set_Dir(lcddev.dir);

	lcd43g_obj.base.type = &tftlcd_lcd43g_type;
	draw_global = &lcd43r_glcd;

	return MP_OBJ_FROM_PTR(&lcd43g_obj);
}

//------------------------------------------------------------------------------------------------------
#if MICROPY_PY_PICLIB
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_Picture_obj, 1, tftlcd_Picture);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_CachePicture_obj, 1, tftlcd_CachePicture);
#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tftlcd_lcd43g_write_buf_obj, 1, 6, tftlcd_write_buf);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_printStr_obj, 1, tftlcd_printStr);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_drawCircle_obj, 1, tftlcd_drawCircle);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_drawRect_obj, 1, tftlcd_drawRect);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_clear_obj, 1, tftlcd_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_drawL_obj, 4, tftlcd_drawL);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd43g_drawPixel_obj, 1, tftlcd_drawPixel);
//=======================================================================================================
STATIC const mp_rom_map_elem_t tftlcd_lcd43g_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&tftlcd_lcd43g_clear_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&tftlcd_lcd43g_drawPixel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&tftlcd_lcd43g_drawL_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&tftlcd_lcd43g_drawRect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&tftlcd_lcd43g_drawCircle_obj) },
	{ MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&tftlcd_lcd43g_printStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&tftlcd_lcd43g_write_buf_obj) },
	
	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&tftlcd_lcd43g_Picture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&tftlcd_lcd43g_CachePicture_obj) },
	#endif
};

MP_DEFINE_CONST_DICT(tftlcd_lcd43g_locals_dict, tftlcd_lcd43g_locals_dict_table);

const mp_obj_type_t tftlcd_lcd43g_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD43R,
    .print = tftlcd_lcd43g_print,
    .make_new = tftlcd_lcd43g_make_new,
    .locals_dict = (mp_obj_dict_t *)&tftlcd_lcd43g_locals_dict,
};

#endif





