/********************************************************************************
	* This file is part of the MicroPython project
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name					:	mod_code_recognition.c
	* Author					:	Folktale
	* Version					:	v1.0
	* date						:	2022/3/09
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

#if MICROPY_ENABLE_CODE_RECOGNITION
#include "esp_log.h"
#include "esp_system.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "global.h"

#include "mod_code_recognition.h"

#include "esp_code_scanner.h"

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

//===================================================================================================================
typedef struct _code_recognition_obj_t {
  mp_obj_base_t base;
}code_recognition_obj_t;

//---------------------------华丽的分割线-------------------------------------------------------------------
#define AI_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 4)
#define AI_TASK_STACK_SIZE      (16 * 1024)

static TaskHandle_t TaskFrame_handle = NULL;
static uint8_t is_code = 0;

static char read_buf[200];

esp_code_scanner_symbol_t result;

static void deinit_task(void)
{
	if(TaskFrame_handle){
		vTaskDelete(TaskFrame_handle);
	}
}
void display_box(uint16_t *buf,uint16_t lcd_width,uint16_t color)
{
	for(uint16_t i=0; i<25; i++){
		for(uint16_t j=0; j<25; j++){
			buf[i*lcd_width+j] = color;
		}
	}
}
static void task_camfb_handler(void *arg)
{
	camera_fb_t *frame = NULL;
	static uint16_t x=0,y=0;
    while (1)
    {
        frame = esp_camera_fb_get();
        if (frame == NULL){
			continue;
		}
		
		// Decode Progress
		esp_image_scanner_t *esp_scn = esp_code_scanner_create();
		esp_code_scanner_config_t config = {ESP_CODE_SCANNER_MODE_FAST, ESP_CODE_SCANNER_IMAGE_RGB565, frame->width, frame->height};
		esp_code_scanner_set_config(esp_scn, config);
		int decoded_num = esp_code_scanner_scan_image(esp_scn, frame->buf);
		
        if(decoded_num){
			is_code = 1;
            result = esp_code_scanner_result(esp_scn);
			memset(read_buf, '\0', 200);
			strncpy(read_buf,result.data , result.datalen);
			display_box((uint16_t *)frame->buf,frame->width,0x001F);
        }else{
			is_code = 0;
		}
        esp_code_scanner_destroy(esp_scn);
		
		x = (lcddev.width - frame->width)>>1;
		y = (lcddev.height - frame->height)>>1;

		grap_drawCam(x,y,frame->width, frame->height,(uint16_t *)frame->buf);

		esp_camera_fb_return(frame);
		
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

STATIC mp_obj_t code_recognition_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
	xTaskCreate( task_camfb_handler, "cam_fb", AI_TASK_STACK_SIZE / sizeof(StackType_t), NULL, AI_TASK_PRIORITY+1, &TaskFrame_handle );	

	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(code_recognition_start_obj,0, code_recognition_start);


STATIC mp_obj_t code_recognition_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	deinit_task();
	vTaskDelay(100 / portTICK_RATE_MS);
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(code_recognition_stop_obj,0, code_recognition_stop);

STATIC mp_obj_t code_recognition_read(mp_obj_t self_in)
{
	#if MICROPY_HW_ESPAI
	if(is_code){
		return mp_obj_new_str(read_buf, strlen(read_buf));
	}
	#endif
	
	return mp_const_none;
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(code_recognition_read_obj, code_recognition_read);

//----------------------------------------------------------------------------------
STATIC mp_obj_t code_recognition_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);

	code_recognition_obj_t *code_recognition_obj;
	code_recognition_obj = m_new_obj(code_recognition_obj_t);
	code_recognition_obj->base.type = type;

	return MP_OBJ_FROM_PTR(code_recognition_obj);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t code_recognition_locals_dict_table[] = {
	
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_senser_code_recognition) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&code_recognition_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&code_recognition_start_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&code_recognition_read_obj) },
};

STATIC MP_DEFINE_CONST_DICT(code_recognition_locals_dict,code_recognition_locals_dict_table);

const mp_obj_type_t code_recognition_type = {
    { &mp_type_type },
    .name = MP_QSTR_code_recognition,
    .make_new = code_recognition_make_new,
    .locals_dict = (mp_obj_dict_t *)&code_recognition_locals_dict,
};

//-----------------------------------------------------------------------------------

#endif

