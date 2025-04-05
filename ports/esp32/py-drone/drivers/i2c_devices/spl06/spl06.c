/**
 *
 * ESP-Drone Firmware
 *
 * Copyright 2019-2020  Espressif Systems (Shanghai)
 * Copyright (C) 2011 Fabio Varesano <fvaresano@yahoo.it>
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
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spl06.h"
#include "i2cdev.h"
#include "math.h"

#include "py/obj.h"
#include "esp_log.h"

#define P_MEASURE_RATE 			SPL06_MWASURE_16 	//每秒测量次数
#define P_OVERSAMP_RATE 		SPL06_OVERSAMP_64	//过采样率
#define SPL06_PRESSURE_CFG		(P_MEASURE_RATE<<4 | P_OVERSAMP_RATE)

#define T_MEASURE_RATE 			SPL06_MWASURE_16 	//每秒测量次数
#define T_OVERSAMP_RATE 		SPL06_OVERSAMP_8	//过采样率
#define SPL06_TEMPERATURE_CFG	(TEMPERATURE_EXTERNAL_SENSOR<<7 | T_MEASURE_RATE<<4 | T_OVERSAMP_RATE)

#define SPL06_MODE				(SPL06_CONTINUOUS_MODE)
#define  SPL06_TIME_MS	10
const uint32_t scaleFactor[8] = {524288, 1572864, 3670016, 7864320, 253952, 516096, 1040384, 2088960};
static const char* TAG = "SPL06";

typedef enum 
{
	PRESURE_SENSOR, 
	TEMPERATURE_SENSOR
}spl06Sensor_e;

typedef struct 
{
    int16_t c0;
    int16_t c1;
    int32_t c00;
    int32_t c10;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
    int16_t c30;
} spl06CalibCoefficient_t;

spl06CalibCoefficient_t  spl06Calib;

static uint8_t devAddr;
static I2C_Dev *I2Cx;
static bool isInit = false;
static uint32_t lastConv = 0;

int32_t kp = 0;
int32_t kt = 0;
int32_t SPL06RawPressure = 0;
int32_t SPL06RawTemperature = 0;

static void SPL06GetPressure(void);

static bool spl06i2cdevRead(I2C_Dev *dev, uint8_t devAddress, uint8_t memAddress, uint16_t len, uint8_t *data)
{
	return i2cdevReadReg8(dev, devAddress,memAddress,(uint16_t) len, data);
}

void spl0601_get_calib_param(void)
{
	uint8_t buffer[SPL06_CALIB_COEFFICIENT_LENGTH] = {0};
	
	spl06i2cdevRead(I2Cx, devAddr, SPL06_COEFFICIENT_CALIB_REG, SPL06_CALIB_COEFFICIENT_LENGTH, buffer);
	
	spl06Calib.c0 = (int16_t)buffer[0]<<4 | buffer[1]>>4;
	spl06Calib.c0 = (spl06Calib.c0 & 0x0800) ? (spl06Calib.c0 | 0xF000) : spl06Calib.c0;
	
	spl06Calib.c1 = (int16_t)(buffer[1] & 0x0F)<<8 | buffer[2];
	spl06Calib.c1 = (spl06Calib.c1 & 0x0800) ? (spl06Calib.c1 | 0xF000) : spl06Calib.c1;
	
	spl06Calib.c00 = (int32_t)buffer[3]<<12 | (int32_t)buffer[4]<<4 | (int32_t)buffer[5]>>4;
	spl06Calib.c00 = (spl06Calib.c00 & 0x080000) ? (spl06Calib.c00 | 0xFFF00000) : spl06Calib.c00;
	
	spl06Calib.c10 = (int32_t)(buffer[5] & 0x0F)<<16 | (int32_t)buffer[6]<<8 | (int32_t)buffer[7];
	spl06Calib.c10 = (spl06Calib.c10 & 0x080000) ? (spl06Calib.c10 | 0xFFF00000) : spl06Calib.c10;
	
	spl06Calib.c01 = (int16_t)buffer[8]<<8 | buffer[9];
	spl06Calib.c11 = (int16_t)buffer[10]<<8 | buffer[11];
	spl06Calib.c20 = (int16_t)buffer[12]<<8 | buffer[13];
	spl06Calib.c21 = (int16_t)buffer[14]<<8 | buffer[15];
	spl06Calib.c30 = (int16_t)buffer[16]<<8 | buffer[17];
}

void spl0601_rateset(spl06Sensor_e sensor, uint8_t measureRate, uint8_t oversamplRate)
{
	uint8_t reg;
	if (sensor == PRESURE_SENSOR)
	{
		kp = scaleFactor[oversamplRate];
		i2cdevWriteByte(I2Cx, devAddr, SPL06_PRESSURE_CFG_REG, measureRate<<4 | oversamplRate);
		if (oversamplRate > SPL06_OVERSAMP_8)
		{
			i2cdevReadByte(I2Cx, devAddr, SPL06_INT_FIFO_CFG_REG, &reg);
			i2cdevWriteByte(I2Cx, devAddr, SPL06_INT_FIFO_CFG_REG, reg | 0x04);
		}
	}
	else if (sensor == TEMPERATURE_SENSOR)
	{
		kt = scaleFactor[oversamplRate];
		i2cdevWriteByte(I2Cx, devAddr, SPL06_TEMPERATURE_CFG_REG, measureRate<<4 | oversamplRate | 0x80);//Using mems temperature
		if (oversamplRate > SPL06_OVERSAMP_8)
		{
			i2cdevReadByte(I2Cx, devAddr, SPL06_INT_FIFO_CFG_REG, &reg);
			i2cdevWriteByte(I2Cx, devAddr, SPL06_INT_FIFO_CFG_REG, reg | 0x08);
		}
	}
}

bool SPL06Init(I2C_Dev *i2cPort)
{
	uint8_t SPL06ID = 0;
    if (isInit){
		return true;
	}
        
	I2Cx = i2cPort;
	devAddr = SPL06_I2C_ADDR;

	vTaskDelay(50 / portTICK_RATE_MS);
	
	i2cdevReadByte(I2Cx, devAddr, SPL06_CHIP_ID, &SPL06ID);	/* 读取SPL06 ID*/
	
	if(SPL06ID == SPL06_DEFAULT_CHIP_ID)
		ESP_LOGI(TAG,"SPL06 ID IS: 0x%X\n",SPL06ID);
    else
        return false;

    //读取校准数据
	spl0601_get_calib_param();
	spl0601_rateset(PRESURE_SENSOR, SPL06_MWASURE_16, SPL06_OVERSAMP_64);
	spl0601_rateset(TEMPERATURE_SENSOR, SPL06_MWASURE_16, SPL06_OVERSAMP_64);
	
	i2cdevWriteByte(I2Cx, devAddr, SPL06_MODE_CFG_REG, SPL06_MODE);

    isInit = true;
    return true;
}
void SPL06DeInit(void)
{
	isInit = false;
}
void SPL06GetPressure(void)
{
    uint8_t data[SPL06_DATA_FRAME_SIZE];

    spl06i2cdevRead(I2Cx, devAddr, SPL06_PRESSURE_MSB_REG, SPL06_DATA_FRAME_SIZE, data);
	SPL06RawPressure = (int32_t)data[0]<<16 | (int32_t)data[1]<<8 | (int32_t)data[2];
    SPL06RawPressure = (SPL06RawPressure & 0x800000) ? (0xFF000000 | SPL06RawPressure) : SPL06RawPressure;
	
    SPL06RawTemperature = (int32_t)data[3]<<16 | (int32_t)data[4]<<8 | (int32_t)data[5];
	SPL06RawTemperature = (SPL06RawTemperature & 0x800000) ? (0xFF000000 | SPL06RawTemperature) : SPL06RawTemperature;
}

float spl0601_get_temperature(int32_t rawTemperature)
{
    float fTCompensate;
    float fTsc;

    fTsc = rawTemperature / (float)kt;
    fTCompensate =  spl06Calib.c0 * 0.5 + spl06Calib.c1 * fTsc;
    return fTCompensate;
}


float spl0601_get_pressure(int32_t rawPressure, int32_t rawTemperature)
{
    float fTsc, fPsc;
    float qua2, qua3;
    float fPCompensate;

    fTsc = rawTemperature / (float)kt;
    fPsc = rawPressure / (float)kp;
    qua2 = spl06Calib.c10 + fPsc * (spl06Calib.c20 + fPsc* spl06Calib.c30);
    qua3 = fTsc * fPsc * (spl06Calib.c11 + fPsc * spl06Calib.c21);
	//qua3 = 0.9f *fTsc * fPsc * (spl06Calib.c11 + fPsc * spl06Calib.c21);

    fPCompensate = spl06Calib.c00 + fPsc * qua2 + fTsc * spl06Calib.c01 + qua3;
	//fPCompensate = spl06Calib.c00 + fPsc * qua2 + 0.9f *fTsc  * spl06Calib.c01 + qua3;
    return fPCompensate;
}

bool SPL06GetData(float* pressure, float* temperature, float* asl)
{
    static float t;
    static float p;
	static float savedPress, savedTemp;
	
    // Dont reader faster than we can
    uint32_t now = xTaskGetTickCount();
    if ((now - lastConv) < SPL06_TIME_MS) {
        *pressure = savedPress;
        *temperature = savedTemp;
        return 0;
    }
    lastConv = now;

	SPL06GetPressure();

	t = spl0601_get_temperature(SPL06RawTemperature);		
	p = spl0601_get_pressure(SPL06RawPressure, SPL06RawTemperature);		

//	pressureFilter(&p,pressure);
	*temperature = (float)t;/*单位度*/
	*pressure = (float)p ;	/*单位hPa*/	
	
	*asl=SPL06PressureToAltitude(*pressure);	/*转换成海拔*/	
	return 1;
}

/**
 * Converts pressure to altitude above sea level (ASL) in meters
 */
 

float SPL06PressureToAltitude(float pressure/*, float* groundPressure, float* groundTemp*/)
{	
    if(pressure)
    {
		return 44330.f * (powf((1015.7f / pressure), 0.190295f) - 1.0f);
    }
    else
    {
        return 0;
    }
}