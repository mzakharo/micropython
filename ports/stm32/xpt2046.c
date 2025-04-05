/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	xpt2046.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/7/15
	* Description 			 :	
	******************************************************************************
**/

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/stream.h"
#include "math.h"

#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#if (MICROPY_HW_XPT2046 & MICROPY_ENABLE_TOUCH & MICROPY_ENABLE_SPILCD)
	
#include "lcd_spibus.h"

#include "modtftlcd.h"
#include "modtouch.h"
#include "ILI9341.h"

#include "global.h"

#define XPT2046_AVG 4

#define CMD_X_READ  0xD0
#define CMD_Y_READ  0x90

#define FLT_MIN 1.175494351e-38F 

static uint16_t touch_w,touch_h;

static bool is_init = false;

//-----------------------------------------------------------------
typedef struct _xpt2046_obj_t
{
	mp_obj_base_t base;
	const spi_t *spi;
	const pin_obj_t *cs;
	const pin_obj_t *irq;

	int16_t avg_buf_x[XPT2046_AVG];
	int16_t avg_buf_y[XPT2046_AVG];
	uint8_t avg_last;
} xpt2046_obj_t;

typedef struct _xpt_cail_t
{
	short xoff;
	short yoff;
	float xfac;
	float yfac;
} xpt_cail_t;

// Unfortunately, lvgl doesn't pass user_data to callbacks, so we use this global.
// This means we can have only one active touch driver instance, pointed by this global.
STATIC xpt2046_obj_t *g_xpt2046 = NULL;

STATIC xpt_cail_t *xpt_cail = NULL;

static float xfac=0.0, yfac=0.0;
static short xoff, yoff;

STATIC void xtp_delay(void) {
    __asm volatile ("nop\nnop");
}
static float InvSqrt(float x)
{
	float xhalf = 0.5f * x;
	int i = *(int*)&x;
	i = 0x5f3759df - (i>>1);
	x = *(float*)&i;
	x = x*(1.5f-xhalf*x*x);
	return x;
}

void xpt2046_init(xpt2046_obj_t *self)
{
	if(!is_init)
	{
	    // set the pins to default values
    mp_hal_pin_high(self->cs);
		
    // init the pins to be push/pull outputs
    mp_hal_pin_output(self->cs);
		
		mp_hal_pin_config(self->irq, MP_HAL_PIN_MODE_INPUT, GPIO_NOPULL, 0);

		is_init = true;
		
	}
}

void xpt_deinit_internal(void) {
	if(is_init){

		is_init = 0;
	}
}

static uint8_t tp_spi_xchg(xpt2046_obj_t *self, uint8_t data_send)
{
	uint8_t data_rec = 0;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(self->spi->spi, &data_send, &data_rec, 1, 5000);
	if(status != HAL_OK){
		printf("tp_spi_xchg err:%d\r\n",status);
	}
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
	
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    bool valid = true;
    uint8_t buf;

    uint8_t irq = mp_hal_pin_read(self->irq);

    if(irq == 0) {
        mp_hal_pin_write(self->cs, 0);
				xtp_delay();
        tp_spi_xchg(self, CMD_X_READ);         /*Start x read*/
				
        buf = tp_spi_xchg(self, 0);           /*Read x MSB*/
        *x = buf << 8;
        buf = tp_spi_xchg(self, CMD_Y_READ);  /*Until x LSB converted y command can be sent*/
        *x += buf;
        buf =  tp_spi_xchg(self, 0);   /*Read y MSB*/
        *y = buf << 8;

        buf =  tp_spi_xchg(self, 0);   /*Read y LSB*/
        *y += buf;
				xtp_delay();
        mp_hal_pin_write(self->cs, 1);
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
	//uint32_t d1,d2;
	double d1,d2;
	float fac = 0.0;
	
	temp1 = abs(a0 - a2);
	temp2 = abs(a1 - a3);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = InvSqrt(temp1+temp2);

	temp1 = abs(a4 - a6);
	temp2 = abs(a5 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = InvSqrt(temp1+temp2);
	
	fac = (float)(d1/d2);
//printf("fac1:%f\r\n",fac);
	if(fac<0.90||fac>1.1||d1==0||d2==0){
		return 1;
	}
	
	temp1 = abs(a0 - a6);
	temp2 = abs(a1 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = InvSqrt(temp1+temp2);

	temp1 = abs(a2 - a4);
	temp2 = abs(a3 - a5);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = InvSqrt(temp1+temp2);
	
	fac = (float)(d1/d2);
//printf("fac2:%f\r\n",fac);
	if(fac<0.90||fac>1.1){
		return 2;
	}

	temp1 = abs(a2 - a6);
	temp2 = abs(a3 - a7);
	temp1 *= temp1;
	temp2 *= temp2;
	d1 = InvSqrt(temp1+temp2);

	temp1 = abs(a0 - a4);
	temp2 = abs(a1 - a5);
	temp1 *= temp1;
	temp2 *= temp2;
	d2 = InvSqrt(temp1+temp2);
	
	fac = (float)(d1/d2);
//printf("fac3:%f\r\n",fac);
	if(fac<0.90||fac>1.1){
		return 3;
	}
	
	return 0;
}

static bool xpt_write_cail(const char *filename, short x_off, short y_off, float x_fac, float y_fac)
{
	UINT bw;
	ssize_t res = 0;
	mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);

	fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
	FIL	*f_file;
	f_file=(FIL *)m_malloc(sizeof(FIL));
	if(f_file == NULL){
		mp_raise_ValueError(MP_ERROR_TEXT("malloc f_file error"));
	}

	res = f_open(&vfs_fat->fatfs,f_file,filename,FA_WRITE|FA_CREATE_ALWAYS);
	if(res != FR_OK){
		mp_raise_ValueError(MP_ERROR_TEXT("path_buf open file error"));
	}

	xpt_cail->xoff = x_off;
	xpt_cail->yoff = y_off;
	xpt_cail->xfac = x_fac;
	xpt_cail->yfac = y_fac;

	uint8_t *write_buf = (uint8_t *)xpt_cail;

	res =f_write(f_file,(uint8_t *)write_buf, 12,&bw);
	f_close(f_file);
	m_free(f_file);

	if(bw != 12)return 1;
	else return 0;
}

static bool xpt_read_cail(const char *filename, short *x_off, short *y_off, float *x_fac, float *y_fac)
{
	UINT bw;
	ssize_t res = 0;
	
	mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);

	fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
	FIL	*f_file;
	f_file=(FIL *)m_malloc(sizeof(FIL));
	if(f_file == NULL){
		mp_raise_ValueError(MP_ERROR_TEXT("malloc f_file error"));
	}
	
	res = f_open(&vfs_fat->fatfs,f_file,filename,FA_READ);
	if(res != FR_OK){
		mp_raise_ValueError(MP_ERROR_TEXT("path_buf open file error"));
	}
	uint8_t read_buf[12] = {0};
	res =f_read(f_file,read_buf, 12,&bw);
	
	xpt_cail = (xpt_cail_t*)read_buf;

	*x_off = xpt_cail->xoff;
	*y_off = xpt_cail->yoff;
	*x_fac = xpt_cail->xfac;
	*y_fac = xpt_cail->yfac;

	m_free(f_file);
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
	
	grap_drawStr(&ili_glcd, 50, 150, 8*17, 18, 16,"Touch Calibration",RED, WHITE);

cail:
	p_cont = 0;
	grap_drawColorCircle(&ili_glcd, 15, 15, 5, RED); //15,15
	grap_drawLine(&ili_glcd,5,15,25,15,RED);
	grap_drawLine(&ili_glcd,15,5,15,25,RED);
	while(1)
	{
		mp_hal_delay_ms(100);
		if(xpt2046_read(&x, &y)){
			point_buf[p_cont][0] = x;
			point_buf[p_cont][1] = y;
			p_cont++;
			while(!mp_hal_pin_read(XPT_PIN_IRQ));
			switch(p_cont)
			{
				case 1:
					ili9341_Fill(0,0,35,30,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&ili_glcd, lcddev.width-15, 15, 5, RED); //lcddev.width-15, 15
					grap_drawLine(&ili_glcd,lcddev.width-5,15,lcddev.width-25,15,RED);
					grap_drawLine(&ili_glcd,lcddev.width-15,5,lcddev.width-15,25,RED);

				break;
				case 2:
					ili9341_Fill(lcddev.width-30,0,lcddev.width-1,30,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&ili_glcd, lcddev.width-15, lcddev.height-15, 5, RED);//lcddev.width-15, lcddev.height-15
					grap_drawLine(&ili_glcd,lcddev.width-5,lcddev.height-15,lcddev.width-25,lcddev.height-15,RED);
					grap_drawLine(&ili_glcd,lcddev.width-15,lcddev.height-5,lcddev.width-15,lcddev.height-25,RED);

				break;
				case 3:
					ili9341_Fill(lcddev.width-25, lcddev.height-30,lcddev.width-1,lcddev.height-1,WHITE);
					mp_hal_delay_ms(100);
					grap_drawColorCircle(&ili_glcd, 15, lcddev.height-15, 5, RED);//15, lcddev.height-15
					grap_drawLine(&ili_glcd,5,lcddev.height-15,25,lcddev.height-15,RED);
					grap_drawLine(&ili_glcd,15,lcddev.height-5,15,lcddev.height-25,RED);

				break;				
				case 4:
				
					ili9341_Fill(5, lcddev.height-30,30,lcddev.height-1,WHITE);

					if(check_valid(point_buf[0][0],point_buf[0][1],point_buf[1][0],point_buf[1][1],
													point_buf[2][0],point_buf[2][1],point_buf[3][0],point_buf[3][1]))
					{
						goto cail;
					}
					xfac=(float)(lcddev.width-30)/(point_buf[1][0] - point_buf[0][0]);
					xoff=(short)(lcddev.width-xfac*(point_buf[1][0]+point_buf[0][0]))/2;

					yfac=(float)(lcddev.height-30)/(point_buf[3][1] - point_buf[0][1]);
					yoff=(short)(lcddev.height-(yfac*(point_buf[3][1]+point_buf[0][1])))/2;

					if(abs((int)xfac)>2||abs((int)yfac)>2)
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

	self->spi = lcd_spibus->spi;
	
	self->cs 	= XPT_PIN_CS;
	self->irq = XPT_PIN_IRQ;
	
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


xpt_cail = (xpt_cail_t *)m_malloc(sizeof(xpt_cail_t));

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

