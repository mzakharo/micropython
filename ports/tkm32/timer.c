/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "timer.h"
//#include "servo.h"
#include "pin.h"
#include "irq.h"

#include "softtimer.h"
/// \moduleref pyb
/// \class Timer - periodically call a function
///
/// Timers can be used for a great variety of tasks.  At the moment, only
/// the simplest case is implemented: that of calling a function periodically.
///
/// Each timer consists of a counter that counts up at a certain rate.  The rate
/// at which it counts is the peripheral clock frequency (in Hz) divided by the
/// timer prescaler.  When the counter reaches the timer period it triggers an
/// event, and the counter resets back to zero.  By using the callback method,
/// the timer event can call a Python function.
///
/// Example usage to toggle an LED at a fixed frequency:
///
///     tim = pyb.Timer(4)              # create a timer object using timer 4
///     tim.init(freq=2)                # trigger at 2Hz
///     tim.callback(lambda t:pyb.LED(1).toggle())
///
/// Further examples:
///
///     tim = pyb.Timer(4, freq=100)    # freq in Hz
///     tim = pyb.Timer(4, prescaler=0, period=99)
///     tim.counter()                   # get counter (can also set)
///     tim.prescaler(2)                # set prescaler (can also get)
///     tim.period(199)                 # set period (can also get)
///     tim.callback(lambda t: ...)     # set callback for update interrupt (t=tim instance)
///     tim.callback(None)              # clear callback
///
typedef struct
{
  volatile uint32_t CR1;         /*!< TIM control register 1,              Address offset: 0x00 */
  volatile uint32_t CR2;         /*!< TIM control register 2,              Address offset: 0x04 */
  volatile uint32_t SMCR;        /*!< TIM slave mode control register,     Address offset: 0x08 */
  volatile uint32_t DIER;        /*!< TIM DMA/interrupt enable register,   Address offset: 0x0C */
  volatile uint32_t SR;          /*!< TIM status register,                 Address offset: 0x10 */
  volatile uint32_t EGR;         /*!< TIM event generation register,       Address offset: 0x14 */
  volatile uint32_t CCMR[2];       /*!< TIM capture/compare mode register 1, Address offset: 0x18 */
  volatile uint32_t CCER;        /*!< TIM capture/compare enable register, Address offset: 0x20 */
  volatile uint32_t CNT;         /*!< TIM counter register,                Address offset: 0x24 */
  volatile uint32_t PSC;         /*!< TIM prescaler,                       Address offset: 0x28 */
  volatile uint32_t ARR;         /*!< TIM auto-reload register,            Address offset: 0x2C */
  volatile uint32_t RCR;         /*!< TIM repetition counter register,     Address offset: 0x30 */
  volatile uint32_t CCRX[4];        /*!< TIM capture/compare register 1,      Address offset: 0x34 */
  volatile uint32_t BDTR;        /*!< TIM break and dead-time register,    Address offset: 0x44 */
  volatile uint32_t DCR;         /*!< TIM DMA control register,            Address offset: 0x48 */
  volatile uint32_t DMAR;        /*!< TIM DMA address for full transfer,   Address offset: 0x4C */
  volatile uint32_t OR;          /*!< TIM option register,                 Address offset: 0x50 */
} TIM_RegTypeDef;
typedef enum {
    CHANNEL_MODE_PWM_NORMAL,
    CHANNEL_MODE_PWM_INVERTED,
    CHANNEL_MODE_OC_TIMING,
    CHANNEL_MODE_OC_ACTIVE,
    CHANNEL_MODE_OC_INACTIVE,
    CHANNEL_MODE_OC_TOGGLE,
    CHANNEL_MODE_OC_FORCED_ACTIVE,
    CHANNEL_MODE_OC_FORCED_INACTIVE,
    CHANNEL_MODE_IC,
    CHANNEL_MODE_ENC_A,
    CHANNEL_MODE_ENC_B,
    CHANNEL_MODE_ENC_AB,
} pyb_channel_mode;

STATIC const struct {
    qstr name;
    uint32_t oc_mode;
} channel_mode_info[] = {
    { MP_QSTR_PWM,                1 },
    { MP_QSTR_PWM_INVERTED,       2 },
    { MP_QSTR_OC_TIMING,          3 },
    { MP_QSTR_OC_ACTIVE,          4 },
    { MP_QSTR_OC_INACTIVE,        5 },
    { MP_QSTR_OC_TOGGLE,          6 },
    { MP_QSTR_OC_FORCED_ACTIVE,   7 },
    { MP_QSTR_OC_FORCED_INACTIVE, 8 },
    { MP_QSTR_IC,                 0 },
    { MP_QSTR_ENC_A,              8 },
    { MP_QSTR_ENC_B,              8 },
    { MP_QSTR_ENC_AB,             8 },
};

enum {
    BRK_OFF,
    BRK_LOW,
    BRK_HIGH,
};

typedef struct _pyb_timer_channel_obj_t {
    mp_obj_base_t base;
    struct _pyb_timer_obj_t *timer;
    uint8_t channel;
    uint8_t mode;
    mp_obj_t callback;
    struct _pyb_timer_channel_obj_t *next;
} pyb_timer_channel_obj_t;
//TIM_TypeDef
typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    uint8_t tim_id;
    uint8_t is_32bit;
		uint32_t RunMode;
    mp_obj_t callback;
    TIM_TypeDef *tim;
    IRQn_Type irqn;
    pyb_timer_channel_obj_t *channel;
} pyb_timer_obj_t;

// The following yields TIM_IT_Update when channel is zero and
// TIM_IT_CC1..TIM_IT_CC4 when channel is 1..4
#define TIMER_IRQ_MASK(channel) (1 << (channel))
#define TIMER_CNT_MASK(self)    ((self)->is_32bit ? 0xffffffff : 0xffff)
#define TIMER_CHANNEL(self)     ((((self)->channel) - 1) << 2)

/** @defgroup TIM_Lock_level  TIM Lock level
  * @{
  */
#define TIM_LOCKLEVEL_OFF          0x00000000U
#define TIM_LOCKLEVEL_1            (TIM_BDTR_LOCK_0)
#define TIM_LOCKLEVEL_2            (TIM_BDTR_LOCK_1)
#define TIM_LOCKLEVEL_3            (TIM_BDTR_LOCK)

#define TIM_OSSR_ENABLE         (TIM_BDTR_OSSR)
#define TIM_OSSR_DISABLE        0x00000000U

#define TIM_OSSI_ENABLE             (TIM_BDTR_OSSI)
#define TIM_OSSI_DISABLE            0x00000000U

#define TIM_BREAK_ENABLE          (TIM_BDTR_BKE)
#define TIM_BREAK_DISABLE         0x00000000U

#define TIM_BREAKPOLARITY_LOW        0x00000000U
#define TIM_BREAKPOLARITY_HIGH       (TIM_BDTR_BKP)

#define TIM_AUTOMATICOUTPUT_ENABLE           (TIM_BDTR_AOE)
#define TIM_AUTOMATICOUTPUT_DISABLE          0x00000000U

#define TIM_COUNTERMODE_UP                 0x00000000U
#define TIM_COUNTERMODE_DOWN               TIM_CR1_DIR

/**
  * @brief  Gets the TIM Autoreload Register value on runtime.
  * @param  __TIM__: TIM handle.
  * @retval 16-bit or 32-bit value of the timer auto-reload register(TIMx_ARR)
  */
#define TIM_GET_AUTORELOAD(__TIM__) ((__TIM__)->ARR)
//---------------------------------------------------------------------------
#define PYB_TIMER_OBJ_ALL_NUM MP_ARRAY_SIZE(MP_STATE_PORT(pyb_timer_obj_all))

STATIC mp_obj_t pyb_timer_deinit(mp_obj_t self_in);
STATIC mp_obj_t pyb_timer_callback(mp_obj_t self_in, mp_obj_t callback);
STATIC mp_obj_t pyb_timer_channel_callback(mp_obj_t self_in, mp_obj_t callback);

void timer_init0(void) {
    for (uint i = 0; i < PYB_TIMER_OBJ_ALL_NUM; i++) {
        MP_STATE_PORT(pyb_timer_obj_all)[i] = NULL;
    }
}

// unregister all interrupt sources
void timer_deinit(void) {
    for (uint i = 0; i < PYB_TIMER_OBJ_ALL_NUM; i++) {
        pyb_timer_obj_t *tim = MP_STATE_PORT(pyb_timer_obj_all)[i];
        if (tim != NULL) {
            pyb_timer_deinit(MP_OBJ_FROM_PTR(tim));
        }
    }
}

uint32_t timer_get_source_freq(uint32_t tim_id) {
	uint32_t source, clk_div;
	
	SYSCLK_INFO sys_clk;
	GetSysClockInfo(&sys_clk);

	if (tim_id <= 2) {
			// TIM{1,8,9,10,11} are on APB2
			source = sys_clk.PCLK2Clock;
			clk_div = RCC->CFGR & (0x07<<13);

	} else {
			// TIM{3,4,5,6,7,8,9,10} are on APB1
			source = sys_clk.PCLK1Clock;
			clk_div = RCC->CFGR & (0x07<<10);
	}
	if (clk_div != 0) {
			// APB prescaler for this timer is > 1
			source *= 2;
	}
	return source;
}

/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC const mp_obj_type_t pyb_timer_channel_type;

// This is the largest value that we can multiply by 100 and have the result
// fit in a uint32_t.
#define MAX_PERIOD_DIV_100  42949672

// computes prescaler and period so TIM triggers at freq-Hz
STATIC uint32_t compute_prescaler_period_from_freq(pyb_timer_obj_t *self, mp_obj_t freq_in, uint32_t *period_out) {
    uint32_t source_freq = timer_get_source_freq(self->tim_id);
    uint32_t prescaler = 1;
    uint32_t period;

    if (0) {
    #if MICROPY_PY_BUILTINS_FLOAT
    } else if (mp_obj_is_type(freq_in, &mp_type_float)) {
        float freq = mp_obj_get_float_to_f(freq_in);
        if (freq <= 0) {
            goto bad_freq;
        }
        while (freq < 1 && prescaler < 6553) {
            prescaler *= 10;
            freq *= 10.0f;
        }
        period = (uint32_t)((float)source_freq / freq);
    #endif
    } else {
        mp_int_t freq = mp_obj_get_int(freq_in);
        if (freq <= 0) {
            goto bad_freq;
        bad_freq:
            mp_raise_ValueError(MP_ERROR_TEXT("must have positive freq"));
        }
        period = source_freq / freq;
    }
    period = MAX(1, period);
    while (period > TIMER_CNT_MASK(self)) {
        // if we can divide exactly, do that first
        if (period % 5 == 0) {
            prescaler *= 5;
            period /= 5;
        } else if (period % 3 == 0) {
            prescaler *= 3;
            period /= 3;
        } else {
            // may not divide exactly, but loses minimal precision
            prescaler <<= 1;
            period >>= 1;
        }
    }
    *period_out = (period - 1) & TIMER_CNT_MASK(self);
    return (prescaler - 1) & 0xffff;
}

// computes prescaler and period so TIM triggers with a period of t_num/t_den seconds
STATIC uint32_t compute_prescaler_period_from_t(pyb_timer_obj_t *self, int32_t t_num, int32_t t_den, uint32_t *period_out) {

	uint32_t source_freq = timer_get_source_freq(self->tim_id);
	if (t_num <= 0 || t_den <= 0) {
			mp_raise_ValueError(MP_ERROR_TEXT("must have positive freq"));
	}
	uint64_t period = (uint64_t)source_freq * (uint64_t)t_num / (uint64_t)t_den;
	uint32_t prescaler = 1;
	while (period > TIMER_CNT_MASK(self)) {
			// if we can divide exactly, and without prescaler overflow, do that first
			if (prescaler <= 13107 && period % 5 == 0) {
					prescaler *= 5;
					period /= 5;
			} else if (prescaler <= 21845 && period % 3 == 0) {
					prescaler *= 3;
					period /= 3;
			} else {
					// may not divide exactly, but loses minimal precision
					uint32_t period_lsb = period & 1;
					prescaler <<= 1;
					period >>= 1;
					if (period < prescaler) {
							// round division up
							prescaler |= period_lsb;
					}
					if (prescaler > 0x10000) {
							mp_raise_ValueError(MP_ERROR_TEXT("period too large"));
					}
			}
	}
	*period_out = (period - 1) & TIMER_CNT_MASK(self);
	return (prescaler - 1) & 0xffff;
}

// Helper function for determining the period used for calculating percent
STATIC uint32_t compute_period(pyb_timer_obj_t *self) {

    // In center mode,  compare == period corresponds to 100%
    // In edge mode, compare == (period + 1) corresponds to 100%
    uint32_t period = (TIM_GET_AUTORELOAD(self->tim) & TIMER_CNT_MASK(self));
    if (period != 0xffffffff) {
        if ((self->tim->CR1 & (0x03U << 5))==0x0U) {  //CMS bit
            // Edge mode
            period++;
        }
    }
    return period;
}

// Helper function to compute PWM value from timer period and percent value.
// 'percent_in' can be an int or a float between 0 and 100 (out of range
// values are clamped).
STATIC uint32_t compute_pwm_value_from_percent(uint32_t period, mp_obj_t percent_in) {
    uint32_t cmp;
    if (0) {
    #if MICROPY_PY_BUILTINS_FLOAT
    } else if (mp_obj_is_type(percent_in, &mp_type_float)) {
        mp_float_t percent = mp_obj_get_float(percent_in);
        if (percent <= 0.0) {
            cmp = 0;
        } else if (percent >= 100.0) {
            cmp = period;
        } else {
            cmp = (uint32_t)(percent / MICROPY_FLOAT_CONST(100.0) * ((mp_float_t)period));
        }
    #endif
    } else {
        // For integer arithmetic, if period is large and 100*period will
        // overflow, then divide period before multiplying by cmp.  Otherwise
        // do it the other way round to retain precision.
        mp_int_t percent = mp_obj_get_int(percent_in);
        if (percent <= 0) {
            cmp = 0;
        } else if (percent >= 100) {
            cmp = period;
        } else if (period > MAX_PERIOD_DIV_100) {
            cmp = (uint32_t)percent * (period / 100);
        } else {
            cmp = ((uint32_t)percent * period) / 100;
        }
    }
    return cmp;
}

// Helper function to compute percentage from timer perion and PWM value.
STATIC mp_obj_t compute_percent_from_pwm_value(uint32_t period, uint32_t cmp) {
    #if MICROPY_PY_BUILTINS_FLOAT
    mp_float_t percent;
    if (cmp >= period) {
        percent = 100.0;
    } else {
        percent = (mp_float_t)cmp * 100.0 / ((mp_float_t)period);
    }
    return mp_obj_new_float(percent);
    #else
    mp_int_t percent;
    if (cmp >= period) {
        percent = 100;
    } else if (cmp > MAX_PERIOD_DIV_100) {
        percent = cmp / (period / 100);
    } else {
        percent = cmp * 100 / period;
    }
    return mp_obj_new_int(percent);
    #endif
}


// Computes the 8-bit value for the DTG field in the BDTR register.
//
// 1 tick = 1 count of the timer's clock (source_freq) divided by div.
// 0-128 ticks in inrements of 1
// 128-256 ticks in increments of 2
// 256-512 ticks in increments of 8
// 512-1008 ticks in increments of 16
STATIC uint32_t compute_dtg_from_ticks(mp_int_t ticks) {
    if (ticks <= 0) {
        return 0;
    }
    if (ticks < 128) {
        return ticks;
    }
    if (ticks < 256) {
        return 0x80 | ((ticks - 128) / 2);
    }
    if (ticks < 512) {
        return 0xC0 | ((ticks - 256) / 8);
    }
    if (ticks < 1008) {
        return 0xE0 | ((ticks - 512) / 16);
    }
    return 0xFF;
}

// Given the 8-bit value stored in the DTG field of the BDTR register, compute
// the number of ticks.
STATIC mp_int_t compute_ticks_from_dtg(uint32_t dtg) {
    if ((dtg & 0x80) == 0) {
        return dtg & 0x7F;
    }
    if ((dtg & 0xC0) == 0x80) {
        return 128 + ((dtg & 0x3F) * 2);
    }
    if ((dtg & 0xE0) == 0xC0) {
        return 256 + ((dtg & 0x1F) * 8);
    }
    return 512 + ((dtg & 0x1F) * 16);
}

STATIC void config_deadtime(pyb_timer_obj_t *self, mp_int_t ticks, mp_int_t brk) {

	uint32_t tmpbdtr = 0U;
	/* Set the BDTR bits */
	MODIFY_REG(tmpbdtr, TIM_BDTR_DTG, compute_dtg_from_ticks(ticks));
	MODIFY_REG(tmpbdtr, TIM_BDTR_LOCK, TIM_LOCKLEVEL_OFF);
	MODIFY_REG(tmpbdtr, TIM_BDTR_OSSI, TIM_OSSI_DISABLE);
	MODIFY_REG(tmpbdtr, TIM_BDTR_OSSR, TIM_OSSR_DISABLE);
	MODIFY_REG(tmpbdtr, TIM_BDTR_BKE, (brk == BRK_OFF ? TIM_BREAK_DISABLE : TIM_BREAK_ENABLE));
	MODIFY_REG(tmpbdtr, TIM_BDTR_BKP, (brk == BRK_LOW ? TIM_BREAKPOLARITY_LOW : TIM_BREAKPOLARITY_HIGH));
	MODIFY_REG(tmpbdtr, TIM_BDTR_AOE, TIM_AUTOMATICOUTPUT_DISABLE);
	MODIFY_REG(tmpbdtr, TIM_BDTR_MOE, TIM_AUTOMATICOUTPUT_DISABLE);

	/* Set TIMx_BDTR */
	self->tim->BDTR = tmpbdtr;
}

TIM_TypeDef *pyb_timer_get_handle(mp_obj_t timer) {
    if (mp_obj_get_type(timer) != &pyb_timer_type) {
        mp_raise_ValueError(MP_ERROR_TEXT("need a Timer object"));
    }
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(timer);
    return self->tim;
}

STATIC void pyb_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);


    qstr mode = self->RunMode == SOFT_TIMER_MODE_ONE_SHOT ? MP_QSTR_ONE_SHOT : MP_QSTR_PERIODIC;

    if (!(self->tim->CR1 & (0x01U << 0))) {
        mp_printf(print, "Timer(%u)", self->tim_id);
    } else {
        uint32_t prescaler = self->tim->PSC & 0xffff;
        uint32_t period = TIM_GET_AUTORELOAD(self->tim) & TIMER_CNT_MASK(self);
        // for efficiency, we compute and print freq as an int (not a float)
        uint32_t freq = timer_get_source_freq(self->tim_id) / ((prescaler + 1) * (period + 1));

				uint32_t ClockDivision = self->tim->CR1 & (0x03U << 8);
        mp_printf(print, "Timer(%u, freq=%u, prescaler=%u, period=%u, mode=%q, div=%u",
            self->tim_id,
            freq,
            prescaler,
            period,
            mode,
            ClockDivision == TIM_CR1_CKD_1 ? 4 :
            ClockDivision == TIM_CR1_CKD_0 ? 2 : 1);

        if (self->tim == TIM1 || self->tim == TIM2)
        {
            mp_printf(print, ", deadtime=%u",
                compute_ticks_from_dtg(self->tim->BDTR & TIM_BDTR_DTG));
            if ((self->tim->BDTR & TIM_BDTR_BKE) == TIM_BDTR_BKE) {
                mp_printf(print, ", brk=%s",
                    ((self->tim->BDTR & TIM_BDTR_BKP) == TIM_BDTR_BKP) ? "BRK_HIGH" : "BRK_LOW");
            } else {
                mp_printf(print, ", brk=BRK_OFF");
            }
        }

        mp_print_str(print, ")");
    }
}

STATIC mp_obj_t pyb_timer_init_helper(pyb_timer_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // enum { ARG_freq, ARG_prescaler, ARG_period, ARG_tick_hz, ARG_mode, ARG_div, ARG_callback, ARG_deadtime, ARG_brk };
    // static const mp_arg_t allowed_args[] = {
        // { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        // { MP_QSTR_prescaler,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        // { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        // { MP_QSTR_tick_hz,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        // { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = TIM_COUNTERMODE_UP} },
        // { MP_QSTR_div,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        // { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        // { MP_QSTR_deadtime,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        // { MP_QSTR_brk,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = BRK_OFF} },
    // };
		
    enum { ARG_mode, ARG_callback, ARG_period, ARG_tick_hz, ARG_freq, };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SOFT_TIMER_MODE_PERIODIC} },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        { MP_QSTR_tick_hz,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };
		
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set the TIM configuration values
		uint32_t Period = 0,prescaler=0;
    // enable TIM clock
		if(self->tim_id <= 2){
			RCC->APB2ENR |= (1 << (self->tim_id-1));
		}else{
			RCC->APB1ENR |= (1 << (self->tim_id-3));
		}

    if (args[ARG_freq].u_obj != mp_const_none) {
        // set prescaler and period from desired frequency
				prescaler = compute_prescaler_period_from_freq(self, args[ARG_freq].u_obj, &Period);
				self->tim->PSC = prescaler;
				self->tim->ARR = Period;

		#if (0)
    } else if (args[ARG_prescaler].u_int != 0xffffffff && args[ARG_period].u_int != 0xffffffff) {
        // set prescaler and period directly
        self->tim->PSC = args[ARG_prescaler].u_int;
        self->tim->ARR = args[ARG_period].u_int;
		#endif
    } else if (args[ARG_period].u_int != 0xffffffff) {
        // set prescaler and period from desired period and tick_hz scale
        //self->tim->PSC = 
				prescaler = compute_prescaler_period_from_t(self, args[ARG_period].u_int, args[ARG_tick_hz].u_int, &Period);
				self->tim->PSC = prescaler;
				self->tim->ARR = Period;

    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("must specify either freq, period, or prescaler and period"));
    }
		self->tim->CR1 &= ~(0x07U<<4);
		//self->tim->CR1 |= args[ARG_mode].u_int;
		self->tim->CR1 |= TIM_COUNTERMODE_UP;
		
    if (args[ARG_mode].u_int != SOFT_TIMER_MODE_ONE_SHOT && args[ARG_mode].u_int != SOFT_TIMER_MODE_PERIODIC) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid mode (%d)"), args[ARG_mode].u_int);
    }
		self->RunMode = args[ARG_mode].u_int;
		self->tim->CR1 &= ~(0x3U<<8);
    //self->tim->CR1 |= args[ARG_div].u_int == 2 ? 0x100U : args[ARG_div].u_int == 4 ? 0x200U :0x0U;
		self->tim->CR1 |= 0x0U;

		if(self->tim == TIM1 || self->tim == TIM2){
			self->tim->RCR = 0;
		}

    // set IRQ priority (if not a special timer)
    //if (self->tim_id != 5) 
		{
        NVIC_SetPriority(IRQn_NONNEG(self->irqn), IRQ_PRI_TIMX);
        if (self->tim_id == 1) {
            NVIC_SetPriority(TIM1_CC_IRQn, IRQ_PRI_TIMX);
        } else if (self->tim_id == 2) {
            NVIC_SetPriority(TIM2_CC_IRQn, IRQ_PRI_TIMX);
        }
    }

    // init TIM
		if(self->tim == TIM1 || self->tim == TIM2){
        //config_deadtime(self, args[ARG_deadtime].u_int, args[ARG_brk].u_int);
				config_deadtime(self, 0, BRK_OFF);
    }

    // Enable ARPE so that the auto-reload register is buffered.
    // This allows to smoothly change the frequency of the timer.
    self->tim->CR1 |= (0x1U<<7U);

    // Start the timer running
    if (args[ARG_callback].u_obj == mp_const_none) {
        self->tim->CR1 |= 0x1U;
    } else {
        pyb_timer_callback(MP_OBJ_FROM_PTR(self), args[ARG_callback].u_obj);
    }
    return mp_const_none;
}

// This table encodes the timer instance and irq number (for the update irq).
// It assumes that timer instance pointer has the lower 8 bits cleared.
#define TIM_ENTRY(id, irq) [id - 1] = (uint32_t)TIM##id | irq
STATIC const uint32_t tim_instance_table[MICROPY_HW_MAX_TIMER] = {
    TIM_ENTRY(1, TIM1_UP_IRQn),
    TIM_ENTRY(2, TIM2_UP_IRQn),
    TIM_ENTRY(3, TIM3_IRQn),
    TIM_ENTRY(4, TIM4_IRQn),
    TIM_ENTRY(5, TIM5_IRQn),
    TIM_ENTRY(6, TIM6_IRQn),
    TIM_ENTRY(7, TIM7_IRQn),
    TIM_ENTRY(8, TIM8_IRQn),
    TIM_ENTRY(9, TIM9_IRQn),
    TIM_ENTRY(10, TIM10_IRQn),
};
#undef TIM_ENTRY

/// \classmethod \constructor(id, ...)
/// Construct a new timer object of the given id.  If additional
/// arguments are given, then the timer is initialised by `init(...)`.
/// `id` can be 1 to 14, excluding 3.
STATIC mp_obj_t pyb_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the timer id
    mp_int_t tim_id = mp_obj_get_int(args[0]);

    // check if the timer exists
    if (tim_id <= 0 || tim_id > MICROPY_HW_MAX_TIMER || tim_instance_table[tim_id - 1] == 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Timer(%d) doesn't exist"), tim_id);
    }

    pyb_timer_obj_t *self;
    if (MP_STATE_PORT(pyb_timer_obj_all)[tim_id - 1] == NULL) {
        // create new Timer object
				self = m_new_obj(pyb_timer_obj_t);
        memset(self, 0, sizeof(*self));
        self->base.type = &pyb_timer_type;
        self->tim_id = tim_id;
        self->is_32bit = 1;
        self->callback = mp_const_none;
        uint32_t ti = tim_instance_table[tim_id - 1];
        self->tim = (TIM_TypeDef *)(ti & 0xffffff00);
        self->irqn = ti & 0xff;

				self->tim->CR1 = 0;
				if(tim_id <= 2){
					RCC->APB2RSTR |= (1 << (tim_id-1));
					RCC->APB2RSTR &= ~(1 << (tim_id-1));
				}else{
					RCC->APB1RSTR |= (1 << (tim_id-3));
					RCC->APB1RSTR &= ~(1 << (tim_id-3));
				}

        MP_STATE_PORT(pyb_timer_obj_all)[tim_id - 1] = self;
    } else {
        // reference existing Timer object
        self = MP_STATE_PORT(pyb_timer_obj_all)[tim_id - 1];
    }

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_timer_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t pyb_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_timer_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_timer_init_obj, 1, pyb_timer_init);

// timer.deinit()
STATIC mp_obj_t pyb_timer_deinit(mp_obj_t self_in) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Disable the base interrupt
    pyb_timer_callback(self_in, mp_const_none);

    pyb_timer_channel_obj_t *chan = self->channel;
    self->channel = NULL;

    // Disable the channel interrupts
    while (chan != NULL) {
        pyb_timer_channel_callback(MP_OBJ_FROM_PTR(chan), mp_const_none);
        pyb_timer_channel_obj_t *prev_chan = chan;
        chan = chan->next;
        prev_chan->next = NULL;
    }

   // self->tim.State = HAL_TIM_STATE_RESET;
    self->tim->CCER = 0x0000; // disable all capture/compare outputs
    self->tim->CR1 = 0x0000; // disable the timer and reset its state

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_deinit_obj, pyb_timer_deinit);

/// PWM Example:
///
///     timer = pyb.Timer(2, freq=1000)
///     ch2 = timer.channel(2, pyb.Timer.PWM, pin=pyb.Pin.board.X2, pulse_width=210000)
///     ch3 = timer.channel(3, pyb.Timer.PWM, pin=pyb.Pin.board.X3, pulse_width=420000)
STATIC mp_obj_t pyb_timer_channel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,                MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback,            MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_pin,                 MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_pulse_width,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_pulse_width_percent, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_compare,             MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_polarity,            MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
    };

    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
		
		self->tim->CR1 &= ~0x01; //stop cnt
		self->tim->CNT = 0;
		
    mp_int_t channel = mp_obj_get_int(pos_args[1]);

    if (channel < 1 || channel > 4) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid channel (%d)"), channel);
    }

    pyb_timer_channel_obj_t *chan = self->channel;
    pyb_timer_channel_obj_t *prev_chan = NULL;

    while (chan != NULL) {
        if (chan->channel == channel) {
            break;
        }
        prev_chan = chan;
        chan = chan->next;
    }

    // If only the channel number is given return the previously allocated
    // channel (or None if no previous channel).
    if (n_args == 2 && kw_args->used == 0) {
        if (chan) {
            return MP_OBJ_FROM_PTR(chan);
        }
        return mp_const_none;
    }

    // If there was already a channel, then remove it from the list. Note that
    // the order we do things here is important so as to appear atomic to
    // the IRQ handler.
    if (chan) {
        // Turn off any IRQ associated with the channel.
        pyb_timer_channel_callback(MP_OBJ_FROM_PTR(chan), mp_const_none);

        // Unlink the channel from the list.
        if (prev_chan) {
            prev_chan->next = chan->next;
        }
        self->channel = chan->next;
        chan->next = NULL;
    }

    // Allocate and initialize a new channel
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 2, pos_args + 2, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    chan = m_new_obj(pyb_timer_channel_obj_t);
    memset(chan, 0, sizeof(*chan));
    chan->base.type = &pyb_timer_channel_type;
    chan->timer = self;
    chan->channel = channel;
    chan->mode = args[0].u_int;
    chan->callback = args[1].u_obj;

    mp_obj_t pin_obj = args[2].u_obj;
    if (pin_obj != mp_const_none) {
        if (!mp_obj_is_type(pin_obj, &pin_type)) {
            mp_raise_ValueError(MP_ERROR_TEXT("pin argument needs to be be a Pin type"));
        }
        const pin_obj_t *pin = MP_OBJ_TO_PTR(pin_obj);
        const pin_af_obj_t *af = pin_find_af(pin, AF_FN_TIM, self->tim_id);

        if (af == NULL) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Pin(%q) doesn't have an af for Timer(%d)"), pin->name, self->tim_id);
        }
		mp_hal_pin_config(pin, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, af->idx);
    }

    chan->next = self->channel;
    self->channel = chan;
uint32_t tmp = 0U;
    switch (chan->mode) {

        case CHANNEL_MODE_PWM_NORMAL:
        case CHANNEL_MODE_PWM_INVERTED: {
						TIM_RegTypeDef *TIMX = (TIM_RegTypeDef *)self->tim;
	
						TIMX->CCMR[(channel-1)>>1] |= ((0x06U | chan->mode) << (channel%2?4:12));
						TIMX->CCMR[(channel-1)>>1] |= (0x01U  << (channel%2?3:11)); //EN preload
						
            if (args[4].u_obj != mp_const_none) {
                // pulse width percent given
                uint32_t period = compute_period(self);
								uint32_t Pulse = compute_pwm_value_from_percent(period, args[4].u_obj);
								TIMX->PSC = Pulse;
								TIMX->ARR = period;
            } else {

								TIMX->CCRX[channel-1] = args[3].u_int;
            }
            if (chan->callback == mp_const_none) {
								TIMX->CCER |= (0x01U << ( (channel-1) * 4) ); //enable output
							if(self->tim == TIM1 || TIM2){
								self->tim->BDTR |= (1<<15);
							}
							TIMX->EGR |= (0x01U << channel);
							
							self->tim->CR1 |= (1<<0);
            } else {
                pyb_timer_channel_callback(MP_OBJ_FROM_PTR(chan), chan->callback);
            }
            // Start the complimentary channel too (if its supported)
            if (IS_TIM_CCXN_INSTANCE(self->tim, TIMER_CHANNEL(chan))) {
							tmp = TIM_CCER_CC1NE << TIMER_CHANNEL(chan);
							self->tim->CCER &= ~tmp;
							self->tim->CCER |= (uint32_t)(TIM_CCxN_ENABLE << TIMER_CHANNEL(chan));
							/* Enable the Main Output */
							self->tim->BDTR |= (1<<15);
							/* Enable the Peripheral */
							self->tim->CR1 |= (1<<0);
            }

            break;
        }
        default:
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid mode (%d)"), chan->mode);
    }

    return MP_OBJ_FROM_PTR(chan);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_timer_channel_obj, 2, pyb_timer_channel);

/// \method counter([value])
/// Get or set the timer counter.
STATIC mp_obj_t pyb_timer_counter(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get
        return mp_obj_new_int(self->tim->CNT);
    } else {
        // set
				self->tim->CNT = mp_obj_get_int(args[1]);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_counter_obj, 1, 2, pyb_timer_counter);

/// \method source_freq()
/// Get the frequency of the source of the timer.
STATIC mp_obj_t pyb_timer_source_freq(mp_obj_t self_in) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t source_freq = timer_get_source_freq(self->tim_id);
    return mp_obj_new_int(source_freq);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_source_freq_obj, pyb_timer_source_freq);

/// \method freq([value])
/// Get or set the frequency for the timer (changes prescaler and period if set).
STATIC mp_obj_t pyb_timer_freq(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get
        uint32_t prescaler = self->tim->PSC & 0xffff;
        uint32_t period = self->tim->ARR & TIMER_CNT_MASK(self);
        uint32_t source_freq = timer_get_source_freq(self->tim_id);
        uint32_t divide_a = prescaler + 1;
        uint32_t divide_b = period + 1;
        #if MICROPY_PY_BUILTINS_FLOAT
        if (source_freq % divide_a != 0) {
            return mp_obj_new_float((mp_float_t)source_freq / (mp_float_t)divide_a / (mp_float_t)divide_b);
        }
        source_freq /= divide_a;
        if (source_freq % divide_b != 0) {
            return mp_obj_new_float((mp_float_t)source_freq / (mp_float_t)divide_b);
        } else {
            return mp_obj_new_int(source_freq / divide_b);
        }
        #else
        return mp_obj_new_int(source_freq / divide_a / divide_b);
        #endif
    } else {
        // set
        uint32_t period;
        uint32_t prescaler = compute_prescaler_period_from_freq(self, args[1], &period);
        self->tim->PSC = prescaler;

				self->tim->ARR = period;
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_freq_obj, 1, 2, pyb_timer_freq);

/// \method prescaler([value])
/// Get or set the prescaler for the timer.
STATIC mp_obj_t pyb_timer_prescaler(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get
        return mp_obj_new_int(self->tim->PSC & 0xffff);
    } else {
        // set
        self->tim->PSC = mp_obj_get_int(args[1]) & 0xffff;
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_prescaler_obj, 1, 2, pyb_timer_prescaler);

/// \method period([value])
/// Get or set the period of the timer.
STATIC mp_obj_t pyb_timer_period(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get
        return mp_obj_new_int(TIM_GET_AUTORELOAD(self->tim) & TIMER_CNT_MASK(self));
    } else {
        // set
        //__HAL_TIM_SET_AUTORELOAD(&self->tim, mp_obj_get_int(args[1]) & TIMER_CNT_MASK(self));
				self->tim->ARR = (mp_obj_get_int(args[1]) & TIMER_CNT_MASK(self));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_period_obj, 1, 2, pyb_timer_period);

/// \method callback(fun)
/// Set the function to be called when the timer triggers.
/// `fun` is passed 1 argument, the timer object.
/// If `fun` is `None` then the callback will be disabled.
STATIC mp_obj_t pyb_timer_callback(mp_obj_t self_in, mp_obj_t callback) {
    pyb_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (callback == mp_const_none) {
        // stop interrupt (but not timer)
				self->tim->DIER &= ~(TIM_IT_Update);
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
				self->tim->DIER &= ~(TIM_IT_Update);
        self->callback = callback;
				self->tim->SR &= ~(TIM_IT_Update);
				self->tim->DIER |= (TIM_IT_Update);
				self->tim->CR1 |= TIM_CR1_CEN;
        NVIC_EnableIRQ(self->irqn);
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("callback must be None or a callable object"));
    }
		
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_callback_obj, pyb_timer_callback);

STATIC const mp_rom_map_elem_t pyb_timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&pyb_timer_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_counter), MP_ROM_PTR(&pyb_timer_counter_obj) },
    { MP_ROM_QSTR(MP_QSTR_source_freq), MP_ROM_PTR(&pyb_timer_source_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&pyb_timer_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_prescaler), MP_ROM_PTR(&pyb_timer_prescaler_obj) },
    { MP_ROM_QSTR(MP_QSTR_period), MP_ROM_PTR(&pyb_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_UP), MP_ROM_INT(TIM_COUNTERMODE_UP) },
    { MP_ROM_QSTR(MP_QSTR_DOWN), MP_ROM_INT(TIM_COUNTERMODE_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_CENTER), MP_ROM_INT(TIM_COUNTERMODE_CENTERALIGNED1) },
    { MP_ROM_QSTR(MP_QSTR_PWM), MP_ROM_INT(CHANNEL_MODE_PWM_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_PWM_INVERTED), MP_ROM_INT(CHANNEL_MODE_PWM_INVERTED) },
    { MP_ROM_QSTR(MP_QSTR_IC), MP_ROM_INT(CHANNEL_MODE_IC) },
    { MP_ROM_QSTR(MP_QSTR_HIGH), MP_ROM_INT(TIM_OCPOLARITY_HIGH) },
    { MP_ROM_QSTR(MP_QSTR_LOW), MP_ROM_INT(TIM_OCPOLARITY_LOW) },
    { MP_ROM_QSTR(MP_QSTR_RISING), MP_ROM_INT(TIM_ICPolarity_Rising) },
    { MP_ROM_QSTR(MP_QSTR_FALLING), MP_ROM_INT(TIM_ICPolarity_Falling) },
    { MP_ROM_QSTR(MP_QSTR_BOTH), MP_ROM_INT(TIM_ICPolarity_BothEdge) },
    { MP_ROM_QSTR(MP_QSTR_BRK_OFF), MP_ROM_INT(BRK_OFF) },
    { MP_ROM_QSTR(MP_QSTR_BRK_LOW), MP_ROM_INT(BRK_LOW) },
    { MP_ROM_QSTR(MP_QSTR_BRK_HIGH), MP_ROM_INT(BRK_HIGH) },
		
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(SOFT_TIMER_MODE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(SOFT_TIMER_MODE_PERIODIC) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_timer_locals_dict, pyb_timer_locals_dict_table);

const mp_obj_type_t pyb_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = pyb_timer_print,
    .make_new = pyb_timer_make_new,
    .locals_dict = (mp_obj_dict_t *)&pyb_timer_locals_dict,
};

/// \moduleref pyb
/// \class TimerChannel - setup a channel for a timer.
///
/// Timer channels are used to generate/capture a signal using a timer.
///
/// TimerChannel objects are created using the Timer.channel() method.
STATIC void pyb_timer_channel_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_timer_channel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "TimerChannel(timer=%u, channel=%u, mode=%s)",
        self->timer->tim_id,
        self->channel,
        qstr_str(channel_mode_info[self->mode].name));
}

/// \method capture([value])
/// Get or set the capture value associated with a channel.
/// capture, compare, and pulse_width are all aliases for the same function.
/// capture is the logical name to use when the channel is in input capture mode.

/// \method compare([value])
/// Get or set the compare value associated with a channel.
/// capture, compare, and pulse_width are all aliases for the same function.
/// compare is the logical name to use when the channel is in output compare mode.

/// \method pulse_width([value])
/// Get or set the pulse width value associated with a channel.
/// capture, compare, and pulse_width are all aliases for the same function.
/// pulse_width is the logical name to use when the channel is in PWM mode.
///
/// In edge aligned mode, a pulse_width of `period + 1` corresponds to a duty cycle of 100%
/// In center aligned mode, a pulse width of `period` corresponds to a duty cycle of 100%
STATIC mp_obj_t pyb_timer_channel_capture_compare(size_t n_args, const mp_obj_t *args) {
    pyb_timer_channel_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // get
        return mp_obj_new_int(TIM_GET_COMPARE(self->timer->tim, TIMER_CHANNEL(self)) & TIMER_CNT_MASK(self->timer));
    } else {
        // set
        TIM_SET_COMPARE(self->timer->tim, TIMER_CHANNEL(self), mp_obj_get_int(args[1]) & TIMER_CNT_MASK(self->timer));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_channel_capture_compare_obj, 1, 2, pyb_timer_channel_capture_compare);

/// \method pulse_width_percent([value])
/// Get or set the pulse width percentage associated with a channel.  The value
/// is a number between 0 and 100 and sets the percentage of the timer period
/// for which the pulse is active.  The value can be an integer or
/// floating-point number for more accuracy.  For example, a value of 25 gives
/// a duty cycle of 25%.
STATIC mp_obj_t pyb_timer_channel_pulse_width_percent(size_t n_args, const mp_obj_t *args) {
    pyb_timer_channel_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t period = compute_period(self->timer);
    if (n_args == 1) {
        // get
        uint32_t cmp = TIM_GET_COMPARE(self->timer->tim, TIMER_CHANNEL(self)) & TIMER_CNT_MASK(self->timer);
        return compute_percent_from_pwm_value(period, cmp);
    } else {
        // set
        uint32_t cmp = compute_pwm_value_from_percent(period, args[1]);
        TIM_SET_COMPARE(self->timer->tim, TIMER_CHANNEL(self), cmp & TIMER_CNT_MASK(self->timer));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_channel_pulse_width_percent_obj, 1, 2, pyb_timer_channel_pulse_width_percent);

/// \method callback(fun)
/// Set the function to be called when the timer channel triggers.
/// `fun` is passed 1 argument, the timer object.
/// If `fun` is `None` then the callback will be disabled.
STATIC mp_obj_t pyb_timer_channel_callback(mp_obj_t self_in, mp_obj_t callback) {
    pyb_timer_channel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (callback == mp_const_none) {
        // stop interrupt (but not timer)

				self->timer->tim->DIER &= ~TIMER_IRQ_MASK(self->channel);
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
        self->callback = callback;
				self->timer->tim->SR = ~(TIMER_IRQ_MASK(self->channel));

        if (self->timer->tim_id == 1) {
            NVIC_EnableIRQ(TIM1_CC_IRQn);
        } else if (self->timer->tim_id == 2) {
            NVIC_EnableIRQ(TIM2_CC_IRQn);
        } else
        {
            NVIC_EnableIRQ(self->timer->irqn);
        }
        // start timer, so that it interrupts on overflow
        switch (self->mode) {
            case CHANNEL_MODE_PWM_NORMAL:
            case CHANNEL_MODE_PWM_INVERTED:
								TIM_PWM_Start_IT(self->timer->tim, TIMER_CHANNEL(self));
            break;
        }
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("callback must be None or a callable object"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_channel_callback_obj, pyb_timer_channel_callback);

STATIC const mp_rom_map_elem_t pyb_timer_channel_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&pyb_timer_channel_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_pulse_width), MP_ROM_PTR(&pyb_timer_channel_capture_compare_obj) },
    { MP_ROM_QSTR(MP_QSTR_pulse_width_percent), MP_ROM_PTR(&pyb_timer_channel_pulse_width_percent_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture), MP_ROM_PTR(&pyb_timer_channel_capture_compare_obj) },
    { MP_ROM_QSTR(MP_QSTR_compare), MP_ROM_PTR(&pyb_timer_channel_capture_compare_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_timer_channel_locals_dict, pyb_timer_channel_locals_dict_table);

STATIC const mp_obj_type_t pyb_timer_channel_type = {
    { &mp_type_type },
    .name = MP_QSTR_TimerChannel,
    .print = pyb_timer_channel_print,
    .locals_dict = (mp_obj_dict_t *)&pyb_timer_channel_locals_dict,
};

STATIC void timer_handle_irq_channel(pyb_timer_obj_t *tim, uint8_t channel, mp_obj_t callback) {
    uint32_t irq_mask = TIMER_IRQ_MASK(channel);

    if (TIM_GET_FLAG(tim->tim, irq_mask) != RESET) {
        if (TIM_GET_IT_SOURCE(tim->tim, irq_mask) != RESET) {
            // clear the interrupt
            TIM_CLEAR_IT(tim->tim, irq_mask);
            // execute callback if it's set
            if (callback != mp_const_none) {
                mp_sched_lock();
                // When executing code within a handler we must lock the GC to prevent
                // any memory allocations.  We must also catch any exceptions.
                gc_lock();
                nlr_buf_t nlr;
                if (nlr_push(&nlr) == 0) {
                    mp_call_function_1(callback, MP_OBJ_FROM_PTR(tim));
                    nlr_pop();
                } else {
                    // Uncaught exception; disable the callback so it doesn't run again.
                    tim->callback = mp_const_none;
										tim->tim->DIER &= ~(irq_mask);
                    if (channel == 0) {
                        mp_printf(MICROPY_ERROR_PRINTER, "uncaught exception in Timer(%u) interrupt handler\n", tim->tim_id);
                    } else {
                        mp_printf(MICROPY_ERROR_PRINTER, "uncaught exception in Timer(%u) channel %u interrupt handler\n", tim->tim_id, channel);
                    }
                    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
                }
                gc_unlock();
                mp_sched_unlock();
            }
        }
				
			if(tim->RunMode == SOFT_TIMER_MODE_ONE_SHOT) {
					if (tim != NULL) {
							pyb_timer_deinit(MP_OBJ_FROM_PTR(tim));
					}
			}
    }
}

void timer_irq_handler(uint tim_id) {
    if (tim_id - 1 < PYB_TIMER_OBJ_ALL_NUM) {
        // get the timer object
        pyb_timer_obj_t *tim = MP_STATE_PORT(pyb_timer_obj_all)[tim_id - 1];

        if (tim == NULL) {
            return;
        }
        // Check for timer (versus timer channel) interrupt.
        timer_handle_irq_channel(tim, 0, tim->callback);
        uint32_t handled = TIMER_IRQ_MASK(0);

        // Check to see if a timer channel interrupt was pending
        pyb_timer_channel_obj_t *chan = tim->channel;
        while (chan != NULL) {
            timer_handle_irq_channel(tim, chan->channel, chan->callback);
            handled |= TIMER_IRQ_MASK(chan->channel);
            chan = chan->next;
        }
        uint32_t unhandled = tim->tim->DIER & 0xff & ~handled;
        if (unhandled != 0) {
						tim->tim->DIER &= ~(unhandled);
						tim->tim->SR &= ~(unhandled);
            mp_printf(MICROPY_ERROR_PRINTER, "unhandled interrupt SR=0x%02x (now disabled)\n", (unsigned int)unhandled);
        }
    }
}
