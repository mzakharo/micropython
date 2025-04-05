#include "py/runtime.h"
#include "py/mphal.h"
#include "softtimer.h"
#include <stdint.h>
#include <stdbool.h>
#include "../../../lib/lv_bindings/lvgl/lvgl.h"
#include "lv_conf.h"
#include "common.h"

#include "lcd43g.h"
#include "tp_touch.h"
#include "ft54x6.h"
lv_color_t *fb[2] = {NULL, NULL};           // framebuffer pointers
uint32_t w = 0;                             // display width
uint32_t h = 0;                             // display height

STATIC mp_obj_t mp_lvlcd_framebuffer(mp_obj_t n_obj) {
	int n = mp_obj_get_int(n_obj) -1;

	if (n<0 || n>1){
		return mp_const_none;
	}

	if(fb[n]==NULL){
		// allocation on extRAM with 1KB alignment to speed up LTDC burst access on AHB
		
		fb[n] = MP_STATE_PORT(lcd_fb[n]) = m_malloc(sizeof(lv_color_t) * w * h  + 1024);
		fb[n] = (lv_color_t*)((uint32_t)fb[n] + 1024 - (uint32_t)fb[n] % 1024);
		//fb[n] = MP_STATE_PORT(lcd_fb[n]) = m_malloc(sizeof(lv_color_t) * w * h);
		//fb[n] = (lv_color_t*)((uint32_t)fb[n]);
	}

	return mp_obj_new_bytearray_by_ref(sizeof(lv_color_t) * w * h , (void *)fb[n]);
}

STATIC mp_obj_t mp_lvlcd_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_w, ARG_h };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_w, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 800} },
        { MP_QSTR_h, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 480} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    w = args[ARG_w].u_int;
    h = args[ARG_h].u_int;

    mp_lvlcd_framebuffer(mp_obj_new_int(1));

    if (fb[0] == NULL) {
        mp_obj_new_exception_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed allocating frame buffer"));
    }

    // if (BSP_TS_Init(w, h) != TS_OK) {
        // mp_obj_new_exception_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Touchscreen init error"));
    // }

    return mp_const_none;
}

STATIC mp_obj_t mp_lvlcd_deinit() {

    if(fb[0]!=NULL){
    	m_free(MP_STATE_PORT(lcd_fb[0]));
    	fb[0]=NULL;
    }

    if(fb[1]!=NULL){
    	m_free(MP_STATE_PORT(lcd_fb[1]));
    	fb[1]=NULL;
    }

    return mp_const_none;
}

STATIC void mp_lvlcd_flush(struct _disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {

	int32_t x;
	int32_t y;

	for(y = area->y1; y <= area->y2 && y < disp_drv->ver_res; y++){
		for(x = area->x1; x <= area->x2; x++){
			LCD_Fast_DrawPoint(x,y,lv_color_to32(*color_p));
			//LTDC_Buf[lcddev.x_pixel*y+x] = lv_color_to32(*color_p);
			color_p++;
		}
	}
	lv_disp_flush_ready(disp_drv);
}

STATIC bool mp_lv_touch_read(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    static lv_coord_t lastX = 0;
    static lv_coord_t lastY = 0;

		// if(tp_dev.type == 1){
			// touch_read_point();
		// }else if(tp_dev.type == 2){
			// gt911_read_point();
		// }
		if(tp_dev.sta&TP_PRES_DOWN || tp_dev.sta&TP_PRES_MOVE)
		{
			lastX = tp_dev.x[0];
			lastY = tp_dev.y[0];
			data->state = LV_INDEV_STATE_PR;
			// printf("lastX:%d,lastY:%d\r\n",lastX,lastY);
		}else{
			data->state = LV_INDEV_STATE_REL;
		}
		data->point.x = lastX;
		data->point.y = lastY;
    return false;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_lvlcd_init_obj, 0, mp_lvlcd_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_lvlcd_deinit_obj, mp_lvlcd_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_lvlcd_framebuffer_obj, mp_lvlcd_framebuffer);
DEFINE_PTR_OBJ(mp_lvlcd_flush);
DEFINE_PTR_OBJ(mp_lv_touch_read);

STATIC const mp_rom_map_elem_t lvlcd_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_rk043fn48h) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mp_lvlcd_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&mp_lvlcd_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&PTR_OBJ(mp_lvlcd_flush))},
    { MP_ROM_QSTR(MP_QSTR_ts_read), MP_ROM_PTR(&PTR_OBJ(mp_lv_touch_read))},
    { MP_ROM_QSTR(MP_QSTR_framebuffer), MP_ROM_PTR(&PTR_OBJ(mp_lvlcd_framebuffer))},
};

STATIC MP_DEFINE_CONST_DICT(
    mp_module_lvlcd_globals,
    lvlcd_globals_table
    );

const mp_obj_module_t mp_module_lvlcd = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_lvlcd_globals
};

MP_REGISTER_MODULE(MP_QSTR_lvlcd, mp_module_lvlcd, 1);



