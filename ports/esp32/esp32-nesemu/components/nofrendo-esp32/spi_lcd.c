// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"


#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/ets_sys.h"
#include "esp32/rom/gpio.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/ets_sys.h"
#include "esp32s2/rom/gpio.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/ets_sys.h"
#include "esp32s3/rom/gpio.h"
#endif

#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "mpconfigboard.h"

#include "spi_lcd.h"

#if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD)
#include "modtftlcd.h"
#include "ST7789.h"
#endif

#include "esp_log.h"

static const char *TAG = "spi_lcd";

#define LINE_BUFFERS (2)
#define LINE_COUNT   (20)

extern uint16_t myPalette[];

lcd_driver_t display = {
		.display_width = 240,
		.display_height = 240,
		.buffer_size = 20 * 240, // 2 buffers with 20 lines
	};

void nes_buf_init(lcd_driver_t *driver)
{
    //Allocate the buffer memory

	driver->buffer = (uint16_t *)heap_caps_malloc(driver->buffer_size * 2 * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(driver->buffer == NULL){
        ESP_LOGE(TAG, "Display buffer allocation fail");
        return ;
    }
    driver->buffer_primary =  driver->buffer;
    driver->buffer_secondary = driver->buffer + driver->buffer_size;
    driver->current_buffer = driver->buffer_primary;
}

static uint8_t getPixelNES(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2){

    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;
    int x_ratio = (int)(((DEFAULT_WIDTH - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((DEFAULT_HEIGHT - 1) << 16) / h2) + 1;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    index = yv * DEFAULT_WIDTH + xv;

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + DEFAULT_WIDTH];
    d = bufs[index + DEFAULT_WIDTH + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}

void lcd_write_frame(const uint8_t *data)
{

    uint16_t calc_line = 0;
    // uint16_t sending_line = 0;
	int x;
	x = (display.display_width-DEFAULT_WIDTH)/2;

    if(data == NULL){
        for(uint16_t y = 0; y < DEFAULT_HEIGHT; y++){

            for(uint16_t x = 0; x < DEFAULT_WIDTH; x++){
                display.current_buffer[x] = 0x1F;
            }
            
            // sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
			#if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD)
			st7789_Full(x,y,DEFAULT_WIDTH,1,display.current_buffer);
			#endif
        }
    }
    else{
        short outputHeight = DEFAULT_HEIGHT;//240;
        short outputWidth = DEFAULT_WIDTH;//240 + (240 - 240);
        // short xpos = (240 - outputWidth) / 2;

        for (int y = 0; y < outputHeight; y += LINE_COUNT){
            for (int i = 0; i < LINE_COUNT; ++i){
                if ((y + i) >= outputHeight)
                break;

                int index = (i)*outputWidth;

                for (int x = 0; x < outputWidth; x++){ 
                    display.current_buffer[index++]= myPalette[getPixelNES(data, x, (y + i), outputWidth, outputHeight)];
                }
            }

            // sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
			#if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD)
			st7789_Full(x,y,DEFAULT_WIDTH,20,display.current_buffer);
			#endif
        }
    }

}

void nes_lcd_init(void)
{
	nes_buf_init(&display);
}

