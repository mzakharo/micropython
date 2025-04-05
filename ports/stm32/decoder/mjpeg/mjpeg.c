#include "mjpeg.h" 

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
#include "mpu.h"
#include "systick.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#include "lcd43m.h"
#endif

#if MICROPY_HW_LTDC_LCD
#include "ltdc.h"
#endif

#include <stdio.h>
#include <setjmp.h>

struct jpeg_decompress_struct *cinfo;
struct my_error_mgr *jerr;

uint16_t *outlinebuf;		//输出行缓存,≥图像宽度*2字节
uint16_t outlinecnt;			//输出行计数器
uint8_t *jpegbuf;			//jpeg数据缓存指针
uint32_t jbufsize;			//jpeg buf大小
uint16_t imgoffx,imgoffy;	//图像在x,y方向的偏移量

//简单快速的内存分配,以提高速度
#define MJPEG_MAX_MALLOC_SIZE 		38*1024			//最大可以分配35K字节

uint8_t *jmembuf;			//mjpeg解码的 内存池
uint32_t jmempos;			//内存池指针

//mjpeg申请内存
void* mjpeg_malloc(uint32_t num)
{
	uint32_t curpos=jmempos; //此次分配的起始地址
 	jmempos+=num;		//下一次分配的起始地址 
	if(jmempos>38*1024) 
	{
		printf("mem error:%ld,%ld\n",curpos,num);
	}
	return (void *)&jmembuf[curpos];	//返回申请到的内存首地址
}  
//----------------------------------------------------------------------------------
//错误退出
static void my_error_exit(j_common_ptr cinfo)
{ 
#if 0
	my_error_ptr myerr=(my_error_ptr) cinfo->err; 
	(*cinfo->err->output_message) (cinfo);	 
	longjmp(myerr->setjmp_buffer, 1);	 
#endif
} 

METHODDEF(void) my_emit_message(j_common_ptr cinfo, int msg_level)
{

	my_error_ptr myerr=(my_error_ptr) cinfo->err;  
    if(msg_level<0)
  	{
  		printf("emit msg:%d\n",msg_level); 
  		//longjmp(myerr->setjmp_buffer, 1);		
  	}
    (void)myerr;
}

//初始化资源,不执行任何操作
static void init_source(j_decompress_ptr cinfo)
{
    //不需要做任何事情.
    return;
} 
//填充输入缓冲区,一次性读取整帧数据
static boolean fill_input_buffer(j_decompress_ptr cinfo)
{  
	if(jbufsize==0)//结束了
	{
		printf("jd read off\n");
        //填充结束符
        jpegbuf[0] = (uint8_t) 0xFF;
        jpegbuf[1] = (uint8_t) JPEG_EOI;
  		cinfo->src->next_input_byte =jpegbuf;
		cinfo->src->bytes_in_buffer = 2; 
	}else
	{
		cinfo->src->next_input_byte =jpegbuf;
		cinfo->src->bytes_in_buffer = jbufsize;
		jbufsize-=jbufsize;
	}
    return TRUE;
}
//在文件里面,跳过num_bytes个数据
static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{ 
    /* Just a dumb implementation for now.  Could use fseek() except
    * it doesn't work on pipes.  Not clear that being smart is worth
    * any trouble anyway --- large skips are infrequent.
    */
    if (num_bytes > 0)
    {
        while(num_bytes>(long) cinfo->src->bytes_in_buffer)
        {
            num_bytes-=(long)cinfo->src->bytes_in_buffer;
            (void)cinfo->src->fill_input_buffer(cinfo);
            /* note we assume that fill_input_buffer will never
            * return FALSE, so suspension need not be handled.
            */
        }
        cinfo->src->next_input_byte += (size_t) num_bytes;
        cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
    }
} 
//在解码结束后,被jpeg_finish_decompress函数调用
static void term_source(j_decompress_ptr cinfo)
{
    //不做任何处理
    return;
}
//初始化jpeg解码数据源
static void jpeg_filerw_src_init(j_decompress_ptr cinfo)
{ 
    if (cinfo->src == NULL)     /* first time for this JPEG object? */
    {
        cinfo->src = (struct jpeg_source_mgr *)
                     (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                              sizeof(struct jpeg_source_mgr)); 
    } 
    cinfo->src->init_source = init_source;
    cinfo->src->fill_input_buffer = fill_input_buffer;
    cinfo->src->skip_input_data = skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    cinfo->src->term_source = term_source;
    cinfo->src->bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    cinfo->src->next_input_byte = NULL; /* until buffer loaded */
} 


//mjpeg 解码初始化
//offx,offy:x,y方向的偏移
//返回值:0,成功;
//       1,失败
uint8_t mjpegdec_init(uint16_t offx,uint16_t offy)
{
	cinfo=m_malloc(sizeof(struct jpeg_decompress_struct));
	jerr=m_malloc(sizeof(struct my_error_mgr));
	jmembuf=m_malloc(MJPEG_MAX_MALLOC_SIZE);//MJPEG解码内存池申请
	if(cinfo==0||jerr==0||jmembuf==0)
	{
		mjpegdec_free();
		return 1;
	}
	#if MICROPY_HW_LTDC_LCD
	outlinebuf = (uint16_t*)((uint32_t)ltdc_framebuf[0] + ltdcdev.pixsize*(lcddev.x_pixel*lcddev.y_pixel)*2);
	#endif

	//保存图像在x,y方向的偏移量
	imgoffx=offx;
	imgoffy=offy; 

	return 0;
}

void mjpegdec_free(void)
{    
	m_free(cinfo);
	m_free(jerr);
	m_free(jmembuf);
}

//解码一副JPEG图片
//buf:jpeg数据流数组
//bsize:数组大小
//返回值:0,成功
//    其他,错误
uint8_t mjpegdec_decode(uint8_t* buf,uint32_t bsize)
{
    JSAMPARRAY buffer;		
    buffer = 0;

	if(bsize==0)return 1;
	jpegbuf=buf;
	jbufsize=bsize;	   
	jmempos=0;//MJEPG解码,重新从0开始分配
	
	cinfo->err=jpeg_std_error(&jerr->pub); 
	jerr->pub.error_exit = my_error_exit; 
	jerr->pub.emit_message = my_emit_message; 
	if(bsize>20*1024)printf("s:%ld\n",bsize); 

	//if (setjmp(jerr->setjmp_buffer)) //错误处理
	{ 
 		//jpeg_abort_decompress(cinfo);
	//	jpeg_destroy_decompress(cinfo); 
	//	return 2;
	} 
	jpeg_create_decompress(cinfo); 
	jpeg_filerw_src_init(cinfo);  
	jpeg_read_header(cinfo, TRUE); 
	cinfo->dct_method = JDCT_IFAST;
	cinfo->do_fancy_upsampling = 0;  
	jpeg_start_decompress(cinfo); 
	
	if(lcddev.type == 2 || lcddev.type == 3){
		outlinecnt=imgoffy;				//设置行位置 
	}else if(lcddev.type == 1){
		LCD_Set_Window(imgoffx,imgoffy,cinfo->output_width,cinfo->output_height);
		LCD43M_REG = WRAMCMD;
	}
	
	while (cinfo->output_scanline < cinfo->output_height) 
	{ 
		jpeg_read_scanlines(cinfo, buffer, 1);
	} 
	
	if(lcddev.type == 1){
		LCD_Set_Window(0,0,lcddev.width,lcddev.height);//恢复窗口
	}
	jpeg_finish_decompress(cinfo); 
	jpeg_destroy_decompress(cinfo);  
	return 0;
}


 













