/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	gt911.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/11/1
	* Description 			 :	
	******************************************************************************
**/
#ifndef __GT911_H
#define __GT911_H

#ifdef __cplusplus
 extern "C" {
#endif 


#if (MICROPY_HW_GT911 && MICROPY_ENABLE_TFTLCD)

extern const mp_obj_type_t touch_gt911_type;
extern void gt911_read_point(void);

#endif

/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __GT911_H */



