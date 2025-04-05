/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * pid.h - implementation of the PID regulator
 */
#ifndef PID_H_
#define PID_H_

#include <stdbool.h>
#include "config_param.h"


#define DEFAULT_PID_INTEGRATION_LIMIT 		500.0 //默认pid的积分限幅
#define DEFAULT_PID_OUTPUT_LIMIT      		0.0	  //默认pid输出限幅，0为不限幅

typedef struct
{
	float desired;		//< set point
	float error;        //< error
	float prevError;    //< previous error
	float integ;        //< integral
	float deriv;        //< derivative
	float kp;           //< proportional gain
	float ki;           //< integral gain
	float kd;           //< derivative gain
	float outP;         //< proportional output (debugging)
	float outI;         //< integral output (debugging)
	float outD;         //< derivative output (debugging)
	float iLimit;       //< integral limit
	float outputLimit;  //< total PID output limit, absolute value. '0' means no limit.
	float dt;           //< delta-time dt
	float out;			//< out
} PidObject;

/*pid结构体初始化*/
void pidInit(PidObject* pid, const float desired, const pidInit_t pidParam, const float dt);
void pidSetIntegralLimit(PidObject* pid, const float limit);/*pid积分限幅设置*/
void pidSetOutputLimit(PidObject* pid, const float limit);
void pidSetDesired(PidObject* pid, const float desired);	/*pid设置期望值*/
float pidUpdate(PidObject* pid, const float error);			/*pid更新*/
float pidGetDesired(PidObject* pid);	/*pid获取期望值*/
bool pidIsActive(PidObject* pid);		/*pid状态*/
void pidReset(PidObject* pid);			/*pid结构体复位*/
void pidSetError(PidObject* pid, const float error);/*pid偏差设置*/
void pidSetKp(PidObject* pid, const float kp);		/*pid Kp设置*/
void pidSetKi(PidObject* pid, const float ki);		/*pid Ki设置*/
void pidSetKd(PidObject* pid, const float kd);		/*pid Kd设置*/
void pidSetDt(PidObject* pid, const float dt);		/*pid dt设置*/


#endif /* PID_H_ */
