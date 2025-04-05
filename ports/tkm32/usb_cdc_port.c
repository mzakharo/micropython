/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	usb_cd _port.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/4/1
	* Description 			 :	
	******************************************************************************
**/

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "bufhelper.h"
#include "storage.h"

#include "usb.h"
#include "usb_cdc_port.h"

#include "led.h"
/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct {
    mp_obj_base_t base;
} usb_vcp_obj_t;

int usb_cdc_sta = 0;
/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC usb_vcp_obj_t usb_vcp_obj = { {&usb_vcp_type} };

void usb_vcp_init0(void) {
		usb_fifo_init();
    MP_STATE_VM(dupterm_objs[1]) = MP_OBJ_FROM_PTR(&usb_vcp_obj);
		usb_cdc_sta = 1;
}
/******************************************************************************/
// MicroPython bindings for USB VCP (virtual comm port)

STATIC void usb_vcp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_print_str(print, "USB_VCP()");
}
bool usb_cdc_connected(void)
{
	return usb_cdc_sta;
}

STATIC mp_obj_t usb_vcp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &usb_vcp_obj;
}


STATIC mp_obj_t usb_vcp_isconnected(mp_obj_t self_in)
{
    return mp_obj_new_bool(usb_cdc_sta);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(usb_vcp_isconnected_obj, usb_vcp_isconnected);


STATIC mp_obj_t usb_vcp_any(mp_obj_t self_in)
{
    if(usb_cdc_rx_is_empty())
    {
      return mp_const_true;
    }
    else
    {
      return mp_const_false;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(usb_vcp_any_obj, usb_vcp_any);

STATIC mp_obj_t pyb_usb_vcp_setinterrupt(mp_obj_t self_in, mp_obj_t int_chr_in) {
    mp_hal_set_interrupt_char(mp_obj_get_int(int_chr_in));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_usb_vcp_setinterrupt_obj, pyb_usb_vcp_setinterrupt);

STATIC const mp_rom_map_elem_t usb_vcp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_isconnected),  MP_ROM_PTR(&usb_vcp_isconnected_obj) },
		{ MP_ROM_QSTR(MP_QSTR_setinterrupt), MP_ROM_PTR(&pyb_usb_vcp_setinterrupt_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),          MP_ROM_PTR(&usb_vcp_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),     MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline),     MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_write),        MP_ROM_PTR(&mp_stream_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(usb_vcp_locals_dict, usb_vcp_locals_dict_table);


STATIC uint usb_vcp_read(mp_obj_t self_in, void *buf, uint size, int *errcode)
{
		int ret = usb_vcom_recv((byte*)buf, size);
    if(ret == 0)
    {
			// return EAGAIN error to indicate non-blocking
			*errcode = MP_EAGAIN;
			return MP_STREAM_ERROR;
    }
    return ret;
}

STATIC uint usb_vcp_write(mp_obj_t self_in, const void *buf, uint size, int *errcode)
{
		int ret = UsbVcomSend((uint8_t *)buf, size);
    if(!ret)
    {
        // return EAGAIN error to indicate non-blocking
      *errcode = MP_EAGAIN;
      return MP_STREAM_ERROR;
    }
    return ret;
}

STATIC uint usb_vcp_ioctl(mp_obj_t self_in, uint request, uint arg, int *errcode)
{
    uint ret = 0;
    if(request == MP_STREAM_POLL)
    {
        uint flags = arg;
        if((flags & MP_STREAM_POLL_RD) && usb_cdc_rx_is_empty())
        {
          ret |= MP_STREAM_POLL_RD;
        }
        if((flags & MP_STREAM_POLL_WR) && usb_cdc_tx_is_empty())
        {
          ret |= MP_STREAM_POLL_WR;
        }

    }
    else
    {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }

    return ret;
}


STATIC const mp_stream_p_t usb_vcp_stream_p = {
    .read  = usb_vcp_read,
    .write = usb_vcp_write,
    .ioctl = usb_vcp_ioctl,
};


const mp_obj_type_t usb_vcp_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_VCP,
    .print = usb_vcp_print,
    .make_new = usb_vcp_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &usb_vcp_stream_p,
    .locals_dict = (mp_obj_dict_t*)&usb_vcp_locals_dict,
};
