/**
	******************************************************************************
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	lcd43g.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/

#include "py/runtime.h"
#include "py/mphal.h"
#include "bufhelper.h"
#include <math.h>
#include "py/builtin.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "py/obj.h"
#include "pin.h"
#include "pin_static_af.h"
#include "mpu.h"
#include "systick.h"

#if (MICROPY_HW_LTDC_LCD && (MICROPY_HW_LCD7R||MICROPY_HW_LCD43R))
	
#include "modtftlcd.h"
#include "ltdc.h"

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

#include "global.h" 

typedef struct _tftlcd_lcd7r_obj_t {
    mp_obj_base_t base;
} tftlcd_lcd7r_obj_t;

STATIC tftlcd_lcd7r_obj_t lcd7r_obj;
//==============================================================================
#if MICROPY_HW_LCD7R
void lcd7r_init(void)
{
	STATIC bool init_flag = false;
	lcddev.type = 3;
	if(init_flag) return;
	
	mp_hal_pin_config(MICROPY_HW_LTDC_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);

	ltdcdev.pwidth=800;
	ltdcdev.pheight=480;
	ltdcdev.hsw=1;
	ltdcdev.hbp=46;
	ltdcdev.hfp=210;
	
	ltdcdev.vsw=1;
	ltdcdev.vbp=23;	
	ltdcdev.vfp=22;
	
	ltdcdev.layer = 0;
	
	ltdcdev.pixsize=2;
	ltdcdev.ltdc_format = LTDC_PIXEL_FORMAT_RGB565;
	
	lcddev.x_pixel = ltdcdev.pwidth;
	lcddev.y_pixel = ltdcdev.pheight;
	
	#if defined(STM32F4) || defined(STM32F7)
	ltdc_set_clk(6);
	#elif defined(STM32H7)
	ltdc_set_clk(20);
	#endif

	ltdc_init();
	
	ltdc_conf();

	lcddev.backcolor = BLACK;

	mp_hal_pin_high(MICROPY_HW_LTDC_BL);

	init_flag = true;
	lcddev.backcolor = BLACK;
}

void lcd7r_full_cam(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{
	if(x >= lcddev.width || y >= lcddev.height) return;  
#if defined(STM32F4) || defined(STM32F7)
	for(uint32_t i=0;i<height;i++){
		for(uint32_t j=0;j<width;j++){
			ltdc_DrawPoint(j+x,i+y,*color++);
		}
	}

#elif defined(STM32H7) 

	uint32_t offline =0 ;
	uint32_t ltdc_addr = 0;
	uint32_t ltdc_whlen = 0;

	switch (lcddev.dir)
	{
		case 2:
		offline = lcddev.x_pixel - height; 
		ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*x+(lcddev.x_pixel-height-y)));
		ltdc_whlen = (height<<16)|(width);
		break;
		case 3:
		offline = lcddev.x_pixel - width;
		ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-height-y)+(lcddev.x_pixel-width-x)));
		ltdc_whlen = (width<<16)|(height);
		break;
		case 4:
		offline = lcddev.x_pixel - height; 
		ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*(lcddev.y_pixel-width-x)+y));
		ltdc_whlen = (height<<16)|(width);
		break;
		default:
		offline = lcddev.x_pixel - width;
		ltdc_addr = ((uint32_t)ltdc_framebuf[ltdcdev.layer] + ltdcdev.pixsize*(lcddev.x_pixel*y+x));
		ltdc_whlen = ((uint32_t)width<<16)|((uint32_t)height);
		break;
	}

	DMA2D->CR=0<<16;				//存储器到存储器模式
	DMA2D->FGPFCCR=ltdcdev.ltdc_format;//LTDC_PIXEL_FORMAT_RGB565;	//设置颜色格式
	DMA2D->FGOR=0;					//前景层行偏移为0
	DMA2D->OOR=offline;				//设置行偏移 
	DMA2D->CR&=~(1<<0);				//先停止DMA2D
	
	DMA2D->FGPFCCR |= 1<<21;
	DMA2D->CR &=~(0x07<<16);
	DMA2D->CR |= 0x05<<16;
	
	DMA2D->FGMAR=(uint32_t)color;		//源地址

	DMA2D->OMAR=ltdc_addr;				//输出存储器地址
	DMA2D->NLR=ltdc_whlen;	//设定行数寄存器 
	DMA2D->CR|=1<<0;				//启动DMA2D
	while((DMA2D->ISR&(1<<1))==0);
	DMA2D->IFCR|=1<<1;				//清除传输完成标志  	
	#endif
}

STATIC mp_obj_t tftlcd_lcd7r_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcddev.dir = args[ARG_portrait].u_int;

	set_lcd_dir(lcddev.dir);

	lcddev.backcolor = BLACK;

	lcd7r_init();
	ltdc_clear(lcddev.backcolor);
	lcd7r_obj.base.type = &tftlcd_lcd7r_type;

	draw_global = &ltdc_glcd;

	return MP_OBJ_FROM_PTR(&lcd7r_obj);
}
#endif

//==============================================================================================================

//------------------------------------------------------------------------------------------------------

STATIC void tftlcd_lcd7r_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print, "LCD(portrait=%d),width:%u,height:%u,X_PIXEL:%u,Y_PIXEL:%u\n",lcddev.dir,lcddev.width,lcddev.height,
	lcddev.x_pixel,lcddev.y_pixel);
}
//------------------------------------------------------------------------------------------------------

#if MICROPY_HW_LCD43R
STATIC mp_obj_t tftlcd_lcd43r_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	enum { ARG_portrait };
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	lcddev.dir = args[ARG_portrait].u_int;

	set_lcd_dir(lcddev.dir);

	lcddev.backcolor = BLACK;

	lcd43g_init();
	ltdc_clear(lcddev.backcolor);

	lcd7r_obj.base.type = &tftlcd_lcd43r_type;
	draw_global = &ltdc_glcd;
	
	return MP_OBJ_FROM_PTR(&lcd7r_obj);
}
#endif
STATIC mp_obj_t tftlcd_lcd7r_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {
	static const mp_arg_t clear_args[] = {
			{ MP_QSTR_fillcolor,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(clear_args)];
	mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(clear_args), clear_args, args);

	if(args[0].u_obj !=MP_OBJ_NULL) 
	{
		size_t len;
		mp_obj_t *params;
		mp_obj_get_array(args[0].u_obj, &len, &params);
		if(len == 3){
			lcddev.backcolor = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			
			ltdc_fill_rgb565(0,0,lcddev.width,lcddev.height,lcddev.backcolor);
			
			lcddev.clercolor = lcddev.backcolor;

		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("lcd fill parameter error \nCorrect call:fill((r,g,b))"));
		}
	}
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_clear_obj, 1, tftlcd_lcd7r_clear);
//=======================================================================================================
STATIC mp_obj_t tftlcd_lcd7r_drawp(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drawp_args[] = {
				{ MP_QSTR_x,       MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y,       MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_color,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drawp_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawp_args), drawp_args, args);

    if(args[2].u_obj !=MP_OBJ_NULL) 
    {
			size_t len;
			mp_obj_t *params;
			mp_obj_get_array(args[2].u_obj, &len, &params);
			if(len == 3){
				ltdc_DrawPoint(args[0].u_int,args[1].u_int ,
				get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])));
			}else{
				mp_raise_ValueError(MP_ERROR_TEXT("lcd drawPixel parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
			}
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawp_obj, 1, tftlcd_lcd7r_drawp);

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd7r_drawL(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drawL_args[] = {
		{ MP_QSTR_x0,        	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y0,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_x1,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y1,       	MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_color,   		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drawL_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawL_args), drawL_args, args);

    if(args[4].u_obj !=MP_OBJ_NULL) 
    {
			size_t len;
			mp_obj_t *params;
			mp_obj_get_array(args[4].u_obj, &len, &params);
			
			if(len == 3){
				uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
				grap_drawLine(&ltdc_glcd,args[0].u_int ,args[1].u_int,args[2].u_int,args[3].u_int ,color);
				 
			}else{
				mp_raise_ValueError(MP_ERROR_TEXT("lcd drawL parameter error \nCorrect call:drawPixel(x,y,(r,g,b)"));
			}
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawL_obj, 4, tftlcd_lcd7r_drawL);
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd7r_drawRect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  STATIC const mp_arg_t Rect_args[] = {
		{ MP_QSTR_x,        		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,        		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_width,     		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_height,    		MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_border,    		MP_ARG_INT, {.u_int = 1} }, 
    { MP_QSTR_fillcolor,   	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(Rect_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(Rect_args), Rect_args, args);

  if(args[4].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[4].u_obj, &len, &params);
    if(len == 3){
			 uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			 grap_drawRect(&ltdc_glcd,args[0].u_int,args[1].u_int,args[2].u_int,args[3].u_int,args[5].u_int,color);
			 
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("lcd drawRect parameter error \n"));
    }
  }
    //MP_OBJ_NULL
  if(args[6].u_obj != mp_const_none && args[6].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[6].u_obj, &len, &params);

    if (len != 3) { // Check params len
       mp_raise_ValueError(MP_ERROR_TEXT("lcd fillcolor parameter error"));
    }
	   uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
    for(uint16_t i=0 ; i <= (args[3].u_int-(args[5].u_int*2)); i++ ) {
     grap_drawLine(&ltdc_glcd,args[0].u_int+args[5].u_int,args[1].u_int+args[5].u_int+i,
					args[0].u_int+args[2].u_int-args[5].u_int,args[1].u_int+args[5].u_int+i,color);
		}
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawRect_obj, 1, tftlcd_lcd7r_drawRect);
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t tftlcd_lcd7r_drawCircle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  STATIC const mp_arg_t tft_allowed_args[] = {
	{ MP_QSTR_x, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_y, 			 			MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_radius, 	 		MP_ARG_INT, {.u_int = 0} },
	{ MP_QSTR_color,	  		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	{ MP_QSTR_border, 	 		MP_ARG_INT, {.u_int = 1} }, 
	{ MP_QSTR_fillcolor,	 	MP_ARG_OBJ,	{.u_obj = MP_OBJ_NULL} }, //7
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);
  
//Circlecolor
  if(args[3].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[3].u_obj, &len, &params);
    if(len == 3){
      uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
			for(uint16_t i=0; i < args[4].u_int ;i++) {
				grap_drawColorCircle(&ltdc_glcd,args[0].u_int,args[1].u_int,args[2].u_int-i,color);
			}
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("lcd color parameter error \n"));
    }
  }
//fillcolor
  if(args[5].u_obj != mp_const_none && args[5].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[5].u_obj, &len, &params);

    if (len != 3) { // Check params len
       mp_raise_ValueError(MP_ERROR_TEXT("lcd fillcolor parameter error"));
    }
    uint16_t color=get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));

    for(uint16_t i=0 ; i <= (args[2].u_int-args[4].u_int); i++ ) {
			grap_drawColorCircle(&ltdc_glcd,
						args[0].u_int, args[1].u_int, args[2].u_int-args[4].u_int-i, color);
    }
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_drawCircle_obj, 1, tftlcd_lcd7r_drawCircle);
//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t tftlcd_lcd7r_printStr(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t tft_allowed_args[] = {
		{ MP_QSTR_text,     		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_x,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,        		MP_ARG_REQUIRED |MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_color,    		MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_backcolor,   	MP_ARG_OBJ,{.u_obj = MP_OBJ_NULL} }, 
    { MP_QSTR_size,      		MP_ARG_INT, {.u_int = 2} },
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(tft_allowed_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(tft_allowed_args), tft_allowed_args, args);

  uint16_t text_size = args[5].u_int;
  uint16_t color = 0;
  //color
  if(args[3].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[3].u_obj, &len, &params);
    if(len == 3){
      color = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2]));
    }else{
      mp_raise_ValueError(MP_ERROR_TEXT("printStr color parameter error \n"));
    }
  }

  if(args[4].u_obj != mp_const_none && args[4].u_obj !=MP_OBJ_NULL) 
  {
    size_t len;
    mp_obj_t *params;
    mp_obj_get_array(args[4].u_obj, &len, &params);
  
    if (len != 3) { 
       mp_raise_ValueError(MP_ERROR_TEXT("lcd backolor parameter error"));
    }
    lcddev.backcolor = get_rgb565(mp_obj_get_int(params[0]), mp_obj_get_int(params[1]), mp_obj_get_int(params[2])); 
  }
//
  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter error"));

    } else {
        mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);
        char *str = bufinfo.buf;

        if(text_size == 1)  text_size = 16;
        else if(text_size == 2) text_size = 24;
        else if(text_size == 3) text_size = 32;
        else if(text_size == 4) text_size = 48;
        else mp_raise_ValueError(MP_ERROR_TEXT("lcd size parameter error"));
        grap_drawStr(&ltdc_glcd, args[1].u_int, args[2].u_int, 
									text_size* bufinfo.len, text_size , text_size,str ,color, lcddev.backcolor);
    }
  }
	else{
     mp_raise_ValueError(MP_ERROR_TEXT("lcd text parameter is empty"));
  }
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_printStr_obj, 1, tftlcd_lcd7r_printStr);
//------------------------------------------------------------------------------------------------------
#if MICROPY_PY_PICLIB

STATIC mp_obj_t tftlcd_lcd7r_Picture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t tft_allowed_args[] = { 
    { MP_QSTR_x,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_y,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_cached, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);

  if(args[2].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[2].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter error"));
    } 
		else 
		{
        mp_get_buffer_raise(args[2].u_obj, &bufinfo, MP_BUFFER_READ);

				uint8_t res=0;
				
				mp_obj_t tuple[2];
				
				const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
				const char *ftype = mp_obj_str_get_str(file_type(file_path));
				 
				 mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
				 if(res == 1){
					 vfs = vfs->next;
					 is_sdcard = 1;
				 }else{
					 is_sdcard = 0;
				 }
				 fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
				 //---------------------------------------------------------------
					 if(args[3].u_bool == true){
						 uint8_t file_len = strlen(file_path);
						 char *file_buf = (char *)m_malloc(file_len+7);  
		 
						 memset(file_buf, '\0', file_len+7);
						 sprintf(file_buf,"%s%s",file_path,".cache");
						 res = grap_drawCached(&ltdc_glcd,&vfs_fat->fatfs, args[0].u_int, args[1].u_int, (const char *)file_buf);
						 m_free(file_buf);
						 if(!res) return mp_const_none;
					 }
				 //---------------------------------------------------------------

				 piclib_init();
				 
				if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0)
					{
						
						#if MICROPY_PY_HJPEG_DECODE
						if(hjpgd_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int)){
							printf("hjpgd_decode error\r\n");
						}
						#else
							jpg_decode(&vfs_fat->fatfs,file_path, args[0].u_int,args[1].u_int ,1);
						#endif
					}
				else if(strncmp(ftype , "bmp" , 3) == 0)
					{
						stdbmp_decode(&vfs_fat->fatfs ,file_path, args[0].u_int, args[1].u_int) ;
					}
				else
					{
						mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
						return mp_const_none;
					}

				tuple[0] = mp_obj_new_int(picinfo.S_Height);
				tuple[1] = mp_obj_new_int(picinfo.S_Width);
				return mp_obj_new_tuple(2, tuple);
    }
  }
	else{
      mp_raise_ValueError(MP_ERROR_TEXT("picture parameter is empty"));
  }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_Picture_obj, 1, tftlcd_lcd7r_Picture);

// cached file
STATIC mp_obj_t tftlcd_lcd7r_CachePicture(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

  STATIC const mp_arg_t tft_allowed_args[] = { 
    { MP_QSTR_file,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_path,     MP_ARG_KW_ONLY 	| MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
		{ MP_QSTR_replace, 	MP_ARG_KW_ONLY 	| MP_ARG_BOOL,{.u_bool = false} },
  };

  uint8_t arg_num = MP_ARRAY_SIZE(tft_allowed_args);
  mp_arg_val_t args[arg_num];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, arg_num, tft_allowed_args, args);
	
	uint8_t res=0;
	#if MICROPY_HW_COLUMBUS
	while(Is_FileReadOk){
		Is_FileReadOk = 0;
		mp_hal_delay_ms(1000);
	}
	#else
		mp_hal_delay_ms(1000);
	#endif
	
	char *path_buf = (char *)m_malloc(50);  //最大支持50字符
	memset(path_buf, '\0', 50);
	
  if(args[0].u_obj !=MP_OBJ_NULL) 
  {
    mp_buffer_info_t bufinfo;
    if (mp_obj_is_int(args[0].u_obj)) {
      mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter error,should is .cache"));
    } 
		else 
		{
			mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

			const char *file_path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
			const char *ftype = mp_obj_str_get_str(file_type(file_path));
			 mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
			 
			 if(res == 1){
				 vfs = vfs->next;
				 is_sdcard = 1;
			 }else{
			 	is_sdcard = 0;
			 }
			fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);

			//test file
			FIL		*f_file;
			f_file=(FIL *)m_malloc(sizeof(FIL));
			if(f_file == NULL){
				mp_raise_ValueError(MP_ERROR_TEXT("malloc f_file error"));
			}
			sprintf(path_buf,"%s%s",file_path,".cache");
			res = f_open(&vfs_fat->fatfs,f_file,path_buf,FA_READ);
			f_close(f_file);
			f_sync(f_file);
			if(res == FR_OK && args[2].u_bool == false){
				return mp_const_none;
			}else{
				args[2].u_bool = true;
			}

			memset(path_buf, '\0', 50);
printf("start loading->%s\r\n",file_path);

			piclib_init();
			if(strncmp(ftype,"jpg",3) == 0 || strncmp(ftype,"jpeg",4) == 0)
			{
				#if MICROPY_PY_HJPEG_DECODE
				hjpgd_decode(&vfs_fat->fatfs,file_path, 0, 0 );
				#else
				res = jpg_decode(&vfs_fat->fatfs,file_path, 0, 0 ,1);
				if(res){
					printf("jpg_decode err:%d\r\n",res);
					return mp_const_none;
				}
				#endif
			}
			else if(strncmp(ftype , "bmp" , 3) == 0)
			{
				res = stdbmp_decode(&vfs_fat->fatfs ,file_path, 0, 0) ;
				printf("bmp_decode err:%d\r\n",res);
				if(res)return mp_const_none;
			}
			else
			{
				mp_raise_ValueError(MP_ERROR_TEXT("picture file type error"));
				return mp_const_none;
			}
//-----------------------------------------------------------
			if(args[1].u_obj !=MP_OBJ_NULL)
			{
				mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_READ);
				const char *path = mp_obj_str_get_str(get_path(bufinfo.buf ,&res));
				const char *path_ftype = mp_obj_str_get_str(file_type(path));

				if(strncmp(path_ftype , "cache" , 5))
				{
					mp_raise_ValueError(MP_ERROR_TEXT("CachePicture path file type error"));
					return mp_const_none;
				}
				sprintf(path_buf,"%s",path);
			}else
			{
				sprintf(path_buf,"%s%s",file_path,".cache");
			}
//------------------------------------------------
			res = f_open(&vfs_fat->fatfs,f_file,path_buf,FA_READ);
			f_close(f_file);
			f_sync(f_file);
			
			if(args[2].u_bool == true || res != 0)
			{
				grap_newCached(&ltdc_glcd, is_sdcard,&vfs_fat->fatfs, path_buf,picinfo.S_Width, picinfo.S_Height);	
			}
			
			f_sync(f_file);
			m_free(f_file);
    }
  }
	else{
      mp_raise_ValueError(MP_ERROR_TEXT("CachePicture parameter is empty"));
  }
	m_free(path_buf);
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_lcd7r_CachePicture_obj, 1, tftlcd_lcd7r_CachePicture);

#endif

//---------------------------华丽的分割线-------------------------------------------------------------------
STATIC mp_obj_t tftlcd_write_buf(size_t n_args, const mp_obj_t *args) {
	if(6 != n_args) {
		mp_raise_ValueError(MP_ERROR_TEXT("lcd write_buf parameter error \n"));
	}
	unsigned short start_x = mp_obj_get_int(args[2]);
	unsigned short start_y = mp_obj_get_int(args[3]);
	unsigned short width = mp_obj_get_int(args[4]);
	unsigned short height = mp_obj_get_int(args[5]);

	mp_buffer_info_t lcd_write_data = {0};

	
	mp_get_buffer_raise(args[1], &lcd_write_data, MP_BUFFER_READ);
	
	if(lcd_write_data.buf == NULL || lcd_write_data.len == 0) {
		return mp_obj_new_int(-3);
	}

	grap_drawFull(start_x, start_y,width,height,(uint16_t *)lcd_write_data.buf);

    return mp_obj_new_int(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tftlcd_write_buf_obj, 1, 6, tftlcd_write_buf);


// STATIC mp_obj_t tftlcd_delay_drawp(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // static const mp_arg_t drawp_args[] = {
				// { MP_QSTR_ms,       MP_ARG_INT, {.u_int = 0} },
    // };
    // mp_arg_val_t args[MP_ARRAY_SIZE(drawp_args)];
    // mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drawp_args), drawp_args, args);

		// printf("delay:%d\r\n",args[0].u_int);

		// mp_hal_delay_us(args[0].u_int*1000);
    // return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tftlcd_delay_drawp_obj, 1, tftlcd_delay_drawp);

STATIC const mp_rom_map_elem_t tftlcd_lcd7r_locals_dict_table[] = {
	// instance methods
	 { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&tftlcd_lcd7r_clear_obj) },
	 { MP_ROM_QSTR(MP_QSTR_drawPixel), MP_ROM_PTR(&tftlcd_lcd7r_drawp_obj) },
	 { MP_ROM_QSTR(MP_QSTR_drawLine), MP_ROM_PTR(&tftlcd_lcd7r_drawL_obj) },
	 { MP_ROM_QSTR(MP_QSTR_drawRect), MP_ROM_PTR(&tftlcd_lcd7r_drawRect_obj) },
	 { MP_ROM_QSTR(MP_QSTR_drawCircle), MP_ROM_PTR(&tftlcd_lcd7r_drawCircle_obj) },
	 { MP_ROM_QSTR(MP_QSTR_printStr), MP_ROM_PTR(&tftlcd_lcd7r_printStr_obj) },
	 
	 // { MP_ROM_QSTR(MP_QSTR_delay), MP_ROM_PTR(&tftlcd_delay_drawp_obj) },

	 { MP_ROM_QSTR(MP_QSTR_write_buf), MP_ROM_PTR(&tftlcd_write_buf_obj) },

	#if MICROPY_PY_PICLIB
	{ MP_ROM_QSTR(MP_QSTR_Picture), MP_ROM_PTR(&tftlcd_lcd7r_Picture_obj) },
	{ MP_ROM_QSTR(MP_QSTR_CachePicture), MP_ROM_PTR(&tftlcd_lcd7r_CachePicture_obj) },
	#endif
};

MP_DEFINE_CONST_DICT(tftlcd_lcd7r_locals_dict, tftlcd_lcd7r_locals_dict_table);
#if MICROPY_HW_LCD7R
const mp_obj_type_t tftlcd_lcd7r_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD7R,
    .print = tftlcd_lcd7r_print,
    .make_new = tftlcd_lcd7r_make_new,
    .locals_dict = (mp_obj_dict_t *)&tftlcd_lcd7r_locals_dict,
};
#endif
#if MICROPY_HW_LCD43R
const mp_obj_type_t tftlcd_lcd43r_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD43R,
    .print = tftlcd_lcd7r_print,
    .make_new = tftlcd_lcd43r_make_new,
    .locals_dict = (mp_obj_dict_t *)&tftlcd_lcd7r_locals_dict,
};
#endif

#endif





