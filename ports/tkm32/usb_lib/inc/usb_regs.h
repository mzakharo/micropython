/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : usb_regs.h
* Author             : MCD Application Team
* Version            : V3.0.1
* Date               : 04/27/2009
* Description        : Interface prototype functions to USB cell registers
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_REGS_H
#define __USB_REGS_H

/* endpoints enumeration */
#define ENDP0   ((uint8_t)0)
#define ENDP1   ((uint8_t)1)
#define ENDP2   ((uint8_t)2)
#define ENDP3   ((uint8_t)3)
#define ENDP4   ((uint8_t)4)

/* Exported macro ------------------------------------------------------------*/


/* SetUSB_TOP */
#define _SetUSB_TOP(wRegValue)  	    (USB->rTOP  |= (uint16_t)wRegValue)
/* ClrUSB_TOP */
#define _ClrUSB_TOP(wRegValue)  	    (USB->rTOP  &= (uint16_t)~wRegValue)

/* SetUSB_ISTR */
#define _ClrUSB_INT_STA(wRegValue)  	(USB->rINT_STATE  = (uint16_t)wRegValue)
/* GetUSB_ISTR */
#define _GetUSB_INT_STA()  	            (USB->rINT_STATE)

/* ClrEP_INT_STA ,write 1 clear 0.*/
#define _ClrEP_INT_STA(wRegValue)  	    (USB->rEP_INT_STATE  = (uint16_t)wRegValue)
/* GetEP_INT_STA */
#define _GetEP_INT_STA()  	            (USB->rEP_INT_STATE)

/* ClrEP0_INT_STA */
#define _ClrEP0_INT_STA(wRegValue)  	(USB->rEP0_INT_STATE  = (uint16_t)wRegValue)
/* GetEP0_INT_STA */
#define _GetEP0_INT_STA()  	            (USB->rEP0_INT_STATE)

/* SetUSB_INT_EN */
#define _SetUSB_INT_EN(wRegValue)  	    (USB->rINT_EN  |= (uint16_t)wRegValue)
/* ClrUSB_INT_EN */
#define _ClrUSB_INT_EN(wRegValue)  	    (USB->rINT_EN  &= (uint16_t)~wRegValue)

/* SetUSB_INT_EN */
#define _SetEP_INT_EN(wRegValue)  	    (USB->rEP_INT_EN  |= (uint16_t)wRegValue)
/* SetUSB_INT_EN */
#define _ClrEP_INT_EN(wRegValue)  	    (USB->rEP_INT_EN  &= (uint16_t)~wRegValue)

/* SetEP0_INT_EN */
#define _SetEP0_INT_EN(wRegValue)  	    (USB->rEP0_INT_EN |= (uint16_t)wRegValue)
/* SetEP0_INT_EN */
#define _ClrEP0_INT_EN(wRegValue)  	    (USB->rEP0_INT_EN &= (uint16_t)~wRegValue)

/* ClrEP1_INT_STA */
#define _ClrEP1_INT_STA(wRegValue)  	(USB->rEP1_INT_STATE = (uint16_t)wRegValue)
/* GetEP1_INT_EN */
#define _GetEP1_INT_STA()  	            (USB->rEP1_INT_STATE)

/* ClrEP2_INT_STA */
#define _ClrEP2_INT_STA(wRegValue)  	(USB->rEP2_INT_STATE = (uint16_t)wRegValue)
/* GetEP2_INT_STA */
#define _GetEP2_INT_STA()  	            (USB->rEP2_INT_STATE)

/* ClrEP3_INT_STA */
#define _ClrEP3_INT_STA(wRegValue)  	(USB->rEP3_INT_STATE = (uint16_t)wRegValue)
/* GetEP3_INT_STA */
#define _GetEP3_INT_STA()  	            (USB->rEP3_INT_STATE)

/* ClrEP4_INT_STA */
#define _ClrEP4_INT_STA(wRegValue)  	(USB->rEP4_INT_STATE = (uint16_t)wRegValue)
/* GetEP4_INT_STA */
#define _GetEP4_INT_STA()  	            (USB->rEP4_INT_STATE)

/* ClrEPn_INT_STA ,n = 1,2,3,4*/
#define _ClrEPn_INT_STA(wRegValue,n)  	(*(u32*)((u32)(&(USB->rEP1_INT_STATE))+(n-1)*4) = (uint16_t)wRegValue)
/* GetEPn_INT_EN */
#define _GetEPn_INT_STA(n)  	        (*(u32*)((u32)(&(USB->rEP1_INT_STATE))+(n-1)*4))

/* SetEP1_INT_EN */
#define _SetEP1_INT_EN(wRegValue)  	    (USB->rEP1_INT_EN |= (uint16_t)wRegValue)
/* SetEP1_INT_EN */
#define _ClrEP1_INT_EN(wRegValue)  	    (USB->rEP1_INT_EN &= (uint16_t)~wRegValue)

/* SetEP2_INT_EN */
#define _SetEP2_INT_EN(wRegValue)  	    (USB->rEP2_INT_EN |= (uint16_t)wRegValue)
/* SetEP2_INT_EN */
#define _ClrEP2_INT_EN(wRegValue)  	    (USB->rEP2_INT_EN &= (uint16_t)~wRegValue)

/* SetEP3_INT_EN */
#define _SetEP3_INT_EN(wRegValue)  	    (USB->rEP3_INT_EN |= (uint16_t)wRegValue)
/* SetEP3_INT_EN */
#define _ClrEP3_INT_EN(wRegValue)  	    (USB->rEP3_INT_EN &= (uint16_t)~wRegValue)

/* SetEP4_INT_EN */
#define _SetEP4_INT_EN(wRegValue)  	    (USB->rEP4_INT_EN |= (uint16_t)wRegValue)
/* SetEP4_INT_EN */
#define _ClrEP4_INT_EN(wRegValue)  	    (USB->rEP4_INT_EN &= (uint16_t)~wRegValue)

/* GetEP4_INT_EN */
#define _GetEPn_INT_EN(n)               (*(u32*)((u32)(&(USB->rEP1_INT_EN))+(n-1)*4))
/* GetEP4_INT_EN */
#define _SetEPn_INT_EN(wRegValue,n)     (*(u32*)((u32)(&(USB->rEP1_INT_EN))+(n-1)*4) = (uint16_t)wRegValue)

/* Set_EP_EN */
#define _SetUSB_ADDR(wRegValue)	        (USB->rADDR = (uint16_t)wRegValue)

/* Set_EP_EN */
#define _SetEP_EN(wRegValue)		    (USB->rEP_EN |= (uint16_t)wRegValue)
/* Clr_EP_EN */
#define _ClrEP_EN(wRegValue)		    (USB->rEP_EN &= (uint16_t)~wRegValue)

/* SetUSB_AVIL0 */
#define _SetUSB_AVIL0(wRegValue)  	    (USB->rEP0_AVIL  = (uint16_t)wRegValue)

/* SetUSB_AVIL1 */
#define _SetUSB_AVIL1(wRegValue)  	    (USB->rEP1_AVIL  = (uint16_t)wRegValue)

/* SetUSB_AVIL2 */
#define _SetUSB_AVIL2(wRegValue)  	    (USB->rEP2_AVIL  = (uint16_t)wRegValue)

/* SetUSB_AVIL3 */
#define _SetUSB_AVIL3(wRegValue)  	    (USB->rEP3_AVIL  = (uint16_t)wRegValue)

/* SetUSB_AVIL4 */
#define _SetUSB_AVIL4(wRegValue)  	    (USB->rEP4_AVIL  = (uint16_t)wRegValue)

/* GetUSB_FIFOn NUM */
#define _GetUSB_AVILn(n)  	            (*(u32*)((u32)(&(USB->rEP0_AVIL))+(n)*4))

/* SetUSB_CTRL0 */
#define _SetUSB_CTRL0(wRegValue)  	    (USB->rEP0_CTRL  = (uint16_t)wRegValue)
/* GetUSB_CTRL0 */
#define _GetUSB_CTRL0()  	            (USB->rEP0_CTRL)

/* SetUSB_CTRL1 */
#define _SetUSB_CTRL1(wRegValue)  	    (USB->rEP1_CTRL  = (uint16_t)wRegValue)
/* GetUSB_CTRL1 */
#define _GetUSB_CTRL1()  	            (USB->rEP1_CTRL)

/* SetUSB_CTRL2 */
#define _SetUSB_CTRL2(wRegValue)  	    (USB->rEP2_CTRL  = (uint16_t)wRegValue)
/* GetUSB_CTRL2 */
#define _GetUSB_CTRL2()  	            (USB->rEP2_CTRL)

/* SetUSB_CTRL3 */
#define _SetUSB_CTRL3(wRegValue)  	    (USB->rEP3_CTRL  = (uint16_t)wRegValue)
/* GetUSB_CTRL3 */
#define _GetUSB_CTRL3()  	            (USB->rEP3_CTRL)

/* SetUSB_CTRL4 */
#define _SetUSB_CTRL4(wRegValue)  	    (USB->rEP4_CTRL  = (uint16_t)wRegValue)
/* GetUSB_CTRL4 */
#define _GetUSB_CTRL4()  	            (USB->rEP4_CTRL)

/* SetUSB_FIFO0 */
#define _SetUSB_FIFO0(wRegValue)  	    (USB->rEP0_FIFO  = (uint32_t)wRegValue&0xff)
/* GetUSB_FIFO0 */
#define _GetUSB_FIFO0()  	            (USB->rEP0_FIFO )

/* SetUSB_FIFO1 */
#define _SetUSB_FIFO1(wRegValue)  	    (USB->rEP1_FIFO  = (uint16_t)wRegValue)
/* GetUSB_FIFO1 */
#define _GetUSB_FIFO1()  	            (USB->rEP1_FIFO )

/* SetUSB_FIFO2 */
#define _SetUSB_FIFO2(wRegValue)  	    (USB->rEP2_FIFO  = (uint16_t)wRegValue)
/* GetUSB_FIFO0 */
#define _GetUSB_FIFO2()  	            (USB->rEP2_FIFO )

/* SetUSB_FIFO03 */
#define _SetUSB_FIFO3(wRegValue)  	    (USB->rEP3_FIFO  = (uint16_t)wRegValue)
/* GetUSB_FIFO3 */
#define _GetUSB_FIFO3()  	            (USB->rEP3_FIFO )

/* SetUSB_FIFO4 */
#define _SetUSB_FIFO4(wRegValue)  	    (USB->rEP4_FIFO  = (uint16_t)wRegValue)
/* GetUSB_FIFO0 */
#define _GetUSB_FIFO4()  	            (USB->rEP4_FIFO )

/* SetUSB_FIFO4 */
#define _SetUSB_FIFOn(wRegValue,n)  	(*(u32*)((u32)(&(USB->rEP1_FIFO))+(n-1)*4) = (uint16_t)wRegValue)
/* GetUSB_FIFO0 */
#define _GetUSB_FIFOn(n)  	            (*(u32*)((u32)(&(USB->rEP1_FIFO))+(n-1)*4))

/* SetUSB_HALT */
#define _SetUSB_HALT(wRegValue)  	    (USB->rEP_HALT  |= (uint16_t)wRegValue)
/* ClrUSB_HALT */
#define _ClrUSB_HALT(wRegValue)  	    (USB->rEP_HALT  &= (uint16_t)~wRegValue)
/* GetUSB_HALT */
#define _GetUSB_HALT()  	            (USB->rEP_HALT)

/* SetUSB_POWER */
#define _SetUSB_POWER(wRegValue)  	    (USB->rPOWER  |= (uint16_t)wRegValue)
/* ClrUSB_TOP */
#define _ClrUSB_POWER(wRegValue)  	    (USB->rPOWER  &= (uint16_t)~wRegValue)

 
/* External variables --------------------------------------------------------*/
extern __IO uint16_t wIstr;  /* ISTR register last read value */

/* Exported functions ------------------------------------------------------- */


#endif /* __USB_REGS_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
