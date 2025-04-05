
/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	usb_cd _port.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/4/1
	* Description 			 :	
	******************************************************************************
**/

#ifndef MICROPY_INCLUDED_TKM32_USB_CDC_PORT_H
#define MICROPY_INCLUDED_TKM32_USB_CDC_PORT_H

extern const mp_obj_type_t usb_vcp_type;

void usb_vcp_init0(void);
bool usb_cdc_connected(void);

#endif // MICROPY_INCLUDED_TKM32_USB_CDC_PORT_H
