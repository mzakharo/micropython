
#ifndef MICROPY_INCLUDED_STM32_VIDEO_H
#define MICROPY_INCLUDED_STM32_VIDEO_H

#if MICROPY_ENABLE_VIDEO

#include "avi.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#define AVI_AUDIO_BUF_SIZE    1024*5
#define AVI_VIDEO_BUF_SIZE    1024*60	

//视频播放控制器
typedef struct
{    
	volatile uint8_t status;			//bit0:0,暂停播放;1,继续播放
						//bit1:0,快进/快退中;1,继续播放
						//其他,保留
	volatile uint8_t play_while;

	FIL *file;
	volatile uint8_t i2splaybuf;
	uint8_t* i2sbuf[4]; 
}__attribute__((packed))  __videodev; 

extern __videodev videodev;//视频播放控制器

extern void video_play(void);
extern uint8_t video_play_mjpeg(const 	    char *pname); 
extern void video_time_show(FIL *favi,AVI_INFO *aviinfo);

extern void video_bmsg_show(uint8_t* name,uint16_t index,uint16_t total);
extern uint16_t video_get_tnum(uint8_t *path);
extern uint8_t video_seek(FIL *favi,AVI_INFO *aviinfo,uint8_t *mbuf);

//===========================================================================================
extern const mp_obj_type_t video_video_type;



#endif

#endif // __VIDEO_H__
