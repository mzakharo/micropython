#ifndef __ATTITUDE_PID_H
#define __ATTITUDE_PID_H
#include <stdbool.h>
#include "commander.h"
#include "pid.h"

extern PidObject pidAngleRoll;
extern PidObject pidAnglePitch;
extern PidObject pidAngleYaw;

extern PidObject pidRateRoll;
extern PidObject pidRatePitch;
extern PidObject pidRateYaw;

void attitudeControlInit(float rateDt, float angleDt);
bool attitudeControlTest(void);

void attitudeRatePID(Axis3f *actualRate,attitude_t *desiredRate,control_t *output);
void attitudeAnglePID(attitude_t *actualAngle,attitude_t *desiredAngle,attitude_t *outDesiredRate);
void attitudeControllerResetRollAttitudePID(void);
void attitudeControllerResetPitchAttitudePID(void);
void attitudeResetAllPID(void);
void attitudePIDwriteToConfigParam(void);

#endif /* __ATTITUDE_PID_H */
