
#ifndef __ANOP_H__
#define __ANOP_H__

#include <stdint.h>
#include <stdbool.h>

#define ANO_MAX_DATA_SIZE 50

/*发送到上位机数据头和目标地址*/
#define UP_HEAD 0xAA
#define UP_DADDR 0xFF

typedef struct
{
	uint8_t head;		//数据头
	uint8_t d_addr;		//设备地址
	uint8_t msgID;		//功能码
	uint8_t dataLen;	//数据长度
	uint8_t data[ANO_MAX_DATA_SIZE]; //数据内容
}anop_t;

//匿名上位机通讯协议V7

typedef enum
{
	ID_CHECK = 0,				//数据校验帧
	ID_ACC_GYR_SHOCK_D,			//加速度、陀螺仪传感器数据
	ID_MAG_ALT_TEM_D,			//罗盘、气压、温度传感器数据
	ID_ROL_PIT_YAW_D,			//横滚、俯仰、航向
	ID_V0_V1_V2_V3_D,			//四元数数据
	ID_ALTFU_ALTADD_D,			//高度数据
	ID_MODE_LOCKED_D,			//运行模式
	ID_SPEED_XYZ_D,				//速度数据
	ID_POS_X_Y_D,				//位置偏移数据
	ID_WIND_X_Y_D,				//风速估计
	ID_TAR_ROL_PIT_YAW_D,		//目标姿态数据
	ID_TAR_SPEED_XYZ_D,			//目标速度数据
	ID_RA_RD_D,					//回航信息
	ID_VOTAG_CURRENT_D,			//电压电流数据
	ID_STA_VEL_POS_GPS_ALT_D,	//外接模块工作状态
	ID_BRI_RGBA_D,				//RGB 亮度信息输出
	ID_LOG_STR_D,				//LOG 信息输出--字符串
	ID_LOG_STR_NUM_D,			//LOG 信息输出--字符串+数字
	
	ID_CTRL_PWM_D = 0x20,		//PWM 控制量
	ID_CTRL_R_P_T_D = 0x21,		//姿态控制量
	
	ID_CTRL_JOX_C = 0x40,		//遥控器数据
	ID_CTRL_REAL_C = 0x41,		//实时控制帧
	
	ID_CMD_C = 0xE0,			//CMD 命令帧
	ID_CMD_R = 0xE1,			//CMD 命令帧
	ID_CMD_W = 0xE2,			//CMD 命令帧
}anop_cmdID_t;

void check_data(uint8_t *sum_check, uint8_t *add_check, uint8_t *pData, uint8_t len);
void anopSendPacket(anop_t *p,uint8_t head,uint8_t d_addr);
bool anopSendLogStr(uint8_t *data,uint8_t len,uint8_t color);
void anopInit(void);
void setAnopPlatform(uint8_t set);

#endif
