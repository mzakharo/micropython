#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "uart.h"
#include "usb.h"
#include "usb_cdc_port.h"

#include "led.h"
// this table converts from HAL_StatusTypeDef to POSIX errno
const byte mp_hal_status_to_errno_table[4] = {
    [HAL_OK] = 0,
    [HAL_ERROR] = MP_EIO,
    [HAL_BUSY] = MP_EBUSY,
    [HAL_TIMEOUT] = MP_ETIMEDOUT,
};

NORETURN void mp_hal_raise(HAL_StatusTypeDef status) {
    mp_raise_OSError(mp_hal_status_to_errno_table[status]);
}

MP_WEAK uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if (MP_STATE_PORT(pyb_stdio_uart) != NULL) {
        mp_obj_t pyb_stdio_uart = MP_OBJ_FROM_PTR(MP_STATE_PORT(pyb_stdio_uart));
        int errcode;
        const mp_stream_p_t *stream_p = mp_get_stream(pyb_stdio_uart);
        ret = stream_p->ioctl(pyb_stdio_uart, MP_STREAM_POLL, poll_flags, &errcode);
    }
    return ret | mp_uos_dupterm_poll(poll_flags);
}

MP_WEAK int mp_hal_stdin_rx_chr(void) {
	for (;;) {
		if (MP_STATE_PORT(pyb_stdio_uart) != NULL && uart_rx_any(MP_STATE_PORT(pyb_stdio_uart))) {
				return uart_rx_char(MP_STATE_PORT(pyb_stdio_uart));
				return 0;
		}
#if 0
		if(usb_cdc_connected()){
			uint8_t buf[1];
			int ret = usb_vcom_recv(buf, sizeof(buf));
			if(ret){
				return buf[0];
			}
		}
#else
		int dupterm_c = mp_uos_dupterm_rx_chr();
		if (dupterm_c >= 0) {
				return dupterm_c;
		}
#endif
		MICROPY_EVENT_POLL_HOOK
	}
}

// void mp_hal_stdout_tx_str(const char *str) {
    // mp_hal_stdout_tx_strn(str, strlen(str));
// }

MP_WEAK void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (MP_STATE_PORT(pyb_stdio_uart) != NULL) {
      uart_tx_strn(MP_STATE_PORT(pyb_stdio_uart), str, len);
    }
    #if 0 && defined(USE_HOST_MODE) && MICROPY_HW_HAS_LCD
    lcd_print_strn(str, len);
    #endif
		if(usb_cdc_connected()){
			UsbVcomSend((uint8_t *)str, len);
		}
    //mp_uos_dupterm_tx_strn(str, len);
}

// Efficiently convert "\n" to "\r\n"
// void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // const char *last = str;
    // while (len--) {
        // if (*str == '\n') {
            // if (str > last) {
                // mp_hal_stdout_tx_strn(last, str - last);
            // }
            // mp_hal_stdout_tx_strn("\r\n", 2);
            // ++str;
            // last = str;
        // } else {
            // ++str;
        // }
    // }
    // if (str > last) {
        // mp_hal_stdout_tx_strn(last, str - last);
    // }
// }

#if __CORTEX_M >= 0x03
#if 0
void mp_hal_ticks_cpu_enable(void) {
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}
#endif
#endif

void mp_hal_gpio_clock_enable(GPIO_TypeDef *gpio) {
    uint32_t gpio_idx = ((uint32_t)gpio - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE);
    RCC->AHB1ENR |= ((uint32_t)0x1U << gpio_idx);
}

uint32_t mp_hal_get_gpio_indx(GPIO_TypeDef *gpio) {
  return ((uint32_t)gpio - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE);
}

/*
mp_hal_pin_config(scl, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_NONE, af->idx);
*/
void mp_hal_pin_config(mp_hal_pin_obj_t pin_obj, uint32_t mode, uint32_t pull, uint32_t alt) {
	
    GPIO_TypeDef *gpio = pin_obj->gpio;
    uint32_t pin = pin_obj->pin;  //GPIO_PIN_0-15
    mp_hal_gpio_clock_enable(gpio);
		
		switch(pull)
		{
			case MP_HAL_PIN_PULL_UP:
			gpio->BSRR = (((uint32_t)0x01) << (pin & 0xFFFF));
			break;
			case MP_HAL_PIN_PULL_DOWN:
			gpio->BRR = (((uint32_t)0x01) << (pin & 0xFFFF));
			break;				
			default:
			break;
		}

		if(pin < 8){
			gpio->CRL = (gpio->CRL & ~(0x0FU << (4 * pin))) | ((mode & 0x0FU) << (4 * pin));
			if(alt == 0){ //gpio
				//gpio->AFRL = (gpio->AFRL & ~(0x0FU << (4 * (pin & 7)))) | (0x0B << (4 * (pin & 7)));
			}
			else{
				gpio->AFRL = (gpio->AFRL & ~(0x0FU << (4 * (pin & 7)))) | (alt << (4 * (pin & 7)));
			}

		}else if (pin < 16){
			gpio->CRH = 	(gpio->CRH & ~(0x0FU << (4 * (pin & 7)))) | ((mode & 0x0FU) << (4 * (pin & 7)));
			if(alt == 0){ //gpio
			//	gpio->AFRH = (gpio->AFRH & ~(0x0FU << (4 * (pin & 7)))) | (0x0B << (4 * (pin & 7)));
			}
			else{
				gpio->AFRH = (gpio->AFRH & ~(0x0FU << (4 * (pin & 7)))) | (alt<< (4 * (pin & 7)));
			}
	
		}else {  //GPIOE
			gpio->CRH_EXT = (gpio->CRH_EXT & ~(0x0FU << (4 * (pin & 7)))) | ((mode & 7) << (4 * (pin & 7)));
			gpio->AFRH_EXT = (gpio->AFRH_EXT & ~(0x0FU << (4 * (pin & 7)))) | (alt << (4 * (pin & 7)));

		}
}
//mp_hal_pin_config_alt(pins[i], mode, pull, AF_FN_UART, uart_unit);
bool mp_hal_pin_config_alt(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint8_t fn, uint8_t unit) {
    const pin_af_obj_t *af = pin_find_af(pin, fn, unit);
    if (af == NULL) {
        return false;
    }
    mp_hal_pin_config(pin, mode, pull, af->idx);
    return true;
}

void mp_hal_pin_config_speed(mp_hal_pin_obj_t pin_obj, uint32_t speed) {
	
	#if 0
    GPIO_TypeDef *gpio = pin_obj->gpio;
    uint32_t pin = pin_obj->pin;
    gpio->OSPEEDR = (gpio->OSPEEDR & ~(3 << (2 * pin))) | (speed << (2 * pin));
	#endif
}

/*******************************************************************************/
// MAC address

// Generate a random locally administered MAC address (LAA)
void mp_hal_generate_laa_mac(int idx, uint8_t buf[6]) {
    uint8_t *id = (uint8_t *)MP_HAL_UNIQUE_ID_ADDRESS;
    buf[0] = 0x02; // LAA range
    buf[1] = (id[11] << 4) | (id[10] & 0xf);
    buf[2] = (id[9] << 4) | (id[8] & 0xf);
    buf[3] = (id[7] << 4) | (id[6] & 0xf);
    buf[4] = id[2];
    buf[5] = (id[0] << 2) | idx;
}

// A board can override this if needed
MP_WEAK void mp_hal_get_mac(int idx, uint8_t buf[6]) {
    mp_hal_generate_laa_mac(idx, buf);
}

void mp_hal_get_mac_ascii(int idx, size_t chr_off, size_t chr_len, char *dest) {
    static const char hexchr[16] = "0123456789ABCDEF";
    uint8_t mac[6];
    mp_hal_get_mac(idx, mac);
    for (; chr_len; ++chr_off, --chr_len) {
        *dest++ = hexchr[mac[chr_off >> 1] >> (4 * (1 - (chr_off & 1))) & 0xf];
    }
}
