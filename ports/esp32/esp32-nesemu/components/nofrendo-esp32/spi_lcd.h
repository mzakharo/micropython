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
#ifndef _SPI_LCD_H_
#define _SPI_LCD_H_
#include <stdint.h>

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#define  DEFAULT_WIDTH        240//256//
#define  DEFAULT_HEIGHT       240

#include "driver/spi_master.h"

typedef struct lcd_driver {
	size_t buffer_size;
	uint16_t *buffer;
	uint16_t *buffer_primary;
	uint16_t *buffer_secondary;
	uint16_t *current_buffer;

	uint8_t queue_fill;
	uint16_t display_width;
	uint16_t display_height;

	uint16_t data;
	uint16_t command;
} lcd_driver_t;


void lcd_write_frame(const uint8_t *data);
void nes_lcd_init(void);

// extern uint16_t lcd_width,lcd_height;
#ifdef __cplusplus
}
#endif

#endif //  __SPI_H__
