#ifndef __POSITION_PID_H
#define __POSITION_PID_H
#include "stabilizer_types.h"
#include "pid.h"


extern PidObject pidVX;
extern PidObject pidVY;
extern PidObject pidVZ;

extern PidObject pidX;
extern PidObject pidY;
extern PidObject pidZ;

void positionControlInit(float ratePidDt, float posPidDt);
void positionResetAllPID(void);
void positionController(float* thrust, attitude_t *attitude, setpoint_t *setpoint, const state_t *state, float dt);
void positionPIDwriteToConfigParam(void);
float getAltholdThrust(void);
	
#endif /* __POSITION_PID_H */
