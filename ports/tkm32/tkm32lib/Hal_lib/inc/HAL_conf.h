/**
******************************************************************************
* @file  HAL_conf.h
* @author  IC Applications Department
* @version  V0.8
* @date  2019_08_02
* @brief  This file contains Header file for generic microcontroller.
******************************************************************************
* @copy
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, HOLOCENE SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2016 HOLOCENE</center></h2>
*/ 
#ifndef MICROPY_INCLUDED_HAL_CONF_H
#define MICROPY_INCLUDED_HAL_CONF_H

#include "HAL_device.h"
#include "HAL_adc.h"
#include "HAL_dma.h"
#include "HAL_exti.h"
#include "HAL_flash.h"
#include "HAL_gpio.h"
#include "HAL_i2c.h"
#include "HAL_iwdg.h"
#include "HAL_pwr.h"
#include "HAL_rcc.h"
#include "HAL_spi.h"
#include "HAL_tim.h"
#include "HAL_uart.h"
#include "HAL_wwdg.h"
#include "HAL_misc.h"
#include "HAL_syscfg.h"
#include "HAL_can.h"



// Oscillator values in Hz
#define HSI_VALUE (16000000)
#define LSI_VALUE (40000)

// SysTick has the highest priority
#define TICK_INT_PRIORITY (0x00)

// Miscellaneous HAL settings
#define DATA_CACHE_ENABLE           1
#define INSTRUCTION_CACHE_ENABLE    1
#define PREFETCH_ENABLE             1
#define USE_RTOS                    0

// HAL parameter assertions are disabled
#define assert_param(expr) ((void)0)

#endif

/*-------------------------(C) COPYRIGHT 2016 HOLOCENE ----------------------*/
