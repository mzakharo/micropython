
/********************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modov2640.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/22
	* Description			:	
******************************************************************************/

#ifndef MICROPY_INCLUDED_ESP32_MODOV2640_H
#define MICROPY_INCLUDED_ESP32_MODOV2640_H

#if MICROPY_HW_OV2640


//===========================================================================================
extern const mp_obj_type_t sensor_ov2640_type;

extern esp_err_t camera_init();
#endif

#endif // MICROPY_INCLUDED_ESP32_MODOV2640_H
