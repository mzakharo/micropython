
/********************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	ili9341.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/1
	* Description			:	
******************************************************************************/

#ifndef MICROPY_INCLUDED_ESP32_ILI9341_H
#define MICROPY_INCLUDED_ESP32_ILI9341_H

#include "py/obj.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#if (MICROPY_HW_LCD32 && MICROPY_ENABLE_TFTLCD)
	
#include "modtftlcd.h"

extern Graphics_Display g_lcd;

extern const mp_obj_type_t ILI9341_type;

extern void ili9341_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color);
extern void ili9341_Full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
extern void ili9341_cam_full(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
extern void ili9341_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
extern uint16_t ili9341_readPoint(uint16_t x, uint16_t y);
extern void ili9341_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color);
extern void ili9341_set_dir(uint8_t dir);
extern void  mp_init_ILI9341(void);
#endif

#endif // MICROPY_INCLUDED_ESP32_ILI9341_H
