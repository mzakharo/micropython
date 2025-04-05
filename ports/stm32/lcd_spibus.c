/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	ST7789.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/9/22
	* Description 			 :	
	******************************************************************************
**/

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"


#if (MICROPY_ENABLE_SPILCD)
	
#include "lcd_spibus.h"
#include "modtftlcd.h"
#include "pin.h"
#include "bufhelper.h"
#include "spi.h"

lcd_spibus_t *lcd_spibus = NULL;

STATIC bool is_init = 0;

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

// uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color)
// {
	// uint16_t B_color = (b_color >> 3) & 0x001F;
	// uint16_t G_color = ((g_color >> 2) << 5) & 0x07E0;
	// uint16_t R_color = ((r_color >> 3) << 11) & 0xF800;

	// return (uint16_t) (R_color | G_color | B_color);
// }

STATIC void lcd_delay(void) {
    __asm volatile ("nop\nnop");
}
//--------------------------------------------------------
void lcd_spibus_send(lcd_spibus_t *self, const uint8_t * data, uint32_t length)
{
	mp_hal_pin_low(self->pin_cs1); // CS=0; enable
	lcd_delay();
	HAL_SPI_Transmit(self->spi->spi, (uint8_t *)data, length, 1000);

	lcd_delay();
	mp_hal_pin_high(self->pin_cs1); // CS=1; disable
}

//---------------------------------------------------------
void lcd_spibus_fill(lcd_spibus_t *self, uint16_t data, uint32_t length)
{
	if (length == 0) return;           //no need to send anything
	mp_hal_pin_high(self->pin_dc); /*Data mode*/
	uint8_t *t_data = (uint8_t *)m_malloc(length*2);
	if(t_data == NULL){
		printf("fill malloc error\r\n");
	}
	for(uint32_t i=0; i < length; i++){
		t_data[2*i] = (data >> 8);
		t_data[2*i+1] = (data & 0xFF);
	}
	lcd_spibus_send(self, t_data, length*2);
	m_free(t_data);
}
//---------------------------------------------------------
void lcd_spibus_send_cmd(lcd_spibus_t *self, uint8_t cmd)
{
	mp_hal_pin_low(self->pin_dc);	 /*Command mode*/
	lcd_delay();
	lcd_spibus_send(self, &cmd, 1);
}

//---------------------------------------------------------
void lcd_spibus_send_data(lcd_spibus_t *self, const void * data, uint32_t length)
{
	mp_hal_pin_high(self->pin_dc); /*Data mode*/
	lcd_delay();
	lcd_spibus_send(self, data, length);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void lcd_spibus_read(lcd_spibus_t *self, uint8_t *buf, uint32_t len) {
	mp_hal_pin_low(self->pin_cs1); // CS=0; enable
	lcd_delay();
	HAL_StatusTypeDef status = HAL_SPI_Receive(self->spi->spi, buf, len, 5000);
	lcd_delay();
	mp_hal_pin_high(self->pin_cs1); // CS=1; disable
	(void)status;
}

void lcd_bus_init(void)
{
	if(!is_init)
	{
		lcd_spibus_t *lcd = (lcd_spibus_t *)m_malloc(sizeof(lcd_spibus_t));
		// init the SPI bus
		lcd->spi 			= &spi_obj[1]; // spi2
		lcd->pin_dc		= LCD_PIN_DC;
		lcd->pin_rst	= LCD_PIN_RST;
		lcd->pin_cs1	= LCD_PIN_CS;
		
		SPI_InitTypeDef *init = &lcd->spi->spi->Init;
		init->Mode = SPI_MODE_MASTER;
		
		uint spi_clock;
    if (lcd->spi->spi->Instance == SPI1) {
        // SPI1 is on APB2
        spi_clock = HAL_RCC_GetPCLK2Freq();
    } else {
        // SPI2 and SPI3 are on APB1
        spi_clock = HAL_RCC_GetPCLK1Freq();
    }
		
    //uint br_prescale = spi_clock / 16000000; // datasheet says LCD can run at 20MHz, but we go for 16MHz
		uint br_prescale = spi_clock / 5000000; // datasheet says LCD can run at 20MHz, but we go for 16MHz
    if (br_prescale <= 2) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    } else if (br_prescale <= 4) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    } else if (br_prescale <= 8) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    } else if (br_prescale <= 16) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    } else if (br_prescale <= 32) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    } else if (br_prescale <= 64) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    } else if (br_prescale <= 128) {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    } else {
        init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    }
		
    // data is sent bigendian, latches on rising clock
    init->CLKPolarity = SPI_POLARITY_HIGH;
    init->CLKPhase = SPI_PHASE_2EDGE;
    init->Direction = SPI_DIRECTION_2LINES;
    init->DataSize = SPI_DATASIZE_8BIT;
    init->NSS = SPI_NSS_SOFT;
    init->FirstBit = SPI_FIRSTBIT_MSB;
    init->TIMode = SPI_TIMODE_DISABLED;
    init->CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    init->CRCPolynomial = 0;

    // init the SPI bus
    spi_init(lcd->spi, false);
		
    // set the pins to default values
    mp_hal_pin_high(lcd->pin_cs1);
    mp_hal_pin_high(lcd->pin_rst);
    mp_hal_pin_high(lcd->pin_dc);

    // init the pins to be push/pull outputs
    mp_hal_pin_output(lcd->pin_cs1);
    mp_hal_pin_output(lcd->pin_rst);
    mp_hal_pin_output(lcd->pin_dc);
		
	    // init the LCD
    mp_hal_delay_ms(1); // wait a bit
    mp_hal_pin_low(lcd->pin_rst); // RST=0; reset
    mp_hal_delay_ms(100); // wait for reset; 2us min
    mp_hal_pin_high(lcd->pin_rst); // RST=1; enable
    mp_hal_delay_ms(100); // wait for reset; 2us min
		is_init = 1;
		lcd_spibus = lcd;
	}
}

void lcd_spibus_deinit(void) {
	if(is_init){
		spi_deinit(lcd_spibus->spi);
		is_init = 0;
	}
}

//-------------------------------------------------------------------------------------------
#endif
