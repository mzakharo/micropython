/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	modusbcam.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/8/05
	* Description 			 :	
	******************************************************************************
**/

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

#if MICROPY_HW_USB_CAM

#include "jpegd2.h"

#if MICROPY_ENABLE_STREAM
#include "http_stream.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "modusbcam.h" 
#include "usb_uvc_port.h"
#include "uvc_stream.h"
#include "global.h" 

#if MICROPY_HW_LCD32
#include "ILI9341.h"
#endif

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif


// MicroPython runs as a task under FreeRTOS
#define UVC_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 4)
#define UVC_TASK_STACK_SIZE      (16 * 1024)

//===================================================================================================================
typedef struct _usbcam_obj_t {
  mp_obj_base_t base;
}usbcam_obj_t;

typedef enum {
	FRAMESIZE_160X120,
	FRAMESIZE_320X240,
	FRAMESIZE_640X480,
} uvc_framesize_t;
//=====================================================================================================================

static bool is_display = false; //目前在显示图像
static bool is_snapshot = false;

static bool is_init = 0;

static uvc_framesize_t uvc_framesize;

static TaskHandle_t uvc_display_handle = NULL;

uvcam_fb_t *pic = NULL;
uint16_t *outbuf = NULL;

//=======================================================================================================
STATIC mp_obj_t usbcam_snapshot(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t snapshot_args[] = {
    { MP_QSTR_filepath,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(snapshot_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(snapshot_args), snapshot_args, args);

    //MP_OBJ_NULL
  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("snapshot text parameter error"));
    } else {
			mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);
			char *filename = bufinfo.buf;
			
			is_snapshot = true;
			vTaskDelay(200 / portTICK_RATE_MS);

			for(uint16_t i=0;i<2;i++){
				pic = uvc_camera_fb_get();
				if(i == 1){
					jpg_save(filename, pic->buf, pic->len);
				}
				if (pic) {
					uvc_camera_fb_return(pic);
					pic = NULL;
				}
				vTaskDelay(10 / portTICK_RATE_MS);
			}
			
			is_snapshot = false;

   		return mp_obj_new_str(filename, strlen(filename));
    }
  }
	else
  {
     mp_raise_ValueError(MP_ERROR_TEXT("snapshot text parameter is empty"));
  }

  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(usbcam_snapshot_obj, 1, usbcam_snapshot);
//=====================================================================================================
#if MICROPY_ENABLE_TFTLCD
static void display_task(void *pvParameter)
{
	static uint16_t x=0,y=0;
	// uint32_t lcd_size = lcddev.width*lcddev.height;

	while (is_display)
	{
		if(!is_snapshot&&is_display){
			pic = uvc_camera_fb_get();
			if (!pic) {
				mp_raise_ValueError(MP_ERROR_TEXT("uvc Camera display failed"));
			}
			x = (lcddev.width - pic->width)>>1;
			y = (lcddev.height - pic->height)>>1;

			mjpegdraw(pic->buf, pic->len,outbuf);

			uint32_t i;
			uint8_t * color_u8 = (uint8_t *) outbuf;
			uint8_t color_tmp;

			for(i = 0; i < pic->width*pic->height * 2; i += 2) {
				color_tmp = color_u8[i + 1];
				color_u8[i + 1] = color_u8[i];
				color_u8[i] = color_tmp;
			}
			// ili9341_cam_full(x,y,pic->width,pic->height,outbuf);
			grap_drawCam(x,y,pic->width,pic->height,outbuf);
			if (pic) {
				uvc_camera_fb_return(pic);
				pic = NULL;
			}

		}else{
			vTaskDelay(1000 / portTICK_RATE_MS);
		}
	}
	vTaskDelete(NULL);
}

STATIC mp_obj_t usbcam_display(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
	if(uvc_framesize > FRAMESIZE_320X240){
		mp_raise_ValueError(MP_ERROR_TEXT("UVC Camera Display not supported"));
	}
	outbuf = m_malloc(lcddev.width * lcddev.height + 2);
	if(outbuf == NULL){
		mp_raise_ValueError(MP_ERROR_TEXT("uvc outbuf display malloc failed"));
	}
	
	is_display = true;
	if(xTaskCreate( display_task, "display_task", UVC_TASK_STACK_SIZE / sizeof(StackType_t), 
				NULL, UVC_TASK_PRIORITY, &uvc_display_handle ) != pdPASS)		{
		mp_raise_ValueError(MP_ERROR_TEXT("display xTaskCreate failed"));
	}
	
	
	is_init = 1;
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(usbcam_display_obj,0, usbcam_display);
//----------------------------------------------------------------------------------
STATIC mp_obj_t usbcam_display_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
	is_display = false;
	vTaskDelay(200 / portTICK_RATE_MS);

	// if( uvc_display_handle != NULL )
	// {
	 // vTaskDelete( uvc_display_handle );
	// }

	if(outbuf){
		m_free(outbuf);
	}
	grap_drawFill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);
	return mp_obj_new_int(is_display);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(usbcam_display_stop_obj,0, usbcam_display_stop);
#endif

//---------------------------------------------------------------------------------
#if MICROPY_ENABLE_STREAM
STATIC mp_obj_t usbcam_stream(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	init_httpd_app(80, 1);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(usbcam_stream_obj,0, usbcam_stream);
#endif
//----------------------------------------------------------------------------------
STATIC mp_obj_t usbcam_deinit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	
	is_display = false;

	vTaskDelay(2000 / portTICK_RATE_MS);

	esp_err_t err = uvc_streaming_stop();

	if (err != ESP_OK) {
		mp_raise_ValueError(MP_ERROR_TEXT("UVC Camera deinit Failed"));
	}

	// if( uvc_display_handle != NULL ){
	 // vTaskDelete( uvc_display_handle );
	// }

	if(outbuf){
		m_free(outbuf);
	}
  return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(usbcam_deinit_obj,0, usbcam_deinit);

//----------------------------------------------------------------------------------

STATIC mp_obj_t usbcam_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_framesize, MP_ARG_INT, {.u_int = FRAMESIZE_320X240} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uvc_framesize = args[0].u_int;
#if MICROPY_ENABLE_TFTLCD
	// mp_init_ILI9341();
	// ili9341_set_dir(4);
	// lcddev.backcolor = 0x0000;
	// ili9341_Fill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);
	// lcddev.clercolor = lcddev.backcolor;
#endif
	esp_err_t err = 0;

	switch(uvc_framesize)
	{
		case FRAMESIZE_160X120:
			err = uvc_app_init(160, 120, 2, 5);
		break;
		case FRAMESIZE_320X240:
			err = uvc_app_init(320, 240, 2, 3);
		break;
		case FRAMESIZE_640X480:
			err = uvc_app_init(640, 480, 2, 1);
		break;
		default :
			mp_raise_ValueError(MP_ERROR_TEXT("set UVC Camera size Failed"));
		break;
	}

	if (err != ESP_OK) {
		 mp_raise_ValueError(MP_ERROR_TEXT("UVC Camera Init Failed"));
	}

  usbcam_obj_t *usbcam_obj;
  usbcam_obj = m_new_obj(usbcam_obj_t);
  usbcam_obj->base.type = &usbcam_type;

	return MP_OBJ_FROM_PTR(usbcam_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t usbcam_locals_dict_table[] = {
	
	{ MP_OBJ_NEW_QSTR(MP_QSTR_QQVGA), MP_OBJ_NEW_SMALL_INT(FRAMESIZE_160X120)},     /* 160x120   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_QVGA),  MP_OBJ_NEW_SMALL_INT(FRAMESIZE_320X240)},     /* 320x240   */
	{ MP_OBJ_NEW_QSTR(MP_QSTR_VGA),   MP_OBJ_NEW_SMALL_INT(FRAMESIZE_640X480)},     /* 640x480 */

	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_usbcam) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&usbcam_deinit_obj) },

	{ MP_ROM_QSTR(MP_QSTR_snapshot), MP_ROM_PTR(&usbcam_snapshot_obj) },
	#if MICROPY_ENABLE_STREAM
	{ MP_ROM_QSTR(MP_QSTR_stream), MP_ROM_PTR(&usbcam_stream_obj) },
	#endif
	#if MICROPY_ENABLE_TFTLCD
	{ MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&usbcam_display_obj) },
	{ MP_ROM_QSTR(MP_QSTR_display_stop), MP_ROM_PTR(&usbcam_display_stop_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(usbcam_locals_dict,usbcam_locals_dict_table);
const mp_obj_type_t usbcam_type = {
    { &mp_type_type },
    .name = MP_QSTR_CAM,
    .make_new = usbcam_make_new,
    .locals_dict = (mp_obj_dict_t *)&usbcam_locals_dict,
};

#endif

