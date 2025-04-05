/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : mass_mal.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Medium Access Layer interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "mass_mal.h"
#include "sdio_sdcard.h"
#include "qspi_fun.h"
#include <string.h>
#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"

#include "pin.h"
#include "pin_static_af.h"
#include "bufhelper.h"

#include "led.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint64_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint64_t Mass_Block_Count[2];

/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the TKM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

STATIC bool sd_is_present = 0;
uint16_t MAL_Init(uint8_t lun)
{
  uint16_t status = MAL_OK;  

	#if defined(MICROPY_HW_SDCARD_DETECT_PIN)
	sd_is_present =  GPIO_ReadInputDataBit(MICROPY_HW_SDCARD_DETECT_PIN->gpio, MICROPY_HW_SDCARD_DETECT_PIN->pin_mask) == MICROPY_HW_SDCARD_DETECT_PRESENT;
	#endif
  switch (lun)
  {		
    case 0:	
			if(sd_is_present){

			}else{

			}
     break;			 
    default:
      return MAL_FAIL;
  }

  return status;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint8_t *Writebuff, uint16_t Transfer_Length)
{

	uint8_t STA;
  switch (lun)
  {
		case 0:		 
			STA=0;
			led_state(1, 1);
			if(sd_is_present){
				STA=SD_WriteDisk((uint8_t*)Writebuff, Memory_Offset>>9, Transfer_Length>>9);
			}else{
				w25qxx_write((uint8_t*)Writebuff, QFLASH_BASE_ADDR+Memory_Offset, Transfer_Length);  
				//QFLASH_Check_FunProgram(QFLASH_BASE_ADDR+Memory_Offset,(uint8_t*)Writebuff,Transfer_Length);
			}
			led_state(1, 0);
			break;
			
    default:
      return MAL_FAIL;
  }
  if(STA!=0)return MAL_FAIL;
	return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/

 uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint8_t *Readbuff, uint16_t Transfer_Length)
{
	uint8_t STA;
  switch (lun)
  {
		case 0:		 
		STA = MAL_OK;	

		if(sd_is_present){
			STA=SD_ReadDisk((uint8_t*)Readbuff, Memory_Offset>>9, Transfer_Length>>9);
		}else{
			w25qxx_read((u8*)Readbuff, QFLASH_BASE_ADDR+Memory_Offset, Transfer_Length);
			//QspiFlashRead(QFLASH_BASE_ADDR+Memory_Offset,(uint8_t*)Readbuff,Transfer_Length);
		}

		break;
		default:
		return MAL_FAIL;
  }
  if(STA!=0)return MAL_FAIL;
	return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

uint16_t MAL_GetStatus (uint8_t lun)
{
	switch(lun)
    {
    case 0:
		
		if(sd_is_present){
			Mass_Memory_Size[lun]=SDCardInfo.CardCapacity;
			Mass_Block_Size[lun] =512;
			Mass_Block_Count[lun]=Mass_Memory_Size[lun]/Mass_Block_Size[lun];
		}else{
			Mass_Memory_Size[lun]=(MICROPY_HW_QFLASH_SIZE_BITS-(QFLASH_BASE_ADDR * 8));
			Mass_Block_Size[lun] =512;
			Mass_Block_Count[lun]=(Mass_Memory_Size[lun]/8/Mass_Block_Size[lun]);
		}

    return MAL_OK;
		
    default:
        return MAL_FAIL;
    }
  return MAL_FAIL;
}




/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
