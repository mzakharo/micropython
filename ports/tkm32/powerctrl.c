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

#include "py/mperrno.h"
#include "py/mphal.h"
#include "powerctrl.h"
#include "rtc.h"
#include "genhdr/pllfreqtable.h"
#include "stdio.h"

#define RCC_SR          CSR
#define RCC_SR_SFTRSTF  RCC_CSR_SFTRSTF
#define RCC_SR_RMVF     RCC_CSR_RMVF

// Location in RAM of bootloader state (just after the top of the stack)
extern uint32_t _estack[];
#define BL_STATE ((uint32_t *)&_estack)

static inline void powerctrl_disable_hsi_if_unused(void) {
    RCC_HSICmd(DISABLE);

}

NORETURN void powerctrl_mcu_reset(void) {
    BL_STATE[1] = 1; // invalidate bootloader address
    #if __DCACHE_PRESENT == 1
    SCB_CleanDCache();
    #endif
    NVIC_SystemReset();
}

NORETURN void powerctrl_enter_bootloader(uint32_t r0, uint32_t bl_addr) {
    BL_STATE[0] = r0;
    BL_STATE[1] = bl_addr;
    #if __DCACHE_PRESENT == 1
    SCB_CleanDCache();
    #endif
		
    NVIC_SystemReset();
		
}


typedef struct _sysclk_scaling_table_entry_t {
    uint16_t mhz;
    uint16_t value;
} sysclk_scaling_table_entry_t;


int powerctrl_set_sysclk(uint32_t sysclk, uint32_t ahb, uint32_t apb1, uint32_t apb2) {

    return 0;
}


void powerctrl_enter_stop_mode(void) {
	// Disable IRQs so that the IRQ that wakes the device from stop mode is not
	// executed until after the clocks are reconfigured
	uint32_t irq_state = disable_irq();


	PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

	// reconfigure the system clock after waking up

	// enable clock
	RCC_HSEConfig(RCC_HSE_ON);
	while (!RCC_GetFlagStatus(RCC_FLAG_HSERDY)) {
	}

	// enable PLL
	RCC_PLLCmd(ENABLE);
	while (!RCC_GetFlagStatus(RCC_FLAG_PLLRDY)) {
	}
	// select PLL as system clock source
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);//选择外部时钟作为系统时钟

	powerctrl_disable_hsi_if_unused();

	// Enable IRQs now that all clocks are reconfigured
	enable_irq(irq_state);
}

void powerctrl_enter_standby_mode(void) {

}
