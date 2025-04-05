/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_color_detection.c
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

#if MICROPY_ENABLE_COLOR_DETECTION

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "global.h"

#include "mod_color_detection.h"
#include "who_color_detection.h"

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

//===================================================================================================================
typedef struct _color_detection_obj_t {
  mp_obj_base_t base;
}color_detection_obj_t;

typedef enum
{
	COLOR_RED = 0, 
	COLOR_ORANGE, 
	COLOR_YELLOW,
	COLOR_GREEN, 
	COLOR_CYAN, 
	COLOR_BLUE,
	COLOR_PURPLE, 
	COLOR_WHITE, 
	COLOR_GRAY,
	COLOR_END,
}color_index;

//---------------------------华丽的分割线-------------------------------------------------------------------
#define AI_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 4)
#define AI_TASK_STACK_SIZE      (16 * 1024)

static TaskHandle_t TaskFrame_handle = NULL;
static TaskHandle_t TaskLCD_handle = NULL;
static TaskHandle_t TaskFace_handle = NULL;

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueLCDFrame = NULL;
static QueueHandle_t xQueueEventLogic = NULL; //控制参数

static bool is_deint= 0;
STATIC camera_fb_t *get_frame = NULL;
static color_detection_state_t color_detection_state;
static uint8_t Identify = 0;
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
	if (xQueueEventLogic) {
        vQueueDelete(xQueueEventLogic);
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

static void init_color_detection(void)
{
	if(Identify >= COLOR_END){
		xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
		xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));
		xQueueEventLogic = xQueueCreate(1, sizeof(int));

		xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

		register_color_detection(xQueueAIFrame, xQueueEventLogic, NULL, xQueueLCDFrame, false);

		xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);
		
		vTaskDelay(100 / portTICK_RATE_MS);
		color_detection_state = OPEN_REGISTER_COLOR_BOX;
		xQueueSend(xQueueEventLogic, &color_detection_state, portMAX_DELAY);
	}
}
//Identify 识别
STATIC mp_obj_t color_detection_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
	if(Identify < COLOR_END || is_deint==1){
		xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
		xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));
		xQueueEventLogic = xQueueCreate(1, sizeof(int));

		xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

		register_color_detection(xQueueAIFrame, xQueueEventLogic, NULL, xQueueLCDFrame, false);

		xTaskCreatePinnedToCore(task_lcd_handler, "cam_lcd", 10 * 1024, NULL, 5, &TaskLCD_handle, 1);
		
		color_detection_state = CLOSE_REGISTER_COLOR_BOX;
	}else{
		color_detection_state = REGISTER_COLOR;
	}

	vTaskDelay(100 / portTICK_RATE_MS);
	xQueueSend(xQueueEventLogic, &color_detection_state, portMAX_DELAY);
	
	is_deint = 0;
	
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(color_detection_start_obj,0, color_detection_start);

//-----------------------------------------------------------------------------------------------
STATIC mp_obj_t color_detection_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	color_deinit_task();
	deinit_task();
	grap_drawFill(0,0,lcddev.width,lcddev.height,0x0000);
	vTaskDelay(100 / portTICK_RATE_MS);
	is_deint = 1;
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(color_detection_stop_obj,0, color_detection_stop);


STATIC mp_obj_t color_detection_read(mp_obj_t self_in)
{
	#if MICROPY_HW_ESPAI
	mp_obj_t tuple[21];
	uint16_t *pTuple = (uint16_t *)&pCdetection;
	for(uint16_t i=0; i < 21; i++){
		tuple[i] = mp_obj_new_int(pTuple[i]);
	}
	if(pCdetection.results){
		return mp_obj_new_tuple(pCdetection.results*4+1, tuple);
	}else{
		return mp_obj_new_tuple(1, tuple);
	}
	#endif
	
	return mp_const_true;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_detection_read_obj, color_detection_read);

//----------------------------------------------------------------------------------
STATIC mp_obj_t color_detection_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args){

	STATIC const mp_arg_t color_args[] = {
		{ MP_QSTR_color,       MP_ARG_INT, {.u_int = 0} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(color_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(color_args), color_args, args);

	Identify = args[0].u_int;
	
	setColorIndex(Identify);
	init_color_detection();

	color_detection_obj_t *color_detection_obj;
	color_detection_obj = m_new_obj(color_detection_obj_t);
	color_detection_obj->base.type = type;

	return MP_OBJ_FROM_PTR(color_detection_obj);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t color_detection_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_color_detection) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&color_detection_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&color_detection_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&color_detection_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(color_detection_locals_dict,color_detection_locals_dict_table);

const mp_obj_type_t color_detection_type = {
    { &mp_type_type },
    .name = MP_QSTR_color_detection,
    .make_new = color_detection_make_new,
    .locals_dict = (mp_obj_dict_t *)&color_detection_locals_dict,
};

#endif

