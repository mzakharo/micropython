#include "mp3play.h"
#include "mp3dec.h"

#include <stdio.h>
#include <string.h>
#include "py/mphal.h"
#include "py/runtime.h"
#include "pin.h"
#include "i2c.h"
#include "irq.h"
#include "genhdr/pins.h"
#include "dma.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include <errno.h> // used by mp_is_nonblocking_error
#include "py/nlr.h"
#include "py/gc.h"
#include "shared/runtime/pyexec.h"
#include "gccollect.h"
#include "py/ringbuf.h"
#include "py/objstr.h"
#include "py/objlist.h"
#include "bufhelper.h"

#if MICROPY_ENABLE_MP3

#include "global.h"
#if MICROPY_HW_WM8978
#include "wm8978.h"
#endif

#if (MICROPY_HW_GT1151 && MICROPY_HW_LCD43M)
#include "gt1151.h"
#include "lcd43m.h"
#endif
#if MICROPY_ENABLE_TOUCH
#include "gui_button.h"
#include "modtouch.h"
#endif

__mp3ctrl * mp3ctrl; 
volatile uint8_t mp3transferend=0;
volatile uint8_t mp3witchbuf=0;	

void mp3_tx_callback(void) 
{
	uint16_t i;
	if(DMA1_Stream4->CR&(1<<19)){
		mp3witchbuf=0;
		if((audiodev.status&0X01)==0 || audiodev.status == 0){
			for(i=0;i<2304*2;i++)audiodev.i2sbuf1[i]=0;
		}
	}else{
		mp3witchbuf=1;
		if((audiodev.status&0X01)==0 || audiodev.status == 0){
			for(i=0;i<2304*2;i++)audiodev.i2sbuf2[i]=0;
		}
	} 

	mp3transferend=1;
} 

void mp3_fill_buffer(uint16_t* buf,uint16_t size,uint8_t nch)
{
	uint16_t i; 
	uint16_t *p;
	while(mp3transferend==0);
	mp3transferend=0;
	if(mp3witchbuf==0){
		p=(uint16_t*)audiodev.i2sbuf1;
	}else {
		p=(uint16_t*)audiodev.i2sbuf2;
	}
	if(nch==2)for(i=0;i<size;i++)p[i]=buf[i];
	else{
		for(i=0;i<size;i++){
			p[2*i]=buf[i];
			p[2*i+1]=buf[i];
		}
	}
} 

//解析ID3V1 
uint8_t mp3_id3v1_decode(uint8_t* buf,__mp3ctrl *pctrl)
{
	ID3V1_Tag *id3v1tag;
	id3v1tag=(ID3V1_Tag*)buf;
	if (strncmp("TAG",(char*)id3v1tag->id,3)==0){
		if(id3v1tag->title[0])strncpy((char*)pctrl->title,(char*)id3v1tag->title,30);
		if(id3v1tag->artist[0])strncpy((char*)pctrl->artist,(char*)id3v1tag->artist,30); 
	}else return 1;
	return 0;
}
//解析ID3V2 
uint8_t mp3_id3v2_decode(uint8_t* buf,uint32_t size,__mp3ctrl *pctrl)
{
	ID3V2_TagHead *taghead;
	ID3V23_FrameHead *framehead; 
	uint32_t t;
	uint32_t tagsize;	//tag大小
	uint32_t frame_size;	//帧大小 
	taghead=(ID3V2_TagHead*)buf; 
	if(strncmp("ID3",(const char*)taghead->id,3)==0){
		tagsize=((uint32_t)taghead->size[0]<<21)|((uint32_t)taghead->size[1]<<14)|((uint16_t)taghead->size[2]<<7)|taghead->size[3];
		pctrl->datastart=tagsize;	
		if(tagsize>size)tagsize=size;	
		if(taghead->mversion<3){
			printf("not supported mversion!\r\n");
			return 1;
		}
		t=10;
		while(t<tagsize){
			framehead=(ID3V23_FrameHead*)(buf+t);
			frame_size=((uint32_t)framehead->size[0]<<24)|((uint32_t)framehead->size[1]<<16)|((uint32_t)framehead->size[2]<<8)|framehead->size[3];
 			if (strncmp("TT2",(char*)framehead->id,3)==0||strncmp("TIT2",(char*)framehead->id,4)==0){
				strncpy((char*)pctrl->title,(char*)(buf+t+sizeof(ID3V23_FrameHead)+1),AUDIO_MIN(frame_size-1,MP3_TITSIZE_MAX-1));
			}
 			if (strncmp("TP1",(char*)framehead->id,3)==0||strncmp("TPE1",(char*)framehead->id,4)==0){
				strncpy((char*)pctrl->artist,(char*)(buf+t+sizeof(ID3V23_FrameHead)+1),AUDIO_MIN(frame_size-1,MP3_ARTSIZE_MAX-1));
			}
			t+=frame_size+sizeof(ID3V23_FrameHead);
		} 
	}else pctrl->datastart=0;//不存在ID3,mp3数据是从0开始
	return 0;
} 

uint8_t mp3_get_info	(FATFS *fs, const char *pname , __mp3ctrl* pctrl)
{
	HMP3Decoder decoder;
	MP3FrameInfo frame_info;
	MP3_FrameXing* fxing;
	MP3_FrameVBRI* fvbri;
	FIL*fmp3;
	uint8_t *buf;
	UINT br;
	uint8_t res;
	int offset=0;
	uint32_t p;
	short samples_per_frame;	
	uint32_t totframes;	
	
	fmp3=m_malloc(sizeof(FIL)); 
	buf=m_malloc(5*1024);	
	if(fmp3&&buf){ 		
		f_open(fs,fmp3,pname,FA_READ);//打开文件
		res=f_read(fmp3,(char*)buf,5*1024,&br);
		if(res==0){  
			mp3_id3v2_decode(buf,br,pctrl);	
			f_lseek(fmp3,fmp3->obj.objsize-128);
			f_read(fmp3,(char*)buf,128,&br);
			mp3_id3v1_decode(buf,pctrl);	
			decoder=MP3InitDecoder(); 		
			f_lseek(fmp3,pctrl->datastart);	
			f_read(fmp3,(char*)buf,5*1024,&br);
 			offset=MP3FindSyncWord(buf,br);	
			if(offset>=0&&MP3GetNextFrameInfo(decoder,&frame_info,&buf[offset])==0)	
			{ 
				p=offset+4+32;
				fvbri=(MP3_FrameVBRI*)(buf+p);
				if(strncmp("VBRI",(char*)fvbri->id,4)==0)
				{
					if (frame_info.version==MPEG1)samples_per_frame=1152;
					else samples_per_frame=576;
 					totframes=((uint32_t)fvbri->frames[0]<<24)|((uint32_t)fvbri->frames[1]<<16)|((uint16_t)fvbri->frames[2]<<8)|fvbri->frames[3];
					pctrl->totsec=totframes*samples_per_frame/frame_info.samprate;
				}else	{  
					if (frame_info.version==MPEG1){
						p=frame_info.nChans==2?32:17;
						samples_per_frame = 1152;	
					}else{
						p=frame_info.nChans==2?17:9;
						samples_per_frame=576;
					}
					p+=offset+4;
					fxing=(MP3_FrameXing*)(buf+p);
					if(strncmp("Xing",(char*)fxing->id,4)==0||strncmp("Info",(char*)fxing->id,4)==0){
						if(fxing->flags[3]&0X01){
							totframes=((uint32_t)fxing->frames[0]<<24)|((uint32_t)fxing->frames[1]<<16)|((uint16_t)fxing->frames[2]<<8)|fxing->frames[3];
							pctrl->totsec=totframes*samples_per_frame/frame_info.samprate;
						}else{
							pctrl->totsec=fmp3->obj.objsize/(frame_info.bitrate/8);
						} 
					}else{
						pctrl->totsec=fmp3->obj.objsize/(frame_info.bitrate/8);
					}
				} 
				pctrl->bitrate=frame_info.bitrate;		
				mp3ctrl->samplerate=frame_info.samprate;
				if(frame_info.nChans==2)mp3ctrl->outsamples=frame_info.outputSamps;
				else mp3ctrl->outsamples=frame_info.outputSamps*2; 

				#if 0
					printf("title:%s\r\n",mp3ctrl->title); 
					printf("artist:%s\r\n",mp3ctrl->artist); 
					printf("totsec:%ld s\r\n",mp3ctrl->totsec);	
					printf("bitrate:%ld bps\r\n",mp3ctrl->bitrate);	
					printf("samplerate:%ld\r\n", mp3ctrl->samplerate);	
					printf("outsamples:%d\r\n",mp3ctrl->outsamples); 
					printf("datastart:%ld\r\n", mp3ctrl->datastart);
					mp_hal_delay_ms(10);
				#endif 
			}else res=0XFE;
			MP3FreeDecoder(decoder);//释放内存		
		} 
		f_close(fmp3);
	}else{
		res=0XFF;
		
	} 
	
	m_free(fmp3);
	m_free(buf);	
	return res;	
}  

void mp3_get_curtime(FIL*fx,__mp3ctrl *mp3x)
{
	uint32_t fpos=0;  	 
	if(fx->fptr>mp3x->datastart)fpos=fx->fptr-mp3x->datastart;
	mp3x->cursec=fpos*mp3x->totsec/(fx->obj.objsize-mp3x->datastart);	
}

uint8_t mp3_play_song(const char* fname)
{ 
	HMP3Decoder mp3decoder=NULL;
	MP3FrameInfo mp3frameinfo;
	FRESULT res;
	uint8_t* buffer;

	uint8_t* readptr;	
	int offset=0;
	int outofdata=0;
	int bytesleft=0;
	UINT br=0; 
	int err=0;  

	const char *file_path = mp_obj_str_get_str(get_path(fname ,&res));
	mp_vfs_mount_t *vfs = MP_STATE_VM(vfs_mount_table);

	if(res == 1){
	vfs = vfs->next;
	}
	fs_user_mount_t *vfs_fat = MP_OBJ_TO_PTR(vfs->obj);

 	mp3ctrl=m_malloc(sizeof(__mp3ctrl)); 
	
	audiodev.file=(FIL*)m_malloc(sizeof(FIL));
	audiodev.tbuf=m_malloc(2304*2);
	buffer=m_malloc(MP3_FILE_BUF_SZ);
	#if defined(STM32F4) || defined(STM32F7)
	audiodev.i2sbuf1=m_malloc(2304*2);
	audiodev.i2sbuf2=m_malloc(2304*2);
	if(!mp3ctrl||!audiodev.i2sbuf1||!audiodev.i2sbuf2||!audiodev.tbuf){
	#elif defined(STM32H7)

	if(!mp3ctrl){
	#endif
		m_free(mp3ctrl);
		m_free(buffer);
		m_free(audiodev.file);
		m_free(audiodev.tbuf);
		
		#if defined(STM32F4) || defined(STM32F7)
		m_free(audiodev.i2sbuf1);
		m_free(audiodev.i2sbuf2);
		#endif
		
		mp_raise_ValueError(MP_ERROR_TEXT("mp3 malloc error"));
		return 0;	
	} 

	memset(audiodev.i2sbuf1,0,2304*2);
	memset(audiodev.i2sbuf2,0,2304*2);

	memset(mp3ctrl,0,sizeof(__mp3ctrl)); 

	res=mp3_get_info(&vfs_fat->fatfs,file_path,mp3ctrl);  
	if(res==0){ 
		WM8978_I2S_CFG(2,0);
 
		#if defined(STM32F4) || defined(STM32F7)
		audio_init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED); 
		I2S2_SampleRate_Set(mp3ctrl->samplerate);	
		#elif defined(STM32H7)
		audio_init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED,mp3ctrl->samplerate); 
		#endif
	
		I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,mp3ctrl->outsamples);
		i2s_tx_callback=mp3_tx_callback;	
		audiodev.status=0;
		__HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);
		#if defined(STM32H7)
		SPI2->CR1 &= ~SPI_CR1_CSTART; //启动i2s
		SPI2->CR1 &= ~SPI_CR1_SPE;
		#endif
		mp3decoder=MP3InitDecoder();
		res=f_open(&vfs_fat->fatfs,audiodev.file,file_path,FA_READ);
	}

	if(res==0&&mp3decoder!=0){ 
		f_lseek(audiodev.file,mp3ctrl->datastart);	
		audiodev.status=3;

		DMA1_Stream4->CR|=1<<0;
		__HAL_DMA_ENABLE(&I2S2_TXDMA_Handler);
		#if defined(STM32H7)
		SPI2->CR1 |= SPI_CR1_SPE;
		SPI2->CR1 |= SPI_CR1_CSTART; //启动i2s
		#endif
		while(res==0){
			readptr=buffer;	
			offset=0;	
			outofdata=0;	
			bytesleft=0;	
			res=f_read(audiodev.file,buffer,MP3_FILE_BUF_SZ,&br);
			if(res){
			  audiodev.status=0;
				mp_raise_ValueError(MP_ERROR_TEXT("mp3 error"));
				break;
			}
			if(br==0)	{
				audiodev.status=0;
				break;
			}
			bytesleft+=br;	
			err=0;			
			while(!outofdata){
				offset=MP3FindSyncWord(readptr,bytesleft);
				if(offset<0)	{ 
					outofdata=1;
				}else	{
					readptr+=offset;
					bytesleft-=offset;	
					err=MP3Decode(mp3decoder,&readptr,&bytesleft,(short*)audiodev.tbuf,0);
					if(err!=0){
						mp_raise_ValueError(MP_ERROR_TEXT("mp3 decode error"));
						break;
					}else{
						MP3GetLastFrameInfo(mp3decoder,&mp3frameinfo);
						if(mp3ctrl->bitrate!=mp3frameinfo.bitrate){	
							mp3ctrl->bitrate=mp3frameinfo.bitrate; 
						}
						mp3_fill_buffer((uint16_t*)audiodev.tbuf,mp3frameinfo.outputSamps,mp3frameinfo.nChans);
					}
					if(bytesleft<MAINBUF_SIZE*2){ 
						memmove(buffer,readptr,bytesleft);
						f_read(audiodev.file,buffer+bytesleft,MP3_FILE_BUF_SZ-bytesleft,&br);

						if(br<MP3_FILE_BUF_SZ-bytesleft){
							memset(buffer+bytesleft+br,0,MP3_FILE_BUF_SZ-bytesleft-br); 
						}
						bytesleft=MP3_FILE_BUF_SZ;  
						readptr=buffer; 
					} 
					#if MICROPY_ENABLE_TOUCH 
					#if MICROPY_GUI_BUTTON	
					if(touch_is_init){
						gui_read_points();
						button_task();
					}
					#endif
					#endif
					if(audiodev.status == 0){
						res = 1;
						break;
					}
					while((audiodev.status&0X01)==0){
						mp_hal_delay_ms(100);
						#if MICROPY_ENABLE_TOUCH
						#if MICROPY_GUI_BUTTON
						if(touch_is_init){
							gui_read_points();
							button_task();
						}
						#endif
						#endif
						if(audiodev.status == 0){
							res=1;
							outofdata=1;
							break;
						}
					}
				}	
			} 
		}
	}
	audiodev.status=0;
	__HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);
	#if defined(STM32H7)
	SPI2->CR1 &= ~SPI_CR1_CSTART; //启动i2s
	SPI2->CR1 &= ~SPI_CR1_SPE;
	#endif
	f_close(audiodev.file);
	MP3FreeDecoder(mp3decoder);
	m_free(mp3ctrl);
	
	m_free(audiodev.file);
	#if defined(STM32F4) || defined(STM32F7)
	m_free(audiodev.i2sbuf1);
	m_free(audiodev.i2sbuf2);
	#endif
	m_free(audiodev.tbuf);
	m_free(buffer);
	m_free(audiodev.tbuf);
	return res;
}

#endif


