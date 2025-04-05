#include "piclib.h"

#if MICROPY_PY_PICLIB

_pic_info picinfo;	 	//图片信息
_pic_phy pic_phy;		//图片显示物理接口	

//画图初始化,在画图之前,必须先调用此函数
//指定画点/读点
void piclib_init(void)
{
	#if MICROPY_ENABLE_TFTLCD
	switch (lcddev.type)
		{
			case 1:

			break;
			case 2:

			break;
			case 3:

			break;
			case 4:
			#if MICROPY_HW_LCD32
				pic_phy.read_point=ili9341_readPoint;
				pic_phy.draw_point=ili9341_DrawPoint;
				pic_phy.fill=ili9341_Fill;
				pic_phy.draw_hline=ili9341_draw_hline;
				pic_phy.fillcolor=ili9341_Full;
			#endif
			break;
			case 5:
			#if MICROPY_HW_LCD15
				pic_phy.read_point=st7789_readPoint;
				pic_phy.draw_point=st7789_DrawPoint;
				pic_phy.fill=st7789_Fill;
				pic_phy.draw_hline=st7789_draw_hline;
				pic_phy.fillcolor=st7789_Full;
			#endif
			break;
			case 6:
			#if MICROPY_HW_LCD18
				pic_phy.read_point=st7735_readPoint;
				pic_phy.draw_point=st7735_DrawPoint;
				pic_phy.fill=st7735_Fill;
				pic_phy.draw_hline=st7735_draw_hline;
				pic_phy.fillcolor=st7735_Full;
			#endif
			break;
		}

	picinfo.lcdwidth=lcddev.width;	//得到LCD的宽度像素
	picinfo.lcdheight=lcddev.height;//得到LCD的高度像素
	#else
	picinfo.lcdwidth=320;
	picinfo.lcdheight=240;
	pic_phy.read_point=NULL;
	pic_phy.draw_point=NULL;
	pic_phy.fill=NULL;
	pic_phy.draw_hline=NULL;
	pic_phy.fillcolor=NULL;
	#endif
	
	picinfo.ImgWidth=0;	//初始化宽度为0
	picinfo.ImgHeight=0;//初始化高度为0
	picinfo.Div_Fac=0;	//初始化缩放系数为0
	picinfo.S_Height=0;	//初始化设定的高度为0
	picinfo.S_Width=0;	//初始化设定的宽度为0
	picinfo.S_XOFF=0;	//初始化x轴的偏移量为0
	picinfo.S_YOFF=0;	//初始化y轴的偏移量为0
	picinfo.staticx=0;	//初始化当前显示到的x坐标为0
	picinfo.staticy=0;	//初始化当前显示到的y坐标为0
}

//快速ALPHA BLENDING算法.
//src:源颜色
//dst:目标颜色
//alpha:透明程度(0~32)
//返回值:混合后的颜色.
uint16_t piclib_alpha_blend(uint16_t src,uint16_t dst,uint8_t alpha)
{
	uint32_t src2;
	uint32_t dst2;	 
	//Convert to 32bit |-----GGGGGG-----RRRRR------BBBBB|
	src2=((src<<16)|src)&0x07E0F81F;
	dst2=((dst<<16)|dst)&0x07E0F81F;   
	dst2=((((dst2-src2)*alpha)>>5)+src2)&0x07E0F81F;
	return (dst2>>16)|dst2;  
}
//初始化智能画点
//内部调用
void draw_init(void)
{
	float temp,temp1;	   
	temp=(float)picinfo.S_Width/picinfo.ImgWidth;
	temp1=(float)picinfo.S_Height/picinfo.ImgHeight;						 
	if(temp<temp1)temp1=temp;//取较小的那个	 
	if(temp1>1)temp1=1;	  
	//使图片处于所给区域的中间
	picinfo.S_XOFF+=(uint32_t)((picinfo.S_Width-temp1*picinfo.ImgWidth)/2);
	picinfo.S_YOFF+=(uint32_t)((picinfo.S_Height-temp1*picinfo.ImgHeight)/2);
	temp1*=8192;//扩大8192倍	 
	picinfo.Div_Fac=(uint32_t)temp1;
	picinfo.staticx=0xffff;
	picinfo.staticy=0xffff;//放到一个不可能的值上面			 										    
}   
//判断这个像素是否可以显示
//(x,y) :像素原始坐标
//chg   :功能变量. 
//返回值:0,不需要显示.1,需要显示
uint8_t is_element_ok(uint16_t x,uint16_t y,uint8_t chg)
{				  
	if(x!=picinfo.staticx||y!=picinfo.staticy)
	{
		if(chg==1)
		{
			picinfo.staticx=x;
			picinfo.staticy=y;
		} 
		return 1;
	}else return 0;
}
#endif




