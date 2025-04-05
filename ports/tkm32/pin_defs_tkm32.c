#include "py/obj.h"
#include "pin.h"

// Returns the pin mode. This value returned by this macro should be one of:
// GPIO_MODE_INPUT, GPIO_MODE_OUTPUT_PP, GPIO_MODE_OUTPUT_OD,
// GPIO_MODE_AF_PP, GPIO_MODE_AF_OD, or GPIO_MODE_ANALOG.

uint32_t pin_get_mode(const pin_obj_t *pin) {

    GPIO_TypeDef *gpio = pin->gpio;

    uint32_t pin_num = pin->pin;  //GPIO_PIN_0-15
		uint32_t mode = 0;
		if(pin_num < 8){
			mode = (gpio->CRL >> ((pin_num & 7) * 4)) & 0x0000000FU;
		}else if (pin_num < 16){
			mode = (gpio->CRH >> ((pin_num & 7) * 4)) & 0x0000000FU;
		}else {  //GPIOE
			mode = (gpio->CRH_EXT >> ((pin_num & 7) * 4)) & 0x0000000FU;
		}
		uint32_t modeflag = 0;
		modeflag = mode & 0x00000003U;
		if(modeflag > 0) mode |= 0x00000003U;
		return mode;
}

// Returns the pin pullup/pulldown. The value returned by this macro should
// be one of GPIO_NOPULL, GPIO_PULLUP, or GPIO_PULLDOWN.

uint32_t pin_get_pull(const pin_obj_t *pin) {
    //return (pin->gpio->PUPDR >> (pin->pin * 2)) & 3;
		return 0;
}

// Returns the af (alternate function) index currently set for a pin.

uint32_t pin_get_af(const pin_obj_t *pin) {

		uint32_t pin_num = pin->pin;  //GPIO_PIN_0-15
		
		if(pin_num < 8){
			return (pin->gpio->AFRL >> ((pin->pin & 7) * 4)) & 0xf;
		}else if (pin_num < 16){
			return (pin->gpio->AFRH >> ((pin->pin & 7) * 4)) & 0xf;
			
		}else {  //GPIOE
			return (pin->gpio->AFRH_EXT >> ((pin->pin & 7) * 4)) & 0xf;
		}
		
}

