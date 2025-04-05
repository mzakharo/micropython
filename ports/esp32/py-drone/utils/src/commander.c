#include <math.h>
#include "maths.h"
#include "commander.h"

#include "config_param.h"
#include "position_pid.h"
//#include "remoter_ctrl.h"
#include "stabilizer.h"
#include "state_estimator.h"
#include "attitude_pid.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ledseq.h"
#define CLIMB_RATE			100.f
#define MAX_CLIMB_UP		100.f
#define MAX_CLIMB_DOWN		60.f

#define MIN_THRUST  	5000
#define MAX_THRUST  	60000

static bool isRCLocked;				/* 遥控锁定状态 */
static bool isLastRCLocked;				/* 遥控锁定状态 */
static ctrlValCache_t remoteCache;	/* 遥控缓存数据 */
static ctrlValCache_t wifiCache;	/* wifi缓存数据 */
static ctrlValCache_t* nowCache = &remoteCache;/*默认为遥控*/
static ctrlVal_t ctrlValLpf = {0.f};/* 控制数据低通 */

static float minAccZ = 0.f; 
static float maxAccZ = 0.f; 

static YawModeType yawMode =CAREFREE;// XMODE;	/* 默认为X飞行模式 */

static commanderBits_t commander;
static float landing_dis = 80.0f;

void setLandingDis(float dis)
{
	landing_dis = dis;
}
float getLandingDis(void)
{
	return landing_dis;
}

static void commanderLevelRPY(void)
{
	ctrlValLpf.roll = 0;
	ctrlValLpf.pitch = 0;
	ctrlValLpf.yaw = 0;
}

static void commanderDropToGround(void)
{
	commanderLevelRPY();
	ctrlValLpf.thrust = 0;
	if(commander.keyFlight)	/*飞行过程中，遥控器信号断开，一键降落*/
	{
		commander.keyLand = true;
		commander.keyFlight = false;
	}	
}
bool getRCLocked(void)
{
	return	isRCLocked;
}
uint32_t timestamp = 0;
/********************************************************
 *ctrlDataUpdate()	更新控制数据
 *遥控数据 优先级高于wifi控制数据
*********************************************************/
static void ctrlDataUpdate(void)	
{
	static float lpfVal = 0.2f;
	uint32_t tickNow = xTaskGetTickCount();	

	timestamp = tickNow - wifiCache.timestamp;
	
	if ((tickNow - remoteCache.timestamp) < COMMANDER_WDT_TIMEOUT_STABILIZE) 
	{
		isRCLocked = false;			/*解锁*/
		nowCache = &remoteCache;	/* 遥控缓存数据 */
	}else if ((tickNow - wifiCache.timestamp) < COMMANDER_WDT_TIMEOUT_STABILIZE) 
	{
		isRCLocked = false;			/*解锁*/
		nowCache = &wifiCache;		/* wifi缓存数据 */
	}else if ((tickNow - remoteCache.timestamp) < COMMANDER_WDT_TIMEOUT_SHUTDOWN) 
	{
		nowCache = &remoteCache;	/* 遥控缓存数据 */
		commanderLevelRPY();
	}else if ((tickNow - wifiCache.timestamp) < COMMANDER_WDT_TIMEOUT_SHUTDOWN) 
	{
		nowCache = &wifiCache;		/* wifi缓存数据 */
		commanderLevelRPY();
	} else 
	{
		isRCLocked = true;			/*锁定*/
		nowCache = &remoteCache;
		if(commander.emerStop == false) commanderDropToGround();
		
		if(isLastRCLocked == true)
		{
			ledseqStop(LINK_LED, seq_linkup);
			isLastRCLocked = false;
		}
	}
	
	if(isRCLocked == false)	/*解锁状态*/
	{
		if(isLastRCLocked == false)
		{
			ledseqRun(LINK_LED, seq_linkup);
			isLastRCLocked = true;
		}
		
		ctrlVal_t ctrlVal =  nowCache->tarVal[nowCache->activeSide];	/*读取缓存*/

		ctrlValLpf.thrust += (ctrlVal.thrust - ctrlValLpf.thrust) * lpfVal;
		ctrlValLpf.pitch += (ctrlVal.pitch - ctrlValLpf.pitch) * lpfVal;
		ctrlValLpf.roll += (ctrlVal.roll - ctrlValLpf.roll) * lpfVal;
		ctrlValLpf.yaw += (ctrlVal.yaw - ctrlValLpf.yaw) * lpfVal;
		
		configParam.trimP = ctrlVal.trimPitch;	/*更新微调值*/
		configParam.trimR = ctrlVal.trimRoll;
		
		if (ctrlValLpf.thrust < MIN_THRUST)
			ctrlValLpf.thrust = 0;	
		else 		
			ctrlValLpf.thrust = (ctrlValLpf.thrust>=MAX_THRUST) ? MAX_THRUST:ctrlValLpf.thrust;
	}
	
}

/************************************************************************
* 四轴carefree(无头模式)，参考世界坐标系，当四轴围绕YAW旋转后，
* 四轴前方任然保持开始的方向，这个模式对新手非常实用
************************************************************************/
static void rotateYawCarefree(setpoint_t *setpoint, const state_t *state)
{
	float yawRad = state->attitude.yaw * DEG2RAD;
	float cosy = cosf(yawRad);
	float siny = sinf(yawRad);
	
	if(setpoint->mode.x ==  modeDisable || setpoint->mode.y ==  modeDisable)	/*手动和定高模式*/
	{
		float originalRoll = setpoint->attitude.roll;
		float originalPitch = setpoint->attitude.pitch;

		setpoint->attitude.roll = originalRoll * cosy + originalPitch * siny;
		setpoint->attitude.pitch = originalPitch * cosy - originalRoll * siny;
	}
	else if(setpoint->mode.x ==  modeVelocity || setpoint->mode.y ==  modeVelocity)	/*定点模式*/
	{
		float originalVy = setpoint->velocity.y;
		float originalVx = setpoint->velocity.x;

		setpoint->velocity.y = originalVy * cosy + originalVx * siny;
		setpoint->velocity.x = originalVx * cosy - originalVy * siny;
	}
}

/*飞控数据缓存*/
void flightCtrldataCache(ctrlSrc_e ctrlSrc, ctrlVal_t pk)
{
	switch(ctrlSrc)
	{
		case ATK_REMOTER:
			remoteCache.tarVal[!remoteCache.activeSide] = pk;
			remoteCache.activeSide = !remoteCache.activeSide;
			remoteCache.timestamp = xTaskGetTickCount();
			break;
		
		case WIFI:
			wifiCache.tarVal[!wifiCache.activeSide] = pk;
			wifiCache.activeSide = !wifiCache.activeSide;
			wifiCache.timestamp = xTaskGetTickCount();
			break;
		default :
			break;
	}
}

extern bool isExitFlip;			/*是否退出空翻*/
/********************************************************
* flyerAutoLand()
* 四轴自动降落
*********************************************************/
void flyerAutoLand(setpoint_t *setpoint,const state_t *state)
{	
	static uint8_t lowThrustCnt = 0;
	static float stateVelLpf  = -30.f;
	
	setpoint->mode.z = modeVelocity;
	stateVelLpf += (state->velocity.z -  stateVelLpf) * 0.1f;	/*速率低通*/
	setpoint->velocity.z = -70.f - stateVelLpf;	/*降落速度 单位cm/s*/

	if(getAltholdThrust() < 20000.f)	/*定高油门值较低*/
	{
		lowThrustCnt++;
		if(lowThrustCnt > 10)
		{
			lowThrustCnt = 0;
			commander.keyLand = false;
			commander.keyFlight = false;
			estRstAll();	/*复位估测*/
		}	
	}else
	{
		lowThrustCnt = 0;
	}
	
	if(isExitFlip == true)	/*退出空翻，再检测加速度*/
	{
		float accZ = state->acc.z;
		if(minAccZ > accZ)
			minAccZ = accZ;
		if(maxAccZ < accZ)
			maxAccZ = accZ;
	}
	
	if (minAccZ < -80.f && maxAccZ > 320.f)
	{
		commander.keyLand = false;
		commander.keyFlight = false;
		estRstAll();	/*复位估测*/
	}	
}

static bool initHigh = false;
static bool isAdjustingPosZ = false;/*调整Z位置*/
static bool isAdjustingPosXY = true;/*调整XY位置*/

static float errorPosX = 0.f;		/*X位移误差*/
static float errorPosY = 0.f;		/*Y位移误差*/
static float errorPosZ = 0.f;		/*Z位移误差*/


void commanderGetSetpoint(setpoint_t *setpoint, state_t *state)
{	
	static float maxAccZ = 0.f;

	ctrlDataUpdate();	/*更新控制数据*/
	
	state->isRCLocked = isRCLocked;	/*更新遥控器锁定状态*/
	
	if(commander.ctrlMode & 0x01)/*定高模式*/
	{
		if(commander.keyLand)/*一键降落*/
		{
			flyerAutoLand(setpoint, state);
		}
		else if(commander.keyFlight)/*一键起飞*/ 
		{	
			setpoint->thrust = 0;
			setpoint->mode.z = modeAbs;		
			
			if (initHigh == false)
			{
				initHigh = true;	
				isAdjustingPosXY = true;
				errorPosX = 0.f;
				errorPosY = 0.f;
				errorPosZ = 0.f;

				setFastAdjustPosParam(0, 1, landing_dis);	/*一键起飞高度80cm*/			
			}		
				
			float climb = ((ctrlValLpf.thrust - 32767.f) / 32767.f);
			if(climb > 0.f) 
				climb *= MAX_CLIMB_UP;
			else
				climb *= MAX_CLIMB_DOWN;
			
			if (fabsf(climb) > 5.f)
			{
				isAdjustingPosZ = true;												
				setpoint->mode.z = modeVelocity;
				setpoint->velocity.z = climb;

				if(climb < -(CLIMB_RATE/5.f))	/*油门下拉过大*/
				{
					if(isExitFlip == true)		/*退出空翻，再检测加速度*/
					{
						if(maxAccZ < state->acc.z)
							maxAccZ = state->acc.z;
						if(maxAccZ > 250.f)		/*油门下拉过大，飞机触地停机*/
						{
							commander.keyFlight = false;
							estRstAll();	/*复位估测*/
						}
					}
				}else
				{
					maxAccZ = 0.f;
				}
			}
			else if (isAdjustingPosZ == true)
			{
				isAdjustingPosZ = false;
			
				setpoint->mode.z = modeAbs;
				setpoint->position.z = state->position.z + errorPosZ;	/*调整新位置*/									
			}
			else if(isAdjustingPosZ == false)	/*Z位移误差*/
			{
				errorPosZ = setpoint->position.z - state->position.z;
				errorPosZ = constrainf(errorPosZ, -10.f, 10.f);	/*误差限幅 单位cm*/
			}
		}
		else/*着陆状态*/
		{
			setpoint->mode.z = modeDisable;
			setpoint->thrust = 0;
			setpoint->velocity.z = 0;
			setpoint->position.z = 0;
			initHigh = false;
			isAdjustingPosZ = false;
		}
	}
	else /*手动飞模式*/
	{
		setpoint->mode.z = modeDisable;
		setpoint->thrust = ctrlValLpf.thrust;
	}	
 	
	setpoint->attitude.roll = ctrlValLpf.roll;
	setpoint->attitude.pitch = ctrlValLpf.pitch;
	setpoint->attitude.yaw  = -ctrlValLpf.yaw;	/*摇杆方向和yaw方向相反*/
	{
		setpoint->mode.x = modeDisable;
		setpoint->mode.y = modeDisable;		
	}
	
	setpoint->mode.roll = modeDisable;	
	setpoint->mode.pitch = modeDisable;	
	
	if(commander.flightMode)/*无头模式*/
	{
		yawMode = CAREFREE;		
		rotateYawCarefree(setpoint, state);
	}		
	else	/*X飞行模式*/
	{
		yawMode = XMODE;
	}		
}

/* 读取并更新微调值 */
void getAndUpdateTrim(float* pitch, float* roll)
{
	*pitch = nowCache->tarVal[nowCache->activeSide].trimPitch;
	*roll = nowCache->tarVal[nowCache->activeSide].trimRoll;
}

void setCommanderCtrlMode(uint8_t set)
{
	commander.ctrlMode = (set & 0x03);
}
uint8_t getCommanderCtrlMode(void)
{
	return (commander.ctrlMode & 0x03);
}

uint8_t getCommanderFlightMode(void)
{
	return (yawMode & 0x01);
}

void setCommanderKeyFlight(bool set)
{
	commander.keyFlight = set;
	if(set == true)	/*一键起飞，清零最大最小值*/
	{
		minAccZ = 0.f;
		maxAccZ = 0.f;
	}
}
bool getCommanderKeyFlight(void)
{
	return commander.keyFlight;
}

void setCommanderKeyland(bool set)
{
	commander.keyLand = set;
}
bool getCommanderKeyland(void)
{
	return commander.keyLand;
}

void setCommanderFlightmode(bool set)
{
	commander.flightMode = set;
}

void setCommanderEmerStop(bool set)
{
	commander.emerStop = set;
}
bool getCommanderEmerStop(void)
{
	return commander.emerStop;
}

void print_commander(void)
{
	printf("ctrlMode:%d,keyFlight:%d,keyLand:%d,emerStop:%d,flightMode:%d\r\n",
	commander.ctrlMode,commander.keyFlight,commander.keyLand,commander.emerStop,commander.flightMode);
}





