
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	tp_touch.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2021/5/11
	* Description			:	
******************************************************************************/

#ifndef __MODTOUCH_H
#define __MODTOUCH_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "py/obj.h"
#include "py/stream.h"

#if MICROPY_ENABLE_TOUCH


#define TP_PRES_DOWN 			0x80  //触屏被按下	  
#define TP_CATH_UP 				0x40  //按下弹起 
#define TP_PRES_MOVE			0x20  //有按键移动 
#define TP_INACTIVE  			0X00 //没有动作


#define TOUCH_MAX_TOUCH  5    //电容屏支持的点数,固定为5点

//触摸屏控制器
typedef struct 
{
	volatile	uint16_t x[TOUCH_MAX_TOUCH];
	volatile	uint16_t y[TOUCH_MAX_TOUCH];	

	volatile	uint8_t  sta;	
	volatile	uint8_t  dir;	
	volatile	uint8_t  type; //1:FT,2:GT911	,3 XPT2046
									//b7:按下1/松开0; 
												//b6:0,没有按键按下;1,有按键按下. 
									//b5:保留
									//b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
	uint8_t id;				// ID
}TP_DEV;

extern TP_DEV tp_dev;

extern const mp_obj_type_t touch_xpt2046_type;


extern void tp_touch_down(int8_t id, uint16_t x, uint16_t y, uint8_t w);
extern void tp_touch_up(int8_t id);
extern void xpt_deinit_internal(void);


/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif

#endif /* __MODTOUCH_H */



