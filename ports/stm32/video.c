
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


#include "py/mperrno.h" // used by mp_is_nonblocking_error
#include "py/nlr.h"
#include "py/gc.h"

#if MICROPY_ENABLE_VIDEO
#include "video.h"
#include "wm8978.h"

#include "global.h" 

#if (MICROPY_HW_GT1151 && MICROPY_HW_LCD43M)
#include "gt1151.h"
#include "lcd43m.h"
#endif

#if MICROPY_ENABLE_TOUCH
#include "gui_button.h"
#include "modtouch.h"
#endif

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#if MICROPY_PY_HJPEG_DECODE
#include "hjpgd.h"
#endif

#ifdef MICROPY_PY_PICLIB
#include "piclib.h"
#endif

#include "mjpeg.h" 
#include "avi.h"
#include "video.h"
//===================================================================================================================

__videodev videodev;		//视频播放控制器

//===================================================================================================================
//音频数据I2S DMA传输回调函数
void audio_i2s_dma_callback(void) 
{
	videodev.i2splaybuf++;
	if(videodev.i2splaybuf>3)videodev.i2splaybuf=0;

	if(DMA1_Stream4->CR&(1<<19)){
		DMA1_Stream4->M0AR=(uint32_t)videodev.i2sbuf[videodev.i2splaybuf];//指向下一个buf
	}else {
		DMA1_Stream4->M1AR=(uint32_t)videodev.i2sbuf[videodev.i2splaybuf];//指向下一个buf
	}
}
//==============================================================================================================
typedef struct _video_video_obj_t {
    mp_obj_base_t base;
    mp_obj_t callback;
} video_obj_t;

STATIC void video_callback(video_obj_t *self) ;

//-------------------------------------------------------------------------------------------------------------------
uint8_t video_play_mjpeg(const char *pname)
{   

	uint8_t* framebuf=NULL;		//视频解码buf	 

	uint8_t* pbuf=0;			//buf指针   
	uint8_t play_flag = 0;
	uint8_t  res=0;
	uint16_t offset=0; 
	UINT	nr;  
	uint8_t i2ssavebuf; 
	uint16_t videoheight = 800;	//视频显示区域高度
	int yoff=0;

	videodev.file=(FIL*)m_malloc(sizeof(FIL)); 			//申请videodev.file内存 
	
	framebuf=m_malloc(AVI_VIDEO_BUF_SIZE);				//申请视频buf
	videodev.i2sbuf[0]=m_malloc(AVI_AUDIO_BUF_SIZE);	//申请音频内存
	videodev.i2sbuf[1]=m_malloc(AVI_AUDIO_BUF_SIZE);	//申请音频内存
	videodev.i2sbuf[2]=m_malloc(AVI_AUDIO_BUF_SIZE);	//申请音频内存
	videodev.i2sbuf[3]=m_malloc(AVI_AUDIO_BUF_SIZE);	//申请音频内存
	if(!videodev.i2sbuf[3]||!framebuf){ 
		mp_raise_ValueError(MP_ERROR_TEXT("video play memory error!"));
		res=0X21;
	}

	if(res == 0){
		memset(videodev.i2sbuf[0],0,AVI_AUDIO_BUF_SIZE);
		memset(videodev.i2sbuf[1],0,AVI_AUDIO_BUF_SIZE); 
		memset(videodev.i2sbuf[2],0,AVI_AUDIO_BUF_SIZE);
		memset(videodev.i2sbuf[3],0,AVI_AUDIO_BUF_SIZE);  
	}

	const char *file_path = mp_obj_str_get_str(get_path(pname,&res));
	mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);
	 if(res == 1){
		 vfs = vfs->next;
	 }
	 fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);

	res=f_open(&vfs_fat->fatfs,videodev.file,file_path,FA_READ);
	if(res==0){
		pbuf=framebuf;			
		res=f_read(videodev.file,pbuf,AVI_VIDEO_BUF_SIZE,&nr);
		if(res){
			mp_raise_ValueError(MP_ERROR_TEXT("read video file error!"));
		} 

		//开始avi解析
		res=avi_init(pbuf,AVI_VIDEO_BUF_SIZE);
		if(res){
			printf("res:%d\r\n",res);
			mp_raise_ValueError(MP_ERROR_TEXT("avi init error!"));
		} 	
		if(avix.Height>videoheight||avix.Width>lcddev.width){
			res=0X22;
		mp_raise_ValueError(MP_ERROR_TEXT("avi size error!"));
		}
		offset=avi_srarch_id(pbuf,AVI_VIDEO_BUF_SIZE,"movi");
		avi_get_streaminfo(pbuf+offset+4);
		f_lseek(videodev.file,offset+12);

		res=mjpegdec_init((lcddev.width-avix.Width)>>1,((lcddev.height-avix.Height)>>1)+yoff);

		if(avix.SampleRate){
			WM8978_I2S_CFG(2,0);	//飞利浦标准,16位数据长度
			wm8978_adda_cfg(1,0);			//开启DAC
			#if defined(STM32F4) || defined(STM32F7)
			audio_init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED);
			I2S2_SampleRate_Set(avix.SampleRate);	//设置采样率
			#elif defined(STM32H7)
			audio_init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED,avix.SampleRate); 
			#endif

			I2S2_TX_DMA_Init(videodev.i2sbuf[1],videodev.i2sbuf[2],avix.AudioBufSize/2);
			i2s_tx_callback=audio_i2s_dma_callback;
			videodev.i2splaybuf=0;
			i2ssavebuf=0; 
			__HAL_DMA_ENABLE(&I2S2_TXDMA_Handler);
			#if defined(STM32H7)
			SPI2->CR1 |= SPI_CR1_SPE;
			SPI2->CR1 |= SPI_CR1_CSTART; //启动i2s
			#endif
		}

		videodev.status = 3;
		DMA1_Stream4->CR|=1<<0;	
		videodev.play_while = 1;
		video_obj_t *self;
		self = MP_STATE_PORT(video_obj_rom);

		while(videodev.play_while){
			if(videodev.status&(1<<0)&&videodev.status&(1<<1)){
				play_flag = 1;
				if(avix.StreamID==AVI_VIDS_FLAG){
					pbuf=framebuf;
					f_read(videodev.file,pbuf,avix.StreamSize+8,&nr);
					res=mjpegdec_decode(pbuf,avix.StreamSize);

					if(res){
						mp_raise_ValueError(MP_ERROR_TEXT("avi decode error!"));
					} 
					if(0){
					#if MICROPY_ENABLE_TOUCH 
					}
					else if(touch_is_init){
						#if MICROPY_GUI_BUTTON
						gui_read_points();
						button_task();
						#endif
					#endif
					}else{
						mp_hal_delay_us(100);
					}

					video_callback(self);
				}
				else{		  
					i2ssavebuf++;
					if(i2ssavebuf>3)i2ssavebuf=0;
					do{
						nr=videodev.i2splaybuf;
						if(nr)nr--;
						else nr=3; 
					}while(i2ssavebuf==nr);
					f_read(videodev.file,videodev.i2sbuf[i2ssavebuf],avix.StreamSize+8,&nr);
					pbuf=videodev.i2sbuf[i2ssavebuf];  
				} 
			}else{
				if(0){
					#if MICROPY_ENABLE_TOUCH 
					}
				else if(touch_is_init){
					#if MICROPY_GUI_BUTTON
								mp_hal_delay_ms(10);
								gui_read_points();
								button_task();
					#endif
					#endif
				}else{
							mp_hal_delay_ms(500);
				}
				video_callback(self);
			}
			if(play_flag && videodev.status&(1<<0)&&videodev.status&(1<<1)){
				if(avi_get_streaminfo(pbuf+avix.StreamSize)){
					break; 
				}
			}
		}
		__HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);
		mjpegdec_free();	//释放内存
		f_close(videodev.file); 
	}

	m_free(videodev.i2sbuf[0]);
	m_free(videodev.i2sbuf[1]);
	m_free(videodev.i2sbuf[2]);
	m_free(videodev.i2sbuf[3]);
	m_free(framebuf);

	m_free(videodev.file);

	return res;
}

//=====================================================================================================================
STATIC void video_callback(video_obj_t *self) {
    if (self->callback != mp_const_none) {
      mp_sched_lock();
        gc_lock();
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_1(self->callback, self);
            nlr_pop();
        } else {
            // Uncaught exception; disable the callback so it doesn't run again.
            self->callback = mp_const_none;
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
        mp_sched_unlock();
    }
}
//-----------------------------------------------------------------------------------
STATIC mp_obj_t video_video_callback(mp_obj_t self_in, mp_obj_t callback) {
    video_obj_t *self = self_in;
    if (callback == mp_const_none) {
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
        self->callback = callback;
    } else {
		nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError
			,MP_ERROR_TEXT("callback must be None or a callable object")));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(video_video_callback_obj, video_video_callback);
STATIC char FileName[50]={0};
//=====================================================================================================
STATIC mp_obj_t video_video_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

		//printf("play name:%s\n",FileName);
		const char *type = mp_obj_str_get_str(file_type((const char *)FileName));
		if(strncmp(type , "avi" , 3) == 0 ||strncmp(type , "AVI" , 3) == 0) {
			wm8978_adda_cfg(1,0);
			video_play_mjpeg(FileName);
		}else{
			mp_raise_ValueError(MP_ERROR_TEXT("play video type error"));
		}

		return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_play_obj, 0, video_video_play);

//-----------------------------------------------------------------------------------
STATIC mp_obj_t video_video_continue_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
		wm8978_adda_cfg(1,0);
		videodev.status = 3;
		return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_continue_play_obj, 0, video_video_continue_play);
//-----------------------------------------------------------------------------------

STATIC mp_obj_t video_video_pause(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    videodev.status&=~(1<<0);
		wm8978_adda_cfg(0,0);
		return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_pause_obj, 0, video_video_pause);
//-----------------------------------------------------------------------------------
STATIC mp_obj_t video_video_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    videodev.play_while = 0;
		__HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);
		wm8978_adda_cfg(0,0);
		return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_stop_obj, 0, video_video_stop);
//-----------------------------------------------------------------------------------
STATIC mp_obj_t video_video_volume(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  STATIC const mp_arg_t volume_args[] = {
    { MP_QSTR_vol,    MP_ARG_INT, {.u_int = 80} }, 
  };

  mp_arg_val_t args[MP_ARRAY_SIZE(volume_args)];
  mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(volume_args), volume_args, args);

	uint8_t vul = (uint8_t)(args[0].u_int * 0.63);
	wm8978_spk_vol(vul);
	wm8978_hspk_vol(vul,vul);
  return mp_obj_new_int(vul);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_volume_obj, 0, video_video_volume);

//----------------------------------------------------------------------------------------------------

STATIC mp_obj_t video_video_load(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)  {

		STATIC const mp_arg_t video_args[] = {
			{ MP_QSTR_filename, 		MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} }, 
		};
		mp_arg_val_t args[MP_ARRAY_SIZE(video_args)];
		mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(video_args), video_args, args);

		if(args[0].u_obj !=MP_OBJ_NULL) 
		{
			mp_buffer_info_t bufinfo;
			if (mp_obj_is_int(args[0].u_obj)) {
				mp_raise_ValueError(MP_ERROR_TEXT("video name parameter error"));
			} else {
					mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);
					//printf("load name:%s",(char *)bufinfo.buf);
          const char *type = mp_obj_str_get_str(file_type((const char *)bufinfo.buf));
          if(strncmp(type , "avi" , 3) == 0 ||strncmp(type , "AVI" , 3) == 0) {
						memset(FileName, '\0', sizeof(FileName));
						strncpy(FileName,(char *)bufinfo.buf,bufinfo.len);
          }else{
            mp_raise_ValueError(MP_ERROR_TEXT("video type error"));
          }
			}
		}
		else{
			 mp_raise_ValueError(MP_ERROR_TEXT("video load name is empty"));
		}

		return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(video_video_load_obj, 1, video_video_load);

//----------------------------------------------------------------------------------
//==============================================================================
STATIC void video_video_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print,"video fun");
}
//----------------------------------------------------------------------------------
STATIC mp_obj_t video_video_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	// check arguments
	mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, false);

	wm8978_init();
	wm8978_spk_vol(50);
	wm8978_hspk_vol(50,50);
	wm8978_adda_cfg(0,0);
  video_obj_t *video_obj;
  video_obj = m_new_obj(video_obj_t);
  video_obj->base.type = &video_video_type;
  video_obj->callback = mp_const_none;
	MP_STATE_PORT(video_obj_rom)= video_obj;
  
	return MP_OBJ_FROM_PTR(video_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t video_video_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_video) },

	{ MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&video_video_load_obj) },
	{ MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&video_video_play_obj) },
	{ MP_ROM_QSTR(MP_QSTR_continue_play), MP_ROM_PTR(&video_video_continue_play_obj) },
	{ MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&video_video_pause_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&video_video_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_volume), MP_ROM_PTR(&video_video_volume_obj)},

  { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&video_video_callback_obj) },

};
STATIC MP_DEFINE_CONST_DICT(video_video_locals_dict,video_video_locals_dict_table);

const mp_obj_type_t video_video_type = {
    { &mp_type_type },
    .name = MP_QSTR_video,
    .print = video_video_print,
    .make_new = video_video_make_new,
    .locals_dict = (mp_obj_dict_t *)&video_video_locals_dict,
};

#endif

