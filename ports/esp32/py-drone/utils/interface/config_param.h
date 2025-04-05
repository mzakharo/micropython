#ifndef __CONFIG_PARAM_H
#define __CONFIG_PARAM_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "math.h"

typedef struct 
{
	float kp;
	float ki;
	float kd;
} pidInit_t;

typedef struct
{
	pidInit_t roll;
	pidInit_t pitch;	
	pidInit_t yaw;	
} pidParam_t;

typedef struct
{
	pidInit_t vx;
	pidInit_t vy;
	pidInit_t vz;
	
	pidInit_t x;
	pidInit_t y;
	pidInit_t z;
} pidParamPos_t;

typedef struct
{
	int16_t accZero[3];
	int16_t accGain[3];
} accBias_t;

typedef struct
{
	int16_t magZero[3];
} magBias_t;

typedef struct 
{
    int16_t rollDeciDegrees;
    int16_t pitchDeciDegrees;
    int16_t yawDeciDegrees;
} boardAlignment_t;

typedef struct	
{
	uint8_t version;				/*软件版本号*/
	pidParam_t pidAngle;	/*角度PID*/	
	pidParam_t pidRate;		/*角速度PID*/	
	pidParamPos_t pidPos;	/*位置PID*/
//	accBias_t accBias;		/*加速度校准值*/
//	magBias_t magBias;		/*磁力计校准值*/
	float trimP;			/*pitch微调*/
	float trimR;			/*roll微调*/
	uint16_t thrustBase;			/*油门基础值*/
	uint8_t cksum;				/*校验*/
} configParam_t;


extern configParam_t configParam;

void configParamInit(void);	/*参数配置初始化*/
void configParamTask(void* param);	/*参数配置任务*/
bool configParamTest(void);

void configParamGiveSemaphore(void);
void resetConfigParamPID(void);
void saveConfigAndNotify(void);

#endif /*__CONFIG_PARAM_H */

