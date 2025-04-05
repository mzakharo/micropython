/**
******************************************************************************
* @file    usbreg.c
* @author  IC Applications Department
* @version  V0.8
* @date  2019_08_02
* @brief   This file provides all the usbreg firmware functions.
******************************************************************************
* @copy
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, HOLOCENE SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2016 HOLOCENE</center></h2>
*/ 
#include "usb.h"
#include "HAL_device.h"
#include "stdio.h"

#include "usbprop.h"
volatile uint8_t USB_EP_STALL[5];
volatile uint8_t USB_EP_IN_STALL[5];
volatile uint8_t USB_EP_OUT_STALL[5];
//volatile uint8_t usb_setup_flag;
//volatile uint8_t usb_out_flag;
//volatile uint8_t usb_in_flag;
volatile uint8_t USB_FINISH_Flag[5];
//volatile uint8_t usb_reset_flag;
//volatile uint8_t usb_suspend_flag;
//volatile uint8_t usb_resume_flag;
//volatile uint8_t usb_state_suspend = 0;
//volatile uint8_t usb_state_resume = 0;
volatile USB_STATE_t usb_state;
volatile uint8_t usb_ep_flag;
//
volatile uint8_t USB_SEND_OPEN_STALL[5];
volatile uint32_t irq_state = 0;
/********************************************************************************************************
**函数信息 ：USB_HP_CAN1_TX_IRQHandler(void)
**功能描述 ：USB中断处理函数
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
#include "py/mphal.h"
void USB_CDC_IRQHandler(void)
{
    uint32_t temp;
    uint32_t temp_ep;
    uint32_t temp_epn;
    temp = USB->rINT_STATE ;				//读取USB中断状态
    USB->rINT_STATE = temp;					//清USB中断状态,写1清零
    if(temp & USB_INT_STATE_EPINTF)			//端点中断
    {
        temp_ep = USB->rEP_INT_STATE;			//读取端点中断号 0x1,0x2,0x4,0x8,0x10分别对应0,1,2,3,4端点中断

        USB->rEP_INT_STATE = temp_ep;		//清端点号中断
        if(temp_ep & EP_INT_STATE_EP0F )		//端点0中断
        {
            //GPIO_SetBits(GPIOA, GPIO_Pin_5);//用于触发
            usb_ep_flag = USB_EP0;			//记录端点号
            temp_epn = USB->rEP0_INT_STATE;	//读取端点0中断状态位

            USB->rEP0_INT_STATE = temp_epn; //清端点0中断

            if(temp_epn & EPn_INT_STATE_SETUP) //SETUP中断
            {
              usb_setup_handle();			//setup包处理,进入控制传输setup阶段
            }
            if(temp_epn & EPn_INT_STATE_OUTACK)	//端点0 OUT包应答中断，收到数据
            {
              usb_out_handle();				//OUT包处理
            }
            if(temp_epn & EPn_INT_STATE_INNACK)	//IN包非应答中断，准备写入数据
            {
              usb_in_handle();				//IN包处理
              USB->rEP0_INT_STATE |= EPn_INT_STATE_INNACK;//清端点中断
            }

            if(temp_epn & (EPn_INT_EN_OUTSTALLIE | EPn_INT_EN_INSTALLIE)) //端点关闭，可以加入自定义函数
            {

                USB_EP_STALL[0] = 1;
            }
            if(temp_epn & EPn_INT_STATE_INSTALL)
            {
                USB_EP_IN_STALL[0] = 1;
            }
            if(temp_epn & EPn_INT_STATE_OUTSTALL)
            {
                USB_EP_OUT_STALL[0] = 1;
            }
        }
#if ((EPOUT!=EPIN))	 							//if epin != epout,
        if(temp_ep & EPn_INT(EPOUT))				//输出端点号中断,同上面端点0判断,判断是否为输出端点(这里设置端点3为输出端点)中断
        {
            usb_ep_flag = USB_EPn(EPOUT);

            temp_epn = USB->rEP3_INT_STATE;
            USB->rEP3_INT_STATE = temp_epn;
            if(temp_epn & EPn_INT_STATE_OUTACK)	//OUT包应答中断
            {
							//irq_state = disable_irq();
							usb_out_handle();
						//	printf("q\r\n");
							//printf("EPn_INT_STATE_OUTACK\r\n");
							//EP3_USB_OUT_Data();
            }
            if(temp_epn & EPn_INT_STATE_END)		//传输完成中断
            {
                USB_FINISH_Flag[EPOUT] = 1;
            }
            if(temp_epn & (EPn_INT_EN_OUTSTALLIE | EPn_INT_EN_INSTALLIE))
            {
                USB_EP_STALL[EPOUT] = 1;
            }
            if(temp_epn & EPn_INT_STATE_INSTALL)
            {
                USB_EP_IN_STALL[EPOUT] = 1;
            }
            if(temp_epn & EPn_INT_STATE_OUTSTALL)
            {
                USB_EP_OUT_STALL[EPOUT] = 1;
            }
        }
#endif
        if(temp_ep & EPn_INT(EPIN))
        {
            usb_ep_flag = USB_EPn(EPIN);
            temp_epn = USB->rEP1_INT_STATE;
            USB->rEP1_INT_STATE = temp_epn;
            if(temp_epn & EPn_INT_STATE_INNACK)	//IN包非应答中断
            {
                usb_in_handle();
            }
            if(temp_epn & EPn_INT_STATE_END)
                USB_FINISH_Flag[EPIN] = 1;
            if(temp_epn & (EPn_INT_EN_OUTSTALLIE | EPn_INT_EN_INSTALLIE))
            {
                USB_EP_STALL[EPIN] = 1;
            }
            if(temp_epn & EPn_INT_STATE_INSTALL)
            {
                USB_EP_IN_STALL[EPIN] = 1;
            }
            if(temp_epn & EPn_INT_STATE_OUTSTALL)
            {
                USB_EP_OUT_STALL[EPIN] = 1;
            }
        }
    }
    else if(temp & USB_INT_STATE_RSTF)			//USB复位中断
    {
        usb_reset_handle();
    }
    else if(temp & USB_INT_STATE_SUSPENDF )	//USB挂起中断,自定义编写
    {
        usb_suspend_handle();
    }
    else if(temp & USB_INT_STATE_RESUMF  )		//USB恢复中断,自定义编写
    {
        usb_resume_handle();
    }
}



/********************************************************************************************************
**函数信息 ：Read_Mreg32( uint32_t mreg)
**功能描述 ：指定地址读32bit数据
**输入参数 ：uint32_t mreg(地址)
**输出参数 ：return(*(volatile uint32_t *)mreg);数据
**备    注 ：
********************************************************************************************************/
uint32_t Read_Mreg32( uint32_t mreg)
{
    return(*(volatile uint32_t *)mreg);
}
/********************************************************************************************************
**函数信息 ：Read_Mreg32( uint32_t mreg)
**功能描述 ：指定地址写32bit数据
**输入参数 ：uint32_t mreg(地址), uint32_t val(数据)
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Write_Mreg32( uint32_t mreg, uint32_t val)
{
    *(volatile uint32_t *)mreg = (uint32_t)val;
}

/********************************************************************************************************
**函数信息 ：Read_Mreg32( uint32_t mreg)
**功能描述 ：指定地址读8bit数据
**输入参数 ：uint32_t mreg(地址)
**输出参数 ：
**备    注 ：
********************************************************************************************************/
uint8_t read_mreg8( uint32_t mreg)
{
    return(*(volatile uint8_t *)mreg);
}
/********************************************************************************************************
**函数信息 ：write_mreg8( uint32_t mreg, uint8_t val)
**功能描述 ：指定地址写8bit数据
**输入参数 ：uint32_t mreg(地址), uint8_t val(数据)
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void write_mreg8( uint32_t mreg, uint8_t val)
{
    *(volatile uint8_t *)mreg = (uint8_t)val;
}
/********************************************************************************************************
**函数信息 ：usb_delay1ms(uint32_t dly)
**功能描述 ：usb内部延时，初始化及唤醒操作
**输入参数 ：uint32_t dly(延时参数)
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_delay1ms(uint32_t dly)
{
    uint32_t cnt, i, j, k;

    for(cnt = 0; cnt < dly; cnt++)
    {
        for(i = 0; i < 24; i++)
        {
            for(j = 0; j < 2; j++)
            {
                for(k = 0; k < 100; k++);
            }
        }
    }

}


/*-------------------------(C) COPYRIGHT 2016 HOLOCENE ----------------------*/
