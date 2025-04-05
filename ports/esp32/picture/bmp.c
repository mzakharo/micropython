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

#include "py/objstr.h"
#include "py/objlist.h"    
#include "py/stream.h"

#if MICROPY_PY_PICLIB

uint8_t stdbmp_decode(void *fs ,const char *filename,uint16_t sx,uint16_t sy) 
{

  ssize_t br;
	static mp_obj_t f_bmp;
	
  uint16_t count;		    	   
	uint8_t  rgb ,color_byte;
	uint16_t x ,y,color;	  
	uint16_t countpix=0; 

	uint16_t  realx=0;
	uint16_t realy=0;
	uint8_t  yok=1;  				   

	uint8_t *databuf;    		
 	uint16_t readlen=BMP_DBUF_SIZE;

	uint8_t *bmpbuf;			  	
	uint8_t biCompression=0;	
	
	uint16_t rowlen;	  		 
	BITMAPINFO *pbmp;		

	databuf=(uint8_t*)m_malloc(readlen);		
	if(databuf==NULL)					
	{		 
		m_free(databuf);
		return 1;				
	} 	 
	
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_rb),
	};
	f_bmp = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
				  
	//if(res==0)
	{ 	
		br = mp_stream_posix_read(f_bmp, databuf, readlen);
		
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
							color=bmpbuf[count]>>3; //B
							break ;	   
						case 1: 	 
							color+=((uint16_t)bmpbuf[count]<<3)&0X07E0;//G
							break;	  
						case 2 : 
							color+=((uint16_t)bmpbuf[count]<<8)&0XF800;//R	  
							break ;			
					}   
				}else if(color_byte==2)  //16位颜色图
				{
					switch(rgb)
					{
						case 0 : 
							if(biCompression==BI_RGB)//RGB:5,5,5
							{
								color=((uint16_t)bmpbuf[count]&0X1F);	 	//R
								color+=(((uint16_t)bmpbuf[count])&0XE0)<<1; //G
							}else		//RGB:5,6,5
							{
								color=bmpbuf[count];  			//G,B
							}  
							break ;   
						case 1 : 			  			 
							if(biCompression==BI_RGB)//RGB:5,5,5
							{
								color+=(uint16_t)bmpbuf[count]<<9;  //R,G
							}else  		//RGB:5,6,5
							{
								color+=(uint16_t)bmpbuf[count]<<8;	//R,G
							}  									 
							break ;	 
					}		     
				}else if(color_byte==4)//32位颜色图
				{
					switch (rgb)
					{
						case 0:				  
							color=bmpbuf[count]>>3; //B
							break ;	   
						case 1: 	 
							color+=((uint16_t)bmpbuf[count]<<3)&0X07E0;//G
							break;	  
						case 2 : 
							color+=((uint16_t)bmpbuf[count]<<8)&0XF800;//R	  
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
			br = mp_stream_posix_read(f_bmp, databuf, readlen);
			if(br!=readlen)readlen=br;	//最后一批数据		  
			if(br == -1 || br==0)break;		//读取出错
			bmpbuf=databuf;
	 	 	count=0;
		}  
		mp_stream_close(f_bmp);
	} 

	m_free(databuf);	 

	return 0;  					   
}		 

uint8_t minibmp_decode(void *fs ,const char *filename,uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t acolor)//尺寸小于240*320的bmp图片解码.
{
  mp_obj_t f_bmp = NULL;
  ssize_t br;
	uint8_t  color_byte;
	uint16_t ty,color;	 
	//tx,ty的实际坐标	
	uint8_t res = 0;
	uint16_t i,j;
	uint8_t *databuf;    		//数据读取存放地址
 	uint16_t readlen=BMP_DBUF_SIZE;//一次从SD卡读取的字节数长度,不能小于LCD宽度*3!!!

	uint8_t *bmpbuf;			  	//数据解码地址
	uint16_t *color_bmp;
	uint8_t biCompression=0;		//记录压缩方式
	
	uint16_t rowcnt;				//一次读取的行数
	uint16_t rowlen;	  		 	//水平方向字节数  
	uint16_t rowpix=0;			//水平方向像素数	  
	uint8_t rowadd;				//每行填充字节数

	BITMAPINFO *pbmp;   	//临时指针		 
	//得到窗体尺寸
	picinfo.S_Height=height;
	picinfo.S_Width=width;
		
	databuf=(uint8_t*)m_malloc(readlen);
	if(databuf==NULL )
	{		 
		m_free(databuf);
		return 1;				
	} 	 
	
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_rb),
	};
	
	f_bmp = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

	//if(res != NULL)//打开成功.
	{ 
		br = mp_stream_posix_read(f_bmp, &databuf[0], sizeof(BITMAPINFO));//读出BITMAPINFO信息 

		pbmp=(BITMAPINFO*)databuf;					//得到BMP的头部信息   
		color_byte=pbmp->bmiHeader.biBitCount/8;	//彩色位 16/24/32  
		biCompression=pbmp->bmiHeader.biCompression;//压缩方式
		picinfo.ImgHeight=pbmp->bmiHeader.biHeight;	//得到图片高度
		picinfo.ImgWidth=pbmp->bmiHeader.biWidth;  	//得到图片宽度   
		//水平像素必须是4的倍数!!
		if((picinfo.ImgWidth*color_byte)%4)
			rowlen=((picinfo.ImgWidth*color_byte)/4+1)*4;
		else 
			rowlen=picinfo.ImgWidth*color_byte;
		
		rowadd=rowlen-picinfo.ImgWidth*color_byte;	//每行填充字节数
 		//开始解码BMP   
		color=0;//颜色清空	 													 

		ty=picinfo.ImgHeight-1;
		
		if(picinfo.ImgWidth <= picinfo.S_Width && picinfo.ImgHeight <= picinfo.S_Height)
		{  							   
			rowcnt=readlen/rowlen;						//一次读取的行数
			readlen=rowcnt*rowlen;						//一次读取的字节数
			rowpix=picinfo.ImgWidth;					//水平像素数就是宽度 
			mp_stream_posix_lseek(f_bmp, pbmp->bmfHeader.bfOffBits-br, MP_SEEK_CUR);		//偏移到数据起始位置 	 
			color_bmp = (uint16_t*)m_malloc(rowpix);
			if(color_bmp==NULL )
			{		 
				m_free(color_bmp);
				return 1;				
			} 	
			while(1)
			{	     
				br = mp_stream_posix_read(f_bmp, &databuf[0], readlen);
				
				bmpbuf=databuf;									//数据首地址  
				if(br!=readlen)
				{
					rowcnt=br/rowlen;				//最后剩下的行数
				}
				switch(color_byte)
				{
				case 2: //16bit BMP
					for(j=0;j<rowcnt;j++)//每次读到的行数
					{
						if(biCompression==BI_RGB)//RGB:5,5,5
						{
							for(i=0;i<rowpix;i++)
							{
								color=((uint16_t)*bmpbuf&0X1F);			//R
								color+=(((uint16_t)*bmpbuf++)&0XE0)<<1; 	//G
		 						color+=((uint16_t)*bmpbuf++)<<9;  	    //R,G	 
								color_bmp[i] = color;
							}
						}else  //RGB 565
						{
							for(i=0; i < rowpix; i++)
							{				
								color_bmp[i] = *bmpbuf++;
								color_bmp[i] += (((uint16_t)*bmpbuf++)<<8);
							}
						}
						pic_phy.fillcolor(x, y+ty, rowpix, 1, color_bmp);//color fill 
						bmpbuf+=rowadd;//跳过填充区
						ty--;
					}
				break;
				case 3: //24bit BMP
					for(j=0; j<rowcnt; j++)	//每次读到的行数
					{
						for(i=0;i<rowpix;i++)//写一行像素
						{
							color=(*bmpbuf++)>>3;		   		 	//B
							color+=((uint16_t)(*bmpbuf++)<<3)&0X07E0;	//G
							color+=(((uint16_t)*bmpbuf++)<<8)&0XF800;	//R
							color_bmp[i] = color;
						}
						pic_phy.fillcolor(x, y+ty, rowpix, 1, color_bmp);//color fill 
						bmpbuf+=rowadd;//跳过填充区
						ty--;
					}
				break;
				
				default:
				break;
				}

				if(br!=readlen||res)break;	 
			}	 
			
			m_free(color_bmp);
		}	
		mp_stream_close(f_bmp);//关闭文件      
	}//else res=1;//图片尺寸错误	
	
	m_free(databuf);	
	
	return res;
}            
#endif


