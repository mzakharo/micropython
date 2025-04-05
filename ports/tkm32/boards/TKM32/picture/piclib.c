#include "piclib.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

_pic_info picinfo;	 	//图片信息
_pic_phy pic_phy;		//图片显示物理接口	

static void piclib_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	#if MICROPY_ENABLE_TFTLCD
	grap_drawHline(x0,y0,len,color);	
	#endif
}

static uint16_t piclib_read_point(uint16_t x,uint16_t y)
{
	#if MICROPY_ENABLE_TFTLCD
	return grap_ReadPoint(x,y);	
	#endif
}

static void piclib_draw_point(uint16_t x,uint16_t y,uint16_t color)
{
	#if MICROPY_ENABLE_TFTLCD
	grap_drawPoint(x,y,color);	
	#endif
}

static void piclib_fill(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color)
{
	#if MICROPY_ENABLE_TFTLCD
	grap_drawFill(x,y,width,height,color);	
	#endif
}
	
static void piclib_fillcolor(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{
	#if MICROPY_ENABLE_TFTLCD
	grap_drawFull(x,y,width,height,color);	
	#endif
}
//画图初始化,在画图之前,必须先调用此函数
void piclib_init(void)
{
	pic_phy.read_point=piclib_read_point;  		//读点函数实现
	pic_phy.draw_point=piclib_draw_point;	//画点函数实现
	pic_phy.fill=piclib_fill;					//填充函数实现
	pic_phy.draw_hline=piclib_draw_hline;  	//画线函数实现
	pic_phy.fillcolor=piclib_fillcolor;  	//颜色填充函数实现 

	picinfo.lcdwidth=lcddev.width;	//得到LCD的宽度像素
	picinfo.lcdheight=lcddev.height;//得到LCD的高度像素

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
//初始化画点
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




