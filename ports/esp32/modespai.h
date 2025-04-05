/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	modgamepad.h
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/


#ifndef MICROPY_INCLUDED_ESP32_MODESPAI_H
#define MICROPY_INCLUDED_ESP32_MODESPAI_H

#if MICROPY_HW_ESPAI

typedef struct {
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
	uint16_t leye_x;
	uint16_t leye_y;
	uint16_t reye_x;
	uint16_t reye_y;
	uint16_t nose_x;
	uint16_t nose_y;
	uint16_t lcorner_x;
	uint16_t lcorner_y;
	uint16_t rcorner_x;
	uint16_t rcorner_y;
}global_point_t;

typedef struct {
	uint16_t results;
	global_point_t piont[2];
} face_detection_t;

typedef struct {
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
}color_point_t;

typedef struct {
	uint16_t results;
	color_point_t piont[5];
} color_detection_t;

extern face_detection_t pFdetection;
extern color_detection_t pCdetection;

#endif

#endif // MICROPY_INCLUDED_ESP32_MODESPAI_H
