#include "piclib.h"
#include "bmp.h"
#include "string.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "pin.h"
#include "pin_static_af.h"
#include "systick.h"

#include "py/objstr.h"
#include "py/objlist.h"   
 
#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

uint8_t stdbmp_decode(FATFS *fs ,const char *filename,uint16_t sx,uint16_t sy) 
{
	FIL* f_bmp;
  uint16_t br;

  uint16_t count;		    	   
	uint8_t  rgb ,color_byte;
	uint16_t x ,y;	  
	uint32_t color;
	uint16_t countpix=0; 

	uint16_t  realx=0;
	uint16_t realy=0;
	uint8_t  yok=1;  				   
	uint8_t res;

	uint8_t *databuf;    		
 	uint16_t readlen=BMP_DBUF_SIZE;

	uint8_t *bmpbuf;			  	
	uint8_t biCompression=0;	
	
	uint16_t rowlen;	  		 
	BITMAPINFO *pbmp;		

	databuf=(uint8_t*)m_malloc(readlen);		
	f_bmp=(FIL *)m_malloc(sizeof(FIL));	
	if(f_bmp==NULL || databuf==NULL)					
	{		 
		m_free(databuf);
		return 1;				
	} 	 
	res=f_open(fs,f_bmp,(const TCHAR*)filename,FA_READ);					  
	if(res==0)
	{ 
		f_read(f_bmp,databuf,readlen,(UINT*)&br);	
		pbmp=(BITMAPINFO*)databuf;				
		count=pbmp->bmfHeader.bfOffBits;        	
		color_byte=pbmp->bmiHeader.biBitCount/8;	 
		biCompression=pbmp->bmiHeader.biCompression;
		picinfo.ImgHeight=pbmp->bmiHeader.biHeight;	
		picinfo.ImgWidth=pbmp->bmiHeader.biWidth;  	

		picinfo.S_YOFF=sy;
		picinfo.S_XOFF=sx;

		picinfo.S_Height=picinfo.ImgHeight;
		picinfo.S_Width=picinfo.ImgWidth;

		draw_init();	
	
		if((picinfo.ImgWidth*color_byte)%4)rowlen=((picinfo.ImgWidth*color_byte)/4+1)*4;
		else rowlen=picinfo.ImgWidth*color_byte;
 
		color=0;												 
		x=0 ;
		y=picinfo.ImgHeight;
		rgb=0;      
		//对于尺寸小于等于设定尺寸的图片,进行快速解码
		realy=(y*picinfo.Div_Fac)>>13;
		bmpbuf=databuf;
		while(1)
		{				 
			while(count<readlen)  //读取一簇1024扇区 (SectorsPerClust 每簇扇区数)
		    {
				if(color_byte==3)   //24位颜色图
				{
					switch (rgb) 
					{
						case 0:				  
							color=bmpbuf[count]; //B
							break ;	   
						case 1: 	 
							color+=((uint16_t)bmpbuf[count]<<8);//G
							break;	  
						case 2 :   
							color+=((uint16_t)bmpbuf[count]<<16);//R
							break ;			
					}   
				}else if(color_byte==2)  //16位颜色图
				{
					switch(rgb)
					{
						case 0 : 
							if(biCompression==BI_RGB)//RGB:5,5,5
							{
								//color=((uint16_t)bmpbuf[count]&0X1F);	 	//R
								//color+=(((uint16_t)bmpbuf[count])&0XE0)<<1; //G
							}else		//RGB:5,6,5
							{	
								color = ((bmpbuf[count] & 0xE0)<<5) | ((bmpbuf[count] & 0x1F)<<3);//G,B
							}  
							break ;   
						case 1 : 			  			 
							if(biCompression==BI_RGB)//RGB:5,5,5
							{
							//	color+=(uint16_t)bmpbuf[count]<<9;  //R,G
							}else  		//RGB:5,6,5
							{
								color += ((bmpbuf[count] & 0xE0)<<8) | ((bmpbuf[count] & 0x1F)<<16);//R,G
							}  									 
							break ;	 
					}		     
				}else if(color_byte==4)//32位颜色图
				{
					switch (rgb)
					{
						case 0:				  
							color=bmpbuf[count]; //B
							break ;	   
						case 1: 	 
							color+=((uint16_t)bmpbuf[count]<<8);//G
							break;	  
						case 2 : 
							color+=((uint16_t)bmpbuf[count]<<16);//R	  
							break ;			
						case 3 :
							//alphabend=bmpbuf[count];//不读取  ALPHA通道
							break ;  		  	 
					}	
				}else if(color_byte==1)//8位色,暂时不支持,需要用到颜色表.
				{
				} 
				rgb++;	  
				count++ ;		  
				if(rgb==color_byte) //水平方向读取到1像素数数据后显示
				{	
					if(x<picinfo.ImgWidth)	 					 			   
					{	
						realx=(x*picinfo.Div_Fac)>>13;//x轴实际值
						if(is_element_ok(realx,realy,1)&&yok)//符合条件
						{						 				 	  	       
							pic_phy.draw_point(realx+picinfo.S_XOFF,realy+picinfo.S_YOFF-1,color);//显示图片	
						}   									    
					}
					x++;//x轴增加一个像素 
					color=0x00; 
					rgb=0;  		  
				}
				countpix++;//像素累加
				if(countpix>=rowlen)//水平方向像素值到了.换行
				{		 
					y--; 
					if(y==0)break;			 
					realy=(y*picinfo.Div_Fac)>>13;//实际y值改变	 
					if(is_element_ok(realx,realy,0))yok=1;//此处不改变picinfo.staticx,y的值	 
					else yok=0; 
					x=0; 
					countpix=0;
					color=0x00;
					rgb=0;
				}	 
			} 		
			res=f_read(f_bmp,databuf,readlen,(UINT *)&br);//读出readlen个字节
			if(br!=readlen)readlen=br;	//最后一批数据		  
			if(res||br==0)break;		//读取出错
			bmpbuf=databuf;
	 	 	count=0;
		}  
		f_close(f_bmp);//关闭文件
	} 

	m_free(databuf);	 
	m_free(f_bmp);		 

	return res;		//BMP显示结束.    					   
}		 

uint8_t minibmp_decode(FATFS *fs ,char *filename,uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t acolor,uint8_t mode)//尺寸小于240*320的bmp图片解码.
{
	FIL* f_bmp;
    uint16_t br;
	uint8_t  color_byte;
	uint16_t tx,ty,tcolor;
	uint32_t color;	 
	//tx,ty的实际坐标	
	uint8_t res;
	uint16_t i,j;
	uint8_t *databuf;    		//数据读取存                                                                       放地址
 	uint16_t readlen=BMP_DBUF_SIZE;//一次从SD卡读取的字节数长度,不能小于LCD宽度*3!!!

	uint8_t *bmpbuf;			  	//数据解码地址
	uint8_t biCompression=0;		//记录压缩方式
	
	uint16_t rowcnt;				//一次读取的行数
	uint16_t rowlen;	  		 	//水平方向字节数  
	uint16_t rowpix=0;			//水平方向像素数	  
	uint8_t rowadd;				//每行填充字节数

	uint16_t tmp_color;

	uint8_t alphabend=0xff;		//代表透明色为0，完全不透明
	uint8_t alphamode=mode>>6;	//得到模式值,0/1/2
	BITMAPINFO *pbmp;   	//临时指针		 
	//得到窗体尺寸
	picinfo.S_Height=height;
	picinfo.S_Width=width;
		
	databuf=(uint8_t*)m_malloc(readlen);		//开辟readlen字节的内存区域
	f_bmp=(FIL *)m_malloc(sizeof(FIL));	//开辟FIL字节的内存区域 
	if(f_bmp==NULL || databuf==NULL )								//内存申请失败.
	{		 
		m_free(databuf);
		return 1;				
	} 	 

	res=f_open(fs,f_bmp,(const TCHAR*)filename,FA_READ);//打开文件	 						  
	if(res==0)//打开成功.
	{ 
		f_read(f_bmp,databuf,sizeof(BITMAPINFO),(UINT*)&br);//读出BITMAPINFO信息 
		pbmp=(BITMAPINFO*)databuf;					//得到BMP的头部信息   
		color_byte=pbmp->bmiHeader.biBitCount/8;	//彩色位 16/24/32  
		biCompression=pbmp->bmiHeader.biCompression;//压缩方式
		picinfo.ImgHeight=pbmp->bmiHeader.biHeight;	//得到图片高度
		picinfo.ImgWidth=pbmp->bmiHeader.biWidth;  	//得到图片宽度   
		//水平像素必须是4的倍数!!
		if((picinfo.ImgWidth*color_byte)%4)rowlen=((picinfo.ImgWidth*color_byte)/4+1)*4;
		else rowlen=picinfo.ImgWidth*color_byte;
		rowadd=rowlen-picinfo.ImgWidth*color_byte;	//每行填充字节数
 		//开始解码BMP   
		color=0;//颜色清空	 													 
		tx=0 ;
		ty=picinfo.ImgHeight-1;
		if(picinfo.ImgWidth<=picinfo.S_Width&&picinfo.ImgHeight<=picinfo.S_Height)
		{  							   
			rowcnt=readlen/rowlen;						//一次读取的行数
			readlen=rowcnt*rowlen;						//一次读取的字节数
			rowpix=picinfo.ImgWidth;					//水平像素数就是宽度 
			f_lseek(f_bmp,pbmp->bmfHeader.bfOffBits);	//偏移到数据起始位置 	  
			while(1)
			{	     
				res=f_read(f_bmp,databuf,readlen,(UINT *)&br);	//读出readlen个字节
				bmpbuf=databuf;									//数据首地址  
				if(br!=readlen)rowcnt=br/rowlen;				//最后剩下的行数
				if(color_byte==3)  			//24位BMP图片
				{
					for(j=0;j<rowcnt;j++)	//每次读到的行数
					{
						for(i=0;i<rowpix;i++)//写一行像素
						{
							color=(*bmpbuf++);		   		 	//B
							color+=((uint16_t)(*bmpbuf++)<<8);	//G
							color+=(((uint16_t)*bmpbuf++)<<16);	//R
 						 	pic_phy.draw_point(x+tx,y+ty,color);//显示图片	
							tx++;
						}
						bmpbuf+=rowadd;//跳过填充区
						tx=0;
						ty--;
					}
				}else if(color_byte==2)//16位BMP图片
				{
					for(j=0;j<rowcnt;j++)//每次读到的行数
					{
						if(biCompression==BI_RGB)//RGB:5,5,5
						{
							for(i=0;i<rowpix;i++)
							{
								color=((uint16_t)*bmpbuf&0X1F);			//R
								color+=(((uint16_t)*bmpbuf++)&0XE0)<<1; 	//G
		 						color+=((uint16_t)*bmpbuf++)<<9;  	    //R,G	 
							  pic_phy.draw_point(x+tx,y+ty,color);//显示图片	
								tx++;
							}
						}else  //RGB 565
						{
							for(i=0;i<rowpix;i++)
							{											 
								tcolor = *bmpbuf++;  			//G,B
								color = ((tcolor & 0xE0)<<5) | ((tcolor & 0x1F)<<3);
								tcolor = *bmpbuf++;
								color += ((tcolor & 0xE0)<<8) | ((tcolor & 0x1F)<<16);
							  	pic_phy.draw_point(x+tx,y+ty,color);//显示图片	
								tx++;
							}
						}
						bmpbuf+=rowadd;//跳过填充区
						tx=0;
						ty--;
					}	
				}else if(color_byte==4)		//32位BMP图片
				{
					for(j=0;j<rowcnt;j++)	//每次读到的行数
					{
						for(i=0;i<rowpix;i++)
						{
							color=(*bmpbuf++);		   		 	//B
							color+=((uint16_t)(*bmpbuf++)<<8);	//G
							color+=(((uint16_t)*bmpbuf++)<<16);	//R
							alphabend=*bmpbuf++;					//ALPHA通道
							if(alphamode!=1) //需要读取底色
							{
								tmp_color=pic_phy.read_point(x+tx,y+ty);//读取颜色		   
							    if(alphamode==2)//需要附加的alphablend
								{
									tmp_color=piclib_alpha_blend(tmp_color,acolor,mode&0X1F);	//与指定颜色进行blend		 
								}
								color=piclib_alpha_blend(tmp_color,color,alphabend/8); 			//和底色进行alphablend
							}else tmp_color=piclib_alpha_blend(acolor,color,alphabend/8);		//与指定颜色进行blend
 							pic_phy.draw_point(x+tx,y+ty,color);//显示图片				   
							tx++;//x轴增加一个像素 	  
						}
						bmpbuf+=rowadd;//跳过填充区
						tx=0;
						ty--;
					}

				}
				if(br!=readlen||res)break;	 
			}	 
		}	
		f_close(f_bmp);//关闭文件      
	}else res=1;//图片尺寸错误	

	m_free(databuf);	 
	m_free(f_bmp);		 

	return res;
}            
 
uint8_t bmp_encode(FATFS *fs,char *filename,uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t mode)
{				
	FIL* f_bmp;
	UINT bw;
	uint16_t bmpheadsize;			//bmp头大小	   	
 	BITMAPINFO hbmp;			//bmp头	 
	uint8_t res=0;
	uint16_t tx,ty;				   	//图像尺寸
	uint16_t *databuf;				//数据缓存区地址	   	
	uint16_t pixcnt;				   	//像素计数器
	uint16_t bi4width;		       	//水平像素字节数	   
	if(width==0||height==0)return 1;	//区域错误
	if((x+width-1)>lcddev.width)return 1;		//区域错误
	if((y+height-1)>lcddev.height)return 1;	//区域错误 

	databuf=(uint16_t*)m_malloc(1024);		//开辟至少bi4width大小的字节的内存区域 ,对240宽的屏,480个字节就够了.
	f_bmp=(FIL *)m_malloc(sizeof(FIL));	//开辟FIL字节的内存区域 
	if(f_bmp==NULL || databuf==NULL)								//内存申请失败.
	{		 
		m_free(databuf);
		return 1;				
	} 	 
	      
	bmpheadsize=sizeof(hbmp);//得到bmp文件头的大小   
	memset((uint8_t*)&hbmp,0,sizeof(hbmp));//置零空申请到的内存.	    
	hbmp.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//信息头大小
	hbmp.bmiHeader.biWidth=width;	 	//bmp的宽度
	hbmp.bmiHeader.biHeight=height; 	//bmp的高度
	hbmp.bmiHeader.biPlanes=1;	 		//恒为1
	hbmp.bmiHeader.biBitCount=16;	 	//bmp为16位色bmp
	hbmp.bmiHeader.biCompression=BI_BITFIELDS;//每个象素的比特由指定的掩码决定。
 	hbmp.bmiHeader.biSizeImage=hbmp.bmiHeader.biHeight*hbmp.bmiHeader.biWidth*hbmp.bmiHeader.biBitCount/8;//bmp数据区大小
 				   
	hbmp.bmfHeader.bfType=((uint16_t)'M'<<8)+'B';//BM格式标志
	hbmp.bmfHeader.bfSize=bmpheadsize+hbmp.bmiHeader.biSizeImage;//整个bmp的大小
   	hbmp.bmfHeader.bfOffBits=bmpheadsize;//到数据区的偏移

	hbmp.RGB_MASK[0]=0X00F800;	 		//红色掩码
	hbmp.RGB_MASK[1]=0X0007E0;	 		//绿色掩码
	hbmp.RGB_MASK[2]=0X00001F;	 		//蓝色掩码

	if(mode==1)res=f_open(fs,f_bmp,(const TCHAR*)filename,FA_READ|FA_WRITE);//尝试打开之前的文件
 	if(mode==0||res==0x04)res=f_open(fs,f_bmp,(const TCHAR*)filename,FA_WRITE|FA_CREATE_NEW);//模式0,或者尝试打开失败,则创建新文件		   
 	if((hbmp.bmiHeader.biWidth*2)%4)//水平像素(字节)不为4的倍数
	{
		bi4width=((hbmp.bmiHeader.biWidth*2)/4+1)*4;//实际要写入的宽度像素,必须为4的倍数.	
	}else bi4width=hbmp.bmiHeader.biWidth*2;		//刚好为4的倍数	 
 	if(res==FR_OK)//创建成功
	{
		res=f_write(f_bmp,(uint8_t*)&hbmp,bmpheadsize,&bw);//写入BMP首部  
		for(ty=y+height-1;hbmp.bmiHeader.biHeight;ty--)
		{
			pixcnt=0;
 			for(tx=x;pixcnt!=(bi4width/2);)
			{
				if(pixcnt<hbmp.bmiHeader.biWidth)databuf[pixcnt]=pic_phy.read_point(tx,ty);//读取坐标点的值 
				else databuf[pixcnt]=0Xffff;//补充白色的像素.  
				pixcnt++;
				tx++;
			}
			hbmp.bmiHeader.biHeight--;
			res=f_write(f_bmp,(uint8_t*)databuf,bi4width,&bw);//写入数据
		}
		f_close(f_bmp);
	}	    

	m_free(databuf);	 
	m_free(f_bmp);		 

	return res;
}




