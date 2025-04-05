/********************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	ST7789.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/9/22
	* Description			:	
******************************************************************************/


#ifndef MICROPY_INCLUDED_ESP32_LCD_SPIBUS_H
#define MICROPY_INCLUDED_ESP32_LCD_SPIBUS_H

#include "py/obj.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#if (MICROPY_ENABLE_TFTLCD)

typedef struct {
	mp_obj_base_t base;
	spi_device_handle_t spi;
	uint8_t spihost;
	uint8_t mhz;
	int miso;
	int mosi;
	int clk;
	int cs;
	int dc;
	int rst;
} lcd_spibus_t;

extern lcd_spibus_t *lcd_spibus;

extern void lcd_bus_init(void);
extern void lcd_spibus_send_data(lcd_spibus_t *self, const void * data, uint32_t length);
extern void lcd_spibus_send_cmd(lcd_spibus_t *self, uint8_t cmd);
extern void lcd_spibus_fill(lcd_spibus_t *self, uint16_t data, uint32_t length);
extern void lcd_spibus_send(lcd_spibus_t *self, const uint8_t * data, uint32_t length);
extern void lcd_spibus_deinit(void);
extern uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color);
#endif

#endif // MICROPY_INCLUDED_ESP32_LCD_SPIBUS_H
