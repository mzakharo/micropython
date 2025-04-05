/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	tp_iic.h
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
#ifndef __TP_IIC_H
#define __TP_IIC_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "py/obj.h"

#if (MICROPY_ENABLE_TOUCH)


void TP_IIC_Init(void);
void TP_IIC_START(void);
void TP_IIC_Send_Byte(uint8_t txd);
void TP_IIC_NAck(void);
void TP_IIC_Ack(void);
uint8_t TP_IIC_Read_Byte(unsigned char ack);
void TP_IIC_Stop(void);
uint8_t TP_IIC_Wait_Ack(void);

#endif

/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __TP_IIC_H */



