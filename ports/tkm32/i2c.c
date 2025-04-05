/**
	******************************************************************************
	* This file is part of the MicroPython project, http://micropython.org/
	* Copyright (C), 2021 -2023, 01studio Tech. Co., Ltd.http://bbs.01studio.org/
	* File Name 				 :	i2c.c
	* Author						 :	Folktale
	* Version 					 :	v1.0
	* date							 :	2021/3/16
	* Description 			 :	
	******************************************************************************
**/
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "i2c.h"
#include "stdio.h"
#include "systick.h"

#if MICROPY_HW_ENABLE_HW_I2C


STATIC uint16_t i2c_timeout_ms[MICROPY_HW_MAX_I2C];

int i2c_init(i2c_t *i2c, mp_hal_pin_obj_t scl, mp_hal_pin_obj_t sda, uint32_t freq, uint16_t timeout_ms) {
  uint32_t i2c_id = ((uint32_t)i2c - I2C1_BASE) / (I2C2_BASE - I2C1_BASE);

	// Init pins
	if (!mp_hal_pin_config_alt(scl, MP_HAL_PIN_MODE_OPEN, MP_HAL_PIN_PULL_NONE, AF_FN_I2C, i2c_id + 1)) {
		printf("init pins error\n");
		return -MP_EPERM;
	}
	if (!mp_hal_pin_config_alt(sda, MP_HAL_PIN_MODE_OPEN, MP_HAL_PIN_PULL_NONE, AF_FN_I2C, i2c_id + 1)) {
		printf("init pins error\n");
		return -MP_EPERM;
	}

	i2c_timeout_ms[i2c_id] = timeout_ms;

	RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST << i2c_id;
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C1RST << i2c_id);

	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN << i2c_id;
	volatile uint32_t tmp = RCC->APB1ENR; // delay after RCC clock enable
	(void)tmp;
		
  i2c->IC_ENABLE &= (uint16_t)0xFFFE;

	SYSCLK_INFO clock;
	GetSysClockInfo(&clock);

	uint32_t PCLK1 = clock.PCLK1Clock;
	uint32_t pclk1Period = 0;
	uint32_t i2cPeriod = 0;
	uint32_t minSclLowTime = 0;
	uint16_t I2C_Speed;
	uint16_t tmpreg = 0;

	freq = MIN(freq, 400000);

  pclk1Period = 1000000000/PCLK1;
	
  i2cPeriod = 1000000000/freq; //ns unit
  tmpreg = 0;
	
  tmpreg = i2c->IC_CON;
  tmpreg &= ((uint16_t)0xFE8A);
	
  if (freq <= 100000){  
    minSclLowTime = i2cPeriod/pclk1Period;
		i2c->IC_SS_SCL_LCNT = minSclLowTime/2;
		i2c->IC_SS_SCL_HCNT = minSclLowTime - i2c->IC_SS_SCL_LCNT;
		I2C_Speed = I2C_Speed_STANDARD;
  }
  else {
		minSclLowTime = i2cPeriod/pclk1Period;
		i2c->IC_FS_SCL_LCNT = minSclLowTime/2;
		i2c->IC_FS_SCL_HCNT = minSclLowTime - i2c->IC_FS_SCL_LCNT;
		I2C_Speed = I2C_Speed_FAST;
  }
  
  tmpreg = TX_EMPTY_CTRL | IC_SLAVE_DISABLE | IC_RESTART_EN |IC_7BITADDR_MASTER | I2C_Speed | I2C_Mode_MASTER;//默认主机模式
  /* Write to I2Cx IC_CON */
  i2c->IC_CON = tmpreg;
  /* Get the I2Cx IC_INTR_MASK value */
  tmpreg = i2c->IC_INTR_MASK;
  /* clear the I2Cx IC_INTR_MASK value */
  tmpreg &= ((uint16_t)0xC000);
  /* Write to IC_INTR_MASK */
  i2c->IC_INTR_MASK = tmpreg;
  
  i2c->IC_RX_TL = 0x0; //rxfifo depth is 1
  i2c->IC_TX_TL = 0x1; //tcfifo depth is 1

  return 0;
}

STATIC int i2c_CheckStatus(i2c_t *i2c, uint32_t mask){
	uint32_t i2c_id = ((uint32_t)i2c - I2C1_BASE) / (I2C2_BASE - I2C1_BASE);
	uint32_t t0 = HAL_GetTick();
	while (!(i2c->IC_RAW_INTR_STAT & mask)) {
			if (HAL_GetTick() - t0 >= i2c_timeout_ms[i2c_id]) {
					return -MP_ETIMEDOUT;
			}
	}
	return 0;
}

int i2c_readfrom(i2c_t *i2c, uint16_t addr,uint8_t mem_addr ,uint8_t *dest, size_t len, bool stop) {
	int ret;
	uint8_t i;

	i2c->IC_ENABLE &= ((uint16_t)0xFFFE);
	i2c->IC_TAR = addr;
	i2c->IC_ENABLE |= ((uint16_t)0x0001);
	
	i2c->IC_DATA_CMD =(mem_addr);
	ret = i2c_CheckStatus(i2c,I2C_FLAG_TX_EMPTY);
	if(ret){
		return -MP_ETIMEDOUT;
	}
	
	for (i=0;i < len; i++) {
		i2c->IC_DATA_CMD = ((uint16_t)0x0100); //read dir
		 ret = i2c_CheckStatus(i2c,I2C_FLAG_RX_FULL);
		 if(ret){
			 return -MP_ETIMEDOUT;
		 }
		*dest++ = (uint8_t)i2c->IC_DATA_CMD;
	}
	i2c->IC_ENABLE &= ((uint16_t)0xFFFE);
	return 0;
}

int i2c_writeto(i2c_t *i2c, uint16_t addr,uint8_t mem_addr, uint8_t *src, size_t len, bool stop) {
  int ret;
	uint8_t temp;
	uint8_t *pData = src;

	i2c->IC_ENABLE &= ((uint16_t)0xFFFE);
	i2c->IC_TAR = addr;
	i2c->IC_ENABLE |= ((uint16_t)0x0001);

	i2c->IC_DATA_CMD =(mem_addr & 0xFF);
	ret = i2c_CheckStatus(i2c,I2C_FLAG_TX_EMPTY);
	
	if(ret){
		return -MP_ETIMEDOUT;
	}
	
	for (uint8_t i=0; i < len; i++) {
		temp = *pData;
		pData++;
		
		if(i==(len-1) ){
			//if(stop)
			{	
				i2c->IC_DATA_CMD = temp | 0x200; // //muaul set stop flag
			}
			//else
			{
				i2c->IC_DATA_CMD = temp;
				ret = i2c_CheckStatus(i2c,I2C_FLAG_TX_EMPTY);
				if(ret){
				 return -MP_ETIMEDOUT;
				}
			}
		}else{
			i2c->IC_DATA_CMD = temp;
			ret = i2c_CheckStatus(i2c,I2C_FLAG_TX_EMPTY);
			if(ret){
			 return -MP_ETIMEDOUT;
			}
			
		}
	}
	i2c->IC_ENABLE &= ((uint16_t)0xFFFE);
	return 0;
}
//

void test_iic(void)
{
	uint8_t addr = 0x68;
	uint8_t mem_addr = 0X6B;
	uint8_t buf[2]={0};
	
	//i2c_init(I2C3, pin_C1, pin_C0, 100000,500);
	i2c_init(I2C1, pin_B2, pin_B0, 100000,500);
	printf("start test iic\r\n");
	
	buf[0] = 0x80;
	i2c_writeto(I2C1, addr,mem_addr, buf, 1, 1);
mp_hal_delay_ms(100);
	buf[0] = 0x00;
	i2c_writeto(I2C1, addr,mem_addr, buf, 1, 1);
	
		buf[0] = 0x00;
	i2c_writeto(I2C1, addr,0x38, buf, 1, 1);
			buf[0] = 0x00;
	i2c_writeto(I2C1, addr,0x6A, buf, 1, 1);
				buf[0] = 0x00;
	i2c_writeto(I2C1, addr,0x23, buf, 1, 1);
	
int ret = i2c_readfrom(I2C1, addr,0x75 ,buf, 1, 1) ;
printf("ret:%d,%x\r\n",ret,buf[0]);
}

#endif // MICROPY_HW_ENABLE_HW_I2C




















