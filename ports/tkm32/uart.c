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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "shared/runtime/interrupt_char.h"
#include "shared/runtime/mpirq.h"
#include "uart.h"
#include "irq.h"
#include "pendsv.h"
#include "systick.h"

#define UART_RXNE_IS_SET(uart) ((uart)->CSR & UART_FLAG_RXAVL) //接受数据不为空 

#define UART_RXNE_IT_EN(uart) do { (uart)->IER |= UART_IT_RXIEN; } while (0)  //接收中断使能
#define UART_RXNE_IT_DIS(uart) do { (uart)->IER &= ~UART_IT_RXIEN; } while (0)

#define USART_IE_ALL (UART_IT_RXBRKEN | UART_IT_ERR | UART_IT_PE | UART_OVER_ERR | UART_TIMEOUT_ERR | UART_IT_RXIEN | UART_IT_TXIEN)


#define GCR_UE_BIT                ((uint16_t)0x0001)  /* UART Enable Mask */

extern void NORETURN __fatal_error(const char *msg);
STATIC bool uart_wait_flag_set(pyb_uart_obj_t *self, uint32_t flag, uint32_t timeout);
typedef struct _pyb_uart_irq_map_t {
    uint16_t irq_en;
    uint16_t flag;
} pyb_uart_irq_map_t;

STATIC const pyb_uart_irq_map_t mp_uart_irq_map[] = {
		{ UART_IT_RXBRKEN, 	UART_FLAG_RXBRK}, // 接收断开帧中断使能位
		{ UART_IT_ERR,   		UART_FLAG_RXFERR},   // 帧错误中断
		{ UART_IT_PE,  			UART_FLAG_RXPERR},  // 奇偶校验错误中断
		{ UART_OVER_ERR,   	UART_FLAG_RXOERR},   // 接收溢出错误中断
		{ UART_IT_RXIEN,  	UART_FLAG_RXE},  // 接收缓冲中断 
		{ UART_IT_TXIEN,   	UART_FLAG_TXE},   // 发送缓冲空中断
};

// unregister all interrupt sources
void uart_deinit_all(void) {
    for (int i = 0; i < MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all)); i++) {
        pyb_uart_obj_t *uart_obj = MP_STATE_PORT(pyb_uart_obj_all)[i];
        if (uart_obj != NULL && !uart_obj->is_static) {
            uart_deinit(uart_obj);
            MP_STATE_PORT(pyb_uart_obj_all)[i] = NULL;
        }
    }
}

bool uart_exists(int uart_id) {
    if (uart_id > MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all))) {
        // safeguard against pyb_uart_obj_all array being configured too small
        return false;
    }
    switch (uart_id) {
        #if defined(MICROPY_HW_UART1_TX) && defined(MICROPY_HW_UART1_RX)
        case PYB_UART_1:
            return true;
        #endif

        #if defined(MICROPY_HW_UART2_TX) && defined(MICROPY_HW_UART2_RX)
        case PYB_UART_2:
            return true;
        #endif

        #if defined(MICROPY_HW_UART3_TX) && defined(MICROPY_HW_UART3_RX)
        case PYB_UART_3:
            return true;
        #endif

        #if defined(MICROPY_HW_UART4_TX) && defined(MICROPY_HW_UART4_RX)
        case PYB_UART_4:
            return true;
        #endif

        #if defined(MICROPY_HW_UART5_TX) && defined(MICROPY_HW_UART5_RX)
        case PYB_UART_5:
            return true;
        #endif
        default:
            return false;
    }
}

// assumes Init parameters have been set up correctly
bool uart_init(pyb_uart_obj_t *uart_obj,
    uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop, uint32_t flow) {
    UART_TypeDef *UARTx;
    IRQn_Type irqn;
    int uart_unit;

    const pin_obj_t *pins[4] = {0};

    switch (uart_obj->uart_id) {
        #if defined(MICROPY_HW_UART1_TX) && defined(MICROPY_HW_UART1_RX)
        case PYB_UART_1:
            uart_unit = 8;
            UARTx = UART1;
            irqn = UART1_IRQn;
            pins[0] = MICROPY_HW_UART1_TX;
            pins[1] = MICROPY_HW_UART1_RX;
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART1, ENABLE);
            break;
        #endif

        #if defined(MICROPY_HW_UART2_TX) && defined(MICROPY_HW_UART2_RX)
        case PYB_UART_2:
            uart_unit = 7;
            UARTx = UART2;
            irqn = UART2_IRQn;
            pins[0] = MICROPY_HW_UART2_TX;
            pins[1] = MICROPY_HW_UART2_RX;
            #if defined(MICROPY_HW_UART2_RTS)
            if (flow & UART_HWCONTROL_RTS) {
                pins[2] = MICROPY_HW_UART2_RTS;
            }
            #endif
            #if defined(MICROPY_HW_UART2_CTS)
            if (flow & UART_HWCONTROL_CTS) {
                pins[3] = MICROPY_HW_UART2_CTS;
            }
            #endif
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART2, ENABLE);
            break;
        #endif

        #if defined(MICROPY_HW_UART3_TX) && defined(MICROPY_HW_UART3_RX)
        case PYB_UART_3:
            uart_unit = 7;
            UARTx = UART3;
            irqn = UART3_IRQn;
            pins[0] = MICROPY_HW_UART3_TX;
            pins[1] = MICROPY_HW_UART3_RX;
            #if defined(MICROPY_HW_UART3_RTS)
            if (flow & UART_HWCONTROL_RTS) {
                pins[2] = MICROPY_HW_UART3_RTS;
            }
            #endif
            #if defined(MICROPY_HW_UART3_CTS)
            if (flow & UART_HWCONTROL_CTS) {
                pins[3] = MICROPY_HW_UART3_CTS;
            }
            #endif
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART3, ENABLE);
            break;
        #endif

        #if defined(MICROPY_HW_UART4_TX) && defined(MICROPY_HW_UART4_RX)
        case PYB_UART_4:
            uart_unit = 7;
            UARTx = UART4;
            irqn = UART4_IRQn;
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART4, ENABLE);
            pins[0] = MICROPY_HW_UART4_TX;
            pins[1] = MICROPY_HW_UART4_RX;
            #if defined(MICROPY_HW_UART4_RTS)
            if (flow & UART_HWCONTROL_RTS) {
                pins[2] = MICROPY_HW_UART4_RTS;
            }
            #endif
            #if defined(MICROPY_HW_UART4_CTS)
            if (flow & UART_HWCONTROL_CTS) {
                pins[3] = MICROPY_HW_UART4_CTS;
            }
            #endif
            break;
        #endif

        #if defined(MICROPY_HW_UART5_TX) && defined(MICROPY_HW_UART5_RX)
        case PYB_UART_5:
            uart_unit = 7;
            UARTx = UART5;
            irqn = UART5_IRQn;
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART5, ENABLE);
            pins[0] = MICROPY_HW_UART5_TX;
            pins[1] = MICROPY_HW_UART5_RX;
            break;
        #endif

        default:
            // UART does not exist or is not configured for this board
            return false;
    }

	uart_obj->uartx = UARTx;

	mp_hal_pin_config(pins[0], MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, uart_unit);
	mp_hal_pin_config(pins[1], MP_HAL_PIN_MODE_IPD_UP,MP_HAL_PIN_PULL_NONE , uart_unit);

	SYSCLK_INFO Clock;
	GetSysClockInfo(&Clock);

	uint32_t tmpreg = 0x30;//默认支支持8bit数据
	uart_obj->uartx->BRR = (Clock.PCLK2Clock/(baudrate << 4));  //设置波特率
	uart_obj->uartx->GCR = 0x0U;

	tmpreg |= stop;
	tmpreg |= parity;
	uart_obj->uartx->CCR |= tmpreg;

	tmpreg = 0x1B;//默认正常发送接收使能，使能模块
	tmpreg |= flow;
	uart_obj->uartx->GCR = tmpreg;

	uart_obj->uartx->ICR = 0xFFU;

	uart_obj->uartx->IER = 0x00U; //关闭所有中断
	uart_obj->uartx->CSR = 0x09U;

	NVIC_SetPriority(IRQn_NONNEG(irqn), IRQ_PRI_UART);
	NVIC_EnableIRQ(irqn);

	uart_obj->is_enabled = true;
	uart_obj->attached_to_repl = false;

	uart_obj->char_mask = 0x7f;
	uart_obj->char_width = CHAR_WIDTH_8BIT;

	uart_obj->mp_irq_trigger = 0;
	uart_obj->mp_irq_obj = NULL;


	uint32_t rx_reg = UARTx->RDR; //清除接收超时中断
	(void)rx_reg;
	uart_obj->uartx->ICR = 0x0U;  //清除所有中断标志

	uart_obj->uartx->CSR = 0x09;


int errcode;
uart_tx_data(uart_obj, "OK\r\n", 4, &errcode);

	return true;
}

void uart_irq_config(pyb_uart_obj_t *self, bool enable) {
    if (self->mp_irq_trigger) {
        for (size_t entry = 0; entry < MP_ARRAY_SIZE(mp_uart_irq_map); ++entry) {
            if (mp_uart_irq_map[entry].flag & MP_UART_RESERVED_FLAGS) {
                continue;
            }
            if (mp_uart_irq_map[entry].flag & self->mp_irq_trigger) {
                if (enable) {
                    self->uartx->IER |= mp_uart_irq_map[entry].irq_en;
                } else {
                    self->uartx->IER &= ~mp_uart_irq_map[entry].irq_en;
                }
            }
        }
    }
}

void uart_set_rxbuf(pyb_uart_obj_t *self, size_t len, void *buf) {
    self->read_buf_head = 0;
    self->read_buf_tail = 0;
    self->read_buf_len = len;
    self->read_buf = buf;
    if (len == 0) {
        UART_RXNE_IT_DIS(self->uartx);
    } else {
        UART_RXNE_IT_EN(self->uartx);
    }
}

void uart_deinit(pyb_uart_obj_t *self) {
    self->is_enabled = false;

    // Disable UART
		self->uartx->GCR &= ~GCR_UE_BIT;

    // Reset and turn off the UART peripheral
    if (self->uart_id == 1) {
        NVIC_DisableIRQ(UART1_IRQn);
				RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART1,DISABLE);
    #if defined(USART2)
    } else if (self->uart_id == 2) {
				NVIC_DisableIRQ(UART2_IRQn);
    #endif
    #if defined(USART3)
    } else if (self->uart_id == 3) {
				NVIC_DisableIRQ(UART3_IRQn);
    #endif
    #if defined(UART4)
    } else if (self->uart_id == 4) {
				NVIC_DisableIRQ(UART4_IRQn);
    #endif
    #if defined(UART5)
    } else if (self->uart_id == 5) {
				NVIC_DisableIRQ(UART1_IRQn);
    #endif
    }
		RCC->APB2ENR &= ~((uint32_t)(1<<(self->uart_id + 1))); //关闭对应串口时钟
}

void uart_attach_to_repl(pyb_uart_obj_t *self, bool attached) {
    self->attached_to_repl = attached;
}

uint32_t uart_get_baudrate(pyb_uart_obj_t *self) {

	SYSCLK_INFO Clock;
	GetSysClockInfo(&Clock);

	return (Clock.PCLK2Clock/(self->uartx->BRR << 4));
}

mp_uint_t uart_rx_any(pyb_uart_obj_t *self) {
    int buffer_bytes = self->read_buf_head - self->read_buf_tail;
    if (buffer_bytes < 0) {
        return buffer_bytes + self->read_buf_len;
    } else if (buffer_bytes > 0) {
        return buffer_bytes;
    } else {
        return UART_RXNE_IS_SET(self->uartx) != 0;
    }
}

// Waits at most timeout milliseconds for at least 1 char to become ready for
// reading (from buf or for direct reading).
// Returns true if something available, false if not.
bool uart_rx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    for (;;) {
        if (self->read_buf_tail != self->read_buf_head || UART_RXNE_IS_SET(self->uartx)) {
            return true; // have at least 1 char ready for reading
        }
        if (HAL_GetTick() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

// assumes there is a character available
int uart_rx_char(pyb_uart_obj_t *self) {
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via IRQ
        int data;
        if (self->char_width == CHAR_WIDTH_9BIT) {
            data = ((uint16_t *)self->read_buf)[self->read_buf_tail];
        } else {
            data = self->read_buf[self->read_buf_tail];
        }
        self->read_buf_tail = (self->read_buf_tail + 1) % self->read_buf_len;
        if (UART_RXNE_IS_SET(self->uartx)) {
            // UART was stalled by flow ctrl: re-enable IRQ now we have room in buffer
            UART_RXNE_IT_EN(self->uartx);
        }
        return data;
    } else {
        // no buffering
				while((self->uartx->CSR &0x1) == 0);
        return self->uartx->RDR & self->char_mask;
    }
}

// Waits at most timeout milliseconds for TX register to become empty.
// Returns true if can write, false if can't.
bool uart_tx_wait(pyb_uart_obj_t *self, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    for (;;) {
        if (uart_tx_avail(self)) {
            return true; // tx register is empty
        }
        if (HAL_GetTick() - start >= timeout) {
            return false; // timeout
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

// Waits at most timeout milliseconds for UART flag to be set.
// Returns true if flag is/was set, false on timeout.
STATIC bool uart_wait_flag_set(pyb_uart_obj_t *self, uint32_t flag, uint32_t timeout) {
    // Note: we don't use WFI to idle in this loop because UART tx doesn't generate
    // an interrupt and the flag can be set quickly if the baudrate is large.
    uint32_t start = HAL_GetTick();
    for (;;) {
        if (self->uartx->CSR & flag) {
            return true;
        }
        if (timeout == 0 || HAL_GetTick() - start >= timeout) {
            return false; // timeout
        }
    }
}

// src - a pointer to the data to send (16-bit aligned for 9-bit chars)
// num_chars - number of characters to send (9-bit chars count for 2 bytes from src)
// *errcode - returns 0 for success, MP_Exxx on error
// returns the number of characters sent (valid even if there was an error)
size_t uart_tx_data(pyb_uart_obj_t *self, const void *src_in, size_t num_chars, int *errcode) {
    if (num_chars == 0) {
        *errcode = 0;
        return 0;
    }

    uint32_t timeout;
		#if 0
   if (self->uartx->CR3 & USART_CR3_CTSE) {  //使能流控
        // CTS can hold off transmission for an arbitrarily long time. Apply
        // the overall timeout rather than the character timeout.
      //  timeout = self->timeout;
    } else 
		#endif
		{
        // The timeout specified here is for waiting for the TX data register to
        // become empty (ie between chars), as well as for the final char to be
        // completely transferred.  The default value for timeout_char is long
        // enough for 1 char, but we need to double it to wait for the last char
        // to be transferred to the data register, and then to be transmitted.
        timeout = 2 * self->timeout_char;
    }

    const uint8_t *src = (const uint8_t *)src_in;
    size_t num_tx = 0;
    UART_TypeDef *uart = self->uartx;

    while (num_tx < num_chars) {
        if (!uart_wait_flag_set(self, UART_FLAG_TXE, timeout)) {
            *errcode = MP_ETIMEDOUT;
            return num_tx;
        }
        uint32_t data;
        if (self->char_width == CHAR_WIDTH_9BIT) {
            data = *((uint16_t *)src) & 0x1ff;
            src += 2;
        } else {
            data = *src++;
        }
        uart->TDR = data;
        ++num_tx;
    }

    // wait for the UART frame to complete
    if (!uart_wait_flag_set(self, UART_FLAG_TXE, timeout)) {
        *errcode = MP_ETIMEDOUT;
        return num_tx;
    }

    *errcode = 0;
    return num_tx;
}

void uart_tx_strn(pyb_uart_obj_t *uart_obj, const char *str, uint len) {
    int errcode;
    uart_tx_data(uart_obj, str, len, &errcode);
}

// this IRQ handler is set up to handle RXNE interrupts only
void uart_irq_handler(mp_uint_t uart_id) {
    // get the uart object
    pyb_uart_obj_t *self = MP_STATE_PORT(pyb_uart_obj_all)[uart_id - 1];
		
    if (self == NULL) {
        // UART object has not been set, so we can't do anything, not
        // even disable the IRQ.  This should never happen.
        return;
    }
    if (UART_RXNE_IS_SET(self->uartx)) {
        if (self->read_buf_len != 0) {
            uint16_t next_head = (self->read_buf_head + 1) % self->read_buf_len;
            if (next_head != self->read_buf_tail) {
                int data = self->uartx->RDR; // clears UART_FLAG_RXNE
								self->uartx->ICR |= 1<<1; 
                data &= self->char_mask;
                if (self->attached_to_repl && data == mp_interrupt_char) {
                    // Handle interrupt coming in on a UART REPL
                    pendsv_kbd_intr();
                } else 
								{
                    if (self->char_width == CHAR_WIDTH_9BIT) {
                        ((uint16_t *)self->read_buf)[self->read_buf_head] = data;
                    } else {
                        self->read_buf[self->read_buf_head] = data;
                    }
                    self->read_buf_head = next_head;
                }
            } else { // No room: leave char in buf, disable interrupt
                UART_RXNE_IT_DIS(self->uartx);
            }
        }
    }
    // If RXNE is clear but ORE set then clear the ORE flag (it's tied to RXNE IRQ)
    else if (self->uartx->ISR & UART_FLAG_RXOERR) {  //判断超时错误，溢出错误
        //(void)self->uartx->RDR;
    }
    // Set user IRQ flags
    self->mp_irq_flags = 0;
    if (self->uartx->ISR & MP_UART_ALLOWED_FLAGS) {  //判断空闲中断
        self->mp_irq_flags |= MP_UART_ALLOWED_FLAGS;
    }
		
    // Check the flags to see if the user handler should be called
    if (self->mp_irq_trigger & self->mp_irq_flags) {
        mp_irq_handler(self->mp_irq_obj);
    }

		self->uartx->ICR |= 1<<1;  //清除接受中断
}

STATIC mp_uint_t uart_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uart_irq_config(self, false);
    self->mp_irq_trigger = new_trigger;
    uart_irq_config(self, true);
    return 0;
}

STATIC mp_uint_t uart_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (info_type == MP_IRQ_INFO_FLAGS) {
        return self->mp_irq_flags;
    } else if (info_type == MP_IRQ_INFO_TRIGGERS) {
        return self->mp_irq_trigger;
    }
    return 0;
}

const mp_irq_methods_t uart_irq_methods = {
    .trigger = uart_irq_trigger,
    .info = uart_irq_info,
};
