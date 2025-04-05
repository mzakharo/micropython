/*
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	machine_i2c.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/16
	* Description 			 :	
	******************************************************************************
**/
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "i2c.h"
#include "modmachine.h"

#if MICROPY_HW_ENABLE_HW_I2C

#define I2C_POLL_DEFAULT_TIMEOUT_US (50000) // 50ms

#define HW_I2C_MASTER (0)
#define HW_I2C_SLAVE  (1)

typedef struct _hard_i2c_obj_t {
    mp_obj_base_t base;
    i2c_t *i2c;
    mp_hal_pin_obj_t scl;
    mp_hal_pin_obj_t sda;
		uint32_t us_timeout;
} hard_i2c_obj_t;

typedef struct _mp_i2c_buf_t {
    size_t len;
    uint8_t *buf;
} mp_i2c_buf_t;
STATIC uint32_t i2c_freq[MICROPY_HW_MAX_I2C];

STATIC const hard_i2c_obj_t hard_i2c_obj[MICROPY_HW_MAX_I2C] = {
    #if defined(MICROPY_HW_I2C1_SCL)
    [0] = {{&hard_i2c_type}, I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA,100000},
    #endif
    #if defined(MICROPY_HW_I2C2_SCL)
    [1] = {{&hard_i2c_type}, I2C2, MICROPY_HW_I2C2_SCL, MICROPY_HW_I2C2_SDA,100000},
    #endif
    #if defined(MICROPY_HW_I2C3_SCL)
    [2] = {{&hard_i2c_type}, I2C3, MICROPY_HW_I2C3_SCL, MICROPY_HW_I2C3_SDA,100000},
    #endif
    #if defined(MICROPY_HW_I2C4_SCL)
    [3] = {{&hard_i2c_type}, I2C4, MICROPY_HW_I2C4_SCL, MICROPY_HW_I2C4_SDA,100000},
    #endif
};


STATIC void hard_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
	uint32_t i2c_id = self - &hard_i2c_obj[0] + 1;
	
	mp_printf(print, "I2C(%u, scl=%q, sda=%q, freq=%u, timeout=%u)",
			i2c_id,
			self->scl->name, self->sda->name, i2c_freq[i2c_id-1], self->us_timeout);
}

STATIC mp_obj_t hard_i2c_scan(mp_obj_t self_in) {
	hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
	mp_obj_t list = mp_obj_new_list(0, NULL);
	
	volatile uint16_t j=0;

	// 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
	for (uint16_t addr = 8; addr < 128; addr++) {
		self->i2c->IC_ENABLE &= ~0x1U;
		self->i2c->IC_TAR = addr;
		self->i2c->IC_ENABLE |= 0x1U;
		
		self->i2c->IC_DATA_CMD =(1<< (addr|0x100));
		for(j=0;j<1000;j++){
			if(!(self->i2c->IC_RAW_INTR_STAT & (1<<8))){
				j=3000;
				while(--j){}
			 if(!(self->i2c->IC_RAW_INTR_STAT & (1<<9))){
				 mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
				 if((self->i2c->IC_RAW_INTR_STAT & 0x100U)==0x100U){
					 self->i2c->IC_ENABLE &= ~(0x1U<<1);
					 self->i2c->IC_ENABLE |= (0x1U<<1);
				 }
				 break;
			 }
			 if((self->i2c->IC_RAW_INTR_STAT & 0x100U)==0x100U){
				 self->i2c->IC_ENABLE &= ~(0x1U<<1);
				 self->i2c->IC_ENABLE |= (0x1U<<1);
			 }
			 break;
		 }
		}
		 j=6000;
		while(--j){}
	}

	return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(hard_i2c_scan_obj, hard_i2c_scan);

STATIC void hard_i2c_init(hard_i2c_obj_t *self, uint32_t freq, uint32_t timeout_us) {
    uint32_t timeout_ms = (timeout_us + 999) / 1000;
    i2c_init(self->i2c, self->scl, self->sda, freq, timeout_ms);
}

mp_obj_t hard_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
   // MP_MACHINE_I2C_CHECK_FOR_LEGACY_SOFTI2C_CONSTRUCTION(n_args, n_kw, all_args);

    // parse args
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout, ARG_timingr };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = I2C_POLL_DEFAULT_TIMEOUT_US} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // work out i2c bus
    int i2c_id = 0;
    if (mp_obj_is_str(args[ARG_id].u_obj)) {
        const char *port = mp_obj_str_get_str(args[ARG_id].u_obj);
        if (0) {
        #ifdef MICROPY_HW_I2C1_NAME
        } else if (strcmp(port, MICROPY_HW_I2C1_NAME) == 0) {
            i2c_id = 1;
        #endif
        #ifdef MICROPY_HW_I2C2_NAME
        } else if (strcmp(port, MICROPY_HW_I2C2_NAME) == 0) {
            i2c_id = 2;
        #endif
        #ifdef MICROPY_HW_I2C3_NAME
        } else if (strcmp(port, MICROPY_HW_I2C3_NAME) == 0) {
            i2c_id = 3;
        #endif
        #ifdef MICROPY_HW_I2C4_NAME
        } else if (strcmp(port, MICROPY_HW_I2C4_NAME) == 0) {
            i2c_id = 4;
        #endif
        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%s) doesn't exist"), port);
        }
    } else {
        i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
        if (i2c_id < 1 || i2c_id > MP_ARRAY_SIZE(hard_i2c_obj)
            || hard_i2c_obj[i2c_id - 1].base.type == NULL) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
        }
    }

    // get static peripheral object
    hard_i2c_obj_t *self = (hard_i2c_obj_t *)&hard_i2c_obj[i2c_id - 1];

    // here we would check the scl/sda pins and configure them, but it's not implemented
    if (args[ARG_scl].u_obj != MP_OBJ_NULL || args[ARG_sda].u_obj != MP_OBJ_NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("explicit choice of scl/sda is not implemented"));
    }
		self->us_timeout = args[ARG_timeout].u_int;
    // initialise the I2C peripheral
    hard_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int);

		i2c_freq[i2c_id-1] = args[ARG_freq].u_int;

    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_arg_t i2c_mem_allowed_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_arg,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_addrsize, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

STATIC mp_obj_t i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum { ARG_addr, ARG_memaddr, ARG_n, ARG_addrsize };
	mp_arg_val_t args[MP_ARRAY_SIZE(i2c_mem_allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,MP_ARRAY_SIZE(i2c_mem_allowed_args), i2c_mem_allowed_args, args);

	// create the buffer to store data into
	vstr_t vstr;
	vstr_init_len(&vstr, mp_obj_get_int(args[ARG_n].u_obj));

	hard_i2c_obj_t *self = pos_args[0];

	// do the transfer	
	int ret = i2c_readfrom(self->i2c, args[ARG_addr].u_int,args[ARG_memaddr].u_int,(uint8_t *)vstr.buf, vstr.len, 0);
	
	if (ret < 0) {
			mp_raise_OSError(-ret);
	}

	return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(i2c_readfrom_mem_obj, 1, i2c_readfrom_mem);

STATIC mp_obj_t i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_addrsize };
	mp_arg_val_t args[MP_ARRAY_SIZE(i2c_mem_allowed_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
			MP_ARRAY_SIZE(i2c_mem_allowed_args), i2c_mem_allowed_args, args);

	// get the buffer to write the data from
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
	
	hard_i2c_obj_t *self = pos_args[0];

	// do the transfer
	int ret = i2c_writeto(self->i2c, args[ARG_addr].u_int,args[ARG_memaddr].u_int, bufinfo.buf, bufinfo.len,0);
	if (ret < 0) {
			mp_raise_OSError(-ret);
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(i2c_writeto_mem_obj, 1, i2c_writeto_mem);

STATIC const mp_rom_map_elem_t hard_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&hard_i2c_scan_obj) },
		
    // memory operations
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&i2c_readfrom_mem_obj) },
		{ MP_ROM_QSTR(MP_QSTR_writeto_mem), MP_ROM_PTR(&i2c_writeto_mem_obj) },
		
		{ MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(HW_I2C_MASTER) },
		{ MP_ROM_QSTR(MP_QSTR_SLAVE), MP_ROM_INT(HW_I2C_SLAVE) },
};
MP_DEFINE_CONST_DICT(hard_i2c_locals_dict, hard_i2c_locals_dict_table);

const mp_obj_type_t hard_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = hard_i2c_print,
    .make_new = hard_i2c_make_new,
    .locals_dict = (mp_obj_dict_t *)&hard_i2c_locals_dict,
};

#endif // MICROPY_HW_ENABLE_HW_I2C
