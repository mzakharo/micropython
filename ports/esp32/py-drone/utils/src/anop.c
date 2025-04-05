#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "anop.h"
#include "mod_wifllink.h"
#include "stabilizer_types.h"
#include "commander.h"
#include "stabilizer.h"
#include "pm_esplane.h"
#include "sensors_mpu6050_spl06.h"
#include "sensfusion6.h"
#include "power_distribution.h"
#include "state_estimator.h"
#include "mod_wifllink.h"

#include <math.h>
#include "esp_log.h"

#define TAG	"anop"
//数据拆分宏定义
#define  BYTE0(dwTemp)       ( *( (uint8_t *)(&dwTemp)	)  )
#define  BYTE1(dwTemp)       ( *( (uint8_t *)(&dwTemp) + 1) )
#define  BYTE2(dwTemp)       ( *( (uint8_t *)(&dwTemp) + 2) )
#define  BYTE3(dwTemp)       ( *( (uint8_t *)(&dwTemp) + 3) )

//数据返回周期时间（单位ms）

#define  PERIOD_SENSOR 		40  //姿态数据
#define  PERIOD_HEIGHT		50
#define  PERIOD_SENSOR2 	50 //加速度、陀螺仪传感器数据
#define  PERIOD_MOTOR		40
#define  PERIOD_Q			50 //四元数数据
#define  PERIOD_USERDATA   	60
#define  PERIOD_STATUS		70
#define  PERIOD_RCDATA 		80
#define  PERIOD_SPEED   	90
#define  PERIOD_SENSOR3 	300 //罗盘、气压、温度传感器数据
#define  PERIOD_POWER 		500

//摇杆中间软件死区值（ADC值）
#define MID_DB_VAL			10
#define VEL_LIMIT			50
#define MID_MAX				255.0

#define MID_DB_THRUST		120	
#define MID_DB_YAW			135	
#define MID_DB_PITCH		116
#define MID_DB_ROLL			132

#define  LOW_SPEED_THRUST   (95.0)
#define  LOW_SPEED_PITCH    (10.0)
#define  LOW_SPEED_ROLL     (10.0)

#define  MID_SPEED_THRUST   (95.0)
#define  MID_SPEED_PITCH    (18.0)
#define  MID_SPEED_ROLL     (18.0)

#define  HIGH_SPEED_THRUST  (95.0)
#define  HIGH_SPEED_PITCH   (30.0)
#define  HIGH_SPEED_ROLL    (30.0)

#define  MIN_THRUST			(25.0)
#define  ALT_THRUST		    (50.0)
#define  MAX_YAW			(200.0)

static bool isInit = 0;

static bool isOffse = 0;
static ctrlVal_t wifiCtrl;/*发送到commander姿态控制数据*/
static uint8_t AnopPlatform = 0; //默认上位机
uint8_t getAnopPlatform(void)
{
	return AnopPlatform;	
}

void setAnopPlatform(uint8_t set)
{
	AnopPlatform = set;	
}
static float limit(float value,float min, float max)
{
	if(value > max)
	{
		value = max;
	}
	else if(value < min)
	{
		value = min;
	}
	return value;
}

float adcGetFlyData(uint8_t adc_val,uint8_t mid_db)
{

	float temp = (float)(adc_val-mid_db);
	
	if(fabs(temp)<VEL_LIMIT) temp = 0;
	
	return (float)(temp/MID_MAX) * 2;
}

//与匿名上位机通讯协议V7 详细 查看 http://anotc.com/
void check_data(uint8_t *sum_check, uint8_t *add_check, uint8_t *pData, uint8_t len)
{
	for(uint8_t i = 0; i < len; i++)
	{
		*sum_check += pData[i];
		*add_check += *sum_check;
	}
}

//发送一包数据
void anopSendPacket(anop_t *p,uint8_t head,uint8_t d_addr)
{
	uint8_t sum_check = 0;
	uint8_t add_check = 0;

	uint8_t *pData =  (uint8_t*)p;

	p->head = head;
	p->d_addr = d_addr;
	
	check_data(&sum_check, &add_check, pData, p->dataLen+4);
	p->data[p->dataLen] = sum_check;
	p->data[p->dataLen+1] = add_check;
	//发送数据
	wifiSendData(p->dataLen+6, pData);
}
void sendAppPack(uint8_t* pData,uint8_t len)
{
	wifiSendData(len,pData);
}
//返回校验帧
//gitID：需要获取的ID
//对于ID的校验码
static void sendCheck(uint8_t getID,uint8_t sumcheck,uint8_t addcheck)
{
	uint8_t cnt = 0;
	uint8_t sum_check = 0;
	uint8_t add_check = 0;
	
	uint8_t temp[9] = {0};
	
	temp[cnt++] = UP_HEAD;
	temp[cnt++] = UP_DADDR;
	
	temp[cnt++] = ID_CHECK;
	
	temp[cnt++] = 3;
	temp[cnt++] = getID;
	temp[cnt++] = sumcheck;
	temp[cnt++] = addcheck;
	
	check_data(&sum_check, &add_check, temp,temp[3]+4);
	
	temp[cnt++] = sum_check;
	temp[cnt++] = add_check;
	
	wifiSendData(cnt, temp);
}

//发送参数
static void sendParmeData(uint16_t parmeID,int32_t par_val)
{
	uint8_t cnt = 0;
	uint8_t sum_check = 0;
	uint8_t add_check = 0;
	
	uint8_t temp[12] = {0};
	
	temp[cnt++] = UP_HEAD;
	temp[cnt++] = UP_DADDR;
	
	temp[cnt++] = ID_CMD_W;
	
	temp[cnt++] = 6;
	temp[cnt++] = BYTE0(parmeID);
	temp[cnt++] = BYTE1(parmeID);

	temp[cnt++] = BYTE0(par_val);
	temp[cnt++] = BYTE1(par_val);
	temp[cnt++] = BYTE2(par_val);
	temp[cnt++] = BYTE3(par_val);
	
	check_data(&sum_check, &add_check, temp,temp[3]+4);
	
	temp[cnt++] = sum_check;
	temp[cnt++] = add_check;
	
	wifiSendData(cnt, temp);
}
//发送log信息
bool anopSendLogStr(uint8_t *data,uint8_t len,uint8_t color)
{
	uint8_t cnt = 0;
	uint8_t sum_check = 0;
	uint8_t add_check = 0;
	anop_t p;
	
	p.head = UP_HEAD;
	p.d_addr = UP_DADDR;
	p.msgID = ID_LOG_STR_D;
	p.data[cnt++] = color;
	
	for(uint8_t i=0; i < len; i++)
	{
		p.data[cnt++] = data[i];
	}
	p.dataLen = cnt;

	check_data(&sum_check, &add_check, p.data, p.dataLen+4);
	p.data[p.dataLen+4] = sum_check;
	p.data[p.dataLen+5] = add_check;

	//发送数据
	return wifiSendData(p.dataLen+6, (uint8_t *)&p);
}


//发送 加速度、陀螺仪传感器数据
static void sendSenserAGYS(int16_t a_x, int16_t a_y, int16_t a_z,
				int16_t g_x, int16_t g_y, int16_t g_z,uint8_t shock)
{
	uint8_t cnt = 0;
	int16_t temp;
	anop_t p;

	p.msgID = ID_ACC_GYR_SHOCK_D;
	
	temp = a_x;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	temp = a_y;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	temp = a_z;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	
	temp = g_x;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	temp = g_y;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	temp = g_z;
	p.data[cnt++] = BYTE0(temp);
	p.data[cnt++] = BYTE1(temp);
	
	p.data[cnt++] = shock;
	
	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
	
}

//发送罗盘、气压、温度传感器数据
static void sendSenseMLT(int16_t m_x, int16_t m_y, int16_t m_z, 
			int32_t bar, int16_t temperature, uint8_t b_sta,uint8_t m_sta)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_MAG_ALT_TEM_D;
	
	p.data[cnt++] = BYTE0(m_x);
	p.data[cnt++] = BYTE1(m_x);
	
	p.data[cnt++] = BYTE0(m_y);
	p.data[cnt++] = BYTE1(m_y);
	
	p.data[cnt++] = BYTE0(m_z);
	p.data[cnt++] = BYTE1(m_z);
	
	p.data[cnt++]=BYTE0(bar);
	p.data[cnt++]=BYTE1(bar);
	p.data[cnt++]=BYTE2(bar);
	p.data[cnt++]=BYTE3(bar);

	p.data[cnt++] = BYTE0(temperature);
	p.data[cnt++] = BYTE1(temperature);
	
	p.data[cnt++] = b_sta;
	p.data[cnt++] = m_sta;
	
	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//飞控姿态：欧拉角格式
static void sendSenseRolPIT(int16_t rol, int16_t pit, int16_t yaw,uint8_t fusion_sta)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_ROL_PIT_YAW_D;
	
	p.data[cnt++] = BYTE0(rol);
	p.data[cnt++] = BYTE1(rol);

	p.data[cnt++] = BYTE0(pit);
	p.data[cnt++] = BYTE1(pit);

	p.data[cnt++] = BYTE0(yaw);
	p.data[cnt++] = BYTE1(yaw);

	p.data[cnt++] = fusion_sta;

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//四元数数据
static void sendSenseV0123(int16_t v0, int16_t v1, int16_t v2,int16_t v3,uint8_t fusion_sta)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_V0_V1_V2_V3_D;
	
	p.data[cnt++] = BYTE0(v0);
	p.data[cnt++] = BYTE1(v0);

	p.data[cnt++] = BYTE0(v1);
	p.data[cnt++] = BYTE1(v1);

	p.data[cnt++] = BYTE0(v2);
	p.data[cnt++] = BYTE1(v2);

	p.data[cnt++] = BYTE0(v3);
	p.data[cnt++] = BYTE1(v3);

	p.data[cnt++] = fusion_sta;

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}



static void sendControlledQuantiy(int16_t rol, int16_t pit, int16_t thr,int16_t yaw)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_CTRL_R_P_T_D;

	p.data[cnt++] = BYTE0(rol);
	p.data[cnt++] = BYTE1(rol);

	p.data[cnt++] = BYTE0(pit);
	p.data[cnt++] = BYTE1(pit);

	p.data[cnt++] = BYTE0(thr);
	p.data[cnt++] = BYTE1(thr);

	p.data[cnt++] = BYTE0(yaw);
	p.data[cnt++] = BYTE1(yaw);
	
	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}
//高度数据
static void sendSenseAltHeight(int32_t alt_fu, int32_t alt_add, uint8_t alt_sta)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_ALTFU_ALTADD_D;

	p.data[cnt++]=BYTE0(alt_fu);
	p.data[cnt++]=BYTE1(alt_fu);
	p.data[cnt++]=BYTE2(alt_fu);
	p.data[cnt++]=BYTE3(alt_fu);
	
	p.data[cnt++]=BYTE0(alt_add);
	p.data[cnt++]=BYTE1(alt_add);
	p.data[cnt++]=BYTE2(alt_add);
	p.data[cnt++]=BYTE3(alt_add);

	p.data[cnt++] = alt_sta;

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//运行模式
static void sendSenseYunMode(uint8_t mode,uint8_t locked,uint8_t cid,uint8_t cmd0,uint8_t cmd1)
{
	uint8_t cnt = 0;
	anop_t p;

	p.msgID = ID_MODE_LOCKED_D;

	p.data[cnt++]=mode;
	p.data[cnt++]=locked;
	p.data[cnt++]=cid;
	p.data[cnt++]=cmd0;
	p.data[cnt++]=cmd1;

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//电压电流数据
static void sendSenseVI(uint16_t votage, uint16_t current)
{
	uint8_t cnt = 0;
	anop_t p;
	
	p.msgID = ID_VOTAG_CURRENT_D;

	p.data[cnt++]=BYTE0(votage);
	p.data[cnt++]=BYTE1(votage);
	
	p.data[cnt++]=BYTE0(current);
	p.data[cnt++]=BYTE1(current);

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//PWM 控制量
static void sendSenseMotorPWM(uint16_t pwm1, uint16_t pwm2, uint16_t pwm3, uint16_t pwm4, 
								uint16_t pwm5, uint16_t pwm6, uint16_t pwm7, uint16_t pwm8)
{
	uint8_t cnt = 0;
	anop_t p;
	
	p.msgID = ID_CTRL_PWM_D;

	p.data[cnt++]=BYTE0(pwm1);
	p.data[cnt++]=BYTE1(pwm1);
	
	p.data[cnt++]=BYTE0(pwm2);
	p.data[cnt++]=BYTE1(pwm2);
	
	p.data[cnt++]=BYTE0(pwm3);
	p.data[cnt++]=BYTE1(pwm3);
	
	p.data[cnt++]=BYTE0(pwm4);
	p.data[cnt++]=BYTE1(pwm4);
	
	p.data[cnt++]=BYTE0(pwm5);
	p.data[cnt++]=BYTE1(pwm5);
	
	p.data[cnt++]=BYTE0(pwm6);
	p.data[cnt++]=BYTE1(pwm6);
	
	p.data[cnt++]=BYTE0(pwm7);
	p.data[cnt++]=BYTE1(pwm7);
	
	p.data[cnt++]=BYTE0(pwm8);
	p.data[cnt++]=BYTE1(pwm8);

	p.dataLen = cnt;
	
	anopSendPacket(&p,UP_HEAD,UP_DADDR);
}

//-----------------------------------------------------------------------------------------
static void anoPReceiverHandle(UDPPacket *pData)
{
	uint16_t prame_ID = 0; //参数ID
	uint8_t cmdID = 0;
	
	cmdID = *(pData->data + 2);
	if(0){
		
	}else if (cmdID == 0xE1)
	{
		prame_ID = *(pData->data + 4) + *(pData->data + 5) * 256;
		ESP_LOGI(TAG,"Received prame_ID= %x ", prame_ID);
		// switch(prame_ID)
		// {
			// case 0x00:
			// break;
		// }
		sendParmeData(prame_ID, 0x80000000);
	}else if(cmdID == 0xE0){  //控制命令帧
		uint8_t cmdCID = *(pData->data + 4);
		uint16_t cidCMD = ( (*(pData->data + 5) *256) + *(pData->data + 6) );
		if(cmdCID == 0x01){
			
		}else if(cmdCID == 0x10){
			ESP_LOGI(TAG,"Received cidCMD= %x ", cidCMD);
			switch (cidCMD)
			{
				case 0x0001:  //解锁
				
				break;
				case 0x0002:  //解锁
					setCommanderKeyFlight(false);
					setCommanderKeyland(false);
					setCommanderEmerStop(true);
				break;
				
				case 0x0005:  //一键起飞
				if(getCommanderKeyFlight() != true)
				{
					
					setCommanderCtrlMode(1);
					setCommanderKeyFlight(true);
					setCommanderKeyland(false);
				}
				else
				{
					setCommanderKeyFlight(false);
					setCommanderKeyland(true);
				}
				break;
				case 0x0006:  
					setCommanderKeyFlight(false);
					setCommanderKeyland(true);
				break;
				
			}
			
		}
		sendCheck(cmdID,*(pData->data + 15),*(pData->data + 16));
	}
}
static void anoPControllerHandle(UDPPacket *pData)
{
	uint8_t Key_status = 0;
	uint8_t temp = 0;
	uint8_t mid_data = 0;
	float fTemp = 0.0f;
	static uint16_t fly_sta = 0;
	Key_status = *(pData->data + 6);
	temp = *(pData->data + 7);
	Key_status = ((Key_status & 0xF0)| ((temp & 0xF0)>>4));
	temp = (*(pData->data + 6) & 0x0F);
	
	if(Key_status & 0x80){ //x
		setCommanderKeyFlight(false); //急停
		setCommanderKeyland(false);
		setCommanderEmerStop(true);
	}else if(Key_status & 0x40){ //a
		setCommanderKeyFlight(false); //降落
		setCommanderKeyland(true);
	}else if(Key_status & 0x20){ //b

	}else if(Key_status & 0x10){ //y 起飞
		fly_sta ++;
		if(fly_sta >= 20)
		{
			if(getCommanderKeyFlight() != true )
			{
				setCommanderFlightmode(1);///有
				if(!isOffse){
					attitude_t attitude;
					getAttitudeData(&attitude);

					wifiCtrl.trimPitch = (attitude.pitch*0.8);
					wifiCtrl.trimRoll = (attitude.roll*0.95);
					isOffse = 1;
				}

				setCommanderCtrlMode(1);
				setCommanderKeyFlight(true);
				setCommanderKeyland(false);
				
			}

			fly_sta = 0;
		}

	}else if(Key_status & 0x08){ //左

	}else if(Key_status & 0x04){ //右

	}else if(Key_status & 0x02){ //start

	}else if(Key_status & 0x01){ //back

	}else{
		fly_sta = 0;
		switch(temp)
		{
			case 0:
			
			break;
			case 1:
			
			break;
			case 2:
			
			break;
			case 3:
			
			break;
			case 4:
			
			break;
			case 5:
			
			break;
			case 6:
			
			break;
			case 7:
			
			break;
			case 8:
			
			mid_data = *(pData->data + 2); //x
			fTemp = adcGetFlyData(mid_data,MID_DB_ROLL)  * LOW_SPEED_ROLL ;
			wifiCtrl.roll = limit(fTemp,-LOW_SPEED_ROLL, LOW_SPEED_ROLL);

			mid_data = *(pData->data + 3); //x
			fTemp = adcGetFlyData(mid_data,MID_DB_PITCH)  * LOW_SPEED_PITCH ;
			wifiCtrl.pitch = limit(fTemp,-LOW_SPEED_PITCH, LOW_SPEED_PITCH);

			mid_data = *(pData->data + 4); //x
			fTemp = adcGetFlyData(mid_data,MID_DB_YAW)  * MAX_YAW ;
			wifiCtrl.yaw = limit(fTemp,-MAX_YAW, MAX_YAW);

			mid_data = *(pData->data + 5); 
			fTemp = (adcGetFlyData(mid_data,MID_DB_THRUST)  * ALT_THRUST ) + ALT_THRUST ;
			fTemp = limit(fTemp,0, 100);

			wifiCtrl.thrust = (uint16_t)(fTemp* 655.35f);
			
			//printf("wifiCtrl.thrust:%d\r\n",wifiCtrl.thrust);

			break;
		}
	}
	
}

static uint8_t AppBuf[18] = {0};
static void flySendPackApp(void)
{
	static uint16_t cnt = 0;

	if(!getUDPConnectedStatus()) return;
	if(!(cnt % 10)){
		uint8_t len= 0;
		int16_t tmp = 0;
		AppBuf[len++] = 0xAA;

		attitude_t attitude;
		getAttitudeData(&attitude);
		
		tmp = (int16_t)(attitude.roll*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);
		
		tmp = (int16_t)(attitude.pitch*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);

		tmp = (int16_t)(attitude.yaw*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);
		
		tmp = (int16_t)(wifiCtrl.roll*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);
		
		tmp = (int16_t)(wifiCtrl.pitch*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);
		
		tmp = (int16_t)(wifiCtrl.yaw*100);
		AppBuf[len++] = (uint8_t)tmp;
		AppBuf[len++] = (uint8_t)(tmp>>8);
		
		uint16_t bat = (uint16_t)(pmGetBatteryVoltage()*100.0f);
		
		AppBuf[len++] = BYTE0(bat);
		AppBuf[len++] = BYTE1(bat);
		
		AppBuf[len++] = 0xFF;

		sendAppPack(AppBuf,len);
		
		print_commander();
		
	}
	cnt++;
}

//定时1ms调用 发送数据包到上位机
static void anoPSendPeriod(void)
{
	static uint16_t ms_cnt = 0;
	
	if(!getUDPConnectedStatus()) return;
	
	//1、发送飞控姿态：欧拉角格式
	if(!(ms_cnt % PERIOD_SENSOR)){
		attitude_t attitude;
		getAttitudeData(&attitude);
		sendSenseRolPIT(attitude.roll*100, attitude.pitch*100, attitude.yaw*100,1);
	}
	//2、发送 加速度、陀螺仪传感器数据
	if(!(ms_cnt % PERIOD_SENSOR2)){
		Axis3i16 acc;
		Axis3i16 gyro;
		Axis3i16 mag;
		getSensorRawData(&acc, &gyro,&mag);
		sendSenserAGYS(acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z,0);
	}
	//3、发送四元数数据
	if(!(ms_cnt % PERIOD_Q)){
		float q0 = 1.0f;
		float q1 = 0.0f;
		float q2 = 0.0f;
		float q3 = 0.0f;	
		getQuarternion(&q0,&q1,&q2,&q3);
		sendSenseV0123((int16_t )(q0*10000), (int16_t )(q1*10000), (int16_t )(q2*10000),(int16_t )(q3*10000),1);
	}

	//4、发送罗盘、气压、温度传感器数据
	if(!(ms_cnt % PERIOD_SENSOR3)){
		Axis3i16 acc;
		Axis3i16 gyro;
		Axis3i16 mag;
		
		getSensorRawData(&acc, &gyro,&mag);
		
		int baroData = getBaroData();
		
		int16_t temp= (int16_t)(getBaroTemp()*100.0f);
		sendSenseMLT(mag.x, mag.y, mag.z,baroData, temp, 1,1);
	}
	
	if(!(ms_cnt % PERIOD_HEIGHT)){
		control_t get;
		getControlledQuantiy(&get);
		sendControlledQuantiy(get.roll, get.pitch, get.thrust,get.yaw);
	}

	//5、速度数据 //目标姿态/速度数据
	
	//6、PWM 控制量
	if(!(ms_cnt % PERIOD_MOTOR)){
		motorPower_t motorPWM;
		uint16_t m1,m2,m3,m4;
		
		getMotorPWM(&motorPWM);
		
		m1 = (float)motorPWM.m1/65535*1000;
		m2 = (float)motorPWM.m2/65535*1000;
		m3 = (float)motorPWM.m3/65535*1000;
		m4 = (float)motorPWM.m4/65535*1000;

		sendSenseMotorPWM(m1,m2,m3,m4, 0, 0,0, 0);
	}
	//7、电压电流数据
	if(!(ms_cnt % PERIOD_POWER)){
		uint16_t bat = (uint16_t)(pmGetBatteryVoltage()*100.0f);
		sendSenseVI(bat, 500);
	}
	
	//9、高度数据数据
	if(!(ms_cnt % PERIOD_HEIGHT)){
		
		/*读取融合高度 单位cm*/	
		int32_t FusedHeight =(int32_t) (getFusedHeight()*100);
		sendSenseAltHeight(FusedHeight, 0,0);
	}
	if(!(ms_cnt % PERIOD_STATUS)){
		bool isLock = getLockStatus();
		uint8_t CtrlMode = getCommanderCtrlMode();
		sendSenseYunMode(CtrlMode,isLock?0:1,0x10,0x00,0x04);
	}
	
	if(++ms_cnt >= 65535){
		ms_cnt = 0;
	}
}
static void anopTxTask(void *param)
{

	while(1)
	{
		if(AnopPlatform == 0){
			anoPSendPeriod();
			vTaskDelay(1);
		}else{
			flySendPackApp();
			vTaskDelay(100);
		}
	}
}
static void anopRxTask(void *param)
{
	UDPPacket udp_pack;
	
	wifiCtrl.roll   = 0;	/*roll: ±9.5 ±19.2 ±31.7*/
	wifiCtrl.pitch  = 0;	/*pitch:±9.5 ±19.2 ±31.7*/
	wifiCtrl.yaw    = 0;	/*yaw : ±203.2*/				
	wifiCtrl.thrust = 32768;					/*thrust :0~63356*/

	while(1)
	{
		if(wifiGetDataBlocking(&udp_pack))
		{
			switch (AnopPlatform)
			{
				case 0:
				anoPReceiverHandle(&udp_pack);
				break;
				
				case 1:
				anoPControllerHandle(&udp_pack);
				flightCtrldataCache(WIFI, wifiCtrl);
				
				break;
			}
			
		}
		vTaskDelay(1);
	}
}
static void cntDataTask(void *param)
{
	vTaskDelay(2000);

	static uint16_t lastThrust;
	
	wifiCtrl.roll   = 0;	/*roll: ±9.5 ±19.2 ±31.7*/
	wifiCtrl.pitch  = 0;	/*pitch:±9.5 ±19.2 ±31.7*/
	wifiCtrl.yaw    = 0;	/*yaw : ±203.2*/				
	wifiCtrl.thrust = 32768;					/*thrust :0~63356*/

	if(wifiCtrl.thrust==32768 && lastThrust<10000)/*手动飞切换到定高*/
	{
		setCommanderCtrlMode(1);
		setCommanderKeyFlight(false);
		setCommanderKeyland(false);
	}
	else if(wifiCtrl.thrust==0 && lastThrust>256)/*定高切换成手动飞*/
	{
		setCommanderCtrlMode(0);
		wifiCtrl.thrust = 0;
	}
	lastThrust = wifiCtrl.thrust;

	flightCtrldataCache(WIFI, wifiCtrl);

	while(1)
	{
		if(AnopPlatform != 0) break;

		flightCtrldataCache(WIFI, wifiCtrl);
		vTaskDelay(200);
	}
	vTaskDelete(NULL);
}

void anopInit(void)
{
	if(isInit) return;
	

	xTaskCreate(anopTxTask, "ANOP_TX", 4*1024, NULL, 3, NULL);				/*创建anop发送任务任务*/

	xTaskCreate(anopRxTask, "ANOP_RX", 4*1024, NULL, 3, NULL);				/*创建anop发送任务任务*/

	xTaskCreate(cntDataTask, "cont_dat", 3*1024, NULL, 3, NULL);				/*创建anop发送任务任务*/

	isInit = true;
}


