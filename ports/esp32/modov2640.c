
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modov2640.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/7/22
	* Description			:	
******************************************************************************/


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

#if MICROPY_HW_OV2640

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"

#include "esp_camera.h"
#include "cam_hal.h"
#include "sensor.h"
#include "img_converters.h"

#if MICROPY_ENABLE_STREAM
//http
#include "http_stream.h"
#endif

#include "global.h" 
#include "modov2640.h"

#if MICROPY_HW_LCD32
#include "ILI9341.h"
#endif

#if MICROPY_HW_LCD15
#include "ST7789.h"
#endif

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#include "lcd_spibus.h"
#endif

// MicroPython runs as a task under FreeRTOS
#define OV_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 3)
#define OV_TASK_STACK_SIZE      (16 * 1024)

//#define OV2640_DEBUG 1

uint8_t disp_dir = 0;
#ifdef OV2640_DEBUG
#define ov_printf(...) mp_printf(&mp_plat_print, __VA_ARGS__)
#else
#define ov_printf(...)
#endif

//===================================================================================================================
typedef struct _ov2640_obj_t {
  mp_obj_base_t base;
} ov2640_obj_t;

STATIC framesize_t framesize;
STATIC camera_fb_t *pic = NULL;
TaskHandle_t ov2640_handle = NULL;
//=====================================================================================================================

static bool is_display = false; //目前在显示图像
static bool is_snapshot = false;
#if MICROPY_ENABLE_TFTLCD
static bool is_init = 0;
#endif
static uint16_t fb_buf = 1;
//------------------------------------------------------------
static esp_err_t init_camera( pixformat_t pixel_format, framesize_t frame_size)
{
	camera_config_t camera_config = {
		#if CAM_PIN_RESET
		.pin_reset  = CAM_PIN_RESET,
		#else
		.pin_reset  = -1,
		#endif
		.pin_pwdn = -1,
		.pin_xclk = CAM_PIN_XCLK,
		.pin_sscb_sda = CAM_PIN_SIOD,
		.pin_sscb_scl = CAM_PIN_SIOC,

		.pin_d7 = CAM_PIN_D7,
		.pin_d6 = CAM_PIN_D6,
		.pin_d5 = CAM_PIN_D5,
		.pin_d4 = CAM_PIN_D4,
		.pin_d3 = CAM_PIN_D3,
		.pin_d2 = CAM_PIN_D2,
		.pin_d1 = CAM_PIN_D1,
		.pin_d0 = CAM_PIN_D0,
		.pin_vsync = CAM_PIN_VSYNC,
		.pin_href = CAM_PIN_HREF,
		.pin_pclk = CAM_PIN_PCLK,

		//XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
		.xclk_freq_hz = 16000000,
		.ledc_timer = LEDC_TIMER_0,
		.ledc_channel = LEDC_CHANNEL_0,
		.pixel_format = pixel_format, //YUV422,GRAYSCALE,RGB565,JPEG
		.frame_size = frame_size,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
		.jpeg_quality = 12, //0-63 12lower number means higher quality
		.fb_count = fb_buf,       //if more than one, i2s runs in continuous mode. Use only with JPEG
		.grab_mode = CAMERA_GRAB_WHEN_EMPTY
	};

	//initialize the camera
	esp_err_t ret = esp_camera_init(&camera_config);
	if (ret != ESP_OK) {
		esp_camera_deinit();
		mp_raise_ValueError(MP_ERROR_TEXT("camera init Failed"));
	}
	sensor_t *s = esp_camera_sensor_get();
	s->set_framesize(s, frame_size);
	s->set_hmirror(s, 1);

	return ret;
}

//=======================================================================================================
STATIC mp_obj_t sensor_ov2640_reset(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	// sensor_t * s = esp_camera_sensor_get();
	// if (!s) {
		// mp_raise_ValueError(MP_ERROR_TEXT("ov2640 reset Failed"));
	// }

	// s->reset(s); 

	return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_reset_obj, 0, sensor_ov2640_reset);
//=======================================================================================================
STATIC mp_obj_t sensor_ov2640_snapshot(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

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
			mp_hal_delay_ms(10);

			uint32_t jpg_buf_len = 200*1024;
			size_t outsize = 0;
			uint8_t *outbuffer = (uint8_t *)m_malloc(jpg_buf_len);
			if(outbuffer == NULL){
				mp_raise_ValueError(MP_ERROR_TEXT("malloc outbuffer error"));
			}
			
			for(uint16_t i=0;i<2;i++){
				pic = esp_camera_fb_get();
				if(i == 1){
					if(rgb565_2jpg(pic, 50, jpg_buf_len, outbuffer, &outsize))
					{
						ssize_t w_res = jpg_save(filename, outbuffer, outsize);
						if(w_res != outsize){
							printf("jpg save error:%d\r\n",w_res);
						}
					}
					m_free(outbuffer);
				}
				esp_camera_fb_return(pic);
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_snapshot_obj, 1, sensor_ov2640_snapshot);
//=====================================================================================================
STATIC mp_obj_t sensor_ov2640_setframesize(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	STATIC const mp_arg_t ov2640_args[] = {
    { MP_QSTR_framesize,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = FRAMESIZE_QQVGA} },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(ov2640_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(ov2640_args), ov2640_args, args);

	if(args[0].u_int >= FRAMESIZE_QQQVGA && args[0].u_int <= FRAMESIZE_XGA){
		framesize = args[0].u_int;
	}else{
		mp_raise_ValueError(MP_ERROR_TEXT("set framesize error"));
	}

	sensor_t * s = esp_camera_sensor_get();
	if (!s) {
		mp_raise_ValueError(MP_ERROR_TEXT("Framesize Failed"));
	}

	s->set_framesize(s, framesize);
	#if MICROPY_ENABLE_TFTLCD
	if(is_init){
		grap_drawFill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);
	}
	#endif
	return mp_obj_new_int(framesize);

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_setframesize_obj, 1, sensor_ov2640_setframesize);
//----------------------------------------------------------------------------------
STATIC mp_obj_t sensor_ov2640_hmirror(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t hmirror_args[] = {
			{ MP_QSTR_value,MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(hmirror_args)];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(hmirror_args), hmirror_args, args);

	uint8_t direction = args[0].u_int;
	sensor_t * s = esp_camera_sensor_get();
	if (!s) {
		mp_raise_ValueError(MP_ERROR_TEXT("set hmirror Failed"));
	}
	if(direction){
		s->set_hmirror(s, 0);
	}else{
		s->set_hmirror(s, 1);
	}
	return mp_obj_new_int(direction);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_hmirror_obj,0, sensor_ov2640_hmirror);

//----------------------------------------------------------------------------------------------------------
STATIC mp_obj_t sensor_ov2640_vflip(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t vfilp_args[] = {
		{ MP_QSTR_value,MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(vfilp_args)];
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(vfilp_args), vfilp_args, args);

	uint8_t direction = args[0].u_int;
	sensor_t * s = esp_camera_sensor_get();
	if (!s) {
			mp_raise_ValueError(MP_ERROR_TEXT("set vflip Failed"));
		}
	s->set_vflip(s, direction);

	return mp_obj_new_int(direction);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_vflip_obj,0, sensor_ov2640_vflip);
//----------------------------------------------------------------------------------
#if MICROPY_ENABLE_TFTLCD

static void display_task(void *pvParameter)
{
	static uint16_t x=0,y=0;

	while (is_display)
	{
		if(!is_snapshot){
			pic = esp_camera_fb_get();
			x = (lcddev.width - pic->width)>>1;
			y = (lcddev.height - pic->height)>>1;
			grap_drawCam(x,y,pic->width, pic->height,(uint16_t *)pic->buf);
			esp_camera_fb_return(pic);
		}
		else{
			vTaskDelay(1000 / portTICK_RATE_MS);
		}
	}
	vTaskDelete(NULL);
}
STATIC mp_obj_t sensor_ov2640_display(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	if(!is_init){
		lcddev.backcolor = 0x0000;
		
		grap_drawFill(0,0,lcddev.width,lcddev.height,lcddev.backcolor);
		lcddev.clercolor = lcddev.backcolor;
		is_init = 1;
	}
	is_display = true;
	xTaskCreate( display_task, "display_task", OV_TASK_STACK_SIZE / sizeof(StackType_t), NULL, OV_TASK_PRIORITY, &ov2640_handle );		
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_display_obj,0, sensor_ov2640_display);
//----------------------------------------------------------------------------------
STATIC mp_obj_t sensor_ov2640_display_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	is_display = false;
	// if( ov2640_handle != NULL )
	// {
	 // vTaskDelete( ov2640_handle );
	// }
	mp_hal_delay_ms(100);
	grap_drawFill(0,0,lcddev.width,lcddev.height,0x0000);
	return mp_obj_new_int(is_display);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_display_stop_obj,0, sensor_ov2640_display_stop);

#endif
//---------------------------------------------------------------------------------
#if MICROPY_ENABLE_STREAM

STATIC mp_obj_t sensor_ov2640_stream(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

	if(framesize >= FRAMESIZE_VGA){
		mp_raise_ValueError(MP_ERROR_TEXT("Camera framesize !> QVGA"));
	}
	init_httpd_app(80, 0);
	
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_stream_obj,0, sensor_ov2640_stream);
#endif
//----------------------------------------------------------------------------------
STATIC mp_obj_t sensor_ov2640_deinit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

 esp_err_t err = esp_camera_deinit();
	if (err != ESP_OK) {
		mp_raise_ValueError(MP_ERROR_TEXT("Camera deinit Failed"));
		return mp_const_false;
	}
	#if MICROPY_ENABLE_TFTLCD
	lcd_spibus_deinit();
	#endif
	is_display = false;
  return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(sensor_ov2640_deinit_obj,0, sensor_ov2640_deinit);

//----------------------------------------------------------------------------------

STATIC mp_obj_t sensor_ov2640_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_frame, MP_ARG_INT, {.u_int = 1} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	fb_buf = args[0].u_int;
	
	if(fb_buf >= 2) fb_buf = 2;
	else fb_buf = 1;

	// framesize = FRAMESIZE_XGA;
	framesize = FRAMESIZE_WVGA;
	init_camera(PIXFORMAT_RGB565, framesize);

	ov2640_obj_t *ov2640_obj;
	ov2640_obj = m_new_obj(ov2640_obj_t);
	ov2640_obj->base.type = &sensor_ov2640_type;
	return MP_OBJ_FROM_PTR(ov2640_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t sensor_ov2640_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_ov2640) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&sensor_ov2640_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&sensor_ov2640_reset_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_framesize), MP_ROM_PTR(&sensor_ov2640_setframesize_obj) },
	{ MP_ROM_QSTR(MP_QSTR_snapshot), MP_ROM_PTR(&sensor_ov2640_snapshot_obj) },
	#if MICROPY_ENABLE_STREAM
	{ MP_ROM_QSTR(MP_QSTR_stream), MP_ROM_PTR(&sensor_ov2640_stream_obj) },
	#endif
	{ MP_ROM_QSTR(MP_QSTR_set_hmirror), MP_ROM_PTR(&sensor_ov2640_hmirror_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_vflip), MP_ROM_PTR(&sensor_ov2640_vflip_obj) },
	#if MICROPY_ENABLE_TFTLCD
	{ MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&sensor_ov2640_display_obj) },
	{ MP_ROM_QSTR(MP_QSTR_display_stop), MP_ROM_PTR(&sensor_ov2640_display_stop_obj) },
	#endif
};
STATIC MP_DEFINE_CONST_DICT(sensor_ov2640_locals_dict,sensor_ov2640_locals_dict_table);
const mp_obj_type_t sensor_ov2640_type = {
    { &mp_type_type },
    .name = MP_QSTR_OV2640,
    .make_new = sensor_ov2640_make_new,
    .locals_dict = (mp_obj_dict_t *)&sensor_ov2640_locals_dict,
};

#endif

