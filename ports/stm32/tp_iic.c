
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	tp_iic.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/18
	* Description 			 :	
	******************************************************************************
**/
#include "py/runtime.h"
#include "py/mphal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "pin.h"
#include "pin_static_af.h"
#include "mpu.h"
#include "systick.h"
#include "extint.h"

#if (MICROPY_ENABLE_IIC_DEV)

#include "tp_iic.h"

#ifndef MICROPY_HW_TP_SDA
#if MICROPY_HW_BOARD_COLUMBUS
	#define MICROPY_HW_TP_SDA			(pin_F11)
	#define TP_SDA_PIN					GPIO_PIN_11
	#define TP_SDA_GPIO_PORT            GPIOF
	#define MICROPY_TP_SDA_NUM			(11)
	#define MICROPY_HW_TP_SCL			(pin_B0)
	#define TP_SCL_PIN                  GPIO_PIN_0
	#define TP_SCL_GPIO_PORT            GPIOB
#elif MICROPY_HW_BOARD_MAGELLAM
	#define MICROPY_HW_TP_SDA			(pin_I3)
	#define TP_SDA_PIN					GPIO_PIN_3
	#define TP_SDA_GPIO_PORT            GPIOI
	#define MICROPY_TP_SDA_NUM			(3)
	#define MICROPY_HW_TP_SCL			(pin_B0)
	#define TP_SCL_PIN                  GPIO_PIN_0
	#define TP_SCL_GPIO_PORT            GPIOB
#else
	#define MICROPY_HW_TP_SDA			(pin_B11)
	#define TP_SDA_PIN					GPIO_PIN_11
	#define TP_SDA_GPIO_PORT            GPIOB
	#define MICROPY_TP_SDA_NUM			(11)
	#define MICROPY_HW_TP_SCL			(pin_B0)
	#define TP_SCL_PIN                  GPIO_PIN_0
	#define TP_SCL_GPIO_PORT            GPIOB
#endif

#endif

#define TP_SDA_IN()  {TP_SDA_GPIO_PORT->MODER&=~(3<<(2*MICROPY_TP_SDA_NUM));TP_SDA_GPIO_PORT->MODER|=0<<(2*MICROPY_TP_SDA_NUM);}	//INT MODE
#define TP_SDA_OUT() {TP_SDA_GPIO_PORT->MODER&=~(3<<(2*MICROPY_TP_SDA_NUM));TP_SDA_GPIO_PORT->MODER|=1<<(2*MICROPY_TP_SDA_NUM);} 	//OUT MODE

#define MICROPY_TP_INT_PULL       (GPIO_PULLUP)
#define MICROPY_TP_INT_EXTI_MODE  (GPIO_MODE_IT_FALLING)
#define TP_IIC_SCL(Vlue) ((Vlue) ? (TP_SCL_GPIO_PORT->BSRR = TP_SCL_PIN) : (TP_SCL_GPIO_PORT->BSRR = (uint32_t)TP_SCL_PIN << 16U))
#define TP_IIC_SDA(Vlue) ((Vlue) ? (TP_SDA_GPIO_PORT->BSRR = TP_SDA_PIN) : (TP_SDA_GPIO_PORT->BSRR = (uint32_t)TP_SDA_PIN << 16U))
#define TP_SDA_READ (TP_SDA_GPIO_PORT->IDR & TP_SDA_PIN)


//----------------------------------------------------------------------
static void tp_delay(void)
{
	mp_hal_delay_us(2);
}
void TP_IIC_Init(void)
{	
	mp_hal_pin_config(MICROPY_HW_TP_SDA, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(MICROPY_HW_TP_SCL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);

	TP_IIC_SDA(1);
	TP_IIC_SCL(1);	  	

}
//----------------------------------------------------------------------------------
void TP_IIC_START(void)
{
	TP_SDA_OUT();     //sda线输出
	TP_IIC_SDA(1);
	TP_IIC_SCL(1);
	tp_delay();
 	TP_IIC_SDA(0);//START:when CLK is high,DATA change form high to low 
	tp_delay();
	TP_IIC_SCL(0);//钳住I2C总线，准备发送或接收数据 
}
//----------------------------------------------------------------------------------
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答			  
void TP_IIC_Send_Byte(uint8_t txd)
{                        
    uint8_t t;   

	TP_SDA_OUT(); 	    
    TP_IIC_SCL(0);//拉低时钟开始数据传输
	//tp_delay();
	for(t=0;t<8;t++)
    {           
   		if(((txd&0x80)>>7) == 0x01)
			TP_IIC_SDA(1);
		else
			TP_IIC_SDA(0);

		txd<<=1; 
		tp_delay();
		TP_IIC_SCL(1); 
		tp_delay();
		TP_IIC_SCL(0);	
		tp_delay();
    }	 
}
//----------------------------------------------------------------------------------
//不产生ACK应答		    
void TP_IIC_NAck(void)
{
	TP_IIC_SCL(0); 
	TP_SDA_OUT(); 
	tp_delay();
	TP_IIC_SDA(1);
	tp_delay();
	TP_IIC_SCL(1);
	tp_delay();
	TP_IIC_SCL(0); 
}
//产生ACK应答
void TP_IIC_Ack(void)
{
	TP_IIC_SCL(0); 
	TP_SDA_OUT(); 
	tp_delay();
	TP_IIC_SDA(0);
	tp_delay();
	TP_IIC_SCL(1);
	tp_delay();
	TP_IIC_SCL(0); 
}
//----------------------------------------------------------------------------------
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
uint8_t TP_IIC_Read_Byte(unsigned char ack)
{
	uint8_t i,receive=0;
 	TP_SDA_IN();//SDA设置为输入
	mp_hal_delay_us(5);
	for(i=0;i<8;i++ )
	{ 
		TP_IIC_SCL(0); 	    	   
		tp_delay();
		TP_IIC_SCL(1);		
		receive<<=1;
		if(TP_SDA_READ)receive++;   
		tp_delay();
	}	  				 
	if (!ack)TP_IIC_NAck();//发送nACK
	else TP_IIC_Ack(); //发送ACK   
 	return receive;
}
		
//----------------------------------------------------------------------------------
//产生IIC停止信号
void TP_IIC_Stop(void)
{
	TP_SDA_OUT(); //sda线输出
	TP_IIC_SCL(1);
	tp_delay();
	TP_IIC_SDA(0);//STOP:when CLK is high DATA change form low to high
	tp_delay();
	TP_IIC_SDA(1);//发送I2C总线结束信号  
}
//----------------------------------------------------------------------------------
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
uint8_t TP_IIC_Wait_Ack(void)
{
	uint32_t ucErrTime=0;
	TP_SDA_IN();      //SDA设置为输入  
	TP_IIC_SDA(1);	   mp_hal_delay_us(1);
	TP_IIC_SCL(1);	   mp_hal_delay_us(1);
	
	while(TP_SDA_READ)
	{
		ucErrTime++;
		if(ucErrTime>2500)
		{
			TP_IIC_Stop();
			return 1;
		} 
	}
	TP_IIC_SCL(0);//时钟输出0 	   
	return 0;  
}

#endif

/**********************************************************************************************************/
