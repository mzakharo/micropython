
/********************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	ST7789.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/9/22
	* Description			:	
******************************************************************************/

#include "mpconfigboard.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/rtc_io.h"

#include "py/obj.h"
#include <math.h>
#include "py/builtin.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "modmachine.h"

#include "py/binary.h"
#include "py/objarray.h"
#include "py/mperrno.h"
#include "extmod/vfs.h"
#include "py/stream.h"
#include "shared/runtime/pyexec.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "mphalport.h"

#include "extmod/virtpin.h"
#include "machine_rtc.h"
#include "modesp32.h"

#if CONFIG_IDF_TARGET_ESP32C3
#include "hal/gpio_ll.h"
#endif



#if (MICROPY_ENABLE_TFTLCD)
#include "lcd_spibus.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define SPI_DMA_CH	2
#define LCD_HOST    HSPI_HOST
#elif defined CONFIG_IDF_TARGET_ESP32S2
#define LCD_HOST    SPI2_HOST
#define SPI_DMA_CH	3
#elif defined CONFIG_IDF_TARGET_ESP32C3
#define LCD_HOST    	SPI2_HOST
#define SPI_DMA_CH		SPI_DMA_CH_AUTO
#elif defined CONFIG_IDF_TARGET_ESP32S3
#define LCD_HOST    SPI2_HOST
#define SPI_DMA_CH	SPI_DMA_CH_AUTO
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif

lcd_spibus_t *lcd_spibus = NULL;

STATIC bool is_init = 0;

static void lcd_global_init(gpio_num_t gpio,gpio_mode_t io_mode,uint8_t mode)
{
	if (rtc_gpio_is_valid_gpio(gpio)) {
		#if CONFIG_IDF_TARGET_ESP32C3
		// gpio_reset_pin(gpio);
		#else
		rtc_gpio_deinit(gpio);
		#endif
	}
	
    #if CONFIG_IDF_TARGET_ESP32C3
    if (gpio == 18 || gpio == 19) {
        CLEAR_PERI_REG_MASK(USB_DEVICE_CONF0_REG, USB_DEVICE_USB_PAD_ENABLE);
    }
    #endif

	gpio_pad_select_gpio(gpio);
	gpio_set_direction(gpio, io_mode);
	
	gpio_pulldown_dis(gpio);

	gpio_pullup_en(gpio);

	if (GPIO_IS_VALID_OUTPUT_GPIO(gpio)) {
		gpio_hold_dis(gpio);
	}
}

uint16_t get_rgb565(uint8_t r_color, uint8_t g_color , uint8_t b_color)
{
	uint16_t B_color = (b_color >> 3) & 0x001F;
	uint16_t G_color = ((g_color >> 2) << 5) & 0x07E0;
	uint16_t R_color = ((r_color >> 3) << 11) & 0xF800;

	return (uint16_t) (R_color | G_color | B_color);
}

STATIC void lcd_spibus_init(lcd_spibus_t *self)
{
	esp_err_t ret;
	spi_bus_config_t buscfg={
		.miso_io_num=self->miso,
		.mosi_io_num=self->mosi,
		.sclk_io_num=self->clk,
		.quadwp_io_num=-1,
		.quadhd_io_num=-1,
		#if CONFIG_ESP32_SPIRAM_SUPPORT || CONFIG_ESP32S2_SPIRAM_SUPPORT || CONFIG_ESP32S3_SPIRAM_SUPPORT
		.max_transfer_sz=2*240*240+10,
		#else
		.max_transfer_sz=11*1024,
		#endif
	};

	spi_device_interface_config_t devcfg={
		.clock_speed_hz=self->mhz*1000*1000,
		.mode=0,                             //SPI mode 0
		.spics_io_num=self->cs,              //CS pin
		.queue_size=7,
		.flags=SPI_DEVICE_HALFDUPLEX,
	};

	//Initialize the SPI bus
	ret=spi_bus_initialize(self->spihost, &buscfg, SPI_DMA_CH);
	if (ret != ESP_OK){
		mp_raise_ValueError(MP_ERROR_TEXT("lcd Failed initializing SPI bus"));
	}
	//Attach the LCD to the SPI bus
	ret=spi_bus_add_device(self->spihost, &devcfg, &self->spi);
	if (ret != ESP_OK){
		mp_raise_ValueError(MP_ERROR_TEXT("lcd Failed adding SPI device"));
	}
}
//--------------------------------------------------------
void lcd_spibus_send(lcd_spibus_t *self, const uint8_t * data, uint32_t length)
{
	if (length == 0) return;           //no need to send anything

	spi_transaction_t t;
	memset(&t, 0, sizeof(t));       	//Zero out the transaction
	t.length = length * 8;              //Length is in bytes, transaction length is in bits.
	t.tx_buffer = data;               	//Data

	spi_device_transmit(self->spi, &t);
}
//---------------------------------------------------------
void lcd_spibus_fill(lcd_spibus_t *self, uint16_t data, uint32_t length)
{
	if (length == 0) return;           //no need to send anything

	gpio_set_level(self->dc, 1);	 /*Data mode*/

	uint8_t *t_data = (uint8_t *)m_malloc(length*2);
	if(t_data == NULL){
		printf("fill malloc error\r\n");
	}
	for(uint32_t i=0; i < length; i++){
		t_data[2*i] = (data >> 8);
		t_data[2*i+1] = (data & 0xFF);
	}

	spi_transaction_t t;
	memset(&t, 0, sizeof(t));       	//Zero out the transaction
	t.length = length * 8 *2;              //Length is in bytes, transaction length is in bits.
	t.tx_buffer = t_data;               	//Data

	spi_device_transmit(self->spi, &t);
	
	m_free(t_data);
}

//---------------------------------------------------------
void lcd_spibus_send_cmd(lcd_spibus_t *self, uint8_t cmd)
{
	gpio_set_level(self->dc, 0);	 /*Command mode*/
	lcd_spibus_send(self, &cmd, 1);
}

//---------------------------------------------------------
void lcd_spibus_send_data(lcd_spibus_t *self, const void * data, uint32_t length)
{
	gpio_set_level(self->dc, 1);	 /*Data mode*/
	lcd_spibus_send(self, data, length);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void  lcd_bus_init(void)
{
	if(!is_init)
	{
		lcd_spibus = (lcd_spibus_t *)m_malloc(sizeof(lcd_spibus_t));
		if(lcd_spibus == NULL){
			printf("lcd_spibus is NULL\r\n");
		}
		lcd_spibus_t *self = lcd_spibus;
		#if CONFIG_IDF_TARGET_ESP32C3
		self->mhz			= 10;
		#else
		self->mhz			= 50;
		#endif
		self->spi 		= NULL;
		self->miso 		= LCD_PIN_MISO;
		self->mosi 		= LCD_PIN_MOSI;
		self->clk  		= LCD_PIN_CLK;
		self->cs   		= LCD_PIN_CS;
		self->dc   		= LCD_PIN_DC;
		self->rst  		= LCD_PIN_RST;
		self->spihost	= LCD_HOST;
		lcd_spibus_init(self);
		
		lcd_global_init(self->dc,GPIO_MODE_OUTPUT,2);
		lcd_global_init(self->rst,GPIO_MODE_OUTPUT,2);
		
		//Reset the display
		gpio_set_level(self->rst, 0);
		vTaskDelay(100 / portTICK_RATE_MS);
		gpio_set_level(self->rst, 1);
		vTaskDelay(100 / portTICK_RATE_MS);
		
		is_init = 1;
	}
}

void lcd_spibus_deinit(void) {
	if(is_init){
		switch (spi_bus_remove_device(lcd_spibus->spi)) {
			case ESP_ERR_INVALID_ARG:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
				return;

			case ESP_ERR_INVALID_STATE:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI device already freed"));
				return;
		}
		switch (spi_bus_free(lcd_spibus->spihost)) {
			case ESP_ERR_INVALID_ARG:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
				return;

			case ESP_ERR_INVALID_STATE:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI bus already freed"));
				return;
		}
		int8_t pins[6] = {lcd_spibus->miso, lcd_spibus->mosi, lcd_spibus->clk, lcd_spibus->cs, lcd_spibus->dc, lcd_spibus->rst};

		for (int i = 0; i < 6; i++) {
			if (pins[i] != -1) {
				gpio_pad_select_gpio(pins[i]);
				gpio_matrix_out(pins[i], SIG_GPIO_OUT_IDX, false, false);
				gpio_set_direction(pins[i], GPIO_MODE_INPUT);
			}
		}
		is_init = 0;
		m_free(lcd_spibus);
	}
}
//-------------------------------------------------------------------------------------------
#endif
