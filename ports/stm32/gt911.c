
/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2022, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	gt911.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/11/1
	* Description 			 :	
	******************************************************************************
**/
#include "py/runtime.h"
#include "py/mphal.h"
#include "softtimer.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "py/obj.h"

#include "pin.h"
#include "pin_static_af.h"
#include "mpu.h"
#include "systick.h"
#include "extint.h"
#include "py/mpthread.h"

#if (MICROPY_HW_GT911 && MICROPY_ENABLE_TFTLCD)

#include "gt911.h"
#include "modtftlcd.h"
#include "modtouch.h"
#include "tp_iic.h"

#ifndef MICROPY_HW_TP_RST
	#define MICROPY_HW_TP_RST 					(pin_C13)
	#define MICROPY_HW_TP_INT 					(pin_I11)
#endif

#define GT911_ADDR_LEN          2
#define GT911_REGITER_LEN       2
#define GT911_MAX_TOUCH         5
#define GT911_POINT_INFO_NUM   5

#define GT911_COMMAND_REG       0x8040
#define GT911_CONFIG_REG        0x8047

#define GT911_PRODUCT_ID        0x8140
#define GT911_VENDOR_ID         0x814A
#define GT911_READ_STATUS       0x814E

#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址  

#define GT911_POINT1_REG        0x814F
#define GT911_POINT2_REG        0x8157
#define GT911_POINT3_REG        0x815F
#define GT911_POINT4_REG        0x8167
#define GT911_POINT5_REG        0x816F

#define GT911_CHECK_SUM         0x80FF

#define GT_CMD_WR 		0X28     	//写命令
#define GT_CMD_RD 		0X29		//读命令

static uint8_t GT911_CFG_TBL[] =
{
	0x5A,0x00,0x04,0x58,0x02,0x05,0x0d,0x00,0x01,0x0a,
	0x64,0x0f,0x5a,0x3c,0x05,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x08,0x17,0x19,0x1c,0x14,0x87,0x29,0x0a,
	0x4e,0x50,0xeb,0x04,0x00,0x00,0x00,0x00,0x02,0x11,
	0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x46,0x64,0x94,0xc5,0x02,0x07,0x00,0x00,0x04,
	0x9e,0x48,0x00,0x8d,0x4d,0x00,0x7f,0x53,0x00,0x73,
	0x59,0x00,0x67,0x60,0x00,0x67,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,
	0x12,0x14,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x1d,
	0x1e,0x1f,0x20,0x21,0x22,0x24,0x26,0x28,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,//0x48,0x00,
};


static uint8_t id_buf[4];

//---------------------------------------------------
//返回值:0,成功;1,失败.
static uint8_t gt911_write_reg(uint16_t reg, uint8_t *buf, uint16_t len)
{
	uint8_t i;
	uint8_t ret=0;
	TP_IIC_START();	
 	TP_IIC_Send_Byte(GT_CMD_WR);   	//发送写命令 	
	TP_IIC_Wait_Ack();
	TP_IIC_Send_Byte(reg>>8);   	//发送高8位地址
	TP_IIC_Wait_Ack(); 	 										  		   
	TP_IIC_Send_Byte(reg&0XFF);   	//发送低8位地址
	TP_IIC_Wait_Ack();  
	for(i=0;i<len;i++)
	{	   
    	TP_IIC_Send_Byte(buf[i]);  	//发数据
		ret=TP_IIC_Wait_Ack();
		if(ret)break;  
	}
    TP_IIC_Stop();					//产生一个停止条件	    
	return ret; 
}

STATIC void gt911_read_reg(uint16_t reg,uint8_t *buf,uint16_t len)
{
	uint8_t i; 
 	TP_IIC_START();
 	TP_IIC_Send_Byte(GT_CMD_WR);   //发送写命令 	
	TP_IIC_Wait_Ack();
 	TP_IIC_Send_Byte(reg>>8);   	//发送高8位地址
	TP_IIC_Wait_Ack();
 	TP_IIC_Send_Byte(reg&0XFF);   	//发送低8位地址
	TP_IIC_Wait_Ack();  
 	TP_IIC_START();  	 	   
	TP_IIC_Send_Byte(GT_CMD_RD);   //发送读命令		   
	TP_IIC_Wait_Ack();
	for(i=0;i<len;i++)
	{	   
    	buf[i]=TP_IIC_Read_Byte(i==(len-1)?0:1); //发数据	  
	} 
    TP_IIC_Stop();//产生一个停止条件     
} 

//======================================================
void gt911_soft_reset(void)
{
	uint8_t buf[1];
	buf[0] = 0x02;
	gt911_write_reg(GT911_COMMAND_REG, buf, 1);
}

//mode:0,参数不保存到flash
//     1,参数保存到flash
static void gt911_config(uint16_t x, uint16_t y, uint8_t mode)
{
	uint8_t buf[2];
	uint16_t i = 0;

	GT911_CFG_TBL[2] = (uint8_t)(x >> 8);
	GT911_CFG_TBL[1] = (uint8_t)(x & 0xff);
	
	GT911_CFG_TBL[4] = (uint8_t)(y >> 8);
	GT911_CFG_TBL[3] = (uint8_t)(y & 0xff);
	
	gt911_write_reg(GT911_CONFIG_REG, (uint8_t *)GT911_CFG_TBL, sizeof(GT911_CFG_TBL));

	buf[0]=0;
	buf[1]=mode;
	for(i=0;i<sizeof(GT911_CFG_TBL);i++) buf[0]+=GT911_CFG_TBL[i];
		
	buf[0]=(~buf[0])+1;
	
	gt911_write_reg(GT911_CHECK_SUM, buf ,2);
	
}
STATIC void gt911_Init(void)
{
	mp_hal_pin_config(MICROPY_HW_TP_RST, MP_HAL_PIN_MODE_OUTPUT,MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_config(MICROPY_HW_TP_INT, MP_HAL_PIN_MODE_OUTPUT,MP_HAL_PIN_PULL_UP, 0);
	
	mp_hal_pin_low(MICROPY_HW_TP_RST);
	mp_hal_pin_low(MICROPY_HW_TP_INT);
	
	mp_hal_delay_ms(100) ;//10ms
	mp_hal_pin_high(MICROPY_HW_TP_INT);
	mp_hal_delay_ms(100) ;//10ms
	
	mp_hal_pin_high(MICROPY_HW_TP_RST);
	mp_hal_delay_ms(100) ;//10ms
	
	mp_hal_pin_low(MICROPY_HW_TP_INT);
	
	TP_IIC_Init();      	//初始化电容屏的I2C总线
	
	mp_hal_delay_ms(100) ;//10ms

	gt911_config(800, 480, 1);
	
	mp_hal_delay_ms(100) ;//10ms
	
	id_buf[3] = '\0';
	gt911_read_reg(GT911_PRODUCT_ID,id_buf,3);

//	printf("id:GT%s\r\n",id_buf);

}

void gt911_get_x_y_point(void)
{
	uint8_t buf[4]={0};
	gt911_read_reg(0x8048,buf,4);
	
	printf("x:%d,y:%d\r\n",(buf[1]<<8 | buf[0]), (buf[3]<<8 | buf[2]));
}

static uint16_t touch_w,touch_h;
void gt911_read_point(void)
{
	uint8_t touch_num = 0;
	static uint8_t read[8] = {0};
	static uint8_t write_buf[1];
	uint16_t input_x = 0;
	uint16_t input_y = 0;
	int16_t input_w = 0;
	int8_t read_id = 0;
	static uint8_t pre_touch = 0;
	
	gt911_read_reg(GT911_READ_STATUS,read,6);
	touch_num = read[0] & 0xF; 
	
	if (pre_touch > touch_num){
	 tp_touch_up(read_id);
	}
	if ((touch_num == 0)){
		pre_touch = touch_num;
		goto exit;
	}

	if (touch_num){	
		switch (tp_dev.dir)
			{
				case 2:
					input_y = (uint16_t)(touch_h - ((read[3] << 8) + read[2]));
					input_x = (uint16_t)(((read[5] <<8) + read[4]));
				break;
				case 3:
					input_x = (uint16_t)(touch_w - ((read[3] << 8) + read[2]));
					input_y = (uint16_t)(touch_h - ((read[5] << 8) + read[4]));
				break;
				case 4:
					input_y = (uint16_t)( (read[3] << 8) + read[2]);
					input_x = (uint16_t)(touch_w -  ((read[5] <<8) + read[4]));
				break;
				default:
					input_x = (uint16_t)( (read[3] << 8) + read[2]);
					input_y = (uint16_t)( ((read[5] <<8) + read[4]));
				break;
			}

			if(input_x >= touch_w || input_y >= touch_h){
				goto exit;
			}
			//printf("lcddev.dir:%d,x:%d,y:%d,num:%d\r\n",lcddev.dir,input_x,input_y,touch_num);
			tp_touch_down(read_id, input_x, input_y, input_w);
	}else if (pre_touch){
		 tp_touch_up(read_id);
	}
	pre_touch = touch_num;
	
exit:
	write_buf[0] = 0x00;
	gt911_write_reg(GT911_READ_STATUS, write_buf,1);

}

//===============================================================================

/**********************************************************************************************************/

//----------------------------------------------------------------------------------------------------------
typedef struct _touch_gt911_obj_t {
    mp_obj_base_t base;
} touch_gt911_obj_t;

STATIC touch_gt911_obj_t gt911_obj;

//==========================================================================================

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_gt911_scan(void)
{
	gt911_read_point();
	return mp_const_none;
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_gt911_scan_obj, touch_gt911_scan);

STATIC mp_obj_t touch_gt911_read(void)
{
	mp_obj_t tuple[3];
	gt911_read_point();
	if (tp_dev.sta&TP_PRES_DOWN) tuple[0] = mp_obj_new_int(0);
	else if(tp_dev.sta&TP_PRES_MOVE)	tuple[0] = mp_obj_new_int(1); 
	else 	tuple[0] = mp_obj_new_int(2); 
		
	tuple[1] = mp_obj_new_int(tp_dev.x[0]);
	tuple[2] = mp_obj_new_int(tp_dev.y[0]);
	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_gt911_read_obj, touch_gt911_read);


//-----------------------------------------------------------------------------------------------
STATIC void touch_gt911_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print,"TOUCH ID:GT%s\r\n",id_buf);
}
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_gt911_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	
	//if(args[0].u_int != 0){
		tp_dev.dir = args[0].u_int;
	//}
	
	switch (tp_dev.dir){

		case 2:
		touch_w=480;
		touch_h=800;
		break;
		case 3:
		touch_w=800;
		touch_h=480;
		break;
		case 4:
		touch_w=480;
		touch_h=800;
		break;
		default:
		touch_w=800;
		touch_h=480;
		break;
	}
	
	gt911_Init();

	touch_is_init = 1;
	
	tp_dev.type = 2;
	
	gt911_obj.base.type = &touch_gt911_type;
	return MP_OBJ_FROM_PTR(&touch_gt911_type);
}

//===================================================================
STATIC const mp_rom_map_elem_t touch_gt911_locals_dict_table[] = {
	// instance methods
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&touch_gt911_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tick_inc), MP_ROM_PTR(&touch_gt911_scan_obj) },
};

STATIC MP_DEFINE_CONST_DICT(touch_gt911_locals_dict, touch_gt911_locals_dict_table);

const mp_obj_type_t touch_gt911_type = {
    { &mp_type_type },
    .name = MP_QSTR_GT911,
    .print = touch_gt911_print,
    .make_new = touch_gt911_make_new,
    .locals_dict = (mp_obj_dict_t *)&touch_gt911_locals_dict,
};
#endif

/**********************************************************************************************************/
