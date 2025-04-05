/**
	******************************************************************************
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	my_sprintf.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MY_SPRINTF_H
#define __MY_SPRINTF_H

#ifdef __cplusplus
 extern "C" {
#endif 
#include "py/obj.h"


/* Includes ------------------------------------------------------------------*/  


extern int sprintf (char *s, const char *format, ...);
extern int sscanf(const char *buf, const char *fmt, ...);
/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __LCD43G_H */

