/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Original template from ST Cube library.  See below for header.
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


#include <stdio.h>

#include "py/obj.h"
#include "py/mphal.h"
#include "tkm32_it.h"
#include "pendsv.h"
#include "irq.h"
#include "powerctrl.h"
#include "pybthread.h"
#include "gccollect.h"
#include "extint.h"
#include "timer.h"
#include "uart.h"
#include "storage.h"
//#include "dma.h"
//#include "i2c.h"
//#include "usb.h"

extern void __fatal_error(const char *);
#if defined(MICROPY_HW_USB_FS)
//extern PCD_HandleTypeDef pcd_fs_handle;
#endif


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

// Set the following to 1 to get some more information on the Hard Fault
// More information about decoding the fault registers can be found here:
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0646a/Cihdjcfc.html

STATIC char *fmt_hex(uint32_t val, char *buf) {
    const char *hexDig = "0123456789abcdef";

    buf[0] = hexDig[(val >> 28) & 0x0f];
    buf[1] = hexDig[(val >> 24) & 0x0f];
    buf[2] = hexDig[(val >> 20) & 0x0f];
    buf[3] = hexDig[(val >> 16) & 0x0f];
    buf[4] = hexDig[(val >> 12) & 0x0f];
    buf[5] = hexDig[(val >> 8) & 0x0f];
    buf[6] = hexDig[(val >> 4) & 0x0f];
    buf[7] = hexDig[(val >> 0) & 0x0f];
    buf[8] = '\0';

    return buf;
}

STATIC void print_reg(const char *label, uint32_t val) {
    char hexStr[9];

    mp_hal_stdout_tx_str(label);
    mp_hal_stdout_tx_str(fmt_hex(val, hexStr));
    mp_hal_stdout_tx_str("\r\n");
}

STATIC void print_hex_hex(const char *label, uint32_t val1, uint32_t val2) {
    char hex_str[9];
    mp_hal_stdout_tx_str(label);
    mp_hal_stdout_tx_str(fmt_hex(val1, hex_str));
    mp_hal_stdout_tx_str("  ");
    mp_hal_stdout_tx_str(fmt_hex(val2, hex_str));
    mp_hal_stdout_tx_str("\r\n");
}

// The ARMv7M Architecture manual (section B.1.5.6) says that upon entry
// to an exception, that the registers will be in the following order on the
// // stack: R0, R1, R2, R3, R12, LR, PC, XPSR

typedef struct {
    uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
} ExceptionRegisters_t;

int pyb_hard_fault_debug = 0;

void HardFault_C_Handler(ExceptionRegisters_t *regs) {
    if (!pyb_hard_fault_debug) {
        powerctrl_mcu_reset();
    }

    #if MICROPY_HW_ENABLE_USB
    // We need to disable the USB so it doesn't try to write data out on
    // the VCP and then block indefinitely waiting for the buffer to drain.
    pyb_usb_flags = 0;
    #endif

    mp_hal_stdout_tx_str("HardFault\r\n");

    print_reg("R0    ", regs->r0);
    print_reg("R1    ", regs->r1);
    print_reg("R2    ", regs->r2);
    print_reg("R3    ", regs->r3);
    print_reg("R12   ", regs->r12);
    print_reg("SP    ", (uint32_t)regs);
    print_reg("LR    ", regs->lr);
    print_reg("PC    ", regs->pc);
    print_reg("XPSR  ", regs->xpsr);

    #if __CORTEX_M >= 3
    uint32_t cfsr = SCB->CFSR;

    print_reg("HFSR  ", SCB->HFSR);
    print_reg("CFSR  ", cfsr);
    if (cfsr & 0x80) {
        print_reg("MMFAR ", SCB->MMFAR);
    }
    if (cfsr & 0x8000) {
        print_reg("BFAR  ", SCB->BFAR);
    }
    #endif

    if ((void *)&_ram_start <= (void *)regs && (void *)regs < (void *)&_ram_end) {
        mp_hal_stdout_tx_str("Stack:\r\n");
        uint32_t *stack_top = &_estack;
        if ((void *)regs < (void *)&_sstack) {
            // stack not in static stack area so limit the amount we print
            stack_top = (uint32_t *)regs + 32;
        }
        for (uint32_t *sp = (uint32_t *)regs; sp < stack_top; ++sp) {
            print_hex_hex("  ", (uint32_t)sp, *sp);
        }
    }

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {
        __fatal_error("HardFault");
    }
}

// Naked functions have no compiler generated gunk, so are the best thing to
// use for asm functions.
__attribute__((naked))
void HardFault_Handler(void) {

    // From the ARMv7M Architecture Reference Manual, section B.1.5.6
    // on entry to the Exception, the LR register contains, amongst other
    // things, the value of CONTROL.SPSEL. This can be found in bit 3.
    //
    // If CONTROL.SPSEL is 0, then the exception was stacked up using the
    // main stack pointer (aka MSP). If CONTROL.SPSEL is 1, then the exception
    // was stacked up using the process stack pointer (aka PSP).

    #if __CORTEX_M == 0
    __asm volatile (
        " mov r0, lr    \n"
        " lsr r0, r0, #3 \n"    // Shift Bit 3 into carry to see which stack pointer we should use.
        " mrs r0, msp   \n"     // Make R0 point to main stack pointer
        " bcc .use_msp  \n"     // Keep MSP in R0 if SPSEL (carry) is 0
        " mrs r0, psp   \n"     // Make R0 point to process stack pointer
        " .use_msp:     \n"
        " b HardFault_C_Handler \n" // Off to C land
        );
    #else
    __asm volatile (
        " tst lr, #4    \n"     // Test Bit 3 to see which stack pointer we should use.
        " ite eq        \n"     // Tell the assembler that the nest 2 instructions are if-then-else
        " mrseq r0, msp \n"     // Make R0 point to main stack pointer
        " mrsne r0, psp \n"     // Make R0 point to process stack pointer
        " b HardFault_C_Handler \n" // Off to C land
        );
    #endif
}

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void) {
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void) {
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1) {
        __fatal_error("MemManage");
    }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void) {
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1) {
        __fatal_error("BusFault");
    }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void) {
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1) {
        __fatal_error("UsageFault");
    }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void) {
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void) {
}


/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  These functions handle the EXTI interrupt requests.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void) {
    IRQ_ENTER(EXTI0_IRQn);
    Handle_EXTI_Irq(0);
    IRQ_EXIT(EXTI0_IRQn);
}

void EXTI1_IRQHandler(void) {
    IRQ_ENTER(EXTI1_IRQn);
    Handle_EXTI_Irq(1);
    IRQ_EXIT(EXTI1_IRQn);
}

void EXTI2_IRQHandler(void) {
    IRQ_ENTER(EXTI2_IRQn);
    Handle_EXTI_Irq(2);
    IRQ_EXIT(EXTI2_IRQn);
}

void EXTI3_IRQHandler(void) {
    IRQ_ENTER(EXTI3_IRQn);
    Handle_EXTI_Irq(3);
    IRQ_EXIT(EXTI3_IRQn);
}

void EXTI4_IRQHandler(void) {
    IRQ_ENTER(EXTI4_IRQn);
    Handle_EXTI_Irq(4);
    IRQ_EXIT(EXTI4_IRQn);
}

void EXTI9_5_IRQHandler(void) {
    IRQ_ENTER(EXTI9_5_IRQn);
    Handle_EXTI_Irq(5);
    Handle_EXTI_Irq(6);
    Handle_EXTI_Irq(7);
    Handle_EXTI_Irq(8);
    Handle_EXTI_Irq(9);
    IRQ_EXIT(EXTI9_5_IRQn);
}

void EXTI15_10_IRQHandler(void) {
    IRQ_ENTER(EXTI15_10_IRQn);
    Handle_EXTI_Irq(10);
    Handle_EXTI_Irq(11);
    Handle_EXTI_Irq(12);
    Handle_EXTI_Irq(13);
    Handle_EXTI_Irq(14);
    Handle_EXTI_Irq(15);
    IRQ_EXIT(EXTI15_10_IRQn);
}

void PVD_IRQHandler(void) {
    IRQ_ENTER(PVD_IRQn);
    Handle_EXTI_Irq(EXTI_PVD_OUTPUT);
    IRQ_EXIT(PVD_IRQn);
}

void RTC_Alarm_IRQHandler(void) {
    IRQ_ENTER(RTC_Alarm_IRQn);
    Handle_EXTI_Irq(EXTI_RTC_ALARM);
    IRQ_EXIT(RTC_Alarm_IRQn);
}
void WWDG_IRQHandler(void)
{
	IRQ_ENTER(WWDG_IRQn);
	IRQ_EXIT(WWDG_IRQn);
}
void TAMP_STAMP_IRQHandler(void) {
    IRQ_ENTER(TAMP_STAMP_IRQn);
    Handle_EXTI_Irq(EXTI_RTC_TIMESTAMP);
    IRQ_EXIT(TAMP_STAMP_IRQn);
}

void RTC_WKUP_IRQHandler(void) {
    //IRQ_ENTER(RTC_WKUP_IRQn);
    //RTC->ISR &= ~RTC_ISR_WUTF; // clear wakeup interrupt flag
   // //Handle_EXTI_Irq(EXTI_RTC_WAKEUP); // clear EXTI flag and execute optional callback
   // IRQ_EXIT(RTC_WKUP_IRQn);
}


void TIM1_BRK_IRQHandler(void) {
    IRQ_ENTER(TIM1_BRK_IRQn);
    timer_irq_handler(1);
    IRQ_EXIT(TIM1_BRK_IRQn);
}


void TIM1_UP_IRQHandler(void) {
    IRQ_ENTER(TIM1_UP_IRQn);
    timer_irq_handler(1);
    IRQ_EXIT(TIM1_UP_IRQn);
}

void TIM1_TRG_COM_IRQHandler(void) {
    IRQ_ENTER(TIM1_TRG_COM_IRQn);
    timer_irq_handler(1);
    IRQ_EXIT(TIM1_TRG_COM_IRQn);
}

void TIM1_CC_IRQHandler(void) {
    IRQ_ENTER(TIM1_CC_IRQn);
    timer_irq_handler(1);
    IRQ_EXIT(TIM1_CC_IRQn);
}

void TIM2_BRK_IRQHandler(void) {
    IRQ_ENTER(TIM2_BRK_IRQn);
    timer_irq_handler(2);
    IRQ_EXIT(TIM2_BRK_IRQn);
}


void TIM2_UP_IRQHandler(void) {
    IRQ_ENTER(TIM2_UP_IRQn);
    timer_irq_handler(2);
    IRQ_EXIT(TIM2_UP_IRQn);
}

void TIM2_TRG_COM_IRQHandler(void) {
    IRQ_ENTER(TIM2_TRG_COM_IRQn);
    timer_irq_handler(2);
    IRQ_EXIT(TIM2_TRG_COM_IRQn);
}

void TIM2_CC_IRQHandler(void) {
    IRQ_ENTER(TIM2_CC_IRQn);
    timer_irq_handler(2);
    IRQ_EXIT(TIM2_CC_IRQn);
}

void TIM3_IRQHandler(void) {
    IRQ_ENTER(TIM3_IRQn);
    timer_irq_handler(3);
    IRQ_EXIT(TIM3_IRQn);
}

void TIM4_IRQHandler(void) {
    IRQ_ENTER(TIM4_IRQn);
    timer_irq_handler(4);
    IRQ_EXIT(TIM4_IRQn);
}

void TIM5_IRQHandler(void) {
    IRQ_ENTER(TIM5_IRQn);
    timer_irq_handler(5);
    //HAL_TIM_IRQHandler(&TIM5_Handle);
    IRQ_EXIT(TIM5_IRQn);
}
void TIM6_IRQHandler(void) {
    IRQ_ENTER(TIM6_IRQn);
    timer_irq_handler(6);
    IRQ_EXIT(TIM6_IRQn);
}

void TIM7_IRQHandler(void) {
    IRQ_ENTER(TIM7_IRQn);
    timer_irq_handler(7);
    IRQ_EXIT(TIM7_IRQn);
}
#if 1
void TIM8_IRQHandler(void) {
    IRQ_ENTER(TIM8_IRQn);
    timer_irq_handler(8);
    IRQ_EXIT(TIM8_IRQn);
}
#endif
void TIM9_IRQHandler(void) {
    IRQ_ENTER(TIM9_IRQn);
    timer_irq_handler(9);
    IRQ_EXIT(TIM9_IRQn);
}
void TIM10_IRQHandler(void) {
    IRQ_ENTER(TIM10_IRQn);
    timer_irq_handler(10);
    IRQ_EXIT(TIM10_IRQn);
}


// UART/USART IRQ handlers
void UART1_IRQHandler(void) {
    IRQ_ENTER(USART1_IRQn);
    uart_irq_handler(1);
    IRQ_EXIT(USART1_IRQn);
}

void UART2_IRQHandler(void) {
    IRQ_ENTER(USART2_IRQn);
    uart_irq_handler(2);
    IRQ_EXIT(USART2_IRQn);
}

void UART3_IRQHandler(void) {
    IRQ_ENTER(USART3_IRQn);
    uart_irq_handler(3);
    IRQ_EXIT(USART3_IRQn);
}

void UART4_IRQHandler(void) {
    IRQ_ENTER(UART4_IRQn);
    uart_irq_handler(4);
    IRQ_EXIT(UART4_IRQn);
}

void UART5_IRQHandler(void) {
    IRQ_ENTER(UART5_IRQn);
    uart_irq_handler(5);
    IRQ_EXIT(UART5_IRQn);
}


#if MICROPY_PY_PYB_LEGACY

#if defined(MICROPY_HW_I2C1_SCL)
void I2C1_IRQHandler(void) {

}

#endif // defined(MICROPY_HW_I2C1_SCL)

#if defined(MICROPY_HW_I2C2_SCL)
void I2C2_IRQHandler(void) {

}

#endif // defined(MICROPY_HW_I2C2_SCL)

#if defined(MICROPY_HW_I2C3_SCL)
void I2C3_IRQHandler(void) {

}

#endif // defined(MICROPY_HW_I2C3_SCL)

#if defined(MICROPY_HW_I2C4_SCL)
void I2C4_IRQHandler(void) {

}
#endif // defined(MICROPY_HW_I2C4_SCL)

#endif // MICROPY_PY_PYB_LEGACY
