/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	ST7789.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/9/18
	* Description 			 :	
	******************************************************************************
**/

#ifndef MICROPY_INCLUDED_STM32_LCD_SPIBUS_H
#define MICROPY_INCLUDED_STM32_LCD_SPIBUS_H

#include "py/obj.h"
#include "spi.h"
#if (MICROPY_ENABLE_SPILCD)

typedef struct {
  mp_obj_base_t base;
	const spi_t *spi;
	const pin_obj_t *pin_dc;
	const pin_obj_t *pin_rst;
	const pin_obj_t *pin_cs1;
} lcd_spibus_t;

extern lcd_spibus_t *lcd_spibus;

extern void lcd_bus_init(void);
extern void lcd_spibus_send_data(lcd_spibus_t *self, const void * data, uint32_t length);
extern void lcd_spibus_send_cmd(lcd_spibus_t *self, uint8_t cmd);
extern void lcd_spibus_fill(lcd_spibus_t *self, uint16_t data, uint32_t length);
extern void lcd_spibus_send(lcd_spibus_t *self, const uint8_t * data, uint32_t length);
extern void lcd_spibus_deinit(void);
extern void lcd_spibus_read(lcd_spibus_t *self, uint8_t *buf, uint32_t len);
extern uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color);

#endif

#endif // MICROPY_INCLUDED_STM32_LCD_SPIBUS_H
