

#ifndef __SENSORS_MPU6050_SPL06_H__
#define __SENSORS_MPU6050_SPL06_H__

#include "stabilizer_types.h"

void sensorsMpu6050Spl06Init(void);

bool sensorsReadGyro(Axis3f *gyro);
bool sensorsReadAcc(Axis3f *acc);
bool sensorsReadMag(Axis3f *mag);
bool sensorsReadBaro(baro_t *baro);
void sensorsAcquire(sensorData_t *sensors, const uint32_t tick);
bool sensorsAreCalibrated(void);
void getSensorRawData(Axis3i16* acc, Axis3i16* gyro, Axis3i16 *mag );
void getPressureRawData(float* temp, float* press, float*asl );
void setPrintf(uint8_t set);
void debugpeintf(const char *str);
void readBiasVlue(Axis3f *variance);

void sensorsI2CdevDeInit(void);
void sensorsMpu6050Spl06DeInit(void);

#endif /* _DRONE_SENSORS_H_ */
