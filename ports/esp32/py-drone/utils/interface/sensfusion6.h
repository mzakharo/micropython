#ifndef __SENSFUSION6_H
#define __SENSFUSION6_H
#include "stabilizer_types.h"

void imuUpdate(Axis3f acc, Axis3f gyro, state_t *state , float dt);	/*数据融合 互补滤波*/
bool getIsCalibrated(void);
void setCalibrated(bool set);
void imuTransformVectorBodyToEarth(Axis3f * v);	/*机体到地球*/
void imuTransformVectorEarthToBody(Axis3f * v);	/*地球到机体*/
void getQuarternion(float *pQ0,float *pQ1,float *pQ2,float *pQ3);

#endif

