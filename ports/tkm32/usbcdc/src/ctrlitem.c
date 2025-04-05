/**
******************************************************************************
* @file    ctrlitem.c
* @author  IC Applications Department
* @version  V0.8
* @date  2019_08_02
* @brief   This file provides all the ctrlitem firmware functions.
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
#include "usbprop.h"

volatile uint8_t    *p_ctrl_buf = 0;
volatile uint8_t    ctrl_count ;

//=========================== Common Para  =============================
static uint8_t    DeviceDesc[] =
{
    0x12,   /* bLength */
    0x01,     /* bDescriptorType */
    0x10,
    0x01,   /* bcdUSB = 2.00 */
    0x02,   /* bDeviceClass: CDC */
    0x00,   /* bDeviceSubClass */
    0x00,   /* bDeviceProtocol */
    0x40,   /* bMaxPacketSize0 */
    0x55,
    0xF0,   /* idVendor = 0xf055 */
    0x00,
    0x98,   /* idProduct = 0x9800 */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,              /* Index of string descriptor describing manufacturer */
    2,              /* Index of string descriptor describing product */
    3,              /* Index of string descriptor describing the device's serial number */
    0x01    /* bNumConfigurations */
} ;

static uint8_t    ConfigDesc[] =   /*Configuration Descriptor*/
{
    /*Configuation Descriptor*/
    0x09,   /* bLength: Configuation Descriptor size */
    0x02,      /* bDescriptorType: Configuration */
    0x43,       /* wTotalLength:no of returned bytes */
    0x00,
    0x02,   /* bNumInterfaces: 2 interface */
    0x01,   /* bConfigurationValue: Configuration value */
    0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
    0xC0,   /* bmAttributes: self powered */
    0x32,   /* MaxPower 0 mA */
    /*Interface Descriptor*/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,  /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x01,   /* bNumEndpoints: One endpoints used */
    0x02,   /* bInterfaceClass: Communication Interface Class */
    0x02,   /* bInterfaceSubClass: Abstract Control Model */
    0x01,   /* bInterfaceProtocol: Common AT commands */
    0x00,   /* iInterface: */
    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,
    /*Call Managment Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x00,   /* bDataInterface: 1 */
    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 2 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x82,   /* bEndpointAddress: (IN2) */
    0x03,   /* bmAttributes: Interrupt */
    64,      /* wMaxPacketSize: */
    0x00,
    0xFF,   /* bInterval: */
    /*Data class interface descriptor*/
    0x09,   /* bLength: Endpoint Descriptor size */
    0x04,  /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0x0A,   /* bInterfaceClass: CDC */
    0x00,   /* bInterfaceSubClass: */
    0x00,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */
    /*Endpoint 3 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x03,   /* bEndpointAddress: (OUT3) */
    0x02,   /* bmAttributes: Bulk */
    64,             /* wMaxPacketSize: */
    0x00,
    0x00,   /* bInterval: ignore for Bulk transfer */
    /*Endpoint 1 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x81,   /* bEndpointAddress: (IN1) */
    0x02,   /* bmAttributes: Bulk */
    64,             /* wMaxPacketSize: */
    0x00,
    0x00    /* bInterval */
};

//语言ID，声明接下来字符串描述符是哪种语言
static uint8_t    Str0Desc[] = {0x04/*bLength*/, 0x03/*bDescrType=STRING*/, 0x09, 0x04 };
//字符串描述符1
static uint8_t    Str1Desc[] =
{
    24,          /* bLength */
    0x03,        /* bDescriptorType */
    /* Product name: "TK499 Virtual COM Port" */
    'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'P', 0, 'y', 0, 't', 0,
    'h', 0, 'o', 0, 'n',0
};
//字符串描述符2
static uint8_t    Str2Desc[] =
{
    54,          /* bLength */
    0x03,        /* bDescriptorType */
    /* Product name: "TK499 Virtual COM Port" */
    'P', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 
		'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0,
		'C', 0, 'o', 0, 'm', 0, 'm', 0, ' ', 0, 
		'P', 0, 'o', 0, 'r', 0, 't', 0, ' ', 0
};
//字符串描述符3
static uint8_t    Str3Desc[] =
{
    26,           /* bLength */
    0x03,         /* bDescriptorType */
    'T', 0, 'K', 0, 'M', 0, '3', 0, '2', 0
};

#define DeviceDescLen       (sizeof(DeviceDesc))
#define ConfigDescLen       (sizeof(ConfigDesc))
#define Str0DescLen         (sizeof(Str0Desc))
#define Str1DescLen         (sizeof(Str1Desc))
#define Str2DescLen         (sizeof(Str2Desc))
#define Str3DescLen         (sizeof(Str3Desc))
static uint8_t    StrDescLenGroup[] = { Str0DescLen, Str1DescLen, Str2DescLen, Str3DescLen } ;
//=========================== Each Request =============================
/********************************************************************************************************
**函数信息 ：Chap9_SetConfiguration( void)
**功能描述 ：标准请求返回，设置配置，此处为状态阶段，返回空包
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_SetConfiguration( void)								//0x09
{
    if(req_dir == USB_REQ_DIR_OUT)
    {
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_SetConfiguration( void)
**功能描述 ：标准请求返回，设置地址，此处为状态阶段，返回空包
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_SetAddress( void)	    							//0x05
{
    if(req_dir == USB_REQ_DIR_OUT)
    {
        //usb_state = USB_STATE_ADDRESS;
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Get_Desc_Prepare( void)
**功能描述 ：接收获取描述符请求时，将对应描述符地址返回给控制传输指针*p_ctrl_buf
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Get_Desc_Prepare( void)
{
    uint32_t type = wValue & 0xFF00;
    uint32_t index = wValue & 0x00FF;
    switch(type)
    {
        case    DEVICE_DESC_TYPE :			//获取设备述符
            {
                if( wLength > DeviceDescLen )   wLength = DeviceDescLen ;
                p_ctrl_buf = DeviceDesc ;
                break ;
            }
        case    CONFIGURATION_DESC_TYPE :	//获取配置描述符
            {
                if( wLength > ConfigDescLen )   wLength = ConfigDescLen ;
                p_ctrl_buf = ConfigDesc ;
                break ;
            }
        case    STRING_DESC_TYPE :			//获取字符串描述符
            {
                if( index < 4 )		  			//此处对应字符串描述符个数,可自己定义
                {
                    if( wLength > StrDescLenGroup[index] )
                        wLength = StrDescLenGroup[index] ;
                    switch(index)
                    {
                        case 0:
                            p_ctrl_buf = Str0Desc;
                            break;
                        case 1:
                            p_ctrl_buf = Str1Desc;
                            break;
                        case 2:
                            p_ctrl_buf = Str2Desc;
                            break;
                        case 3:
                            p_ctrl_buf = Str3Desc;
                            break;
                    }
                }
                else
                {
                    wLength = 0 ;
                }
                break ;
            }
        default :
            {
                wLength = 0 ;
                break ;
            }
    }
}
/********************************************************************************************************
**函数信息 ：Chap9_GetDescriptor( void)
**功能描述 ：标准请求返回，获取描述符
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_GetDescriptor( void)     							//0x06
{
    static uint16_t    ctrl_buf_ix = 0;

    switch( usb_running_ctrl_state )
    {
        case    USB_CTRL_SETUP :								//控制传输setup阶段
            {
                Get_Desc_Prepare() ;								//获取描述符信息
                usb_running_ctrl_state = USB_CTRL_IN ;
                ctrl_buf_ix = 0;
                break ;
            }
        case    USB_CTRL_IN :
            {
                uint16_t    count = 0;
                uint16_t    ix = 0 ;

                if(wLength > 0)										//数据分批发送
                {
                    count = MIN(EP0_MAX_PACKAGE_SIZE, wLength);
                    while(USB->rEP0_CTRL & 0x80);					//等待上次发送完成
                    for(ix = 0; ix < count; ix++)
                    {
                        USB->rEP0_FIFO = p_ctrl_buf[ctrl_buf_ix];

                        ctrl_buf_ix++;
                    }
                    USB->rEP0_CTRL = count | 0x80;
                    wLength -= count;
                }
                if( wLength == 0)
                {
                    if(count == EP0_MAX_PACKAGE_SIZE)					//发送数据总数为包最大值的整数倍时在末尾帧发送空数据
                    {
                        EP0_Send_Empty_Packet();
                    }
                    else
                    {
                    }
                    usb_running_ctrl_state = USB_IDLE;				//数据发送完成
                    ctrl_buf_ix = 0;
                }
                break ;
            }
        default :
            {
                break ;
            }
    }
    usb_state = USB_STATE_CONFIGURED;
}

/********************************************************************************************************
**函数信息 ：Chap9_GetStatus( void)
**功能描述 ：标准请求返回，获取状态
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_GetStatus( void)         					// 0x00
{
    switch( usb_running_ctrl_state )
    {
        case    USB_CTRL_SETUP :
            {
                usb_running_ctrl_state = USB_CTRL_IN;
                break;
            }
        case USB_CTRL_IN:
            {
                if(req_dir == USB_REQ_DIR_IN)
                {
                    if(req_recip == USB_REQ_RECIP_EP)			//获取端点状态
                    {
                        uint8_t ep = wIndex & 0x0F;

                        USB->rEP0_FIFO = USB_EP_STALL[ep];	//端点是否关闭
                        USB->rEP0_FIFO = 0x00;
                    }
                    else if(req_recip == USB_REQ_RECIP_DEV)	//获取设备状态
                    {
                        USB->rEP0_FIFO = 0x02;				//bit0设备供电模式：0总线供电，1自给供电；bit1设备远程唤醒:1支持，0不支持
                        USB->rEP0_FIFO = 0x00;
                    }
                    else
                    {
                        USB->rEP0_FIFO = 0x00;
                        USB->rEP0_FIFO = 0x00;
                    }
                    USB->rEP0_CTRL = 0x82;
                }
                else										//获取接口状态，保留
                {

                }
                usb_running_ctrl_state = USB_IDLE;
                break;
            }
        default :
            {
                //            usb_running_ctrl_state = USB_IDLE ;
                break ;
            }
    }
}
/********************************************************************************************************
**函数信息 ：Chap9_ClearFeature( void)
**功能描述 ：标准请求返回，清除特性
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_ClearFeature(void)      							// 0x01
{
    switch(usb_running_ctrl_state)
    {
        case    USB_CTRL_SETUP :

            if(req_dir == USB_REQ_DIR_OUT)
            {
                uint8_t ix;
                if((req_recip == USB_REQ_RECIP_EP) && (wValue == ENDPOINT_HALT))				//ENDPOINT_HALT
                {
                    USB->rEP_HALT = 0x00;
                    for(ix = 0; ix < 5; ix++)
                    {
                        USB_EP_STALL[ix] = 0;
                        USB_SEND_OPEN_STALL[ix] = 0;
                    }
                }
                else if((req_recip == USB_REQ_RECIP_DEV) && (wValue == DEVICE_REMOTE_WAKEUP)) //DEVICE_REMOTE_WAKEUP
                {

                }
            }
            else
            {

            }
            usb_running_ctrl_state = USB_CTRL_STATUS;
            break;

        default:
            //		usb_running_ctrl_state = USB_IDLE;
            break;

    }
}
/********************************************************************************************************
**函数信息 ：MLsup_StallEP0( void)
**功能描述 ：保留
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void MLsup_StallEP0( void)          							// reserve      0x02
{
    if(req_dir == USB_REQ_DIR_IN)
    {
        EP0_Send_Empty_Packet();
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：MLsup_StallEP1( void)
**功能描述 ：保留
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void MLsup_StallEP1( void)          							// reserve      0x04
{
    if(req_dir == USB_REQ_DIR_IN)
    {
        EP0_Send_Empty_Packet();
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_SetFeature( void)
**功能描述 ：标准请求，设置特性，返回数据度为0
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_SetFeature( void)	    							// 0x03
{
    if(req_dir == USB_REQ_DIR_OUT)
    {
        //		EP0_Send_Empty_Packet();
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_SetDescriptor( void)
**功能描述 ：标准请求，设置描述符，返回数据度为0
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_SetDescriptor( void)     							//0x07
{
    if(req_dir == USB_REQ_DIR_OUT)
    {
        //		EP0_Send_Empty_Packet();
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_GetConfiguration( void)
**功能描述 ：标准请求，设置配置描述符值，返回可选配置描述符值
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_GetConfiguration( void)								//0x08
{
    if(req_dir == USB_REQ_DIR_IN)
    {
        //		EP0_Send_Empty_Packet();
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_GetInterface( void)
**功能描述 ：标准请求，获取接口，返回可选接口
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_GetInterface( void)									//0x0a
{
    if(req_dir == USB_REQ_DIR_IN)
    {
        //		EP0_Send_Empty_Packet();
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}
/********************************************************************************************************
**函数信息 ：Chap9_SetInterface( void)
**功能描述 ：标准请求，设置接口，返回空数据
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void Chap9_SetInterface( void)									//0x0b
{
    if(req_dir == USB_REQ_DIR_OUT)
    {
        usb_running_ctrl_state = USB_CTRL_STATUS;
    }
    else
    {

    }
}

/********************************************************************************************************
**函数信息 ：EP0_Send_Empty_Packet()
**功能描述 ：端点0发送空数据包
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void EP0_Send_Empty_Packet()
{
    while(USB->rEP0_CTRL & 0x80);
    USB->rEP0_CTRL = 0x80;
}

// =============================== usb IN OUT DATA ==========================


/********************************************************************************************************
**函数信息 ：usb_in_handle( void)
**功能描述 ：IN包处理
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_in_handle( void)       								// IN Request Received
{
    if( usb_ep_flag == USB_EP0 )
    {
        ctrl_in_token_handle() ;
    }
    else
    {
      EP1_USB_IN_Data();
    }
}
/********************************************************************************************************
**函数信息 ：usb_in_handle( void)
**功能描述 ：OUT包处理
**输入参数 ：
**输出参数 ：
**备    注 ：
********************************************************************************************************/
void usb_out_handle( void)      								// OUT Packet Received
{
    if( usb_ep_flag == USB_EP0 )
    {
        ctrl_out_handle();
    }
    else
    {
        EP3_USB_OUT_Data();
    }
}

/**
* @}
*/

/**
* @}
*/

/**
* @}
*/

/*-------------------------(C) COPYRIGHT 2016 HOLOCENE ----------------------*/
