
#ifndef MICROPY_INCLUDED_STM32_GUI_BUTTON_H
#define MICROPY_INCLUDED_STM32_GUI_BUTTON_H

#if MICROPY_GUI_BUTTON

#if MICROPY_ENABLE_VIDEO
#include "avi.h"
#endif

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
//-------------------------------------------------------------------------------------------------------

//按钮状态定义
#define BTN_RELEASE 		0X00//松开
#define BTN_PRES   			0X01 //按下
#define BTN_INACTIVE   	0X02 //没有动作

//#if MICROPY_HW_LCD43R
//typedef uint32_t	color_t;
//#else
typedef uint16_t	color_t;
//#endif

//按钮结构体定义
typedef struct 
{
	volatile uint16_t x; 				  		//按钮顶端坐标
	volatile uint16_t y;               //按钮左端坐标
	volatile uint16_t width; 				  		//宽度
	volatile uint16_t height;						//高度

	volatile uint8_t id;								//按钮ID
	volatile uint8_t sta;							//按钮状态
									//[7]:坐标状态 0,松开.1,按下.(并不是实际的TP状态)
									//[6]:0,此次按键无效;1,此次按键有效.(根据实际的TP状态决定)
									//[5:2]:保留
									//[1:0]:0,激活的(松开);1,按下;2,未被激活的
	volatile uint8_t font;						//caption文字字体
	volatile color_t fcolor;//字体颜色
  char *label;
	color_t upcolor; 				  	//button caption font up color
	color_t downcolor; 				  	//button caption font down color
}__attribute__((packed)) _btn_obj;

//===========================================================================================

extern void button_task(void);

extern const mp_obj_type_t gui_button_type;

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(gui_button_task_handler_obj);



#endif

#endif // __VIDEO_H__
