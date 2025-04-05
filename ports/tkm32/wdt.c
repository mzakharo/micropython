
#include <stdio.h>

#include "py/runtime.h"
#include "wdt.h"

typedef struct _pyb_wdt_obj_t {
    mp_obj_base_t base;
} pyb_wdt_obj_t;

STATIC const pyb_wdt_obj_t pyb_wdt = {{&pyb_wdt_type}};

STATIC mp_obj_t pyb_wdt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse arguments
    enum { ARG_id, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5000} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t id = args[ARG_id].u_int;
    if (id != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("WDT(%d) doesn't exist"), id);
    }

    // timeout is in milliseconds
    mp_int_t timeout = args[ARG_timeout].u_int;

    // compute prescaler
    uint32_t prescaler;
    for (prescaler = 0; prescaler < 6 && timeout >= 409; ++prescaler, timeout /= 2) {
    }

    // convert milliseconds to ticks
    timeout *= 10; // 40kHz / 4 = 10 ticks per millisecond (approx)
    if (timeout <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("WDT timeout too short"));
    } else if (timeout > 0xfff) {
        mp_raise_ValueError(MP_ERROR_TEXT("WDT timeout too long"));
    }
    timeout -= 1;
		
		RCC->CSR |= 0x01U; //LSION ENABLE
	
    // set the reload register
    while (IWDG->SR & 2) {
    }
    IWDG->KR = 0x5555;
    IWDG->RLR = timeout;

    // set the prescaler
    while (IWDG->SR & 1) {
    }
    IWDG->KR = 0x5555;
    IWDG->PR = prescaler;

    // start the watch dog
    IWDG->KR = 0xcccc;

    return MP_OBJ_FROM_PTR(&pyb_wdt);
}

STATIC mp_obj_t pyb_wdt_feed(mp_obj_t self_in) {
    (void)self_in;
    IWDG->KR = 0xaaaa;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_wdt_feed_obj, pyb_wdt_feed);

STATIC const mp_rom_map_elem_t pyb_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&pyb_wdt_feed_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_wdt_locals_dict, pyb_wdt_locals_dict_table);

const mp_obj_type_t pyb_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .make_new = pyb_wdt_make_new,
    .locals_dict = (mp_obj_dict_t *)&pyb_wdt_locals_dict,
};
