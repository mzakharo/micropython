
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "py/obj.h"
#include "anop.h"
#include "system_int.h"
#include "config_param.h"
#include "esp_log.h"

#include "pm_esplane.h"
#include "sensors_mpu6050_spl06.h"
#include "stabilizer.h"
#include "ledseq.h"
#include "motors.h"

static const char* TAG = "system_int";

static bool isInit = 0;

static bool systemTest(void)
{
	bool pass = true;
	
	pass &= ledseqTest();  //led序列测试
	pass &= pmTest();
	//pass &= commTest();
	pass &= stabilizerTest();
	
	return pass;
}
//初始化系统
void systemInit(void)
{
	if(isInit) return;
	ESP_LOGI(TAG, "start systeminit ..");
	
	//初始化电源管理
	ledseqInit();
	ledseqRun(SYS_LED, seq_alive);
	configParamInit();//初始化参数
	motorsInit();
	pmInit();
	//初始化传感器
	sensorsMpu6050Spl06Init();
	//初始化姿态处理
	stabilizerInit();
	systemTest();
}
void systemDeInit(void)
{
	stabilizerDeInit();

	sensorsMpu6050Spl06DeInit();
	motorsDeInit();
	pmDeInit();
	ledseqDeInit();
	//sensorsI2CdevDeInit();
	
	isInit = false;
}


