/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	i2c.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/16
	* Description 			 :	
	******************************************************************************
**/
#ifndef MICROPY_INCLUDED_TKM32_I2C_H
#define MICROPY_INCLUDED_TKM32_I2C_H


typedef I2C_TypeDef i2c_t;

int i2c_init(i2c_t *i2c, mp_hal_pin_obj_t scl, mp_hal_pin_obj_t sda, uint32_t freq, uint16_t timeout);

int i2c_readfrom(i2c_t *i2c, uint16_t addr,uint8_t mem_addr ,uint8_t *dest, size_t len, bool stop);
int i2c_writeto(i2c_t *i2c, uint16_t addr,uint8_t mem_addr,  uint8_t *src, size_t len, bool stop);
void test_iic(void);


#endif // MICROPY_INCLUDED_TKM32_I2C_H
