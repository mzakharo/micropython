/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : usb_int.c
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Endpoint CTR (Low and High) interrupt's service routines
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
extern void (*pEpInt_IN[7])(void);    /*  Handles IN  interrupts   */
extern void (*pEpInt_OUT[7])(void);   /*  Handles OUT interrupts   */

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : CTR_LP.
* Description    : Low priority Endpoint Correct Transfer interrupt's service
*                  routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CTR_LP(void)
{
  uint32_t wEpIntSta = 0,wEpxIntSta = 0;
  u8 i;

  if((wIstr= _GetUSB_INT_STA())  & USB_INT_STATE_EPINTF)  
  {
    _ClrUSB_INT_STA(USB_INT_STATE_EPINTF);  //����˵��ж�
    
    wEpIntSta = _GetEP_INT_STA();           //��ȡ�ж϶˵��

    if(wEpIntSta & EP_INT_STATE_EP0F)       //�˵�0
    {  
        EPindex = 0;
        _ClrEP_INT_STA(EP_INT_STATE_EP0F);
        wEpxIntSta = _GetEP0_INT_STA();     //��ȡ�˵�0�ж�״̬
               
        if(wEpxIntSta & EPn_INT_STATE_SETUP)
        {
          _ClrEP0_INT_STA(EPn_INT_STATE_SETUP) ;

          Setup0_Process();
                
          return;
        }
        
        if(wEpxIntSta & (EPn_INT_STATE_OUTACK))//|EPn_INT_STATE_OUTNACK
        {
          _ClrEP0_INT_STA(EPn_INT_STATE_OUTACK|EPn_INT_STATE_OUTNACK);
          Out0_Process();
          return;
        }
        
        
        if(wEpxIntSta & (EPn_INT_STATE_INNACK))//|EPn_INT_STATE_INACK
        {
          _ClrEP0_INT_STA(EPn_INT_STATE_INNACK|EPn_INT_STATE_INACK) ;
          In0_Process();
            
          return;
        }      
    }/* if(EPindex == 0) */
    if(wEpIntSta&(~EP_INT_STATE_EP0F))
    {
        
      for(i = 1;i < 5;i++)
      {
        if(wEpIntSta&(1<<i))
        {
            EPindex = i;
            _ClrEP_INT_STA(0x1<<EPindex);
            wEpxIntSta = _GetEPn_INT_STA(EPindex);
            if(wEpxIntSta & (EPn_INT_STATE_INNACK|EPn_INT_STATE_INACK))
            {
                _ClrEPn_INT_STA(EPn_INT_STATE_INNACK|EPn_INT_STATE_INACK,EPindex);
                /* call IN service function */
                (*pEpInt_IN[EPindex-1])();
            }
            if(wEpxIntSta & EPn_INT_STATE_OUTACK)
            {
                _ClrEPn_INT_STA(EPn_INT_STATE_OUTACK|EPn_INT_STATE_OUTNACK,EPindex);
                /* call OUT service function */
                (*pEpInt_OUT[EPindex-1])();
            }
        }
      }  
    }/* if(EPindex != 0)*/
    
  }/* while(...) */
}

/*******************************************************************************
* Function Name  : CTR_HP.
* Description    : High Priority Endpoint Correct Transfer interrupt's service 
*                  routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void CTR_HP(void)
{

}

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
