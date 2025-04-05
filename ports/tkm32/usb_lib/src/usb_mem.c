/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : usb_mem.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Utility functions for memory transfers to/from PMA
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : UserToPMABufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf: pointer to user memory area.
*                  - wPMABufAddr: address into PMA.
*                  - wNBytes: no. of bytes to be copied.
* Output         : None.
* Return         : None	.
*******************************************************************************/
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t EPNum, uint16_t wNBytes)
{
    uint32_t i;
    //for (i = wNBytes; i != 0; i--)
			i = wNBytes;
		while(i--)
    {
        if(EPNum == 0)
        {
            _SetUSB_FIFO0(*pbUsrBuf);
        }
        else
        {
            _SetUSB_FIFOn(*pbUsrBuf,EPNum);
        }
        pbUsrBuf++;
    }        
}
/*******************************************************************************
* Function Name  : PMAToUserBufferCopy
* Description    : Copy a buffer from user memory area to packet memory area (PMA)
* Input          : - pbUsrBuf    = pointer to user memory area.
*                  - wPMABufAddr = address into PMA.
*                  - wNBytes     = no. of bytes to be copied.
* Output         : None.
* Return         : None.
*******************************************************************************/
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t EPNum, uint16_t wNBytes)
{
  uint32_t i;
 
  for (i = wNBytes; i != 0; i--)
  {
    if(EPNum == 0)
    {
        *pbUsrBuf = _GetUSB_FIFO0() &0xff;
    }
    else
    {
        *pbUsrBuf = _GetUSB_FIFOn(EPNum);
    }
    pbUsrBuf++;
  }  
}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
