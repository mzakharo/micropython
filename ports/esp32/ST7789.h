
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	ST7789.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/9/18
	* Description			:	
******************************************************************************/

#ifndef MICROPY_INCLUDED_ESP32_ST7789_H
#define MICROPY_INCLUDED_ESP32_ST7789_H

#include "py/obj.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#if (MICROPY_HW_LCD15 && MICROPY_ENABLE_TFTLCD)
	
#include "modtftlcd.h"


extern Graphics_Display st7789_glcd;

extern const mp_obj_type_t ST7789_type;

extern void st7789_set_dir(uint8_t dir);
extern void  mp_init_ST7789(void);
extern uint16_t st7789_readPoint(uint16_t x, uint16_t y);
extern void st7789_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color);
extern void st7789_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
extern void st7789_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
extern void st7789_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color);

#endif

#endif // MICROPY_INCLUDED_ESP32_ST7789_H
