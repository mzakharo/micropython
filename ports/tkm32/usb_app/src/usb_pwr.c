/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : usb_pwr.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Connection/disconnection & power management
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "HAL_device.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "hw_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t bDeviceState = UNCONNECTED; /* USB device status */
__IO bool fSuspendEnabled = TRUE;  /* true when suspend is possible */

struct
{
  __IO RESUME_STATE eState;
  __IO uint8_t bESOFcnt;
}
ResumeS;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : PowerOn
* Description    :
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
RESULT PowerOn(void)
{
    USB_Cable_Config(DISABLE);          //USB连接1.5K内部上拉电阻

    _SetUSB_TOP(USB_TOP_RESET) ;        //USB复位
    _ClrUSB_TOP(USB_TOP_RESET) ;

    _ClrUSB_INT_STA(USB_INT_STATE_RSTF|USB_INT_STATE_EPINTF);   //清USB中断USB_INT_STATE_SOFF|

    _SetUSB_INT_EN(USB_INT_EN_RSTIE|USB_INT_EN_EPINTIE);        //使能USB中断USB_INT_EN_SOFIE|

    _SetEP0_INT_EN( EPn_INT_EN_SETUPIE|EPn_INT_EN_INNACKIE|EPn_INT_EN_OUTACKIE);//|EPn_INT_EN_OUTNACKIE

    _ClrEP_EN((1<<ENDP0)|(1<<ENDP1)|(1<<ENDP2)|(1<<ENDP3)|(1<<ENDP4));

    _SetEP0_INT_EN( EPn_INT_EN_SETUPIE|EPn_INT_EN_INNACKIE|\
                  EPn_INT_EN_OUTACKIE);//EPn_INT_EN_INACKIE|EPn_INT_EN_OUTNACKIE|
    _ClrEP0_INT_STA(EPn_INT_STATE_SETUP|EPn_INT_STATE_INNACK|\
                  EPn_INT_EN_INACKIE|EPn_INT_STATE_OUTACK|EPn_INT_STATE_OUTNACK);

//    _SetEP1_INT_EN( EPn_INT_EN_INNACKIE );                      // EPn_INT_EN_INACKIE
//    _ClrEP1_INT_STA(EPn_INT_STATE_INNACK);                      // EPn_INT_STATE_INACK
//    _SetEP_EN((1<<ENDP0)|(1<<ENDP1));
//    _SetEP_INT_EN((1<<ENDP0)|(1<<ENDP1));  

    USB_Cable_Config(ENABLE);                                   //USB连接1.5K内部上拉电阻
    return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : PowerOff
* Description    : handles switch-off conditions
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
RESULT PowerOff()
{
  _SetUSB_TOP(USB_TOP_RESET) ;      //USB复位
  _ClrUSB_INT_STA(USB_INT_STATE_RSTF|USB_INT_STATE_SOFF|USB_INT_STATE_EPINTF);  //清USB中断
  
  USB_Cable_Config(DISABLE);        //断开连接

  return USB_SUCCESS;
}

/*******************************************************************************
* Function Name  : Suspend
* Description    : sets suspend mode operating conditions
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
void Suspend(void)
{

}

/*******************************************************************************
* Function Name  : Resume_Init
* Description    : Handles wake-up restoring normal operations
* Input          : None.
* Output         : None.
* Return         : USB_SUCCESS.
*******************************************************************************/
void Resume_Init(void)
{

}

/*******************************************************************************
* Function Name  : Resume
* Description    : This is the state machine handling resume operations and
*                 timing sequence. The control is based on the Resume structure
*                 variables and on the ESOF interrupt calling this subroutine
*                 without changing machine state.
* Input          : a state machine value (RESUME_STATE)
*                  RESUME_ESOF doesn't change ResumeS.eState allowing
*                  decrementing of the ESOF counter in different states.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Resume(RESUME_STATE eResumeSetVal)
{
//  uint16_t wCNTR;

  if (eResumeSetVal != RESUME_ESOF)
    ResumeS.eState = eResumeSetVal;

  switch (ResumeS.eState)
  {
    case RESUME_EXTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_OFF;
      break;
    case RESUME_INTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_START;
      break;
    case RESUME_LATER:
      ResumeS.bESOFcnt = 2;
      ResumeS.eState = RESUME_WAIT;
      break;
    case RESUME_WAIT:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
        ResumeS.eState = RESUME_START;
      break;
    case RESUME_START:
      ResumeS.eState = RESUME_ON;
      ResumeS.bESOFcnt = 10;
      break;
    case RESUME_ON:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
      {

        ResumeS.eState = RESUME_OFF;
      }
      break;
    case RESUME_OFF:
    case RESUME_ESOF:
    default:
      ResumeS.eState = RESUME_OFF;
      break;
  }
}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
