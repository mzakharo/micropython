
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	gt1151.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/11/1
	* Description 			 :	
	******************************************************************************
**/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TP_TOUCH_H
#define __TP_TOUCH_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "py/obj.h"

#if (MICROPY_HW_GT1151 && MICROPY_HW_LCD43M)

extern const mp_obj_type_t touch_gt1151_type;
extern void gtxx_read_point(void);

#endif

/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __TP_TOUCH_H */



