/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Damien P. George
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

#include "rtc.h"
#include "rng.h"

#if MICROPY_HW_ENABLE_RNG
STATIC uint32_t pyb_rng_yasmarang(void) {
    static bool seeded = false;
    static uint32_t pad = 0, n = 0, d = 0;
    static uint8_t dat = 0;

    if (!seeded) {
        seeded = true;
        pad = *(uint32_t *)MP_HAL_UNIQUE_ID_ADDRESS ^ SysTick->VAL;
        n = RTC->CNTH;
        d = RTC->CNTL;
    }

    pad += dat + d * n;
    pad = (pad << 3) + (pad >> 29);
    n = pad | 2;
    d ^= (pad << 31) + (pad >> 1);
    dat ^= (char)pad ^ (d >> 8) ^ 1;

    return pad ^ (d << 5) ^ (pad >> 18) ^ (dat << 1);
}

uint32_t rng_get(void) {
    return pyb_rng_yasmarang();
}

// Return a 30-bit hardware generated random number.
STATIC mp_obj_t pyb_rng_get(void) {
    return mp_obj_new_int(rng_get() >> 2);
}

MP_DEFINE_CONST_FUN_OBJ_0(pyb_rng_get_obj, pyb_rng_get);

#endif // MICROPY_HW_ENABLE_RNG

