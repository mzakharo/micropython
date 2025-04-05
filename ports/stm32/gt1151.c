

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
#include "pybthread.h"
#include "py/stackctrl.h"

#if (MICROPY_HW_GT1151 && MICROPY_HW_LCD43M)
#include "modtftlcd.h"
#include "gt1151.h"
#include "modtouch.h"
#include "lcd43m.h"
#include "tp_iic.h"

#ifndef MICROPY_HW_TP_RST
#if MICROPY_HW_BOARD_COLUMBUS
	#define MICROPY_HW_TP_RST 					(pin_C13)
	#define MICROPY_HW_TP_INT 					(pin_F7)
#elif MICROPY_HW_BOARD_MAGELLAM
	#define MICROPY_HW_TP_RST 					(pin_C13)
	#define MICROPY_HW_TP_INT 					(pin_I11)
#endif
#endif

//I2C读写命令	
#define GT_CMD_WR 		0X28     	//写命令
#define GT_CMD_RD 		0X29		//读命令

//GT9147 部分寄存器定义 
#define GT_CTRL_REG 	0X8040   	//GT9147控制寄存器
#define GT_CFGS_REG 	0X8047   	//GT9147配置起始地址寄存器
#define GT_CHECK_REG 	0X80FF   	//GT9147校验和寄存器
#define GT_PID_REG 		0X8140   	//GT9147产品ID寄存器

#define GT_GSTID_REG 	0X814E   	//GT9147当前检测到的触摸情况
#define GT_TP1_REG 		0X8150  	//第一个触摸点数据地址
#define GT_TP2_REG 		0X8158		//第二个触摸点数据地址
#define GT_TP3_REG 		0X8160		//第三个触摸点数据地址
#define GT_TP4_REG 		0X8168		//第四个触摸点数据地址
#define GT_TP5_REG 		0X8170		//第五个触摸点数据地址  

#define GTXX_MAX_TOUCH 5

static uint16_t touch_w,touch_h;

//GT9147配置参数表
const uint8_t GT9147_CFG_TBL[]=
{ 
	0x62,0xE0,0x01,0x20,0x03,0x05,0x35,0xC0,0x01,0x08,
	0x28,0x0F,0x50,0x32,0x03,0x05,0x00,0x00,0x00,0x00,
	0x00,0x00,0x06,0x16,0x16,0x1F,0x14,0x89,0x28,0x0A,
	0x17,0x15,0x31,0x0D,0x00,0x00,0x08,0x22,0x04,0x11,
	0x00,0x00,0x00,0x00,0x00,0x03,0x82,0x08,0x08,0x00,
	0x00,0x0F,0x2C,0x94,0xC5,0x02,0x07,0x00,0x00,0x04,
	0x9D,0x10,0x00,0x84,0x14,0x00,0x70,0x19,0x00,0x5F,
	0x20,0x00,0x55,0x27,0x00,0x54,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
	0x00,0x00,0x1A,0x18,0x16,0x14,0x12,0x10,0x0E,0x0C,
	0x0A,0x08,0x00,0x00,0x00,0x00,0x1F,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0x00,0x00,0x02,0x04,0x05,0x06,0x08,0x0A,0x0C,
	0x0E,0x1D,0x1E,0x1F,0x20,0x22,0x24,0x28,0x29,0xFF,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF6,
	0xFF,0xFF,0xFF,0xFF
}; 

//==========================================================================================
//向GT9147写入一次数据
//reg:起始寄存器地址
//buf:数据缓缓存区
//len:写数据长度
//返回值:0,成功;1,失败.
STATIC uint8_t GT9147_WR_Reg(uint16_t reg,uint8_t *buf,uint8_t len)
{
	uint8_t i;
	uint8_t ret=0;
	TP_IIC_START();	
 	TP_IIC_Send_Byte(GT_CMD_WR);   	//发送写命令 	
	TP_IIC_Wait_Ack();
	//mp_hal_delay_ms(10);
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
//---------------------------------------------------
//从GT9147读出一次数据
//reg:起始寄存器地址
//buf:数据缓缓存区
//len:读数据长度			  
STATIC void GT9147_RD_Reg(uint16_t reg,uint8_t *buf,uint8_t len)
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
#if 0
//发送GT9147配置参数
//mode:0,参数不保存到flash
//     1,参数保存到flash
STATIC uint8_t GT9147_Send_Cfg(uint8_t mode)
{
	uint8_t buf[2];
	uint8_t i=0;
	buf[0]=0;
	buf[1]=mode;	//是否写入到GT9147 FLASH?  即是否掉电保存
	for(i=0;i<sizeof(GT9147_CFG_TBL);i++)
		buf[0]+=GT9147_CFG_TBL[i];//计算校验和
    buf[0]=(~buf[0])+1;
	GT9147_WR_Reg(GT_CFGS_REG,(uint8_t*)GT9147_CFG_TBL,sizeof(GT9147_CFG_TBL));//发送寄存器配置
	GT9147_WR_Reg(GT_CHECK_REG,buf,2);//写入校验和,和配置更新标记
	return 0;
} 
#endif
STATIC void tp_addr_init(void)
{
	mp_hal_pin_config(MICROPY_HW_TP_INT, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_high(MICROPY_HW_TP_INT);

	mp_hal_pin_config(MICROPY_HW_TP_RST, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	mp_hal_pin_low(MICROPY_HW_TP_RST);
	mp_hal_pin_low(MICROPY_HW_TP_INT);
	mp_hal_delay_ms(100);
	mp_hal_pin_high(MICROPY_HW_TP_INT);
	mp_hal_delay_ms(100);
	mp_hal_pin_high(MICROPY_HW_TP_RST);
	mp_hal_delay_ms(100);  //10ms
	mp_hal_pin_config(MICROPY_HW_TP_INT, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_NONE, 0);
}
//----------------------------------------------------------------------
STATIC void TP_Init(void)
{

	tp_addr_init();

	TP_IIC_Init();      	//初始化电容屏的I2C总线

	mp_hal_delay_ms(100) ;//10ms
	GT9147_RD_Reg(GT_PID_REG,tp_dev.id,4);//读取产品ID

#if 0
	if(strcmp((char*)tp_dev.id,"9147")==0 || strcmp((char*)tp_dev.id,"917S")==0)//ID==9147
	{
		temp[0]=0X02;	
		//GT_CTRL_REG
		GT9147_WR_Reg(GT_CTRL_REG,temp,1);//软复位GT9147
		mp_hal_delay_ms(100);  
		GT9147_RD_Reg(GT_CFGS_REG,temp,1);//读取GT_CFGS_REG寄存器
		if(temp[0]<0X61)//默认版本比较低,需要更新flash配置
		{
			GT9147_Send_Cfg(1);//更新并保存配置
		}
		mp_hal_delay_ms(10);
		temp[0]=0X00;	 
		GT9147_WR_Reg(GT_CTRL_REG,temp,1);//结束复位  
	}		
	#endif
}

const uint16_t GT9147_TPX_TBL[5]={GT_TP1_REG,GT_TP2_REG,GT_TP3_REG,GT_TP4_REG,GT_TP5_REG};
//==============================================================================

void gtxx_read_point(void)
{
    uint8_t point_status = 0;
    uint8_t touch_num = 0;
    uint8_t write_buf[1];
    uint8_t read_buf[4] = {0};
    uint16_t input_x = 0;
    uint16_t input_y = 0;
    int16_t input_w = 0;
		int8_t read_id = 0;

    static uint8_t pre_touch = 0;
    /* point status register */
		GT9147_RD_Reg(GT_GSTID_REG,&point_status,1);
    if (point_status == 0)             /* no data */
    {
        goto exit_;
    }
    if ((point_status & 0x80) == 0)    /* data is not ready */
    {
        goto exit_;
    }
    touch_num = point_status & 0x0f;  /* get point num */
    if (touch_num > GTXX_MAX_TOUCH) /* point num is not correct */
    {
        goto exit_;
    }
    /* read point num is read_num */

		GT9147_RD_Reg(GT9147_TPX_TBL[0],read_buf,4); 

    if (pre_touch > touch_num)                                       /* point up */
    {
     tp_touch_up(read_id);
    }
    if (touch_num)                                                 /* point down */
    {	
      switch (tp_dev.dir)
        {
          case 2:
            input_x=800-(((uint16_t)read_buf[3]<<8)+read_buf[2]);
            input_y=(((uint16_t)read_buf[1]<<8)+read_buf[0]);
          break;
          case 3:
            input_x=480-(((uint16_t)read_buf[1]<<8)+read_buf[0]);
            input_y=800-(((uint16_t)read_buf[3]<<8)+read_buf[2]);
          break;
          case 4:
            input_y=480-(((uint16_t)read_buf[1]<<8)+read_buf[0]);
            input_x=((uint16_t)read_buf[3]<<8)+read_buf[2];
          break;
          default:
            input_x=((uint16_t)read_buf[1]<<8)+read_buf[0];
            input_y=((uint16_t)read_buf[3]<<8)+read_buf[2];
          break;
        }
				if(input_x >= touch_w || input_y >= touch_h){
					return;
				}
       	tp_touch_down(read_id, input_x, input_y, input_w);

    }
    else if (pre_touch)
    {
       tp_touch_up(read_id);
    }
    pre_touch = touch_num;

exit_:
    write_buf[0] = 0;

		GT9147_WR_Reg(GT_GSTID_REG,write_buf,1);//清标志 


}
/**********************************************************************************************************/

//----------------------------------------------------------------------------------------------------------
typedef struct _touch_gt1151_obj_t {
    mp_obj_base_t base;
} touch_gt1151_obj_t;

STATIC touch_gt1151_obj_t gt1151_obj;

//==========================================================================================

//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_gt1151_read(void)
{
	mp_obj_t tuple[3];
	gtxx_read_point();
	if (tp_dev.sta&TP_PRES_DOWN) tuple[0] = mp_obj_new_int(0);
	else if(tp_dev.sta&TP_PRES_MOVE)	tuple[0] = mp_obj_new_int(1); 
	else 	tuple[0] = mp_obj_new_int(2); 
		
	tuple[1] = mp_obj_new_int(tp_dev.x[0]);
	tuple[2] = mp_obj_new_int(tp_dev.y[0]);
	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_gt1151_read_obj, touch_gt1151_read);
//------------------------------------------------------------------------------------------------------
STATIC mp_obj_t touch_gt1151_scan(void)
{
	gtxx_read_point();
	return mp_const_none;
}STATIC MP_DEFINE_CONST_FUN_OBJ_0(touch_gt1151_scan_obj, touch_gt1151_scan);
//----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
STATIC void touch_gt1151_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	mp_printf(print,"TP ID:%s\r\n",tp_dev.id);
}
//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------

STATIC mp_obj_t touch_gt1151_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_portrait, MP_ARG_INT, {.u_int = 1} },
	};
	
		mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
		mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
		
		//if(args[0].u_int != 0)
		{
			tp_dev.dir = args[0].u_int;
		}
		
		switch (tp_dev.dir)
		{
			case 1:
			case 3:
			touch_w=480;
			touch_h=800;
			break;
			case 2:
			case 4:
			touch_w=800;
			touch_h=480;
			break;
		}

		TP_Init();
		touch_is_init = 1;
		tp_dev.type = 4;
		
		gt1151_obj.base.type = &touch_gt1151_type;

		return MP_OBJ_FROM_PTR(&touch_gt1151_type);
}
//===================================================================
STATIC const mp_rom_map_elem_t touch_gt1151_locals_dict_table[] = {
    // instance methods
		{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&touch_gt1151_read_obj) },
		{ MP_ROM_QSTR(MP_QSTR_tick_inc), MP_ROM_PTR(&touch_gt1151_scan_obj) },
};

STATIC MP_DEFINE_CONST_DICT(touch_gt1151_locals_dict, touch_gt1151_locals_dict_table);

const mp_obj_type_t touch_gt1151_type = {
    { &mp_type_type },
    .name = MP_QSTR_GT1151,
    .print = touch_gt1151_print,
    .make_new = touch_gt1151_make_new,
    .locals_dict = (mp_obj_dict_t *)&touch_gt1151_locals_dict,
};
#endif

/**********************************************************************************************************/
