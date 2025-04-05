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
 * stabilizer.h: Stabilizer orchestrator
 */
#ifndef __STABILIZER_TYPES_H__
#define __STABILIZER_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include "imu_types.h"



/** Attitude in euler angle form */
typedef struct attitude_s {
  uint32_t timestamp;  // 时间戳

  float roll;
  float pitch;
  float yaw;
} attitude_t;

/* vector */
#define vec3d_size 3
typedef float vec3d[vec3d_size];
typedef float mat3d[vec3d_size][vec3d_size];

/* x,y,z vector */
struct vec3_s {
  uint32_t timestamp; // Timestamp when the data was computed

  float x;
  float y;
  float z;
};

typedef struct vec3_s vector_t;
typedef struct vec3_s point_t;
typedef struct vec3_s velocity_t;
typedef struct vec3_s acc_t;

/* Orientation as a quaternion */
typedef struct quaternion_s {
  uint32_t timestamp;

  union {
    struct {
      float q0;
      float q1;
      float q2;
      float q3;
    };
    struct {
      float x;
      float y;
      float z;
      float w;
    };
  };
} quaternion_t;

typedef struct toaMeasurement_s 
{
	int8_t senderId;
	float x, y, z;
	int64_t rx, tx;
} toaMeasurement_t;

typedef struct tdoaMeasurement_s {
  point_t anchorPosition[2];
  float distanceDiff;
  float stdDev;
} tdoaMeasurement_t;

typedef struct baro_s {
  float pressure;           // mbar
  float temperature;        // degree Celcius
  float asl;                // m (ASL = altitude above sea level)
} baro_t;

typedef struct positionMeasurement_s {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float pos[3];
  };
  float stdDev;
} positionMeasurement_t;

typedef struct poseMeasurement_s {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float pos[3];
  };
  quaternion_t quat;
  float stdDevPos;
  float stdDevQuat;
} poseMeasurement_t;

typedef struct distanceMeasurement_s {
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float pos[3];
  };
  float distance;
  float stdDev;
} distanceMeasurement_t;

typedef struct zRange_s 
{
	uint32_t timestamp;	//时间戳
	float distance;		//测量距离
	float quality;		//可信度
} zRange_t;

typedef struct zDistance_s {
  uint32_t timestamp;
  float distance;           // m
} zDistance_t;

typedef struct sensorData_s {
	Axis3f acc;               // Gs
	Axis3f gyro;              // deg/s
	Axis3f mag;               // gauss
	baro_t baro;
	point_t position;
	zRange_t zrange;
#ifdef LOG_SEC_IMU
  Axis3f accSec;            // Gs
  Axis3f gyroSec;           // deg/s
#endif
  uint64_t interruptTimestamp;
} sensorData_t;

typedef struct state_s {
	attitude_t attitude;      // deg (legacy CF2 body coordinate system, where pitch is inverted)
	quaternion_t attitudeQuaternion;
	point_t position;         // m
	velocity_t velocity;      // m/s
	acc_t acc;                // Gs (but acc.z without considering gravity)
	bool isRCLocked;
} state_t;

enum dir_e
{
	CENTER=0,
	FORWARD,
	BACK,
	LEFT,
	RIGHT,
};

typedef struct control_s {
  int16_t roll;
  int16_t pitch;
  int16_t yaw;
  float thrust;
  enum dir_e flipDir;		/*翻滚方向*/
} control_t;

typedef enum mode_e {
  modeDisable = 0,  /*关闭模式*/
  modeAbs,         /*绝对值模式*/
  modeVelocity     /*速率模式*/
} stab_mode_t;

// typedef struct
// {
	// stab_mode_t x;
	// stab_mode_t y;
	// stab_mode_t z;
	// stab_mode_t roll;
	// stab_mode_t pitch;
	// stab_mode_t yaw;
// }mode_t;
  
typedef struct setpoint_s {
  uint32_t timestamp;

  attitude_t attitude;      // deg
  attitude_t attitudeRate;  // deg/s
  quaternion_t attitudeQuaternion;
  float thrust;
  point_t position;         // m
  velocity_t velocity;      // m/s
  acc_t acceleration;       // m/s^2
  bool velocity_body;       // true if velocity is given in body frame; false if velocity is given in world frame

  struct {
    stab_mode_t x;
    stab_mode_t y;
    stab_mode_t z;
    stab_mode_t roll;
    stab_mode_t pitch;
    stab_mode_t yaw;
    stab_mode_t quat;
  } mode;
} setpoint_t;

/** Estimate of position */
typedef struct estimate_s {
  uint32_t timestamp; // Timestamp when the data was computed

  point_t position;
} estimate_t;

/** Setpoint for althold */
typedef struct setpointZ_s {
  float z;
  bool isUpdate; // True = small update of setpoint, false = completely new
} setpointZ_t;

/** Flow measurement**/
typedef struct flowMeasurement_s {
  uint32_t timestamp;
  union {
    struct {
      float dpixelx;  // Accumulated pixel count x
      float dpixely;  // Accumulated pixel count y
    };
    float dpixel[2];  // Accumulated pixel count
  };
  float stdDevX;      // Measurement standard deviation
  float stdDevY;      // Measurement standard deviation
  float dt;           // Time during which pixels were accumulated
} flowMeasurement_t;


/** TOF measurement**/
typedef struct tofMeasurement_s {
  uint32_t timestamp;
  float distance;
  float stdDev;
} tofMeasurement_t;

/** Absolute height measurement */
typedef struct heightMeasurement_s {
  uint32_t timestamp;
  float height;
  float stdDev;
} heightMeasurement_t;

/** Yaw error measurement */
typedef struct {
  uint32_t timestamp;
  float yawError;
  float stdDev;
} yawErrorMeasurement_t;

/** Sweep angle measurement */
// typedef struct {
//   uint32_t timestamp;
//   const vec3d* sensorPos;    // Sensor position in the CF reference frame
//   const vec3d* rotorPos;     // Pos of rotor origin in global reference frame
//   const mat3d* rotorRot;     // Rotor rotation matrix
//   const mat3d* rotorRotInv;  // Inverted rotor rotation matrix
//   float t;                   // t is the tilt angle of the light plane on the rotor
//   float measuredSweepAngle;
//   float stdDev;
//   const lighthouseCalibrationSweep_t* calib;
//   lighthouseCalibrationMeasurementModel_t calibrationMeasurementModel;
// } sweepAngleMeasurement_t;

// Frequencies to bo used with the RATE_DO_EXECUTE_HZ macro. Do NOT use an arbitrary number.

#define RATE_5_HZ		5
#define RATE_10_HZ		10
#define RATE_25_HZ		25
#define RATE_50_HZ		50
#define RATE_100_HZ		100
#define RATE_200_HZ 	200
#define RATE_250_HZ 	250
#define RATE_500_HZ 	500
#define RATE_1000_HZ 	1000

#define MAIN_LOOP_RATE 	RATE_1000_HZ
#define MAIN_LOOP_DT	(uint32_t)(1000/MAIN_LOOP_RATE)	/*单位ms*/

#define RATE_DO_EXECUTE(RATE_HZ, TICK) ((TICK % (MAIN_LOOP_RATE / RATE_HZ)) == 0)


#endif
