/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * ESP-Drone Firmware
 *
 * Copyright 2019-2020  Espressif Systems (Shanghai)
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
 * pm.c - Power Management driver and functions.
 */

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "sensfusion6.h"
#include "pm_esplane.h"
#include "adc_esp32.h"
#include "led.h"
#include "esp_log.h"
#include "ledseq.h"

#define TAG "PM"

typedef struct _PmSyslinkInfo
{
  union
  {
    uint8_t flags;
    struct
    {
      uint8_t chg    : 1;
      uint8_t pgood  : 1;
      uint8_t unused : 6;
    };
  };
  float vBat;
  float chargeCurrent;
#ifdef PM_SYSTLINK_INLCUDE_TEMP
  float temp;
#endif
}__attribute__((packed)) PmSyslinkInfo;

static float     batteryVoltage;
static uint16_t  batteryVoltageMV;
static float     batteryVoltageMin = 6.0;
static float     batteryVoltageMax = 0.0;

static float     extBatteryVoltage;
static uint16_t  extBatteryVoltageMV;
static uint16_t extBatVoltDeckPin;
static bool      isExtBatVoltDeckPinSet = false;
static float     extBatVoltMultiplier;
static float     extBatteryCurrent;
static uint16_t extBatCurrDeckPin;
static bool      isExtBatCurrDeckPinSet = false;
static float     extBatCurrAmpPerVolt;

#ifdef PM_SYSTLINK_INLCUDE_TEMP
// nRF51 internal temp
static float    temp;
#endif

static uint32_t batteryLowTimeStamp;
static uint32_t batteryCriticalLowTimeStamp;
static bool isInit;
static PMStates pmState;
static PmSyslinkInfo pmSyslinkInfo;

static uint8_t batteryLevel;
static bool isLowpower;
static void pmSetBatteryVoltage(float voltage);

const static float bat671723HS25C[10] =
{
  3.00, // 00%
  3.78, // 10%
  3.83, // 20%
  3.87, // 30%
  3.89, // 40%
  3.92, // 50%
  3.96, // 60%
  4.00, // 70%
  4.04, // 80%
  4.10  // 90%
};

bool pmTest(void)
{
  return isInit;
}

/**
 * Sets the battery voltage and its min and max values
 */

static void pmSetBatteryVoltage(float voltage)
{
  batteryVoltage = voltage;
  batteryVoltageMV = (uint16_t)(voltage * 1000);
  if (batteryVoltageMax < voltage)
  {
    batteryVoltageMax = voltage;
  }
  if (batteryVoltageMin > voltage)
  {
    batteryVoltageMin = voltage;
  }
}

/**
 * Shutdown system
 */
static void pmSystemShutdown(void)
{
#ifdef ACTIVATE_AUTO_SHUTDOWN
//TODO: Implement syslink call to shutdown
#endif
}

/**
 * Returns a number from 0 to 9 where 0 is completely discharged
 * and 9 is 90% charged.
 */
static int32_t pmBatteryChargeFromVoltage(float voltage)
{
  int charge = 0;

  if (voltage < bat671723HS25C[0])
  {
    return 0;
  }
  if (voltage > bat671723HS25C[9])
  {
    return 9;
  }
  while (voltage >  bat671723HS25C[charge])
  {
    charge++;
  }

  return charge;
}

float pmGetBatteryVoltage(void)
{
  return batteryVoltage;
}

float pmGetBatteryVoltageMin(void)
{
  return batteryVoltageMin;
}

float pmGetBatteryVoltageMax(void)
{
  return batteryVoltageMax;
}

// void pmSyslinkUpdate(SyslinkPacket *slp)
// {
  // if (slp->type == SYSLINK_PM_BATTERY_STATE) {
    // memcpy(&pmSyslinkInfo, &slp->data[0], sizeof(pmSyslinkInfo));
    // pmSetBatteryVoltage(pmSyslinkInfo.vBat);
// #ifdef PM_SYSTLINK_INLCUDE_TEMP
    // temp = pmSyslinkInfo.temp;
// #endif
  // }
// }

void pmSetChargeState(PMChargeStates chgState)
{
  // TODO: Send syslink packafe with charge state
}

PMStates pmUpdateState()
{
  PMStates state;
  bool isCharging = pmSyslinkInfo.chg;
  bool isPgood = pmSyslinkInfo.pgood;
  uint32_t batteryLowTime;

  batteryLowTime = xTaskGetTickCount() - batteryLowTimeStamp;

  if (isPgood && !isCharging)
  {
    state = charged;
  }
  else if (isPgood && isCharging)
  {
    state = charging;
  }
  else if (!isPgood && !isCharging && (batteryLowTime > PM_BAT_LOW_TIMEOUT))
  {
    state = lowPower;
  }
  else
  {
    state = battery;
  }

  return state;
}

void pmEnableExtBatteryCurrMeasuring(uint8_t pin, float ampPerVolt)
{
  extBatCurrDeckPin = pin;
  isExtBatCurrDeckPinSet = true;
  extBatCurrAmpPerVolt = ampPerVolt;
}

float pmMeasureExtBatteryCurrent(void)
{
  float current;

  if (isExtBatCurrDeckPinSet)
  {
    current = analogReadVoltage(extBatCurrDeckPin) * extBatCurrAmpPerVolt;
  }
  else
  {
    current = 0.0;
  }

  return current;
}

void pmEnableExtBatteryVoltMeasuring(uint8_t pin, float multiplier)
{
  extBatVoltDeckPin = pin;
  isExtBatVoltDeckPinSet = true;
  extBatVoltMultiplier = multiplier;
}

float pmMeasureExtBatteryVoltage(void)
{
  float voltage;

  if (isExtBatVoltDeckPinSet)
  {
    voltage = analogReadVoltage(extBatVoltDeckPin) * extBatVoltMultiplier;
  }
  else
  {
    voltage = 0.0;
  }

  return voltage;
}

bool pmIsBatteryLow(void) {
  return (pmState == lowPower);
}

bool pmIsChargerConnected(void) {
  return (pmState == charging) || (pmState == charged);
}

bool pmIsCharging(void) {
  return (pmState == charging);
}
// return true if battery discharging
bool pmIsDischarging(void)
{
  PMStates pmState;
  pmState = pmUpdateState();
  return (pmState == lowPower) || (pmState == battery);
}

void pmTask(void *param)
{
	PMStates pmStateOld = battery;
	uint32_t tickCount = 0;
	uint32_t batteryCriticalLowTime = 0;
	#ifdef configUSE_APPLICATION_TASK_TAG
	#if configUSE_APPLICATION_TASK_TAG == 1
	vTaskSetApplicationTaskTag(0, (void *)TASK_PM_ID_NBR);
	#endif
	#endif

	tickCount = xTaskGetTickCount();
	batteryLowTimeStamp = tickCount;
	batteryCriticalLowTimeStamp = tickCount;
	pmSetChargeState(charge300mA);
	//systemWaitStart();

	while (1) {
		vTaskDelay(M2T(100));
		extBatteryVoltage = pmMeasureExtBatteryVoltage();
		extBatteryVoltageMV = (uint16_t)(extBatteryVoltage * 1000);
		extBatteryCurrent = pmMeasureExtBatteryCurrent();
		pmSetBatteryVoltage(extBatteryVoltage);
		batteryLevel = pmBatteryChargeFromVoltage(pmGetBatteryVoltage()) * 10;

		tickCount = xTaskGetTickCount();

		if (pmGetBatteryVoltage() > PM_BAT_LOW_VOLTAGE)
		{
			batteryLowTimeStamp = tickCount;
		}
		if (pmGetBatteryVoltage() > PM_BAT_CRITICAL_LOW_VOLTAGE)
		{
			batteryCriticalLowTimeStamp = tickCount;
		}

		pmState = pmUpdateState();
		// ESP_LOGI(TAG,"batteryLevel=%u extBatteryVoltageMV=%u ,pmState:%d\n", batteryLevel, extBatteryVoltageMV,pmState);
		if (pmState != pmStateOld)
		{
			// Actions on state change
			switch (pmState)
			{
				case charged:
					//ledseqStop(CHG_LED,seq_charging);
					//ledseqRun(CHG_LED, seq_charged);
				break;
				case charging:
					isLowpower = false;

					if(getIsCalibrated())
					{
						ledseqRun(SYS_LED, seq_calibrated);
					}else{
						ledseqRun(SYS_LED, seq_alive);				
					}

				break;

				case lowPower:
					
					batteryCriticalLowTime = tickCount - batteryCriticalLowTimeStamp;
					if (batteryCriticalLowTime > PM_BAT_CRITICAL_LOW_TIMEOUT)
					{
						pmSystemShutdown();
					}
	
					ledseqStop(SYS_LED, seq_alive);
					ledseqStop(SYS_LED, seq_calibrated);
					ledseqRun(LOWBAT_LED, seq_lowbat);
				break;
				case battery:
					//ledseqStop(CHG_LED, seq_charging);
					//ledseqRun(CHG_LED, seq_charged);
				break;
				default:
				//systemSetCanFly(true);
				break;
			}
			pmStateOld = pmState;
		}
	}
}
bool getIsLowpower(void)
{
	return isLowpower;
}

void pmInit(void)
{
	if(isInit) {
		return;
	}
	adcInit();

    pmEnableExtBatteryVoltMeasuring(PM_ADC1_PIN, 5); // ADC1 PIN is fixed to ADC channel

    pmSyslinkInfo.pgood = false;
    pmSyslinkInfo.chg = false;
    pmSyslinkInfo.vBat = 3.7f;
    pmSetBatteryVoltage(pmSyslinkInfo.vBat);

	// xTaskCreate(pmTask, "pmTask", PM_TASK_STACKSIZE, NULL, PM_TASK_PRI, NULL);

    isInit = true;
}

void pmDeInit(void)
{
	
	isInit = false;
	
}

