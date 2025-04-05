
#ifndef MICROPY_INCLUDED_STM32_MP3PLAY_H_H
#define MICROPY_INCLUDED_STM32_MP3PLAY_H_H

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#if MICROPY_ENABLE_MP3

#define MP3_TITSIZE_MAX		50	
#define MP3_ARTSIZE_MAX		50		
#define MP3_FILE_BUF_SZ    5*1024	

#ifndef AUDIO_MIN			
#define AUDIO_MIN(x,y)	((x)<(y)? (x):(y))
#endif


//ID3V1 标签 
typedef struct 
{
	uint8_t id[3];		   	
	uint8_t title[30];	
	uint8_t artist[30];
	uint8_t year[4];	
	uint8_t comment[30];	
	uint8_t genre;
}__attribute__((packed)) ID3V1_Tag;

//ID3V2 标签头 
typedef struct 
{
    uint8_t id[3];	
    uint8_t mversion;	
    uint8_t sversion;	
    uint8_t flags;
    uint8_t size[4];
}__attribute__((packed)) ID3V2_TagHead;

//ID3V2.3 版本帧头
typedef struct 
{
    uint8_t id[4];
    uint8_t size[4];
    uint16_t flags;	
}__attribute__((packed)) ID3V23_FrameHead;

//MP3 Xing帧信息
typedef struct 
{
    uint8_t id[4];
    uint8_t flags[4];
    uint8_t frames[4];
	uint8_t fsize[4];	
}__attribute__((packed)) MP3_FrameXing;
 
typedef struct 
{
    uint8_t id[4];
	uint8_t version[2];	
	uint8_t delay[2];	
	uint8_t quality[2];
	uint8_t fsize[4];	
	uint8_t frames[4];
}__attribute__((packed)) MP3_FrameVBRI;


//MP3控制结构体
typedef struct 
{
    uint8_t title[MP3_TITSIZE_MAX];	//歌曲名字
    uint8_t artist[MP3_ARTSIZE_MAX];	//艺术家名字
    uint32_t totsec ;	
    uint32_t cursec ;	
	
    uint32_t bitrate;
	uint32_t samplerate;
	uint16_t outsamples;	
	
	uint32_t datastart;
}__attribute__((packed)) __mp3ctrl;

extern __mp3ctrl * mp3ctrl;
uint8_t mp3_play_song(const char* fname);
#endif

#endif



























