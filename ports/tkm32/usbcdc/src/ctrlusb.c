/**
******************************************************************************
* @file    ctrlusb.c
* @author  IC Applications Department
* @version  V0.8
* @date  2019_08_02
* @brief   This file provides all the ctrlusb firmware functions.
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
#include "stdio.h"
#include "led.h"

volatile uint8_t   usb_running_state;  							// running stage
volatile uint8_t   usb_running_ctrl_state;  						// running stage
volatile uint8_t   bmRequestType;
volatile uint8_t   bRequest;
volatile uint16_t  wValue;
volatile uint16_t  wIndex;
volatile uint16_t  wLength;
volatile uint8_t   req_dir;										//bmRequestType[7];
volatile uint8_t   req_type;										//bmRequestType[6:5];
volatile uint8_t   req_recip;										//bmRequestType[4:0];
//=========================== Each Request =============================
volatile uint8_t connect_state = USBD_CDC_CONNECT_DISCONNECTED;
//=================================== Handler================================
#if 1

void (*StandardDeviceRequest[MAX_STD_REQUEST])(void) =
{
    Chap9_GetStatus,		//0x00
    Chap9_ClearFeature,		//0x01
    MLsup_StallEP0,			//reserve
    Chap9_SetFeature,		//0x03
    MLsup_StallEP1,			//reserve
    Chap9_SetAddress,		//0x05
    Chap9_GetDescriptor,	//0x06
    Chap9_SetDescriptor,	//0x07
    Chap9_GetConfiguration,	//0x08
    Chap9_SetConfiguration,	//0x09
    Chap9_GetInterface,		//0x0a
    Chap9_SetInterface,		//0x0b
};
#endif
/********************************************************************************************************
**函数信息 ：usb_setup_handle( void)
**功能描述 ：控制传输setup阶段数据读取,将协议相关的值保存至全局变量
**输入参数 ：
**输出参数 ：
**备    注 ：USB是轮询机制的总线，所有的操作都是由host开始
********************************************************************************************************/

void usb_setup_handle( void)
{
    uint8_t    temp ;

    if(usb_running_ctrl_state == USB_IDLE)
    {
        //bmrequest type one byte
        bmRequestType = USB->rSETUP[0];//获取setup包首字节,记录host请求类型
        //brequest one byte

        bRequest = USB->rSETUP[1];
        //wvalue two bytes

        temp = USB->rSETUP[2];

        wValue = USB->rSETUP[3];
        wValue = (wValue << 8) + temp;
        //windex two bytes

        temp = USB->rSETUP[4];

        wIndex = USB->rSETUP[5];

        wIndex = (wIndex << 8) + temp;
        //wlength two bytes

        temp = USB->rSETUP[6];

        wLength = USB->rSETUP[7];
        wLength = (wLength << 8) + temp;
        //direction
        req_dir = bmRequestType & USB_REQ_DIR_MASK ;  //&0x80
        //recipient
        req_recip = bmRequestType & USB_REQ_RECIP_MASK ; //&0x1F
        //type
        req_type = bmRequestType & USB_REQ_TYPE_MASK ;  //&0x60
        //		for(i = 0;i < 8 ;i ++)
        //		{
        //			//printf("%x ",USB->rSETUP[i]);//打印setup数据
        //		}

#if 1
       // printf("bmRequestType = 0x%x\r\n", bmRequestType);
        //printf("bRequest = 0x%x\r\n", bRequest);
       // printf("wValue = 0x%x\r\n", wValue);
        //printf("wIndex = 0x%x\r\n", wIndex);
				//printf("wLength = 0x%x\r\n", wLength);
				//if(req_type == 0x20 && bRequest == 0x22)
				if(bRequest == 0x22){
						if(wValue & 0x01){
							connect_state = USBD_CDC_CONNECT_CONNECTED;
						}else{
							connect_state = USBD_CDC_CONNECT_DISCONNECTED;
						}
					
				}

        
#endif

        usb_running_ctrl_state = USB_CTRL_SETUP ;
    }
    ctrl_deal_handle();

}

/********************************************************************************************************
**函数信息 ：ctrl_deal_handle(void)
**功能描述 ：控制传输数据及USB状态处理
**输入参数 ：
**输出参数 ：
**备    注 ：记录USB状态方便USB正确响应
********************************************************************************************************/
void ctrl_deal_handle(void)
{
    if( req_type == USB_REQ_TYPE_STANDARD )     				// standard request type
    {
        if( bRequest < MAX_STD_REQUEST )    					// handle 0--12
        {
            (*StandardDeviceRequest[bRequest])( ) ;				//调用标准请求函数
        }	
    }

    else if( req_type == USB_REQ_TYPE_CLASS )  					// class request type
    {

        switch(bRequest)										//类请求可根据具体设备类型编写,需参见具体类协议
        {
            //此处参考USB_CDC协议Requests  Abstract Control Model
            case 0x20:											//set_line_coding 20h，配置波特率，停止位，校验位，数据位
                {
                    Class_Set_Line_Coding();
                    break;
                }
            case 0x21:											//get_line_coding 21h，get_line_coding请求实时波特率...
                {
                    Class_Get_Line_Coding();
                    break;
                }
            case 0x22:
                {
                    Class_Set_Control_Line_State();					//反馈状态,用于流控制
                    break;
                }
            default:
                {
                    usb_running_ctrl_state = USB_IDLE ;
                    break;
                }
        }
    }
}

/********************************************************************************************************
**函数信息 ：ctrl_in_token_handle( void)
**功能描述 ：控制传输IN包处理
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/

void ctrl_in_token_handle( void)
{
    if(usb_running_ctrl_state == USB_CTRL_STATUS)
    {
        EP0_Send_Empty_Packet();
        usb_running_ctrl_state = USB_IDLE ;
    }
    else
    {
        ctrl_deal_handle();
				//printf("2\r\n");
    }
}
/********************************************************************************************************
**函数信息 ：ctrl_out_handle( void)
**功能描述 ：控制传输OUT包处理
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void ctrl_out_handle( void)
{

    if(req_dir == USB_REQ_DIR_OUT)								//data stage when control write
    {
        ctrl_deal_handle();
    }
    else 														// status stage when control read, IP will send ack auto
    {
    }
}
/********************************************************************************************************
**函数信息 ：USB_CDC_Init( void)
**功能描述 ：USB初始化
**输入参数 ：
**输出参数 ：
**备    注 ：这里使用端点0(默认端点),1(作为IN端点),3(作为OUT端点)
********************************************************************************************************/
void USB_CDC_Init( void)
{
    uint8_t i;
	
    usb_ep_flag = USB_EP0;
    //	write_finish = 1;
    //	dma_finished_flag[0] = 0;
    //	dma_finished_flag[1] = 0;
    usb_state = USB_STATE_POWERED;
    usb_running_state = USB_IDLE;
    usb_running_ctrl_state = USB_IDLE;
    for(i = 0; i < 5; i++)
    {
        USB_EP_STALL[i] = 0;
        USB_EP_IN_STALL[i] = 0;
        USB_EP_OUT_STALL[i] = 0;
        USB_FINISH_Flag[i] = 0;
        USB_SEND_OPEN_STALL[i] = 0;
    }

    USB->rTOP  = USB_TOP_RESET;//reset usb
    USB->rTOP  &= ~USB_TOP_RESET;
    USB->rTOP  &= ~USB_TOP_CONNECT ;//usb disconnect

    USB->rINT_STATE |= 0;//清USB状态
    USB->rEP_INT_STATE |= 0;
    USB->rEP0_INT_STATE |= 0;
    USB->rEP1_INT_STATE |= 0;
    USB->rEP2_INT_STATE |= 0;
    USB->rEP3_INT_STATE |= 0;
    USB->rEP4_INT_STATE |= 0;

    USB->rEP0_CTRL = 0;//清数据
    USB->rEP1_CTRL = 0;
    USB->rEP2_CTRL = 0;
    USB->rEP3_CTRL = 0;
    USB->rEP4_CTRL = 0;

    USB->rINT_EN = USB_INT_EN_RSTIE | USB_INT_EN_SUSPENDIE | USB_INT_EN_RESUMIE | USB_INT_EN_EPINTIE ; //使能USB相关中断：复位，挂起，恢复，端点中断

    USB->rEP_INT_EN = EP_INT_EN_EP0IE | EP_INT_EN_EP1IE | EP_INT_EN_EP3IE;							//使能USB端点中断：端点0，1，3
    USB->rEP0_INT_EN = EPn_INT_EN_SETUPIE | EPn_INT_EN_ENDIE | EPn_INT_EN_INNACKIE | EPn_INT_EN_OUTACKIE | EPn_INT_EN_OUTSTALLIE | EPn_INT_EN_INSTALLIE; //使能USB端点0中断：setup，传输完成，INNACK,OUTACK,INSTALL,OUTSTALL中断

    USB->rEP1_INT_EN = EPn_INT_EN_ENDIE | EPn_INT_EN_INNACKIE | EPn_INT_EN_INSTALLIE | EPn_INT_EN_OUTSTALLIE; //配置端点1为输入端点(与枚举阶段内端点描述符对应);
    USB->rEP3_INT_EN = EPn_INT_EN_ENDIE | EPn_INT_EN_OUTACKIE | EPn_INT_EN_INSTALLIE | EPn_INT_EN_OUTSTALLIE; //配置端点3为输出端点，由响应方式决定端点方向;

    USB->rEP_EN = EP_EN_EP0EN | EP_EN_EP2EN | EP_EN_EP1EN | EP_EN_EP3EN | EP_EN_EP4EN;	//开端点0&1&3中断

    /////配置更多端点可按照上面方式配置

    USB->rADDR = 0;										//USB设备地址默认为0

    USB->rTOP = USB_TOP_CONNECT | ((~USB_TOP_SPEED) & 0x01); //连接USB进入工作模式
    USB->rPOWER = USB_POWER_SUSPEN | USB_POWER_SUSP;		//由控制器决定挂起状态
		
	connect_state = USBD_CDC_CONNECT_DISCONNECTED;
}


/********************************************************************************************************
**函数信息 ：usb_reset_handle(void)
**功能描述 ：USB复位
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/

void usb_reset_handle(void)
{
    uint32_t val =0;
    USB->rTOP  |= USB_TOP_RESET;//reset usb
    USB->rTOP  &= ~USB_TOP_RESET;
    val = USB->rTOP;
    usb_state = USB_STATE_DEFAULT;
		(void)val;
}

/********************************************************************************************************
**函数信息 ：usb_suspend_handle(void)
**功能描述 ：USB挂起操作
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_suspend_handle(void)
{
    uint32_t val;
    if(usb_state > USB_STATE_ADDRESS)//note it
    {
        USB->rPOWER = USB_POWER_SUSPEN; //enter suspend state, will shutdown phy clock
        val = USB->rTOP;

        /***************JUST FOR RESUME TEST START*******************/
        usb_delay1ms(10000);//10s

        usb_wakeup_handle();

        /***************JUST FOR RESUME TEST END*********************/
        usb_state = USB_STATE_SUSPENDED;
				
    }
		(void)val;
}
/********************************************************************************************************
**函数信息 ：usb_resume_handle(void)
**功能描述 ：USB恢复工作
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_resume_handle(void)
{
    uint32_t val;
    //	val = USB->rTOP;
    //	#ifdef printf
    //	printf("USB got resume 0x%lx\r\n",val);
    //	#endif
		(void)val;
}
/********************************************************************************************************
**函数信息 ：usb_wakeup_handle(void)
**功能描述 ：USB唤醒
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_wakeup_handle(void)
{
    uint32_t val;
    val = USB->rTOP;

    USB->rPOWER = USB_POWER_SUSPEN | USB_POWER_SUSP; //进入工作模式
    USB->rPOWER = USB_POWER_WKUP | USB_POWER_SUSPEN | USB_POWER_SUSP; //唤醒
    usb_delay1ms(3);											//3ms wait bus wakeup

    USB->rPOWER = USB_POWER_SUSPEN | USB_POWER_SUSP; //进入工作模式
    val = USB->rTOP;
    usb_state = USB_STATE_CONFIGURED;
	(void)val;
}

/*-------------------------(C) COPYRIGHT 2016 HOLOCENE ----------------------*/
