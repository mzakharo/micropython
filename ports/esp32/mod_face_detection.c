/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_face_detection.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/2/22
	* Description				:	
********************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "modmachine.h"

#if MICROPY_ENABLE_FACE_DETECTION

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "global.h"

#include "mod_face_detection.h"

#include "who_human_face_detection.h"

#if MICROPY_ENABLE_CAT_DETECTION
#include "who_cat_face_detection.h"
#endif

#if MICROPY_ENABLE_MOTION_DETECTION
#include "who_motion_detection.h"
#endif

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

//===================================================================================================================
typedef struct _face_detection_obj_t {
  mp_obj_base_t base;
}face_detection_obj_t;

//---------------------------华丽的分割线-------------------------------------------------------------------
#define AI_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 4)
#define AI_TASK_STACK_SIZE      (16 * 1024)

static TaskHandle_t TaskFrame_handle = NULL;
static TaskHandle_t TaskLCD_handle = NULL;
static TaskHandle_t TaskFace_handle = NULL;

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueLCDFrame = NULL;

STATIC camera_fb_t *get_frame = NULL;

static void deinit_task(void)
{
	if(TaskFrame_handle){
		vTaskDelete(TaskFrame_handle);
	}
	if(TaskLCD_handle){
		vTaskDelete(TaskLCD_handle);
	}
	if(TaskFace_handle){
		vTaskDelete(TaskFace_handle);
	}
	
	if (xQueueAIFrame) {
        vQueueDelete(xQueueAIFrame);
    }
	if (xQueueLCDFrame) {
        vQueueDelete(xQueueLCDFrame);
    }
}
static void task_camfb_handler(void *arg)
{
    while (true)
    {
        get_frame = esp_camera_fb_get();
        if (get_frame){
			xQueueSend(xQueueAIFrame, &get_frame, portMAX_DELAY);
		}
    }
}
static void task_lcd_handler(void *arg)
{
    camera_fb_t *frame = NULL;
	static uint16_t x=0,y=0;
	
    while (true)
    {
        if (xQueueReceive(xQueueLCDFrame, &frame, portMAX_DELAY))
        {
			x = (lcddev.width - frame->width)>>1;
			y = (lcddev.height - frame->height)>>1;
			grap_drawCam(x,y,frame->width, frame->height,(uint16_t *)frame->buf);
			esp_camera_fb_return(frame);
        }

    }
}
STATIC mp_obj_t face_detection_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));

	xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

	register_human_face_detection(xQueueAIFrame, &TaskFace_handle, NULL, xQueueLCDFrame, false);
	
	xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_detection_start_obj,0, face_detection_start);


STATIC mp_obj_t face_detection_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	deinit_task();
	grap_drawFill(0,0,lcddev.width,lcddev.height,0x0000);
	vTaskDelay(1000 / portTICK_RATE_MS);
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_detection_stop_obj,0, face_detection_stop);

STATIC mp_obj_t face_detection_read(mp_obj_t self_in)
{
	#if MICROPY_HW_ESPAI
	mp_obj_t tuple[29];
	uint16_t *pTuple = (uint16_t *)&pFdetection;
	for(uint16_t i=0; i < 29; i++){
		tuple[i] = mp_obj_new_int(pTuple[i]);
	}
	if(pFdetection.results == 1){
		return mp_obj_new_tuple(15, tuple);
	}else if(pFdetection.results == 2){
		return mp_obj_new_tuple(29, tuple);
	}
	return mp_obj_new_tuple(1, tuple);
	#endif
	return mp_const_true;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(face_detection_read_obj, face_detection_read);

//----------------------------------------------------------------------------------
STATIC mp_obj_t face_detection_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);

	face_detection_obj_t *face_detection_obj;
	face_detection_obj = m_new_obj(face_detection_obj_t);
	face_detection_obj->base.type = type;

	return MP_OBJ_FROM_PTR(face_detection_obj);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t face_detection_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_face_detection) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&face_detection_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&face_detection_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&face_detection_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(face_detection_locals_dict,face_detection_locals_dict_table);

const mp_obj_type_t face_detection_type = {
    { &mp_type_type },
    .name = MP_QSTR_face_detection,
    .make_new = face_detection_make_new,
    .locals_dict = (mp_obj_dict_t *)&face_detection_locals_dict,
};
//-----------------------------------------------------------------------------------
#if MICROPY_ENABLE_CAT_DETECTION
STATIC mp_obj_t cat_detection_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));

	xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

	register_cat_face_detection(xQueueAIFrame, &TaskFace_handle, NULL, xQueueLCDFrame, false);
	
	xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(cat_detection_start_obj,0, cat_detection_start);
//---------------------------------------------------------------------------------------

STATIC mp_obj_t cat_detection_read(mp_obj_t self_in)
{
	#if MICROPY_HW_ESPAI
	mp_obj_t tuple[5];
	uint16_t *pTuple = (uint16_t *)&pFdetection;
	for(uint16_t i=0; i < 5; i++){
		tuple[i] = mp_obj_new_int(pTuple[i]);
	}
	if(pFdetection.results){
		return mp_obj_new_tuple(5, tuple);
	}
	return mp_obj_new_tuple(1, tuple);
	#endif
	return mp_const_true;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(cat_detection_read_obj, cat_detection_read);
//====================================================================================
STATIC const mp_rom_map_elem_t cat_detection_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_face_detection) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&face_detection_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&cat_detection_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&cat_detection_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(cat_detection_locals_dict,cat_detection_locals_dict_table);

const mp_obj_type_t cat_detection_type = {
    { &mp_type_type },
    .name = MP_QSTR_cat_detection,
    .make_new = face_detection_make_new,
    .locals_dict = (mp_obj_dict_t *)&cat_detection_locals_dict,
};
#endif
//====================================================================================
#if MICROPY_ENABLE_MOTION_DETECTION
static uint16_t nthreshold = 50;

STATIC mp_obj_t motion_detection_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
	enum { ARG_threshold };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_threshold, MP_ARG_INT, {.u_int = 50} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	nthreshold = args[ARG_threshold].u_int;

	face_detection_obj_t *face_detection_obj;
	face_detection_obj = m_new_obj(face_detection_obj_t);
	face_detection_obj->base.type = type;

	return MP_OBJ_FROM_PTR(face_detection_obj);
}

STATIC mp_obj_t motion_detection_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));

	xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

	register_motion_detection(xQueueAIFrame, &TaskFace_handle, nthreshold, NULL, xQueueLCDFrame);
	
	xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motion_detection_start_obj,0, motion_detection_start);
//---------------------------------------------------------------------------------------

STATIC mp_obj_t motion_detection_read(mp_obj_t self_in)
{
	return mp_obj_new_int(pFdetection.results);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(motion_detection_read_obj, motion_detection_read);
//====================================================================================
STATIC const mp_rom_map_elem_t motion_detection_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_face_detection) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&face_detection_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&motion_detection_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&motion_detection_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(motion_detection_locals_dict,motion_detection_locals_dict_table);

const mp_obj_type_t motion_detection_type = {
    { &mp_type_type },
    .name = MP_QSTR_motion_detection,
    .make_new = motion_detection_make_new,
    .locals_dict = (mp_obj_dict_t *)&motion_detection_locals_dict,
};
#endif

//-----------------------------------------------------------------------------------

#endif

