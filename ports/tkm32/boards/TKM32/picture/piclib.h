#ifndef __PICLIB_H
#define __PICLIB_H	  		  

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "pin.h"
#include "pin_static_af.h"
#include "systick.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"
#include "py/stream.h"

#if MICROPY_HW_LCD43M
#include "lcd43m.h"
#endif

#if MICROPY_HW_LCD43G
#include "lcd43g.h"
#endif

#include "bmp.h"
#include "tjpgd.h"

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif 
//----------------------------------------------------------------------------------------------------

//图片显示物理层接口  
//在移植的时候,必须由用户自己实现这几个函数
typedef struct 
{
	uint16_t(*read_point)(uint16_t,uint16_t);//uint32_t read_point(uint16_t x,uint16_t y)						读点函数
	void(*draw_point)(uint16_t,uint16_t,uint16_t);			//void draw_point(uint16_t x,uint16_t y,uint32_t color)		    画点函数
	void(*fill)(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);		///void fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint32_t color) 单色填充函数	 
	void(*draw_hline)(uint16_t,uint16_t,uint16_t,uint16_t);		//void draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)  画水平线函数	 
	void(*fillcolor)(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t*);	//void piclib_fill_color(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color) 颜色填充
}_pic_phy; 

extern _pic_phy pic_phy;
//----------------------------------------------------------------------------------------------------

//图像信息
typedef struct
{		
	uint16_t lcdwidth;	//LCD的宽度
	uint16_t lcdheight;	//LCD的高度
	uint32_t ImgWidth; 	//图像的实际宽度和高度
	uint32_t ImgHeight;

	uint32_t Div_Fac;  	//缩放系数 (扩大了8192倍的)
	
	uint32_t S_Height; 	//设定的高度和宽度
	uint32_t S_Width;
	
	uint32_t	S_XOFF;	  	//x轴和y轴的偏移量
	uint32_t	S_YOFF;

	uint32_t staticx; 	//当前显示到的ｘｙ坐标
	uint32_t staticy;																 	
}_pic_info;
extern _pic_info picinfo;//图像信息
//----------------------------------------------------------------------------------------------------
void piclib_init(void);								//初始化画图
uint16_t piclib_alpha_blend(uint16_t src,uint16_t dst,uint8_t alpha);	//alphablend处理
void draw_init(void);							//初始化智能画图
extern uint8_t is_element_ok(uint16_t x,uint16_t y,uint8_t chg);				//判断像素是否有效
//----------------------------------------------------------------------------------------------------

#endif






























