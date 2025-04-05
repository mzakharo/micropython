
#include "config.h"
#include "config_param.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#define VERSION 10	/*10 表示V1.0*/

#define TAG "config_param"
configParam_t configParam;

static configParam_t configParamDefault=
{
	.version = VERSION,		/*软件版本号*/

	.pidAngle=	/*角度PID*/
	{	
		.roll=
		{
			.kp=8.0,
			.ki=0.0,
			.kd=0.0,
		},
		.pitch=
		{
			.kp=8.0,
			.ki=0.0,
			.kd=0.0,
		},
		.yaw=
		{
			.kp=20.0,
			.ki=0.0,
			.kd=1.5,
		},
	},	
	.pidRate=	/*角速度PID*/
	{	
		.roll=
		{
			.kp=300.0,
			.ki=0.0,
			.kd=6.5,
		},
		.pitch=
		{
			.kp=300.0,
			.ki=0.0,
			.kd=6.5,
		},
		.yaw=
		{
			.kp=200.0,
			.ki=18.5,
			.kd=0.0,
		},
	},	
	.pidPos=	/*位置PID*/
	{	
		.vx=
		{
			.kp=4.5,
			.ki=0.0,
			.kd=0.0,
		},
		.vy=
		{
			.kp=4.5,
			.ki=0.0,
			.kd=0.0,
		},
		.vz=
		{
			.kp=100.0,
			.ki=150.0,
			.kd=10.0,
		},
		
		.x=
		{
			.kp=4.0,
			.ki=0.0,
			.kd=0.6,
		},
		.y=
		{
			.kp=4.0,
			.ki=0.0,
			.kd=0.6,
		},
		.z=
		{
			.kp=6.0,
			.ki=0.0,
			.kd=4.5,
		},
	},
	
	.trimP = 0.f,	/*pitch微调*/
	.trimR = 0.f,	/*roll微调*/
	.thrustBase=34000,	/*定高油门基础值*/
};

static uint16_t lenth = 0;
static bool isInit = false;
static bool isConfigParamOK = false;

static SemaphoreHandle_t  xSemaphore = NULL;


static uint8_t configParamCksum(configParam_t* data)
{
	int i;
	uint8_t cksum=0;	
	uint8_t* c = (uint8_t*)data;  	
	size_t len=sizeof(configParam_t);

	for (i=0; i<len; i++)
		cksum += *(c++);
	cksum-=data->cksum;
	
	return cksum;
}

void configParamInit(void)	/*参数配置初始化*/
{
	if(isInit) return;
	
	lenth=sizeof(configParam);
	lenth=lenth/4+(lenth%4 ? 1:0);

	//STMFLASH_Read(CONFIG_PARAM_ADDR, (uint16_t *)&configParam, lenth);
	
	if(configParam.version == VERSION)	/*版本正确*/
	{
		if(configParamCksum(&configParam) == configParam.cksum)	/*校验正确*/
		{
			ESP_LOGI(TAG,"Version V%1.1f check [OK]\r\n", configParam.version / 10.0f);
			isConfigParamOK = true;
		} else
		{
			ESP_LOGE(TAG,"Version check [FAIL]\n");
			isConfigParamOK = false;
		}
	}	
	else	/*版本更新*/
	{
		isConfigParamOK = false;
	}
	
	if(isConfigParamOK == false)	/*配置参数错误，写入默认参数*/
	{
		memcpy((uint8_t *)&configParam, (uint8_t *)&configParamDefault, sizeof(configParam));
		configParam.cksum = configParamCksum(&configParam);				/*计算校验值*/
		//STMFLASH_Write(CONFIG_PARAM_ADDR,(uint16_t *)&configParam, lenth);
		isConfigParamOK=true;
	}	
	
	xSemaphore = xSemaphoreCreateBinary();
	
	isInit=true;
}

void configParamTask(void* param)
{
	uint8_t cksum = 0;
	
	while(1) 
	{	
		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		cksum = configParamCksum(&configParam);		/*数据校验*/
		
		if(configParam.cksum != cksum)	
		{
			//configParam.cksum = cksum;	/*数据校验*/
			//watchdogInit(500);			/*擦除时间比较长，看门狗时间设置大一些*/					
			//STMFLASH_Write(CONFIG_PARAM_ADDR,(uint16_t *)&configParam, lenth);	/*写入stm32 flash*/
			//watchdogInit(WATCHDOG_RESET_MS);		/*重新设置看门狗*/
		}						
	}
}

bool configParamTest(void)
{
	return isInit;
}

void configParamGiveSemaphore(void)
{
	xSemaphoreGive(xSemaphore);		
}

void resetConfigParamPID(void)
{
	configParam.pidAngle = configParamDefault.pidAngle;
	configParam.pidRate = configParamDefault.pidRate;
	configParam.pidPos = configParamDefault.pidPos;
}

void saveConfigAndNotify(void)
{
	uint8_t cksum = configParamCksum(&configParam);		/*数据校验*/
	if(configParam.cksum != cksum)	
	{
		configParam.cksum = cksum;	/*数据校验*/				
		//STMFLASH_Write(CONFIG_PARAM_ADDR,(uint16_t *)&configParam, lenth);
	}
}