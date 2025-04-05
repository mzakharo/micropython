/*
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	spi.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/16
	* Description 			 :	
	******************************************************************************
**/
#ifndef MICROPY_INCLUDED_TKM32_SPI_H
#define MICROPY_INCLUDED_TKM32_SPI_H

#include "drivers/bus/spi.h"
#include "py/mphal.h"

typedef struct _spi_t {
    SPI_TypeDef *spi;
} spi_t;

typedef struct _spi_proto_cfg_t {
    const spi_t *spi;
    uint32_t baudrate;
    uint8_t polarity; //极性电平
    uint8_t phase;		//相位
    uint8_t bits;
    uint8_t firstbit;
} spi_proto_cfg_t;

typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    const spi_t *spi;
} pyb_spi_obj_t;

typedef struct _hard_spi_obj_t {
    mp_obj_base_t base;
    SPI_TypeDef *spi;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
		uint32_t us_timeout;
} hard_spi_obj_t;

typedef struct _machine_hard_spi_obj_t {
    mp_obj_base_t base;
    const spi_t *spi;
} machine_hard_spi_obj_t;

extern const spi_t spi_obj[4];

extern const mp_spi_proto_t spi_proto;
extern const mp_obj_type_t pyb_spi_type;
extern const mp_obj_type_t machine_hard_spi_type;

// A transfer of "len" bytes should take len*8*1000/baudrate milliseconds.
// To simplify the calculation we assume the baudrate is never less than 8kHz
// and use that value for the baudrate in the formula, plus a small constant.
#define SPI_TRANSFER_TIMEOUT(len) ((len) + 100)

void spi_init(SPI_TypeDef *spi, bool enable_nss_pin);
void spi_deinit(const spi_t *spi_obj);
int spi_find_index(mp_obj_t id);
void spi_set_params(SPI_TypeDef *spi, uint32_t prescale, int32_t baudrate,
int32_t polarity, int32_t phase, int32_t bits, int32_t firstbit);
void spi_transfer(const spi_t *self, size_t len, const uint8_t *src, uint8_t *dest, uint32_t timeout);
void spi_print(const mp_print_t *print, const spi_t *spi_obj, bool legacy);
const spi_t *spi_from_mp_obj(mp_obj_t o);

int spi_send(SPI_TypeDef *self ,size_t len,uint8_t *dat,uint32_t timeout);
int spi_send_read(SPI_TypeDef *self ,size_t len,uint8_t *src, uint8_t *dest,uint32_t timeout);

#endif // MICROPY_INCLUDED_TKM32_SPI_H
