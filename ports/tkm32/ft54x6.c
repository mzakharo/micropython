
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	FT54x6.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
#include "py/runtime.h"
#include "py/mphal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "py/obj.h"

#include "pin.h"
#include "pin_static_af.h"
#include "systick.h"
#include "extint.h"
#include "py/stackctrl.h"
#include "i2c.h"
#include "timer.h"

#include "HAL_conf.h"

#if (MICROPY_HW_FT54X6 && MICROPY_HW_LCD43G)

#include "ft54x6.h"
#include "tp_touch.h"
#include "lcd43g.h"

//I2C读写命令	
#define FT_ADDR			0X38
#define FT_PIN_REST 	pin_B3
#define FT_PIN_INT	 	pin_D7

#define FT_I2C			I2C1

static bool is_init = 0;

//----------------------------------------------------------------------
STATIC void ft_Init(void)
{
	i2c_init(FT_I2C, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, 500);
	
	mp_hal_pin_config(FT_PIN_REST, MP_HAL_PIN_MODE_OUTPUT,MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(FT_PIN_INT, MP_HAL_PIN_MODE_IPD_UP,MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_high(FT_PIN_REST);
	mp_hal_delay_ms(100) ;//10ms

	uint8_t temp[5];
	i2c_readfrom(FT_I2C, FT_ADDR,0xA8, temp, 1, 1);
	tp_dev.id = temp[0];
	
	//printf("tp_dev.id:%x\r\n",tp_dev.id);

}
//===============================================================================

static uint16_t touch_w,touch_h;
void touch_read_point(void)
{
	uint8_t touch_num = 0;
	uint8_t read_buf[7] = {0};
	uint16_t input_x = 0;
	uint16_t input_y = 0;
	int16_t input_w = 0;
	int8_t read_id = 0;
	static uint8_t pre_touch = 0;

	i2c_readfrom(FT_I2C, FT_ADDR,1, read_buf, 6, 1);

	touch_num = read_buf[1] % 6; 

	if (pre_touch > touch_num){
		gtxx_touch_up(read_id);
	}
	if (touch_num == 0){
		pre_touch = touch_num;
		return;
	}

	if (touch_num){	
		switch (tp_dev.dir)
		{
			case 2:
			input_y = (uint16_t)(TOUCH_Y_PIXEL-( ((read_buf[4] & 0x0F)<<8) + read_buf[5]));
			input_x = (uint16_t)(TOUCH_X_PIXEL-( ((read_buf[2] & 0x0F)<<8) + read_buf[3]));
			break;
			case 3:
			input_x = (uint16_t)(TOUCH_Y_PIXEL-( ((read_buf[4] & 0x0F)<<8) + read_buf[5]));
			input_y = (uint16_t)( ((read_buf[2] & 0x0F)<<8) + read_buf[3]);
			break;
			case 4:
			input_y = (uint16_t)( ((read_buf[4] & 0x0F)<<8) + read_buf[5]);
			input_x = (uint16_t)( ((read_buf[2] & 0x0F)<<8) + read_buf[3]);
			break;
			default:
			input_x = (uint16_t)( ((read_buf[4] & 0x0F)<<8) + read_buf[5]);
			input_y = (uint16_t)(TOUCH_X_PIXEL - (((read_buf[2] & 0x0F)<<8) + read_buf[3]));
			break;
		}

		//printf("lcddev.dir:%d,x:%d,,y:%d\r\n",lcddev.dir,input_x,input_y);
		if(input_x >= touch_w || input_y >= touch_h){
			return;
		}
		gtxx_touch_down(read_id, input_x, input_y, input_w);
	}else if (pre_touch){
		gtxx_touch_up(read_id);
	}
	pre_touch = touch_num;
}
/**********************************************************************************************************/
void TIM8_Config(uint32_t freq)
{
	RCC->APB1RSTR |= (1 << 5);
	RCC->APB1RSTR &= ~(1 << 5);
	RCC->APB1ENR |= (1 << 5);

	uint32_t source_freq = timer_get_source_freq(8);
	uint32_t prescaler = 1;
	uint32_t period;

	period = source_freq / freq;//hz
	period = MAX(1, period);
	while (period > 0xffffffff) {
		// if we can divide exactly, do that first
		if (period % 5 == 0) {
			prescaler *= 5;
			period /= 5;
		} else if (period % 3 == 0) {
			prescaler *= 3;
			period /= 3;
		} else {
		// may not divide exactly, but loses minimal precision
			prescaler <<= 1;
			period >>= 1;
		}
	}

	TIM8->PSC = (prescaler - 1) & 0xffff;
	TIM8->ARR = (period - 1) & 0xffffffff;

	TIM8->DIER &= ~(TIM_IT_Update);
	TIM8->SR &= ~(TIM_IT_Update);
	TIM8->DIER |= (TIM_IT_Update);
	TIM8->CR1 |= TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM8_IRQn);

	TIM8->CR1 |= (0x1U<<7U);
}

__weak void TIM8_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM8, TIM_IT_Update) != RESET) {
		uint32_t irq_state = disable_irq();
		touch_read_point();
		enable_irq(irq_state);
	}
	TIM8->SR = ~TIM_IT_Update;
}
//----------------------------------------------------------------------------------------------------------
typedef struct _touch_ft_obj_t {
    mp_obj_base_t base;
    uint8_t buf[10];
} touch_ft_obj_t;

STATIC touch_ft_obj_t ft54x6_obj;

//==========================================================================================
STATIC mp_obj_t touch_Ft54xx_read(void)
{
	mp_obj_t tuple[3];
	touch_read_point();
	if (tp_dev.sta&TP_PRES_DOWN) tuple[0] = mp_obj_new_int(0);
	else if(tp_dev.sta&TP_PRES_MOVE)	tuple[0] = mp_obj_new_int(1); 
	else 	tuple[0] = mp_obj_new_int(2); 
		
	tuple[1] = mp_obj_new_int(tp_dev.x[0]);
	tuple[2] = mp_obj_new_int(tp_dev.y[0]);
	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_Ft54xx_read_obj, touch_Ft54xx_read);
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_ft54xx_scan(void)
{
	touch_read_point();
	return mp_const_none;
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_ft54xx_scan_obj, touch_ft54xx_scan);
//-----------------------------------------------------------------------------------------------
STATIC void touch_ft54x6_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print,"TP ID:%s\r\n",tp_dev.id);
	mp_printf(print, "TOUCH(portrait=%d)\n",tp_dev.dir);
}
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_tf54x6_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if(args[0].u_int != 0)
	{
		tp_dev.dir = args[0].u_int;
	}
	
	switch (tp_dev.dir)
	{
		case 2:
		touch_w=480;
		touch_h=800;
		break;
		case 3:
		touch_w=800;
		touch_h=480;
		break;
		case 4:
		touch_w=480;
		touch_h=800;
		break;
		case 1:
		touch_w=800;
		touch_h=480;
		break;
	}

	ft_Init();
	//TIM8_Config(30);
	is_init = 1;
	tp_dev.type = 1;
	ft54x6_obj.base.type = &touch_ft_type;
	return MP_OBJ_FROM_PTR(&touch_ft_type);
}

//===================================================================
STATIC const mp_rom_map_elem_t touch_tf54x6_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&touch_Ft54xx_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tick_inc), MP_ROM_PTR(&touch_ft54xx_scan_obj) },
};

STATIC MP_DEFINE_CONST_DICT(touch_tf54x6_locals_dict, touch_tf54x6_locals_dict_table);

const mp_obj_type_t touch_ft_type = {
    { &mp_type_type },
    .name = MP_QSTR_FT5436,
    .print = touch_ft54x6_print,
    .make_new = touch_tf54x6_make_new,
    .locals_dict = (mp_obj_dict_t *)&touch_tf54x6_locals_dict,
};
#endif

/**********************************************************************************************************/
