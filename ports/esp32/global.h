
/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	global.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/23
	* Description 			 :	
	******************************************************************************
**/

#ifndef __GLOBAL_H
#define __GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/stream.h"

extern mp_obj_t get_path(const char *src_path , uint8_t *res );
extern mp_obj_t file_type(const char *fileName);
extern mp_obj_t get_mystr(const char *sour, const char *start,const char *end,uint8_t *ret);


extern FRESULT f_open_helper(FIL *fp, const TCHAR *path, BYTE mode);
extern uint16_t rgb888to565(uint8_t r_color, uint8_t g_color , uint8_t b_color);

extern char* itoa(int num,char* str,int radix);

extern int atoi(const char *nptr);
extern bool check_sys_file(const char *check_file);
//extern int sscanf (const char *s, const char *format, ...);



#ifdef __cplusplus
}
#endif
#endif 


#ifdef __GLOBAL_H
//-----------------------------------------------------------------------
  __attribute__((weak)) uint16_t rgb888to565(uint8_t r_color, uint8_t g_color , uint8_t b_color)
{
    r_color = ((r_color & 0xF8));
    g_color = ((g_color & 0xFC));
    b_color = ((b_color & 0xF8)>>3);
    return (((uint16_t)r_color << 8) + ((uint16_t)g_color << 3) + b_color );
}

  __attribute__((weak)) char * strrchr(const char * str,int ch)
 {
		char *p = (char *)str;
		while (*str) str++;
		while (str-- != p && *str != (char)ch);
		if (*str == (char)ch)
				 return( (char *)str );
		return(NULL);
 }
//--------------------------------------

//----------------------------------------
#if 0
__attribute__((weak)) int sscanf (const char *s, const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vsscanf(s, format, arg);
  va_end (arg);

  return done;
}

#endif
#if 1
 __attribute__((weak)) char* itoa(int num,char* str,int radix)
{
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	unsigned unum;
	int i=0,j,k;

	if(radix==10&&num<0){
			unum=(unsigned)-num;
			str[i++]='-';
	}
	else unum=(unsigned)num;
	do{
			str[i++]=index[unum%(unsigned)radix];
			unum/=radix;

	}while(unum);
	str[i]='\0';
	if(str[0]=='-') k=1;
	else k=0;

	char temp;
	for(j=k;j<=(i-1)/2;j++){
			temp=str[j];
			str[j]=str[i-1+k-j];
			str[i-1+k-j]=temp;
	}
	return str;
}

 __attribute__((weak)) int atoi(const char *pstr)
{
	int Ret_Integer = 0;
	int Integer_sign = 1;
	/*
	* 判断指针是否为空
	*/
	if(pstr == NULL)
	{
		return 0;
	}
	
	/*
	* 跳过前面的空格字符
	*/
	while(*pstr == '\0')
	{
		pstr++;
	}
	
	/*
	* 判断正负号
	* 如果是正号，指针指向下一个字符
	* 如果是符号，把符号标记为Integer_sign置-1，然后再把指针指向下一个字符
	*/
	if(*pstr == '-')
	{
		Integer_sign = -1;
	}
	if(*pstr == '-' || *pstr == '+')
	{
		pstr++;
	}
	
	/*
	* 把数字字符串逐个转换成整数，并把最后转换好的整数赋给Ret_Integer
	*/
	while(*pstr >= '0' && *pstr <= '9')
	{
		Ret_Integer = Ret_Integer * 10 + *pstr - '0';
		pstr++;
	}
	Ret_Integer = Integer_sign * Ret_Integer;
	
	return Ret_Integer;
}
#else
//-----------------------------------------------------------------------
#define is_digit(c) ((c) >= '0' && (c) <= '9')
 __attribute__((weak)) int is_space( int ch )
{
    return (unsigned long)(ch - 9) < 5u || ' ' == ch;
}
 __attribute__((weak)) int atoi(const char *str)
{
   int sign;
   int n;
   char *p = str;

   while (is_space(*p) ) p++;

   sign = ('-' == *p) ? -1 : 1;
   if ('+' == *p || '-' == *p) p++;

   for (n = 0; is_digit(*p); p++)
      n = 10 * n + (*p - '0');

   return sign*n;
}
 __attribute__((weak)) char * itoa(int n, const char * chBuffer)
{
    int i = 1;
    char * pch = chBuffer;
    if(!pch) return 0;
    while(n / i) i *= 10;

    if(n < 0)
    {
        n = -n;
        *pch++ = '-';
    }
    if (0 == n) i = 10;

    while(i /= 10)
    {
        *pch++ = n / i + '0';
        n %= i;
    }
    *pch = '\0';
    return chBuffer;
}
#endif

#if 0
 __attribute__((weak)) mp_obj_t get_sscan(const char *sour, const char *start,const char *end,uint8_t *ret)
{
	static char *s1;
	static char *s2;
	char *buf;
	char out_buf[1024];
	uint32_t total_len = strlen(sour);
	uint32_t s1_start_len = strlen(start); //s1的长度
	memset(out_buf, '\0', 1024);
	*ret = 0;
	s1 = strstr(sour,start);
	if(s1 != NULL){
		buf = m_malloc(total_len);
		memset(buf, '\0', total_len);
		strncpy(buf,s1+s1_start_len , strlen(s1)-s1_start_len);
		s1_start_len = strlen(buf); //buf len
		s2 = strstr(buf,end);

		if(s2 != NULL){
			total_len = (s1_start_len - strlen(s2));
			strncpy(&out_buf[0],&buf[0] , (size_t)total_len);
			*ret = total_len;
			m_free(buf);
		}else{
			return mp_const_none;
		}
		
	}else{
		return mp_const_none;
	}
	return mp_obj_new_str(out_buf, total_len);
}
#else
 __attribute__((weak)) mp_obj_t get_sscan(const char *sour, const char *start,const char *end,uint8_t *ret)
{
	static char *s1;
	static char *s2;
	char *buf;
	char out_buf[1024];
	uint32_t total_len = strlen(sour);
	uint32_t s1_start_len = strlen(start); //s1的长度
	memset(out_buf, '\0', 1024);
	*ret = 0;
	s1 = strstr(sour,start);
	if(s1 != NULL){
		buf = m_malloc(total_len);
		memset(buf, '\0', total_len);
		memcpy(buf,s1+s1_start_len , strlen(s1)-s1_start_len);
		s1_start_len = strlen(buf); //buf len
		s2 = strstr(buf,end);

		if(s2 != NULL){
			total_len = (s1_start_len - strlen(s2));
			memcpy(&out_buf[0],&buf[0] , (size_t)total_len);
			*ret = total_len;
			m_free(buf);
		}else{
			return mp_const_none;
		}
		
	}else{
		return mp_const_none;
	}
	return mp_obj_new_str(out_buf, total_len);
}
#endif


//-----------------------------------------------------------------------
 //路径解析出来
 //返回0 为flash 1为sd。否则解析失败。
 //src_path:输入路径 ret_path：返回的相对路径
  __attribute__((weak)) mp_obj_t get_path(const char *src_path , uint8_t *res ) {
 
	 uint8_t date_len = 0 ,cp_len =0;
 
	 char upda_str[50];
 
	 char cp_str[7];
	 char *ret_path;
 
	 memset(upda_str, '\0', sizeof(upda_str));
	 // strncpy(upda_str,&src_path[1] , strlen(src_path)-1);
	 memcpy(upda_str,&src_path[1] , strlen(src_path)-1);

	 
	 date_len = strlen(upda_str);

	 ret_path=strchr(upda_str,'/');
	 if(ret_path == 0)	mp_raise_ValueError(MP_ERROR_TEXT("file path error"));
	 
	 cp_len = date_len - strlen(ret_path);
 
	 memset(cp_str, '\0', 7);
	 // strncpy(cp_str,upda_str,cp_len);
	memcpy(cp_str,upda_str,cp_len);
	*res = 2;
	 if(strncmp(cp_str , "flash" , 5) == 0) 	 *res = 0; 
	 else if(strncmp(cp_str , "sd" , 2) == 0)  *res = 1;
	 else	
	 	{
			mp_raise_ValueError(MP_ERROR_TEXT("no find sd or flash path"));
		}
 
	 date_len = (date_len - cp_len - 1);
	 memset(upda_str, '\0', sizeof(upda_str));
	 // strncpy(upda_str,&src_path[cp_len+2] , date_len); 
	memcpy(upda_str,&src_path[cp_len+2] , date_len);
	 return mp_obj_new_str(upda_str, date_len);
 }

  __attribute__((weak)) mp_obj_t file_type(const char *fileName)
{
	char dest[10];
	memset(dest, '\0', sizeof(dest));
	char *ret = strrchr(fileName , '.');
	if(ret == NULL){
		mp_raise_TypeError(MP_ERROR_TEXT("no find file type"));
	}
	// strncpy(dest,&ret[1] , strlen(ret)-1); 
	memcpy(dest,&ret[1] , strlen(ret)-1);
	return mp_obj_new_str(dest, strlen(ret)-1);
}
__attribute__((weak)) bool check_sys_file(const char *check_file) {
	mp_obj_t iter = mp_vfs_ilistdir(0, NULL);
	mp_obj_t next;
	while ((next = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
		const char *flie = mp_obj_str_get_str(mp_obj_subscr(next, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_SENTINEL));
		if(0 == strcmp(check_file,flie)){
			return true;
		}
	}
	return false;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
__attribute__((weak)) ssize_t jpg_save(const char *filename, uint8_t *wbuf, ssize_t len)
{
	mp_obj_t f_new;
	ssize_t res = 0;
	
	mp_obj_t args[2] = {
		mp_obj_new_str(filename, strlen(filename)),
		MP_OBJ_NEW_QSTR(MP_QSTR_wb),
	};
	f_new = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
	
	res = mp_stream_posix_write(f_new, wbuf, len);
	
	mp_stream_close(f_new);
	return res;
}
#endif
