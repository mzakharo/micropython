#include "usb_process.h"
#include "usb_pwr.h"


//volatile uint8_t readWriteFileBuf[512];

void USB_IO_Config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;   
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
		GPIO_PinAFConfig(GPIOA, GPIO_Pin_11 | GPIO_Pin_12, GPIO_AF_USB); //PA11、PA12复用为USB
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11|GPIO_Pin_12;   //PA11&PA12
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);  

    RCC->CFGR |= 4<<17;//USB 时钟，主频的4+1次分频，USB一定要工作在48MHz下，所是240HMz/5=48MHz
		RCC->APB1ENR |= 1<<28;//USB CLOCK ENABLE;
}


#if 0
/********************************************************************************************************
**函数信息 ：void USB_IRQHandler(void)         
**功能描述 ：
**输入参数 ：
**输出参数 ：无
********************************************************************************************************/
void USB_IRQHandler(void)
{
    USB_Istr();
}
#endif
