// We use the ST Cube HAL library for most hardware peripherals
#include TKM32_HAL_H
#include "pin.h"

extern const unsigned char mp_hal_status_to_errno_table[4];

static inline int mp_hal_status_to_neg_errno(HAL_StatusTypeDef status) {
    return -mp_hal_status_to_errno_table[status];
}

NORETURN void mp_hal_raise(HAL_StatusTypeDef status);
void mp_hal_set_interrupt_char(int c); // -1 to disable

// timing functions

#include "irq.h"

#if __CORTEX_M == 0
// Don't have raise_irq_pri on Cortex-M0 so keep IRQs enabled to have SysTick timing
#define mp_hal_quiet_timing_enter() (1)
#define mp_hal_quiet_timing_exit(irq_state) (void)(irq_state)
#else
#define mp_hal_quiet_timing_enter() raise_irq_pri(1)
#define mp_hal_quiet_timing_exit(irq_state) restore_irq_pri(irq_state)
#endif
#define mp_hal_delay_us_fast(us) mp_hal_delay_us(us)

void mp_hal_ticks_cpu_enable(void);
static inline mp_uint_t mp_hal_ticks_cpu(void) {
    //#if __CORTEX_M == 0
		#if 1
    return 0;
    #else
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        mp_hal_ticks_cpu_enable();
    }
    return DWT->CYCCNT;
    #endif
}

// C-level pin HAL

#include "pin.h"

#define MP_HAL_PIN_FMT                  "%q"

#define MP_HAL_PIN_MODE_ANALOG					(0x00)	//模拟输入
#define MP_HAL_PIN_MODE_IN_FLOATING			(0x04)	//浮空输入输入
#define MP_HAL_PIN_MODE_IPD_UP					(0x08)  //上下拉输入
#define MP_HAL_PIN_MODE_OUTPUT          (0x03)	//通用输出模式
#define MP_HAL_PIN_MODE_ALT_OUTPUT      (0x0B)  //复用输出模式
#define MP_HAL_PIN_MODE_OPEN      			(0x0F)  //复用开漏
//#define MP_HAL_PIN_MODE_ALT							(1) //配置复用模式

#define MP_HAL_PIN_MODE_ADC             (0x00)

#define MP_HAL_PIN_MODE_OPEN_DRAIN      (0x07)
#define MP_HAL_PIN_MODE_ALT_OPEN_DRAIN  (0x04)

#define MP_HAL_PIN_PULL_NONE            (0)
#define MP_HAL_PIN_PULL_UP              (1)
#define MP_HAL_PIN_PULL_DOWN            (2)



#define mp_hal_pin_obj_t const pin_obj_t *
#define mp_hal_get_pin_obj(o)   pin_find(o)
#define mp_hal_pin_name(p)      ((p)->name)

#define mp_hal_pin_input(p)     mp_hal_pin_config((p), MP_HAL_PIN_MODE_IN_FLOATING, MP_HAL_PIN_PULL_NONE, 0)
#define mp_hal_pin_output(p)    mp_hal_pin_config((p), MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_NONE, 0)

#define mp_hal_pin_open_drain(p) mp_hal_pin_config((p), MP_HAL_PIN_MODE_OPEN_DRAIN, MP_HAL_PIN_PULL_NONE, 0)


#define mp_hal_pin_high(p)      (((p)->gpio->BSRR) = (p)->pin_mask)
#define mp_hal_pin_low(p)       (((p)->gpio->BRR) = (p)->pin_mask)
#define mp_hal_pin_od_low(p)    mp_hal_pin_low(p)
#define mp_hal_pin_od_high(p)   mp_hal_pin_high(p)
#define mp_hal_pin_read(p)      (((p)->gpio->IDR >> (p)->pin) & 0x1U)
#define mp_hal_pin_write(p, v)  ((v) ? mp_hal_pin_high(p) : mp_hal_pin_low(p))

void mp_hal_gpio_clock_enable(GPIO_TypeDef *gpio);
void mp_hal_pin_config(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt);
bool mp_hal_pin_config_alt(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint8_t fn, uint8_t unit);
void mp_hal_pin_config_speed(mp_hal_pin_obj_t pin_obj, uint32_t speed);
uint32_t mp_hal_get_gpio_indx(GPIO_TypeDef *gpio);
enum {
    MP_HAL_MAC_WLAN0 = 0,
    MP_HAL_MAC_WLAN1,
    MP_HAL_MAC_BDADDR,
    MP_HAL_MAC_ETH0,
};

void mp_hal_generate_laa_mac(int idx, uint8_t buf[6]);
void mp_hal_get_mac(int idx, uint8_t buf[6]);
void mp_hal_get_mac_ascii(int idx, size_t chr_off, size_t chr_len, char *dest);
