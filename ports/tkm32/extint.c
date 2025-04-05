/*
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "pendsv.h"
#include "pin.h"
#include "extint.h"
#include "irq.h"

/// \moduleref pyb
/// \class ExtInt - configure I/O pins to interrupt on external events
///
/// There are a total of 22 interrupt lines. 16 of these can come from GPIO pins
/// and the remaining 6 are from internal sources.
///
/// For lines 0 thru 15, a given line can map to the corresponding line from an
/// arbitrary port. So line 0 can map to Px0 where x is A, B, C, ... and
/// line 1 can map to Px1 where x is A, B, C, ...
///
///     def callback(line):
///         print("line =", line)
///
/// Note: ExtInt will automatically configure the gpio line as an input.
///
///     extint = pyb.ExtInt(pin, pyb.ExtInt.IRQ_FALLING, pyb.Pin.PULL_UP, callback)
///
/// Now every time a falling edge is seen on the X1 pin, the callback will be
/// called. Caution: mechanical pushbuttons have "bounce" and pushing or
/// releasing a switch will often generate multiple edges.
/// See: http://www.eng.utah.edu/~cs5780/debouncing.pdf for a detailed
/// explanation, along with various techniques for debouncing.
///
/// Trying to register 2 callbacks onto the same pin will throw an exception.
///
/// If pin is passed as an integer, then it is assumed to map to one of the
/// internal interrupt sources, and must be in the range 16 thru 22.
///
/// All other pin objects go through the pin mapper to come up with one of the
/// gpio pins.
///
///     extint = pyb.ExtInt(pin, mode, pull, callback)
///
/// Valid modes are pyb.ExtInt.IRQ_RISING, pyb.ExtInt.IRQ_FALLING,
/// pyb.ExtInt.IRQ_RISING_FALLING, pyb.ExtInt.EVT_RISING,
/// pyb.ExtInt.EVT_FALLING, and pyb.ExtInt.EVT_RISING_FALLING.
///
/// Only the IRQ_xxx modes have been tested. The EVT_xxx modes have
/// something to do with sleep mode and the WFE instruction.
///
/// Valid pull values are pyb.Pin.PULL_UP, pyb.Pin.PULL_DOWN, pyb.Pin.PULL_NONE.
///
/// There is also a C API, so that drivers which require EXTI interrupt lines
/// can also use this code. See extint.h for the available functions and
/// usrsw.h for an example of using this.


#define  RESERVE1	201
#define  RESERVE2	202
#define  RESERVE3	203

typedef struct {
    mp_obj_base_t base;
    mp_int_t line;
} extint_obj_t;

STATIC uint8_t pyb_extint_mode[EXTI_NUM_VECTORS];
STATIC bool pyb_extint_hard_irq[EXTI_NUM_VECTORS];

// The callback arg is a small-int or a ROM Pin object, so no need to scan by GC
STATIC mp_obj_t pyb_extint_callback_arg[EXTI_NUM_VECTORS];

STATIC const uint8_t nvic_irq_channel[EXTI_NUM_VECTORS] = {

    EXTI0_IRQn,     EXTI1_IRQn,     EXTI2_IRQn,     EXTI3_IRQn,     EXTI4_IRQn,
    EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,   EXTI9_5_IRQn,
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
    EXTI15_10_IRQn,
		RESERVE1,
		RESERVE2,
		RESERVE3,
		RTCAlarm_IRQn,
		USBAwake_IRQn,
};

STATIC bool is_vlin(uint v_line)
{
	if (v_line >= EXTI_NUM_VECTORS || v_line == 16 || v_line == 17 || v_line == 18) {
		return true;
	}
	
	return false;
}

// Set override_callback_obj to true if you want to unconditionally set the
// callback function.
uint extint_register(mp_obj_t pin_obj, uint8_t mode, uint32_t pull, mp_obj_t callback_obj, bool override_callback_obj) {
	const pin_obj_t *pin = NULL;
	uint v_line;

	if (mp_obj_is_int(pin_obj)) {
			// If an integer is passed in, then use it to identify lines 16 thru 22
			// We expect lines 0 thru 15 to be passed in as a pin, so that we can
			// get both the port number and line number.
			v_line = mp_obj_get_int(pin_obj);
			if (v_line < 16) {
					mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("ExtInt vector %d < 16, use a Pin object"), v_line);
			}
			if (is_vlin(v_line)) {
					mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("ExtInt vector %d >= max of %d"), v_line, EXTI_NUM_VECTORS);
			}
	} else {
			pin = pin_find(pin_obj);
			v_line = pin->pin;
	}
	
	if (mode != EXTI_MODE_IT_RISING &&
			mode != EXTI_MODE_IT_FALLING &&
			mode != EXTI_MODE_IT_RISING_FALLING &&
			mode != EXTI_MODE_EVT_RISING &&
			mode != EXTI_MODE_EVT_FALLING &&
			mode != EXTI_EVT_RISING_FALLING) {
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid ExtInt Mode: %d"), mode);
	}
	if (pull != GPIO_Mode_IN_FLOATING &&
			pull != GPIO_Mode_IPU &&
			pull != GPIO_Mode_IPD) {
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid ExtInt Pull: %d"), pull);
	}

	mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[v_line];
	if (!override_callback_obj && *cb != mp_const_none && callback_obj != mp_const_none) {
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("ExtInt vector %d is already in use"), v_line);
	}
	*cb = callback_obj;
	pyb_extint_mode[v_line] = mode;

	if (*cb != mp_const_none) {
		pyb_extint_hard_irq[v_line] = true;
		pyb_extint_callback_arg[v_line] = MP_OBJ_NEW_SMALL_INT(v_line);
		
		if (pin == NULL) {
				// pin will be NULL for non GPIO EXTI lines
			extint_trigger_mode(v_line, mode);
			extint_enable(v_line);
		} else {
				mp_hal_gpio_clock_enable(pin->gpio);
				GPIO_InitTypeDef exti;
				exti.GPIO_Pin = pin->pin_mask;
				exti.GPIO_Mode = pull;
				exti.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_Init(pin->gpio, &exti);

				RCC->APB2ENR|=1<<14;
				SYSCFG->EXTICR[v_line >> 2] =
				(SYSCFG->EXTICR[v_line >> 2] & ~(0x0f << (4 * (v_line & 0x03))))
				| ((uint32_t)(GPIO_GET_IND(pin->gpio)) << (4 * (v_line & 0x03)));
				
				extint_enable(v_line);
				extint_trigger_mode(v_line, mode);

				NVIC_SetPriority(IRQn_NONNEG(nvic_irq_channel[v_line]), IRQ_PRI_EXTINT);
        NVIC_EnableIRQ(nvic_irq_channel[v_line]);
				
			}

	}
  return v_line;
}

// This function is intended to be used by the Pin.irq() method
void extint_register_pin(const pin_obj_t *pin, uint8_t mode, bool hard_irq, mp_obj_t callback_obj) {
    uint32_t line = pin->pin;

    // Check if the ExtInt line is already in use by another Pin/ExtInt
    mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[line];
    if (*cb != mp_const_none && MP_OBJ_FROM_PTR(pin) != pyb_extint_callback_arg[line]) {
        if (mp_obj_is_small_int(pyb_extint_callback_arg[line])) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("ExtInt vector %d is already in use"), line);
        } else {
            const pin_obj_t *other_pin = MP_OBJ_TO_PTR(pyb_extint_callback_arg[line]);
            mp_raise_msg_varg(&mp_type_OSError,
                MP_ERROR_TEXT("IRQ resource already taken by Pin('%q')"), other_pin->name);
        }
    }

    extint_disable(line);

    *cb = callback_obj;
		
    pyb_extint_mode[line] = mode;

    if (*cb != mp_const_none) {
        // Configure and enable the callback

        pyb_extint_hard_irq[line] = hard_irq;
        pyb_extint_callback_arg[line] = MP_OBJ_FROM_PTR(pin);

				RCC->APB2ENR|=1<<14;  						//使能SYSCFG时钟 

        // Route the GPIO to EXTI
        SYSCFG->EXTICR[line >> 2] =
            (SYSCFG->EXTICR[line >> 2] & ~(0x0f << (4 * (line & 0x03))))
            | ((uint32_t)(GPIO_GET_IND(pin->gpio)) << (4 * (line & 0x03)));

        extint_trigger_mode(line, mode);

				// Configure the NVIC
        NVIC_SetPriority(IRQn_NONNEG(nvic_irq_channel[line]), IRQ_PRI_EXTINT);
        NVIC_EnableIRQ(nvic_irq_channel[line]);
        // Enable the interrupt
        extint_enable(line);
    }
}

void extint_set(const pin_obj_t *pin, uint8_t mode) {
    uint32_t line = pin->pin;

    mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[line];

    extint_disable(line);

    *cb = MP_OBJ_SENTINEL;

    pyb_extint_mode[line] = mode;
    {
        // Configure and enable the callback

        pyb_extint_hard_irq[line] = 1;
        pyb_extint_callback_arg[line] = MP_OBJ_FROM_PTR(pin);

        // Route the GPIO to EXTI

        SYSCFG->EXTICR[line >> 2] =
            (SYSCFG->EXTICR[line >> 2] & ~(0x0f << (4 * (line & 0x03))))
            | ((uint32_t)(GPIO_GET_IND(pin->gpio)) << (4 * (line & 0x03)));

        // Enable or disable the rising detector
        if ((mode & GPIO_MODE_IT_RISING) == GPIO_MODE_IT_RISING) {
            EXTI->RTSR |= 1 << line;
        } else {
            EXTI->RTSR &= ~(1 << line);
        }

        // Enable or disable the falling detector
        if ((mode & GPIO_MODE_IT_FALLING) == GPIO_MODE_IT_FALLING) {
            EXTI->FTSR |= 1 << line;
        } else {
            EXTI->FTSR &= ~(1 << line);
        }

        // Configure the NVIC
        NVIC_SetPriority(IRQn_NONNEG(nvic_irq_channel[line]), IRQ_PRI_EXTINT);
        NVIC_EnableIRQ(nvic_irq_channel[line]);

        // Enable the interrupt
        extint_enable(line);
    }
}

void extint_enable(uint line) {
	if (is_vlin(line)) {
			return;
	}
	if (pyb_extint_mode[line] <= EXTI_MODE_IT_RISING_FALLING) {
			EXTI->IMR |= (1 << line);
	} else {
			EXTI->EMR |= (1 << line);
	}

}

void extint_disable(uint line) {
	if (is_vlin(line)) {
			return;
	}
	EXTI->IMR &= ~(1 << line);
	EXTI->EMR &= ~(1 << line);
}

void extint_swint(uint line) {
    if (line >= EXTI_NUM_VECTORS) {
        return;
    }
    // we need 0 to 1 transition to trigger the interrupt
    EXTI->SWIER &= ~(1 << line);
    EXTI->SWIER |= (1 << line);
}

void extint_trigger_mode(uint line, uint8_t mode) {
	if (is_vlin(line)) {
			return;
	}
	
	EXTI->RTSR &= ~(1 << line);
	EXTI->FTSR &= ~(1 << line);

	if(mode & EXTI_MODE_IT_RISING){
		EXTI->RTSR |= (1 << line);
	}
 	if(mode & EXTI_MODE_IT_FALLING){
		EXTI->FTSR |= (1 << line);
	}
}

/// \method line()
/// Return the line number that the pin is mapped to.
STATIC mp_obj_t extint_obj_line(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->line);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_line_obj, extint_obj_line);

/// \method enable()
/// Enable a disabled interrupt.
STATIC mp_obj_t extint_obj_enable(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    extint_enable(self->line);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_enable_obj, extint_obj_enable);

/// \method disable()
/// Disable the interrupt associated with the ExtInt object.
/// This could be useful for debouncing.
STATIC mp_obj_t extint_obj_disable(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    extint_disable(self->line);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_disable_obj, extint_obj_disable);

/// \method swint()
/// Trigger the callback from software.
STATIC mp_obj_t extint_obj_swint(mp_obj_t self_in) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    extint_swint(self->line);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(extint_obj_swint_obj,  extint_obj_swint);

// TODO document as a staticmethod
/// \classmethod regs()
/// Dump the values of the EXTI registers.
STATIC mp_obj_t extint_regs(void) {

    printf("EXTI_IMR   %08x\n", (unsigned int)EXTI->IMR);
    printf("EXTI_EMR   %08x\n", (unsigned int)EXTI->EMR);
    printf("EXTI_RTSR  %08x\n", (unsigned int)EXTI->RTSR);
    printf("EXTI_FTSR  %08x\n", (unsigned int)EXTI->FTSR);
    printf("EXTI_SWIER %08x\n", (unsigned int)EXTI->SWIER);
    printf("EXTI_PR    %08x\n", (unsigned int)EXTI->PR);
		
for(uint8_t i=0;i<4;i++)
		printf("EXTICR[%d] %08x\n",i, (unsigned int)SYSCFG->EXTICR[i]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(extint_regs_fun_obj, extint_regs);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(extint_regs_obj, MP_ROM_PTR(&extint_regs_fun_obj));

/// \classmethod \constructor(pin, mode, pull, callback)
/// Create an ExtInt object:
///
///   - `pin` is the pin on which to enable the interrupt (can be a pin object or any valid pin name).
///   - `mode` can be one of:
///     - `ExtInt.IRQ_RISING` - trigger on a rising edge;
///     - `ExtInt.IRQ_FALLING` - trigger on a falling edge;
///     - `ExtInt.IRQ_RISING_FALLING` - trigger on a rising or falling edge.
///   - `pull` can be one of:
///     - `pyb.Pin.PULL_NONE` - no pull up or down resistors;
///     - `pyb.Pin.PULL_UP` - enable the pull-up resistor;
///     - `pyb.Pin.PULL_DOWN` - enable the pull-down resistor.
///   - `callback` is the function to call when the interrupt triggers.  The
///   callback function must accept exactly 1 argument, which is the line that
///   triggered the interrupt.
STATIC const mp_arg_t pyb_extint_make_new_args[] = {
    { MP_QSTR_pin,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_pull,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
};
#define PYB_EXTINT_MAKE_NEW_NUM_ARGS MP_ARRAY_SIZE(pyb_extint_make_new_args)

STATIC mp_obj_t extint_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // type_in == extint_obj_type

    // parse args
    mp_arg_val_t vals[PYB_EXTINT_MAKE_NEW_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, PYB_EXTINT_MAKE_NEW_NUM_ARGS, pyb_extint_make_new_args, vals);

    extint_obj_t *self = m_new_obj(extint_obj_t);
    self->base.type = type;
    self->line = extint_register(vals[0].u_obj, vals[1].u_int, vals[2].u_int, vals[3].u_obj, false);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void extint_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    extint_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ExtInt line=%u>", self->line);
}

STATIC const mp_rom_map_elem_t extint_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_line),    MP_ROM_PTR(&extint_obj_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable),  MP_ROM_PTR(&extint_obj_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable), MP_ROM_PTR(&extint_obj_disable_obj) },
    { MP_ROM_QSTR(MP_QSTR_swint),   MP_ROM_PTR(&extint_obj_swint_obj) },
    { MP_ROM_QSTR(MP_QSTR_regs),    MP_ROM_PTR(&extint_regs_obj) },

    // class constants
    /// \constant IRQ_RISING - interrupt on a rising edge
    /// \constant IRQ_FALLING - interrupt on a falling edge
    /// \constant IRQ_RISING_FALLING - interrupt on a rising or falling edge
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING),         MP_ROM_INT(EXTI_MODE_IT_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING),        MP_ROM_INT(EXTI_MODE_IT_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING_FALLING), MP_ROM_INT(EXTI_MODE_IT_RISING_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_EVT_RISING),         MP_ROM_INT(EXTI_MODE_EVT_RISING) },
    { MP_ROM_QSTR(MP_QSTR_EVT_FALLING),        MP_ROM_INT(EXTI_MODE_EVT_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_EVT_RISING_FALLING), MP_ROM_INT(EXTI_EVT_RISING_FALLING) },
};

STATIC MP_DEFINE_CONST_DICT(extint_locals_dict, extint_locals_dict_table);

const mp_obj_type_t extint_type = {
    { &mp_type_type },
    .name = MP_QSTR_ExtInt,
    .print = extint_obj_print,
    .make_new = extint_make_new,
    .locals_dict = (mp_obj_dict_t *)&extint_locals_dict,
};

void extint_init0(void) {
    for (int i = 0; i < PYB_EXTI_NUM_VECTORS; i++) {
        if (MP_STATE_PORT(pyb_extint_callback)[i] == MP_OBJ_SENTINEL) {
            continue;
        }
        MP_STATE_PORT(pyb_extint_callback)[i] = mp_const_none;
        pyb_extint_mode[i] = EXTI_MODE_IT_FALLING;
    }
		
		EXTI->IMR = 0x0U;
		EXTI->EMR = 0x0U;
}

#define		GET_IMR_FLAG(__EXTI_LINE__) 		(EXTI->IMR & (__EXTI_LINE__))

// Interrupt handler
void Handle_EXTI_Irq(uint32_t line) {
	
		if (GPIO_EXTI_GET_FLAG(1 << line) && GET_IMR_FLAG(1 << line)) {
   // if (GPIO_EXTI_GET_FLAG(1 << line)) {
        GPIO_EXTI_CLEAR_FLAG(1 << line);
        if (line < EXTI_NUM_VECTORS) {
            mp_obj_t *cb = &MP_STATE_PORT(pyb_extint_callback)[line];
						
            if (*cb != mp_const_none) {
                // If it's a soft IRQ handler then just schedule callback for later
                if (!pyb_extint_hard_irq[line]) {
                    mp_sched_schedule(*cb, pyb_extint_callback_arg[line]);
                    return;
                }

                mp_sched_lock();
                // When executing code within a handler we must lock the GC to prevent
                // any memory allocations.  We must also catch any exceptions.
                gc_lock();
                nlr_buf_t nlr;
                if (nlr_push(&nlr) == 0) {
                    mp_call_function_1(*cb, pyb_extint_callback_arg[line]);
                    nlr_pop();
                } else {
                    // Uncaught exception; disable the callback so it doesn't run again.
                    *cb = mp_const_none;
                    extint_disable(line);
                    mp_printf(MICROPY_ERROR_PRINTER, "uncaught exception in ExtInt interrupt handler line %u\n", (unsigned int)line);
                    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
                }
                gc_unlock();
                mp_sched_unlock();
            }
        }
    }
}
