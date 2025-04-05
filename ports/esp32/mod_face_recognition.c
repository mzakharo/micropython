/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_face_recognition.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/3/10
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

#if MICROPY_ENABLE_FACE_RECOGNITION

#include "who_human_face_recognition.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "global.h"

#include "mod_face_recognition.h"

#include "modespai.h"

//===================================================================================================================
typedef struct _face_recognition_obj_t {
  mp_obj_base_t base;
}face_recognition_obj_t;

//---------------------------华丽的分割线-------------------------------------------------------------------
#define AI_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 4)
#define AI_TASK_STACK_SIZE      (16 * 1024)

static TaskHandle_t TaskFrame_handle = NULL;
static TaskHandle_t TaskLCD_handle = NULL;
static TaskHandle_t TaskFace_handle = NULL;

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueLCDFrame = NULL;
static QueueHandle_t xQueueEventLogic = NULL;

static QueueHandle_t xQueueSetMOde = NULL;
static QueueHandle_t xQueueDeteleID = NULL;

STATIC camera_fb_t *get_frame = NULL;

static recognizer_state_t recognizer_state;

static uint16_t setMode = 0;

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
	if (xQueueSetMOde) {
        vQueueDelete(xQueueSetMOde);
    }
	if (xQueueDeteleID) {
        vQueueDelete(xQueueDeteleID);
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


STATIC mp_obj_t face_recognition_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueEventLogic = xQueueCreate(1, sizeof(int *));
	xQueueSetMOde = xQueueCreate(1, sizeof(uint8_t *));
	xQueueDeteleID = xQueueCreate(1, sizeof(uint16_t *));
	
	xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

	register_human_face_recognition(xQueueAIFrame, xQueueEventLogic, xQueueSetMOde, xQueueLCDFrame, xQueueDeteleID);

	xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);

	vTaskDelay(100 / portTICK_RATE_MS);

	recognizer_state = DETECT;
	xQueueSend(xQueueEventLogic, &recognizer_state, portMAX_DELAY);

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_start_obj,0, face_recognition_start);

//正常
STATIC mp_obj_t face_recognition_detect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	recognizer_state = DETECT;
	xQueueSend(xQueueEventLogic, &recognizer_state, portMAX_DELAY);

	return mp_obj_new_int(recognizer_state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_detect_obj,0, face_recognition_detect);

//录入
STATIC mp_obj_t face_recognition_enroll(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	recognizer_state = ENROLL;
	xQueueSend(xQueueEventLogic, &recognizer_state, portMAX_DELAY);
		
	if(xQueueReceive(xQueueSetMOde, &setMode, 300)){
		return mp_obj_new_int(setMode);
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_enroll_obj,0, face_recognition_enroll);

//识别
STATIC mp_obj_t face_recognition_recognize(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	recognizer_state = RECOGNIZE;
	xQueueSend(xQueueEventLogic, &recognizer_state, portMAX_DELAY);
	
	if(xQueueReceive(xQueueSetMOde, &setMode, 300)){
		#if MICROPY_HW_ESPAI
		mp_obj_t tuple[15];
		uint16_t *pTuple = (uint16_t *)&pFdetection;
		for(uint16_t i=0; i < 15; i++){
			tuple[i] = mp_obj_new_int(pTuple[i]);
		}
		return mp_obj_new_tuple(15, tuple);
		#endif
	}
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_recognize_obj,0, face_recognition_recognize);

//删除
STATIC mp_obj_t face_recognition_delete(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	STATIC const mp_arg_t de_args[] = {
		{ MP_QSTR_id,       MP_ARG_INT, {.u_int = 0} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(de_args)];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(de_args), de_args, args);

	uint16_t deid = args[0].u_int;
	recognizer_state = DELETE;
	xQueueSend(xQueueEventLogic, &recognizer_state, portMAX_DELAY);
	
	xQueueSend(xQueueDeteleID, &deid, portMAX_DELAY);
	
	if(xQueueReceive(xQueueSetMOde, &setMode, 300)){
		#if MICROPY_HW_ESPAI
		return mp_obj_new_int(setMode);
		#endif
	}
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_delete_obj,0, face_recognition_delete);

STATIC mp_obj_t face_recognition_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	deinit_task();
	grap_drawFill(0,0,lcddev.width,lcddev.height,0x0000);
	vTaskDelay(1000 / portTICK_RATE_MS);
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(face_recognition_stop_obj,0, face_recognition_stop);


STATIC mp_obj_t face_recognition_read(mp_obj_t self_in)
{

	return mp_const_true;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(face_recognition_read_obj, face_recognition_read);

//----------------------------------------------------------------------------------
STATIC mp_obj_t face_recognition_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);

	face_recognition_obj_t *face_recognition_obj;
	face_recognition_obj = m_new_obj(face_recognition_obj_t);
	face_recognition_obj->base.type = type;

	return MP_OBJ_FROM_PTR(face_recognition_obj);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t face_recognition_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_face_recognition) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&face_recognition_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&face_recognition_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&face_recognition_read_obj) },
	
	{ MP_ROM_QSTR(MP_QSTR_detect), MP_ROM_PTR(&face_recognition_detect_obj) },
	{ MP_ROM_QSTR(MP_QSTR_enroll), MP_ROM_PTR(&face_recognition_enroll_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recognize), MP_ROM_PTR(&face_recognition_recognize_obj) },
	{ MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&face_recognition_delete_obj) },
	
};

STATIC MP_DEFINE_CONST_DICT(face_recognition_locals_dict,face_recognition_locals_dict_table);

const mp_obj_type_t face_recognition_type = {
    { &mp_type_type },
    .name = MP_QSTR_face_recognition,
    .make_new = face_recognition_make_new,
    .locals_dict = (mp_obj_dict_t *)&face_recognition_locals_dict,
};

//-----------------------------------------------------------------------------------

#endif

