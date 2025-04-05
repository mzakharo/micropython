/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	lcd7g.c
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

#if (MICROPY_ENABLE_TFTLCD && MICROPY_HW_LCD7R )
	
#include "modtftlcd.h"
#include "lcd7r.h"

#include "HAL_conf.h"
#include "global.h" 

#if MICROPY_PY_PICLIB
#include "piclib.h" 
#endif

//==============================================================================
Graphics_Display lcd7r_glcd;

typedef struct _tftlcd_lcd7r_obj_t {
    mp_obj_base_t base;
	uint8_t buf[1];
} tftlcd_lcd7r_obj_t;

STATIC tftlcd_lcd7r_obj_t lcd7r_obj;

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
static void LCD_Reset(void)
{
	mp_hal_pin_low(MICROPY_HW_LTDC_RST);
	mp_hal_delay_ms(100);					   
	mp_hal_pin_high(MICROPY_HW_LTDC_RST);	 	 
	mp_hal_delay_ms(100);
}
static void Set_LCD7_Timing_to_LTDC(void)//设置LCD的时序到LTDC寄存器中
{
	LCD_FORM_TypeDef LCD_FORM;
	
	LTDC->OUT_EN = 0;
	LTDC->DP_ADDR0 = (uint32_t)LTDC_Buf;
	LTDC->BLK_DATA = 0xFFFF;//背景色
	
	LCD_FORM.sHsyncStart = 0x2E;  //水平激活起始
	LCD_FORM.sHsyncEnd = 0xD2;    //水平激活结束
	LCD_FORM.aHorLen = 800-1;  //水平分辨率
	LCD_FORM.blkHorEnd = 0xff;    //水平消隐

	LCD_FORM.sVsyncStart = 0x17;  //垂直激活起始
	LCD_FORM.sVsyncEnd = 0x16;    //垂直激活结束
	LCD_FORM.aVerLen= 480-1; 	 //垂直分辨率
	LCD_FORM.blkVerEnd = 0x15;   //垂直消隐

	Set_LTDC_REG(&LCD_FORM);
	LTDC->VI_FORMAT = 0x01;

	LTDC->POL_CTL = 0x3;
	LTDC->OUT_EN |= 0x107;
}

void lcd7r_init(void)
{
	STATIC bool init_flag = false;
	lcddev.type = 3;
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

	mp_hal_pin_config(MICROPY_HW_LTDC_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);
	mp_hal_pin_config(MICROPY_HW_LTDC_RST, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0);

	RCC->AHB1ENR |= (1<<31);
	RCC->CR &= ~(1<<28);

	RCC->CR |= 1<<28;

	RCC->PLLDCKCFGR = 0x0<<16;  //分频系数 0~3 --> 2,4,6,8
	RCC->PLLLCDCFGR = 18<<6;   	//倍频系数	
	RCC->PLLLCDCFGR |= 0x03;   	//倍频系数	

	Set_LCD7_Timing_to_LTDC();//设置LCD的时序到LTDC寄存器中
	LCD_Reset();
	
	init_flag = true;
	
	lcddev.backcolor = BLACK;
	
	for(uint32_t i=0; i < lcddev.width*lcddev.height; i++){
		LTDC_Buf[i] = 0x0U;
	}
	mp_hal_pin_high(MICROPY_HW_LTDC_BL);
}


void lcd7r_Set_Dir(uint8_t dir)
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
	lcd7r_glcd.width = lcddev.width;
	lcd7r_glcd.height = lcddev.height;
}	 

void lcd7r_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
	if(x >= lcddev.width || y >= lcddev.height) return;
	uint16_t cbgr = rgb565tobgr565(color);
	switch (lcddev.dir)
	{
		case 2:
		LTDC_Buf[((lcddev.x_pixel*(x+1))-(y)) - 1] = cbgr;
		break;
		case 3:
		LTDC_Buf[((lcddev.x_pixel*lcddev.y_pixel)-(x+(y)*lcddev.x_pixel))-1] = cbgr;
		break;
		case 4:
		LTDC_Buf[(lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)] = cbgr;
		break;
		default:
		LTDC_Buf[lcddev.x_pixel*y+x] = cbgr;
		break;
	}
}

uint16_t lcd7r_ReadPoint(uint16_t x , uint16_t y)
{
	if(x >= lcddev.width || y >= lcddev.height) return 0;
	uint16_t color = 0;
	switch (lcddev.dir)
	{
		case 2:
		color =  LTDC_Buf[((lcddev.x_pixel*(x+1))-(y)) - 1];
		break;
		case 3:
		color = LTDC_Buf[((lcddev.x_pixel*lcddev.y_pixel)-(x+(y)*lcddev.x_pixel))-1];
		break;
		case 4:
		color = LTDC_Buf[(lcddev.x_pixel*lcddev.y_pixel)-((lcddev.x_pixel*(x+1))-y)];
		break;
		default:
		color = LTDC_Buf[lcddev.x_pixel*y+x];
		break;
	}
	return bgr565torgb565(color);
}

void lcd7r_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{  
	uint16_t x,y;
	for(y = sy; y < ey+sy; y++)
	{
		for(x=sx; x < ex+sx; x++)	
		{
			lcd7r_DrawPoint(x,y,*color++);
		}
	}
}

void lcd7r_fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{
	uint16_t x,y;
	for(y = sy; y <= ey; y++)
	{
		for(x=sx; x <= ex; x++)	lcd7r_DrawPoint(x,y,color);
	}
}

void lcd7r_Clear(uint16_t color)
{
	uint32_t i = lcddev.width*lcddev.height;
	while(i--){
		LTDC_Buf[i] = color;
	}
}

Graphics_Display lcd7r_glcd =
{
	16,
	480,
	800,
	lcd7r_DrawPoint,
	lcd7r_ReadPoint,
	NULL,
	NULL,
	lcd7r_fill,
	lcd7r_Full,
	NULL,
	lcd7r_Clear
};
//==============================================================================================================

//------------------------------------------------------------------------------------------------------

STATIC void tftlcd_lcd7r_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print, "LCD7R(portrait=%d),width:%u,height:%u,X_PIXEL:%u,Y_PIXEL:%u\n",lcddev.dir,lcddev.width,lcddev.height,
	lcddev.x_pixel,lcddev.y_pixel);
}

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd7r_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcddev.dir = args[ARG_portrait].u_int;

	lcddev.backcolor = BLACK;
	
	lcddev.x_pixel = 800;
	lcddev.y_pixel = 480;

	lcd7r_Set_Dir(lcddev.dir);
	lcd7r_init();
	lcd7r_obj.base.type = &tftlcd_lcd7r_type;
	
	draw_global = &lcd7r_glcd;

	return MP_OBJ_FROM_PTR(&lcd7r_obj);
}
//=======================================================================================================
#if MICROPY_PY_PICLIB
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_Picture_obj, 1, tftlcd_Picture);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_CachePicture_obj, 1, tftlcd_CachePicture);
#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tftlcd_lcd7r_write_buf_obj, 1, 6, tftlcd_write_buf);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_printStr_obj, 1, tftlcd_printStr);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawCircle_obj, 1, tftlcd_drawCircle);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawRect_obj, 1, tftlcd_drawRect);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_clear_obj, 1, tftlcd_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawL_obj, 4, tftlcd_drawL);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawPixel_obj, 1, tftlcd_drawPixel);

STATIC const mp_rom_map_elem_t tftlcd_lcd7r_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&tftlcd_lcd7r_clear_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&tftlcd_lcd7r_drawPixel_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&tftlcd_lcd7r_drawL_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&tftlcd_lcd7r_drawRect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&tftlcd_lcd7r_drawCircle_obj) },
	{ MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&tftlcd_lcd7r_printStr_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&tftlcd_lcd7r_write_buf_obj) },
	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&tftlcd_lcd7r_Picture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&tftlcd_lcd7r_CachePicture_obj) },
	#endif
};

MP_DEFINE_CONST_DICT(tftlcd_lcd7r_locals_dict, tftlcd_lcd7r_locals_dict_table);

const mp_obj_type_t tftlcd_lcd7r_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD7R,
    .print = tftlcd_lcd7r_print,
    .make_new = tftlcd_lcd7r_make_new,
    .locals_dict = (mp_obj_dict_t *)&tftlcd_lcd7r_locals_dict,
};

#endif





