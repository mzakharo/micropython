#ifndef __STATE_ESTIMATOR_H
#define __STATE_ESTIMATOR_H
#include "stabilizer_types.h"

typedef struct
{
	float vAccDeadband; /* 加速度死区 */
	float accBias[3];	/* 加速度 偏置(cm/s/s)*/
	float acc[3];		/* 估测加速度 单位(cm/s/s)*/
	float vel[3];		/* 估测速度 单位(cm/s)*/
	float pos[3]; 		/* 估测位移 单位(cm)*/
} estimator_t;

void positionEstimate(sensorData_t* sensorData, state_t* state, float dt);	
float getFusedHeight(void);	/*读取融合高度*/
void estRstHeight(void);	/*复位估测高度*/
void estRstAll(void);		/*复位所有估测*/

#endif /* __STATE_ESTIMATOR_H */


