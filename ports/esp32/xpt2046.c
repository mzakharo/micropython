
/********************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	xpt2046.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/15
	* Description			:	
******************************************************************************/

#include "mpconfigboard.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>

#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#if (MICROPY_HW_XPT2046 & MICROPY_ENABLE_TOUCH)
	
#include "lcd_spibus.h"

#include "modtftlcd.h"
#include "modtouch.h"
#include "ILI9341.h"

#include "global.h"

#define XPT2046_AVG 4
//#define CMD_X_READ  0x90
//#define CMD_Y_READ  0xD0

#define CMD_X_READ  0xD0
#define CMD_Y_READ  0x90
static uint16_t touch_w,touch_h;

static bool is_init = false;
typedef struct _mp_file_t {
    mp_obj_base_t base;
    int fd;
} mp_file_t;
//-----------------------------------------------------------------
typedef struct _xpt2046_obj_t
{
    mp_obj_base_t base;

    uint8_t mhz;
    uint8_t spihost;
    uint8_t cs;
    uint8_t irq;
    uint8_t miso;
    uint8_t mosi;
    uint8_t clk;

    int16_t x_min;
    int16_t y_min;
    int16_t x_max;
    int16_t y_max;
    bool x_inv;
    bool y_inv;    
    bool xy_swap;

    spi_device_handle_t spi;
    int16_t avg_buf_x[XPT2046_AVG];
    int16_t avg_buf_y[XPT2046_AVG];
    uint8_t avg_last;

} xpt2046_obj_t;

// Unfortunately, lvgl doesn't pass user_data to callbacks, so we use this global.
// This means we can have only one active touch driver instance, pointed by this global.
STATIC xpt2046_obj_t *g_xpt2046 = NULL;

static float xfac=0.0, yfac=0.0;
static short xoff, yoff;

void xpt2046_init(xpt2046_obj_t *self)
{
    esp_err_t ret;
		
  spi_device_interface_config_t devcfg={
		.clock_speed_hz=self->mhz*1000*1000, //Clock out at DISP_SPI_MHZ MHz
		.mode=0,                             //SPI mode 0
		.spics_io_num=-1,              //CS pin is set manually
		.queue_size=1,
		.pre_cb=NULL,
		.post_cb=NULL,
	};
	
	gpio_set_direction(self->irq, GPIO_MODE_INPUT);
	gpio_set_direction(self->cs, GPIO_MODE_OUTPUT);
	gpio_set_level(self->cs, 1);

	//Attach the touch controller to the SPI bus
	ret=spi_bus_add_device(self->spihost, &devcfg, &self->spi);
	if (ret != ESP_OK){
		mp_raise_ValueError(MP_ERROR_TEXT("xpt2046 Failed adding SPI device"));
	}
	is_init = true;
}

void xpt_deinit_internal(void) {
	if(is_init){
		xpt2046_obj_t *self = g_xpt2046;
		
		switch (spi_bus_remove_device(self->spi)) {
			case ESP_ERR_INVALID_ARG:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
				return;

			case ESP_ERR_INVALID_STATE:
				mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI device already freed"));
				return;
		}
		
		int8_t pins[2] = {self->irq, self->cs};
		for (int i = 0; i < 2; i++) {
			if (pins[i] != -1) {
				gpio_pad_select_gpio(pins[i]);
				gpio_matrix_out(pins[i], SIG_GPIO_OUT_IDX, false, false);
				gpio_set_direction(pins[i], GPIO_MODE_INPUT);
			}
		}
		is_init = 0;
	}
}

static uint8_t tp_spi_xchg(xpt2046_obj_t *self, uint8_t data_send)
{
	uint8_t data_rec = 0;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));  //Zero out the transaction
	t.length = 8;              //Length is in bytes, transaction length is in bits.
	t.tx_buffer = &data_send;  //Data
	t.rx_buffer = &data_rec;

	spi_device_queue_trans(self->spi, &t, portMAX_DELAY);

	spi_transaction_t * rt;
	
	spi_device_get_trans_result(self->spi, &rt, portMAX_DELAY);

	return data_rec;
}

static void xpt2046_avg(xpt2046_obj_t *self, int16_t * x, int16_t * y)
{
    /*Shift out the oldest data*/
    uint8_t i;
    for(i = XPT2046_AVG - 1; i > 0 ; i--) {
        self->avg_buf_x[i] = self->avg_buf_x[i - 1];
        self->avg_buf_y[i] = self->avg_buf_y[i - 1];
    }

    /*Insert the new point*/
    self->avg_buf_x[0] = *x;
    self->avg_buf_y[0] = *y;
    if(self->avg_last < XPT2046_AVG) self->avg_last++;

    /*Sum the x and y coordinates*/
    int32_t x_sum = 0;
    int32_t y_sum = 0;
    for(i = 0; i < self->avg_last ; i++) {
        x_sum += self->avg_buf_x[i];
        y_sum += self->avg_buf_y[i];
    }

    /*Normalize the sums*/
    (*x) = (int32_t)x_sum / self->avg_last;
    (*y) = (int32_t)y_sum / self->avg_last;
}
static bool xpt2046_read(int16_t *x, int16_t *y)
{
    xpt2046_obj_t *self = g_xpt2046;

    if (!self || (!self->spi))
		{
			nlr_raise(mp_obj_new_exception_msg(
                &mp_type_RuntimeError, 
								MP_ERROR_TEXT("xpt2046 instance needs to be created before callback is called!")));
		}			
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    bool valid = true;
    uint8_t buf;

    uint8_t irq = gpio_get_level(self->irq);

    if(irq == 0) {
        gpio_set_level(self->cs, 0);
        tp_spi_xchg(self, CMD_X_READ);         /*Start x read*/
				
        buf = tp_spi_xchg(self, 0);           /*Read x MSB*/
        *x = buf << 8;
        buf = tp_spi_xchg(self, CMD_Y_READ);  /*Until x LSB converted y command can be sent*/
        *x += buf;
        buf =  tp_spi_xchg(self, 0);   /*Read y MSB*/
        *y = buf << 8;

        buf =  tp_spi_xchg(self, 0);   /*Read y LSB*/
        *y += buf;
        gpio_set_level(self->cs, 1);
        /*Normalize Data*/
        *x = *x >> 3;
        *y = *y >> 3;
        //xpt2046_corr(self, x, y);
        xpt2046_avg(self, x, y);
        last_x = *x;
        last_y = *y;
    } else {
        *x = last_x;
        *y = last_y;
        self->avg_last = 0;
        valid = false;
    }

    return valid;
}
static bool check_valid(int16_t a0, int16_t a1, int16_t a2, int16_t a3,
												int16_t a4, int16_t a5, int16_t a6, int16_t a7)
{
	
	uint32_t temp1,temp2;
	uint32_t d1,d2;
	float fac = 0.0;
	
	temp1 = abs(a0 - a2);
	temp2 = abs(a1 - a3);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = sqrt(temp1+temp2);

	temp1 = abs(a4 - a6);
	temp2 = abs(a5 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = sqrt(temp1+temp2);
	
	fac = (float)d1/d2;
//printf("fac1:%f\r\n",fac);
	if(fac<0.90||fac>1.1||d1==0||d2==0){
		return 1;
	}
	
	temp1 = abs(a0 - a6);
	temp2 = abs(a1 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = sqrt(temp1+temp2);

	temp1 = abs(a2 - a4);
	temp2 = abs(a3 - a5);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = sqrt(temp1+temp2);
	
	fac = (float)d1/d2;
//printf("fac2:%f\r\n",fac);
	if(fac<0.90||fac>1.1){
		return 2;
	}

	temp1 = abs(a2 - a6);
	temp2 = abs(a3 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = sqrt(temp1+temp2);

	temp1 = abs(a0 - a4);
	temp2 = abs(a1 - a5);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = sqrt(temp1+temp2);
	
	fac = (float)d1/d2;
//printf("fac3:%f\r\n",fac);
	if(fac<0.90||fac>1.1){
		return 3;
	}
	
	return 0;
}

static bool xpt_write_cail(const char *filename, short x_off, short y_off, float x_fac, float y_fac)
{
	mp_obj_t f_new;
	ssize_t res = 0;
	
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_wb),
	};
	f_new = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
	
	res = mp_stream_posix_write(f_new, &x_off, 2);
	res += mp_stream_posix_write(f_new, &y_off, 2);
	res += mp_stream_posix_write(f_new, &x_fac, 4);
	res += mp_stream_posix_write(f_new, &y_fac, 4);
	mp_stream_close(f_new);

	if(res != 12)return 1;
	else return 0;
}

static bool xpt_read_cail(const char *filename, short *x_off, short *y_off, float *x_fac, float *y_fac)
{
	mp_obj_t f_new;
	ssize_t res = 0;

	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_rb),
	};
	
	f_new = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

	res = mp_stream_posix_read(f_new, x_off, 2);
	res += mp_stream_posix_read(f_new, y_off, 2);
	res += mp_stream_posix_read(f_new, x_fac, 4);
	res += mp_stream_posix_read(f_new, y_fac, 4);
	mp_stream_close(f_new);

	if(res != 12)return 1;
	else return 0;
}

static bool xpt2046_adjust(const char *filename)
{
	uint8_t temp_dir = lcddev.dir;//dir
	static uint8_t p_cont;
	int16_t x, y;
	int16_t point_buf[4][2];
	ili9341_set_dir(1);
	
	ili9341_Fill(0,0,lcddev.width,lcddev.height,WHITE);
	grap_drawStr(&g_lcd, 50, 150, 8*17, 18, 16,"Touch Calibration",RED, WHITE);

cail:
	p_cont = 0;
	grap_drawColorCircle(&g_lcd, 15, 15, 5, RED); //15,15
	grap_drawLine(&g_lcd,5,15,25,15,RED);
	grap_drawLine(&g_lcd,15,5,15,25,RED);
	while(1)
	{
		mp_hal_delay_ms(100);
		if(xpt2046_read(&x, &y)){
			point_buf[p_cont][0] = x;
			point_buf[p_cont][1] = y;
			p_cont++;
			while(!gpio_get_level(XPT_PIN_IRQ));
			switch(p_cont)
			{
				case 1:
					ili9341_Fill(0,0,35,30,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&g_lcd, lcddev.width-15, 15, 5, RED); //lcddev.width-15, 15
					grap_drawLine(&g_lcd,lcddev.width-5,15,lcddev.width-25,15,RED);
					grap_drawLine(&g_lcd,lcddev.width-15,5,lcddev.width-15,25,RED);

				break;
				case 2:
					ili9341_Fill(lcddev.width-30,0,lcddev.width-1,30,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&g_lcd, lcddev.width-15, lcddev.height-15, 5, RED);//lcddev.width-15, lcddev.height-15
					grap_drawLine(&g_lcd,lcddev.width-5,lcddev.height-15,lcddev.width-25,lcddev.height-15,RED);
					grap_drawLine(&g_lcd,lcddev.width-15,lcddev.height-5,lcddev.width-15,lcddev.height-25,RED);

				break;
				case 3:
					ili9341_Fill(lcddev.width-25, lcddev.height-30,lcddev.width-1,lcddev.height-1,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&g_lcd, 15, lcddev.height-15, 5, RED);//15, lcddev.height-15
					grap_drawLine(&g_lcd,5,lcddev.height-15,25,lcddev.height-15,RED);
					grap_drawLine(&g_lcd,15,lcddev.height-5,15,lcddev.height-25,RED);

				break;				
				case 4:
				
					ili9341_Fill(5, lcddev.height-30,30,lcddev.height-1,WHITE);

					if(check_valid(point_buf[0][0],point_buf[0][1],point_buf[1][0],point_buf[1][1],
													point_buf[2][0],point_buf[2][1],point_buf[3][0],point_buf[3][1]))
					{
						goto cail;
					}
					xfac=(float)(lcddev.width-30)/(point_buf[1][0] - point_buf[0][0]);
					xoff=(lcddev.width-xfac*(point_buf[1][0]+point_buf[0][0]))/2;

					yfac=(float)(lcddev.height-30)/(point_buf[3][1] - point_buf[0][1]);
					yoff=(lcddev.height-yfac*(point_buf[3][1]+point_buf[0][1]))/2;

					if(abs(xfac)>2||abs(yfac)>2)
					{
						goto cail;
					}
				break;
			}	
		}
		if(p_cont == 4){
			break;
		}
	}
	ili9341_set_dir(temp_dir);

	ili9341_Fill(0,0,lcddev.width,lcddev.height,lcddev.clercolor);

	return xpt_write_cail(filename, xoff, yoff, xfac, yfac);
}

void xpt_read_point(void)
{
	int16_t x, y;
	uint8_t touch_num = 0;
	static uint8_t pre_touch = 0;
	int8_t read_id = 0;
	uint16_t input_x = 0;
	uint16_t input_y = 0;

	touch_num = xpt2046_read(&x, &y);
	if (pre_touch > touch_num){
	 tp_touch_up(read_id);
	}
	if (touch_num == 0){
		pre_touch = touch_num;
		return;
	}
	if (touch_num)
	{
		switch (tp_dev.dir)
		{
			case 2:
				input_y = touch_h - ((uint16_t)(x*xfac + xoff));
				input_x = ((uint16_t)(y*yfac + yoff));
			break;
			case 3:
				input_x = touch_w - ((uint16_t)(x*xfac + xoff));
				input_y = touch_h - ((uint16_t)(y*yfac + yoff));
			break;
			case 4:
				input_y = ((uint16_t)(x*xfac + xoff));
				input_x = touch_w - ((uint16_t)(y*yfac + yoff));
			break;
			default:
				input_x = (uint16_t)(x*xfac + xoff);
				input_y = (uint16_t)(y*yfac + yoff);
			break;
		}
		if(input_x >= touch_w || input_y >= touch_h){
			input_x = touch_w;
			input_y = touch_h;
			//return;
		}
		tp_touch_down(read_id, input_x, input_y, 1);
	}else if(pre_touch){
    tp_touch_up(read_id);
  }
  pre_touch = touch_num;

}

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t touch_xpt2046_read(mp_obj_t self_in)
{
	mp_obj_t tuple[3];
	xpt_read_point();
	if (tp_dev.sta&TP_PRES_DOWN) tuple[0] = mp_obj_new_int(0);
	else if(tp_dev.sta&TP_PRES_MOVE)	tuple[0] = mp_obj_new_int(1); 
	else 	tuple[0] = mp_obj_new_int(2); 
		
	tuple[1] = mp_obj_new_int(tp_dev.x[0]);
	tuple[2] = mp_obj_new_int(tp_dev.y[0]);
	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(touch_xpt2046_read_obj, touch_xpt2046_read);

STATIC mp_obj_t touch_xpt2046_inc(mp_obj_t self_in)
{
	xpt_read_point();
	return mp_const_none;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(touch_xpt2046_inc_obj, touch_xpt2046_inc);

STATIC mp_obj_t xpt2046_deinit(mp_obj_t self_in) {
	xpt_deinit_internal();
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xpt2046_deinit_obj, xpt2046_deinit);

STATIC mp_obj_t xpt2046_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	xpt2046_obj_t *self = m_new_obj(xpt2046_obj_t);
	self->base.type = type;
	
	self->spi = NULL;
	self->mhz = 5;
	self->spihost = lcd_spibus->spihost;
	self->cs 	= XPT_PIN_CS;
	self->irq = XPT_PIN_IRQ;
	
	self->miso 		= LCD_PIN_MISO;
	self->mosi 		= LCD_PIN_MOSI;
	self->clk  		= LCD_PIN_CLK;

	self->x_min = 1000;
	self->y_min = 1000;
	self->x_max = 3200;
	self->y_max = 2000;
	self->x_inv = 0;
	self->y_inv = 0;
	self->xy_swap = 0;
	
	g_xpt2046 = self;
	xpt2046_init(self);

	if(args[0].u_int != 0)
	{
		tp_dev.dir = args[0].u_int;
	}
	
	switch (tp_dev.dir)
	{
		case 2:
		case 4:
		touch_w=320;
		touch_h=240;
		break;
		case 1:
		case 3:
		touch_w=240;
		touch_h=320;
		break;
	}
	tp_dev.type = 3;
	if(check_sys_file("touch_cail")){
		xpt_read_cail("touch_cail", &xoff, &yoff, &xfac, &yfac);
	}else{
		if(xpt2046_adjust("touch_cail")){
			
			mp_raise_ValueError(MP_ERROR_TEXT("xpt2046 adjust error"));
		}
	}

	return MP_OBJ_FROM_PTR(self);
}
//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC const mp_rom_map_elem_t xpt2046_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_xpt2046) },
	{ MP_ROM_QSTR(MP_QSTR_deint), MP_ROM_PTR(&xpt2046_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&touch_xpt2046_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tick_inc), MP_ROM_PTR(&touch_xpt2046_inc_obj) },
};
STATIC MP_DEFINE_CONST_DICT(xpt2046_locals_dict, xpt2046_locals_dict_table);
//---------------------------华丽的分割线-------------------------------------------------------------------
const mp_obj_type_t touch_xpt2046_type = {
    { &mp_type_type },
    .name = MP_QSTR_XPT2046,
    .make_new = xpt2046_make_new,
    .locals_dict = (mp_obj_dict_t*)&xpt2046_locals_dict,
};

#endif

