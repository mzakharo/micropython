
#include "py/mphal.h"
#include "modmachine.h"
#include "stdio.h"

#if 1

#define NS_PRESCALE (6)
#define NS_OVERHEAD (0)
#define NS_OVERHEAD_LOW (0)

uint32_t mp_hal_delay_ns_calc(uint32_t ns, uint32_t extra_overhead) {
//uint32_t nclk = 240000000 / 1000000 * ns / 1000 / NS_PRESCALE;
uint32_t nclk = 240000000 / 1000000 * ns / 1000 ;
    if (nclk <= NS_OVERHEAD + extra_overhead) {
        nclk = 1;
    } else {
        nclk -= NS_OVERHEAD + extra_overhead;
    }
    return nclk;
}

STATIC mp_obj_t machine_pixelbitstream(mp_obj_t pin_in, mp_obj_t timing_in, mp_obj_t buf_in) {
    // Get the pin to output to
    mp_hal_pin_obj_t pin = mp_hal_get_pin_obj(pin_in);

    // Get timing values (in ns) and convert to machine-dependent loop counters
    uint32_t timing_ns[4];
    mp_obj_t *timing;
    mp_obj_get_array_fixed_n(timing_in, 4, &timing);
    for (size_t i = 0; i < 4; ++i) {
        uint32_t overhead = i == 1 || i == 3 ? NS_OVERHEAD_LOW : 0; // extra overhead for low cycle
        timing_ns[i] = mp_hal_delay_ns_calc(mp_obj_get_int(timing[i]), overhead);
    }

    // Get buffer to write
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
		
    for (size_t i = 0; i < bufinfo.len; i++) {
        uint8_t b = ((uint8_t*)bufinfo.buf)[i];
						for (size_t j = 0; j < 8; j++){
            uint32_t *t = &timing_ns[b >> 6 & 2];
            uint32_t k = t[0];
						pin->gpio->BSRR = pin->pin_mask;
						while (--k) {
							__NOP();
            }
						pin->gpio->BRR = pin->pin_mask;
            k = t[1];
            while (--k) {
              __NOP();
            }
            b <<= 1;
        }
    }

    MICROPY_END_ATOMIC_SECTION(atomic_state);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(machine_pixelbitstream_obj, machine_pixelbitstream);
#endif