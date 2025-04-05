/*
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	spi.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/16
	* Description 			 :	
	******************************************************************************
**/
#include <stdio.h>
#include <string.h>
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "spi.h"
#include "systick.h"
const spi_t spi_obj[4] = {
    #if defined(MICROPY_HW_SPI1_SCK)
    {SPI1},
    #else
    {NULL},
    #endif
    #if defined(MICROPY_HW_SPI2_SCK)
    {SPI2},
    #else
    {NULL},
    #endif
    #if defined(MICROPY_HW_SPI3_SCK)
    {SPI3},
    #else
    {NULL},
    #endif
    #if defined(MICROPY_HW_SPI4_SCK)
    {SPI4},
    #else
    {NULL},
    #endif
};


int spi_find_index(mp_obj_t id) {
	if (mp_obj_is_str(id)) {
			// given a string id
			const char *port = mp_obj_str_get_str(id);
			if (0) {
			#ifdef MICROPY_HW_SPI1_NAME
			} else if (strcmp(port, MICROPY_HW_SPI1_NAME) == 0) {
					return 1;
			#endif
			#ifdef MICROPY_HW_SPI2_NAME
			} else if (strcmp(port, MICROPY_HW_SPI2_NAME) == 0) {
					return 2;
			#endif
			#ifdef MICROPY_HW_SPI3_NAME
			} else if (strcmp(port, MICROPY_HW_SPI3_NAME) == 0) {
					return 3;
			#endif
			#ifdef MICROPY_HW_SPI4_NAME
			} else if (strcmp(port, MICROPY_HW_SPI4_NAME) == 0) {
					return 4;
			#endif
			}
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%s) doesn't exist"), port);
	} else {
			// given an integer id
			int spi_id = mp_obj_get_int(id);
			if (spi_id >= 1 && spi_id <= MP_ARRAY_SIZE(spi_obj)
					&& spi_obj[spi_id - 1].spi != NULL) {
					return spi_id;
			}
			mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), spi_id);
	}
}
STATIC uint32_t spi_get_source_freq(void) {
	SYSCLK_INFO clock;
	GetSysClockInfo(&clock);
	uint32_t pclk_clk = clock.PCLK2Clock;
	return pclk_clk ;
}

// sets the parameters in the SPI_InitTypeDef struct
// if an argument is -1 then the corresponding parameter is not changed
void spi_set_params(SPI_TypeDef *self, uint32_t prescale, int32_t baudrate,
	int32_t polarity, int32_t phase, int32_t bits, int32_t firstbit) {

	if (prescale != 0xffffffff || baudrate != -1) {
			if (prescale == 0xffffffff) {
					// prescaler not given, so select one that yields at most the requested baudrate
					prescale = (spi_get_source_freq() + baudrate - 1) / baudrate;
			}
			if (prescale <= 2) {
					self->SPBRG = 0x2U;
			}else {
					self->SPBRG = prescale;
			}
	}
	
	uint32_t reg_temp = self->CCTL;
//空闲电平 if (polarity != -1)
	if(polarity==0){
		reg_temp &= ~0x02U;
	}else if(polarity > 0){
		reg_temp |= 0x02U;
	}
	if(phase==0){
		reg_temp |= 0x01U;
	}else if(phase > 0){
		reg_temp &= ~0x01U;
	}
	//只支持7bit 和8bit数据
	if(bits != -1){
		if(bits == 8){
			reg_temp |= 0x08U;
		}else{
			reg_temp &= ~0x08U;
		}
	}
	if(firstbit != -1){
		if(firstbit == 1){
			reg_temp &= ~0x04U;
		}else{
			reg_temp |= 0x04U;
		}
	}
	self->CCTL = reg_temp;
}

// TODO allow to take a list of pins to use
void spi_init(SPI_TypeDef *self, bool enable_nss_pin) {

	const pin_obj_t *pins[4] = { NULL, NULL, NULL, NULL };

	if (0) {
	#if defined(MICROPY_HW_SPI1_SCK)
	} else if (self == SPI1) {
			#if defined(MICROPY_HW_SPI1_NSS)
			pins[0] = MICROPY_HW_SPI1_NSS;
			#endif
			pins[1] = MICROPY_HW_SPI1_SCK;
			#if defined(MICROPY_HW_SPI1_MISO)
			pins[2] = MICROPY_HW_SPI1_MISO;
			#endif
			pins[3] = MICROPY_HW_SPI1_MOSI;
			// enable the SPI clock
			RCC->APB2ENR |= (1 << 20);
	#endif
	#if defined(MICROPY_HW_SPI2_SCK)
	} else if (self == SPI2) {
			#if defined(MICROPY_HW_SPI2_NSS)
			pins[0] = MICROPY_HW_SPI2_NSS;
			#endif
			pins[1] = MICROPY_HW_SPI2_SCK;
			#if defined(MICROPY_HW_SPI2_MISO)
			pins[2] = MICROPY_HW_SPI2_MISO;
			#endif
			pins[3] = MICROPY_HW_SPI2_MOSI;
			// enable the SPI clock
			RCC->APB2ENR |= (1 << 21);
	#endif
	#if defined(MICROPY_HW_SPI3_SCK)
	} else if (self == SPI3) {
			#if defined(MICROPY_HW_SPI3_NSS)
			pins[0] = MICROPY_HW_SPI3_NSS;
			#endif
			pins[1] = MICROPY_HW_SPI3_SCK;
			#if defined(MICROPY_HW_SPI3_MISO)
			pins[2] = MICROPY_HW_SPI3_MISO;
			#endif
			pins[3] = MICROPY_HW_SPI3_MOSI;
			// enable the SPI clock
			RCC->APB2ENR |= (1 << 22);
	#endif
	#if defined(MICROPY_HW_SPI4_SCK)
	} else if (self == SPI4) {
			#if defined(MICROPY_HW_SPI4_NSS)
			pins[0] = MICROPY_HW_SPI4_NSS;
			#endif
			pins[1] = MICROPY_HW_SPI4_SCK;
			#if defined(MICROPY_HW_SPI4_MISO)
			pins[2] = MICROPY_HW_SPI4_MISO;
			#endif
			pins[3] = MICROPY_HW_SPI4_MOSI;
			// enable the SPI clock
			RCC->APB2ENR |= (1 << 23);
	#endif
	} else {
			// SPI does not exist for this board (shouldn't get here, should be checked by caller)
			printf("SPI does not exist for this board\n");
			return;
	}
	// init the GPIO lines
	if (enable_nss_pin){
		mp_hal_pin_config(pins[0], MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);
	}
	//SCK
	mp_hal_pin_config(pins[1], MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 5);
	//MISO
	mp_hal_pin_config(pins[2], MP_HAL_PIN_MODE_IPD_UP, MP_HAL_PIN_PULL_UP, 5);
	//MOSI
	mp_hal_pin_config(pins[3], MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 5);
	
/* Clear csn_sel, dmamode, txtlf, rxtlf,data_sel, rxen, txen, mm, int_en, spien bits */
	self->GCTL &= 0xF000;
	self->GCTL |= 0x0CU;  //使能SPI主机发送
		
}

void spi_deinit(const spi_t *spi_obj) {
	SPI_TypeDef *spi = spi_obj->spi;
	if (0) {
	#if defined(MICROPY_HW_SPI1_SCK)
	} else if (spi == SPI1) {
		RCC->APB2RSTR |= (1<<20);
		RCC->APB2RSTR &= ~(1<<20);
		RCC->APB2ENR &= ~(1<<20);
	#endif
	#if defined(MICROPY_HW_SPI2_SCK)
	} else if (spi == SPI2) {
		RCC->APB2RSTR |= (1<<21);
		RCC->APB2RSTR &= ~(1<<21);
		RCC->APB2ENR &= ~(1<<21);
	#endif
	#if defined(MICROPY_HW_SPI3_SCK)
	} else if (spi == SPI3) {
		RCC->APB2RSTR |= (1<<22);
		RCC->APB2RSTR &= ~(1<<22);
		RCC->APB2ENR &= ~(1<<22);
	#endif
	#if defined(MICROPY_HW_SPI4_SCK)
	} else if (spi == SPI4) {
		RCC->APB2RSTR |= (1<<23);
		RCC->APB2RSTR &= ~(1<<23);
		RCC->APB2ENR &= ~(1<<23);
	#endif
	}
}
int spi_send(SPI_TypeDef *self ,size_t len,uint8_t *dat,uint32_t timeout)
{
	uint8_t *pData = dat;
	uint32_t t0 = HAL_GetTick();
	while(len--){
		self->TXREG = *pData;
		pData++;
		while(!(self->CSTAT & 0x01U)){
				if (HAL_GetTick() - t0 >= timeout) {
				return -MP_ETIMEDOUT;
			}
		}
	}
	return 0;
}
int spi_send_read(SPI_TypeDef *self ,size_t len,uint8_t *src, uint8_t *dest,uint32_t timeout)
{
	uint8_t *txData = src;
	int status = 0;
	while(len--){
		self->TXREG = *txData;
		uint32_t t0 = HAL_GetTick();
		while(!(self->CSTAT & 0x02U)){
				if (HAL_GetTick() - t0 >= timeout) {
				status =  -MP_ETIMEDOUT;
				break;
			}
		}
		*dest++ = (uint8_t)self->RXREG;
		txData++;
	}
	return status;
}

void spi_transfer(const spi_t *self, size_t len, const uint8_t *src, uint8_t *dest, uint32_t timeout) {
	int status = 0;
	SPI_TypeDef *spi = self->spi;
	
	spi->SCSR = 0;
	if (dest == NULL) {
		// send only
		MP_HAL_CLEAN_DCACHE(src, len);
		status = spi_send(spi,len,(uint8_t *)src,timeout);
	} else if (src == NULL) {
		// receive only 
		MP_HAL_CLEANINVALIDATE_DCACHE(dest, len);
		uint32_t t_start = HAL_GetTick();
		do {
				while(!(spi->CSTAT & 0x02U)){
						if (HAL_GetTick() - t_start >= timeout) {
						status =  -MP_ETIMEDOUT;
						break;
					}
				}
				len--;
				*dest++ = (uint8_t)spi->RXREG;
		} while (len);
	} else {
			// send and receive
			status = spi_send_read(spi ,len,(uint8_t *)src, dest,timeout);
	}
	spi->SCSR = 1;
	if (status != 0) {
			mp_hal_raise(status);
	}
}

void spi_print(const mp_print_t *print, const spi_t *spi_obj, bool legacy) {
    SPI_TypeDef *spi = spi_obj->spi;

    uint spi_num = 1; // default to SPI1
    if (0) {
    }
    #if defined(SPI2)
    else if (spi == SPI2) {
        spi_num = 2;
    }
    #endif
    #if defined(SPI3)
    else if (spi == SPI3) {
        spi_num = 3;
    }
    #endif
    #if defined(SPI4)
    else if (spi == SPI4) {
        spi_num = 4;
    }
    #endif
    mp_printf(print, "SPI(%u", spi_num);
    if (spi->GCTL & 0x01U) {
        if (spi->GCTL & 0x04U) {
            // compute baudrate
            uint log_prescaler = spi->SPBRG;
            uint baudrate = spi_get_source_freq() >> log_prescaler;
            if (legacy) {
                mp_printf(print, ", SPI.MASTER");
            }
            mp_printf(print, ", baudrate=%u", baudrate);
            if (legacy) {
                mp_printf(print, ", prescaler=%u", log_prescaler);
            }
        } else {
            mp_printf(print, ", SPI.SLAVE");
        }
				uint32_t polarity,phase,bits;
				polarity = spi->CCTL;
				phase = (polarity & 0x01U) ? 0 : 1;
				bits = (polarity & 0x08U) ? 8 : 7;
				polarity = (polarity & 0x02U) ? 1 : 0;
        mp_printf(print, ", polarity=%u, phase=%u, bits=%u", polarity, phase, bits);
    }
    mp_print_str(print, ")");		
}

const spi_t *spi_from_mp_obj(mp_obj_t o) {
    if (mp_obj_is_type(o, &pyb_spi_type)) {
        pyb_spi_obj_t *self = MP_OBJ_TO_PTR(o);
        return self->spi;
    } else if (mp_obj_is_type(o, &machine_hard_spi_type)) {
        machine_hard_spi_obj_t *self = MP_OBJ_TO_PTR(o);
        return self->spi;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("expecting an SPI object"));
    }
}

/******************************************************************************/
// Implementation of low-level SPI C protocol

STATIC int spi_proto_ioctl(void *self_in, uint32_t cmd) {
	spi_proto_cfg_t *self = (spi_proto_cfg_t *)self_in;

	switch (cmd) {
		case MP_SPI_IOCTL_INIT:
			spi_init(self->spi->spi, true);
			spi_set_params(self->spi->spi, 0xffffffff, self->baudrate,
					self->polarity, self->phase, self->bits, self->firstbit);
			self->spi->spi->GCTL |= 0x1U;
			break;

		case MP_SPI_IOCTL_DEINIT:
			spi_deinit(self->spi);
			break;
	}

	return 0;
}

STATIC void spi_proto_transfer(void *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    spi_proto_cfg_t *self = (spi_proto_cfg_t *)self_in;
    spi_transfer(self->spi, len, src, dest, SPI_TRANSFER_TIMEOUT(len));
}

const mp_spi_proto_t spi_proto = {
    .ioctl = spi_proto_ioctl,
    .transfer = spi_proto_transfer,
};

