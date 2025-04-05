/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_face_detection.h
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/


#ifndef MICROPY_INCLUDED_ESP32_FACE_DETECTION_H
#define MICROPY_INCLUDED_ESP32_FACE_DETECTION_H

#if MICROPY_ENABLE_FACE_DETECTION
extern const mp_obj_type_t face_detection_type;
#endif

#if MICROPY_ENABLE_CAT_DETECTION
extern const mp_obj_type_t cat_detection_type;
#endif

#if MICROPY_ENABLE_MOTION_DETECTION
extern const mp_obj_type_t motion_detection_type;
#endif

#endif // MICROPY_INCLUDED_ESP32_FACE_DETECTION_H
