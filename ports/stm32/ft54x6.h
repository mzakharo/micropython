/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	FT54x6.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
#ifndef __FT54X6_H
#define __FT54X6_H

#ifdef __cplusplus
 extern "C" {
#endif 


#if (MICROPY_HW_FT54X6 && MICROPY_ENABLE_TFTLCD)

extern const mp_obj_type_t touch_ft_type;
extern void ft54x6_read_point(void);

#endif

/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __FT54X6_H */



