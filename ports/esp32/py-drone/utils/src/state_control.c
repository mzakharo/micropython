#include <math.h>
#include "state_control.h"
#include "stabilizer.h"
#include "attitude_pid.h"
#include "config_param.h"
#include "position_pid.h"


static float actualThrust;
static attitude_t attitudeDesired;
static attitude_t rateDesired;


void stateControlInit(void)
{
	attitudeControlInit(RATE_PID_DT, ANGEL_PID_DT); /*初始化姿态PID*/	
	positionControlInit(VELOCITY_PID_DT, POSITION_PID_DT); /*初始化位置PID*/
}

bool stateControlTest(void)
{
	bool pass = true;
	pass &= attitudeControlTest();
	return pass;
} 

void stateControl(control_t *control, sensorData_t *sensors, state_t *state, setpoint_t *setpoint, const uint32_t tick)
{
	static uint16_t cnt = 0;
	
	if (RATE_DO_EXECUTE(POSITION_PID_RATE, tick))
	{
		if (setpoint->mode.x != modeDisable || setpoint->mode.y != modeDisable || setpoint->mode.z != modeDisable)
		{
			positionController(&actualThrust, &attitudeDesired, setpoint, state, POSITION_PID_DT);
		}
	}
	
	//角度环（外环）
	if (RATE_DO_EXECUTE(ANGEL_PID_RATE, tick))
	{
		if (setpoint->mode.z == modeDisable)
		{
			actualThrust = setpoint->thrust;
		}
		if (setpoint->mode.x == modeDisable || setpoint->mode.y == modeDisable) 
		{
			attitudeDesired.roll = setpoint->attitude.roll;
			attitudeDesired.pitch = setpoint->attitude.pitch;
		}
		
		if(control->flipDir == CENTER)
		{
			attitudeDesired.yaw += setpoint->attitude.yaw/ANGEL_PID_RATE; /*期望YAW 速率模式*/
			if(attitudeDesired.yaw > 180.0f) 
				attitudeDesired.yaw -= 360.0f;
			if(attitudeDesired.yaw < -180.0f) 
				attitudeDesired.yaw += 360.0f;
		}
			
		attitudeDesired.roll += configParam.trimR;	//叠加微调值
		attitudeDesired.pitch += configParam.trimP;		
		
		attitudeAnglePID(&state->attitude, &attitudeDesired, &rateDesired);
	}
	
	//角速度环（内环）
	if (RATE_DO_EXECUTE(RATE_PID_RATE, tick))
	{
		if (setpoint->mode.roll == modeVelocity)
		{
			rateDesired.roll = setpoint->attitudeRate.roll;
			attitudeControllerResetRollAttitudePID();
		}
		if (setpoint->mode.pitch == modeVelocity)
		{
			rateDesired.pitch = setpoint->attitudeRate.pitch;
			attitudeControllerResetPitchAttitudePID();
		}
		extern uint8_t fstate;
		if (control->flipDir != CENTER && fstate == 4)	/*空翻过程只使用内环PID*/
		{
			rateDesired.pitch = setpoint->attitude.pitch;
			rateDesired.roll = setpoint->attitude.roll;
		}
		
		attitudeRatePID(&sensors->gyro, &rateDesired, control);
	}

	control->thrust = actualThrust;	
	
	if (control->thrust < 5.f)
	{			
		control->roll = 0;
		control->pitch = 0;
		control->yaw = 0;
		
		attitudeResetAllPID();	/*复位姿态PID*/	
		positionResetAllPID();	/*复位位置PID*/
		attitudeDesired.yaw = state->attitude.yaw;		/*复位计算的期望yaw值*/
		
		if(cnt++ > 1500)
		{
			cnt = 0;
			configParamGiveSemaphore();
		}
	}else
	{
		cnt = 0;
	}
}


