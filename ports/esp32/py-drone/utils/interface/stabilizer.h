#ifndef __STABALIZER_H
#define __STABALIZER_H
#include <stdbool.h>
#include <stdint.h>
#include "stabilizer_types.h"

#define RATE_5_HZ		5
#define RATE_10_HZ		10
#define RATE_25_HZ		25
#define RATE_50_HZ		50
#define RATE_100_HZ		100
#define RATE_200_HZ 	200
#define RATE_250_HZ 	250
#define RATE_500_HZ 	500
#define RATE_1000_HZ 	1000

#define RATE_DO_EXECUTE(RATE_HZ, TICK) ((TICK % (MAIN_LOOP_RATE / RATE_HZ)) == 0)


#define MAIN_LOOP_RATE 			RATE_1000_HZ
#define MAIN_LOOP_DT			(uint32_t)(1000/MAIN_LOOP_RATE)	/*单位ms*/

#define ATTITUDE_ESTIMAT_RATE	RATE_250_HZ	//姿态解算速率
#define ATTITUDE_ESTIMAT_DT		(1.0/RATE_250_HZ)

#define POSITION_ESTIMAT_RATE	RATE_250_HZ	//位置预估速率
#define POSITION_ESTIMAT_DT		(1.0/RATE_250_HZ)

#define RATE_PID_RATE			RATE_500_HZ //角速度环（内环）PID速率
#define RATE_PID_DT				(1.0/RATE_500_HZ)

#define ANGEL_PID_RATE			ATTITUDE_ESTIMAT_RATE //角度环（外环）PID速率
#define ANGEL_PID_DT			(1.0/ATTITUDE_ESTIMAT_RATE)

#define VELOCITY_PID_RATE		POSITION_ESTIMAT_RATE //速度环（内环）PID速率
#define VELOCITY_PID_DT			(1.0/POSITION_ESTIMAT_RATE)

#define POSITION_PID_RATE		POSITION_ESTIMAT_RATE //位置环（外环）PID速率
#define POSITION_PID_DT			(1.0/POSITION_ESTIMAT_RATE)



void stabilizerInit(void);
void stabilizerDeInit(void);
void stabilizerTask(void* param);
bool stabilizerTest(void);

void getAttitudeData(attitude_t* get);
float getBaroData(void);
float getBaroTemp(void);
bool getLockStatus(void);

void getSensorData(sensorData_t* get);	
void getStateData(Axis3f* acc, Axis3f* vel, Axis3f* pos);
void setFastAdjustPosParam(uint16_t velTimes, uint16_t absTimes, float height);/*设置快速调整位置参数*/
void getControlledQuantiy(control_t* get);


#endif /* __STABALIZER_H */
