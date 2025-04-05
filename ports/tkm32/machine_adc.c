

#include "py/runtime.h"
#include "py/mphal.h"
#include "stdio.h"

// Timeout for waiting for end-of-conversion
#define ADC_TIMEOUT_MS (50)

	STATIC void adc_wait_eoc(ADC_TypeDef *adc,uint8_t channel, int32_t timeout_ms) {
    uint32_t t0 = mp_hal_ticks_ms();
    while (!(adc->ADSTA & 0x1U))
    {
        if (mp_hal_ticks_ms() - t0 > timeout_ms) {
            break; // timeout
        }
    }
		adc->ADSTA |= 0x01;
}

STATIC uint32_t adc_config_and_read_u16(ADC_TypeDef *adc, uint8_t channel, uint32_t sample_time) {

		adc_wait_eoc(adc,channel,ADC_TIMEOUT_MS);
    uint32_t raw = (adc->ADDR[channel] & 0xFFFF);
    return raw;
}

/******************************************************************************/
// MicroPython bindings for machine.ADC

const mp_obj_type_t machine_adc_type;

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    ADC_TypeDef *adc;
    uint8_t channel;
    uint32_t sample_time;
} machine_adc_obj_t;

STATIC void machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<ADC channel=%u>", self->channel);
}

STATIC uint8_t adc_is_int = 0;
// ADC(id)
STATIC mp_obj_t machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Check number of arguments
	mp_arg_check_num(n_args, n_kw, 1, 1, false);
	mp_obj_t source = all_args[0];
	uint8_t channel;

	if(mp_obj_is_int(source)){
		channel = mp_obj_get_int(source);
	}else{
		const pin_obj_t *pin = pin_find(source);
		if(pin->adc_num != 3){
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Pin(%q) does not have ADC capabilities"), pin->name);
		}
		channel = pin->adc_channel;
	}
	if(channel > 5){
		mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("ADC Only support channel 0 ~ 5"));
	}
	ADC2->ADCFG &= ~0x1;
	ADC2->ADCR &= ~(0x1<<8);
	if(!adc_is_int)
	{
		RCC->APB2ENR |= 0x1<<25;
		SYSCFG->CFGR &= ~(0x01<<16);
		
		ADC2->ADCFG = 0x70;
		ADC2->ADCR = 0x400;
		ADC2->ADCR |= (0x01<<3); //DMA

		ADC2->ADCHS = 0x0U;
		
		adc_is_int = 1;
	}
	
	ADC2->ADCHS |= (0x01U<<channel);
	
	ADC2->ADCFG |= 0x1; 
	
	ADC2->ADCR |= 0x1<<8;

	machine_adc_obj_t *o = m_new_obj(machine_adc_obj_t);
	o->base.type = &machine_adc_type;
	o->adc = ADC2;
	o->channel = channel;
	o->sample_time = 1;

	return MP_OBJ_FROM_PTR(o);
}

// read_u16()
STATIC mp_obj_t machine_adc_read_u16(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(adc_config_and_read_u16(self->adc, self->channel, self->sample_time));
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_u16_obj, machine_adc_read_u16);

STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_u16), MP_ROM_PTR(&machine_adc_read_u16_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

const mp_obj_type_t machine_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = machine_adc_print,
    .make_new = machine_adc_make_new,
    .locals_dict = (mp_obj_dict_t *)&machine_adc_locals_dict,
};
