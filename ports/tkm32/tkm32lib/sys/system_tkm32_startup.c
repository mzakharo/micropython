/**
  ******************************************************************************
  * @file    system_stm32f4xx.c
  * @author  MCD Application Team
  * @version V2.6.1
  * @date    14-February-2017
  * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32f4xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
  */

#include "HAL_conf.h"
#include "sys.h"

/******************************************************************************/

//uint32_t SystemCoreClock = 16000000;
//const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
//const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};


/********************************************************************************************************
**函数信息 ：BootLoadClkInit()                   
**功能描述 ：
**输入参数 ：
**输出参数 ：无
********************************************************************************************************/
#define RCC_DELAY 100000
uint8_t clkStatus = 0;
void BootLoadClkInit(int a)
{
    int i;
    int overTime;
    uint32_t tempReg;
    if(((RCC->CR>>1) & 0x01)==0x00)//使能内部时钟
    {
        RCC->CR |= 1<<0;
        overTime = 0;
        while(((RCC->CR>>1) & 0x01)==0x00)
        {
            overTime++;
            if(overTime > RCC_DELAY)
            {
                clkStatus |= 0x1;
                break;
            }
        }
    }
    if(((RCC->CR>>17) & 0x01)==0x00)//使能外部时钟
    {
        RCC->CR |= 1<<16;
        i = 1000;while(i--);
        overTime = 0;
        while(((RCC->CR>>17) & 0x01)==0x00)
        {
            overTime++;
            if(overTime > RCC_DELAY)
            {
                clkStatus |= 0x2;
                break;
            }
        }
    }
    
    if((RCC->CR>>17)&0x1)
    {
      tempReg = RCC->CFGR;
      tempReg &= ~0xf;
      tempReg |= 0x01;
      RCC->CFGR = tempReg;
      overTime = 0;
      while((RCC->CFGR & 0xf) != 0x5) //HSE做系统时钟
      {
        overTime++;
        if(overTime > RCC_DELAY)
        {
            clkStatus |= 0x8;
            break;
        }
      }
    }
    else
    {
      tempReg = RCC->CFGR;
      tempReg &= ~0xf;
      tempReg |= 0x00;
      RCC->CFGR = tempReg;
      overTime = 0;
      while((RCC->CFGR & 0xf) != 0x0) //HSI做系统时钟
      {
        overTime++;
        if(overTime > RCC_DELAY)
        {
            clkStatus |= 0x10;
            break;
        }
      }
    }
    RCC->CR &= ~(1<< 24);//PLL失能
    RCC->PLLCFGR &= 0xFFBFE000;
    RCC->CFGR &= ~(1<<16);//hse 不分频做pll源

		RCC->PLLCFGR |= 1<<13;
    RCC->PLLCFGR |= 1<<22;//sele hse
    RCC->PLLCFGR |= (a<<6);//4*12 = 48M
    
    RCC->CR |= 1<< 24;//PLL使能
    overTime = 0;
    while(((RCC->CR>>25) & 0x01)==0x00)//等待PLL ready
    {
        overTime++;
        if(overTime > RCC_DELAY)
        {
            clkStatus |= 4;
            break;
        }
    }
    if((clkStatus&0x04)==0)//PLL ready 系统时钟切换到PLL
    {
        RCC->CFGR &= 0xfffffff0;
        RCC->CFGR |= 0x2;
        overTime = 0;
        while(((RCC->CFGR) & 0x0f)!=0x0a)//等待sys ready
        {
            overTime++;
            if(overTime > RCC_DELAY)
            {
                clkStatus |= 0x80;
                break;
            }
        }
    }
}

void  AI_Responder_enable_BOOT(void)
{	
  AI_Responder->ADDR1= 0x70807030;
  AI_Responder->CCR &= ~(0x3<<3);
  AI_Responder->CCR |= 1;
  while((AI_Responder->SR & 0x3) != 2);
}



/********************************************************************************************************
**函数信息 ：UartInit(UART_TypeDef* UARTx)                      
**功能描述 ：初始化串口
**输入参数 ：UART_TypeDef* UARTx ，选择UART1、UART2、UART3
**输出参数 ：无
********************************************************************************************************/

void UartInit(UART_TypeDef* UARTx,u32 baud)
{
    UART_InitTypeDef       UART_InitStructure;  
    GPIO_InitTypeDef  GPIO_InitStructure;   

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_Pin_9 | GPIO_Pin_10, GPIO_AF8_UART1); //PA9、PA10复用为串口1
    
    UART_InitStructure.UART_BaudRate =  baud; //波特率
    UART_InitStructure.UART_WordLength = UART_WordLength_8b;//数据位
    UART_InitStructure.UART_StopBits = UART_StopBits_1;//停止位
    UART_InitStructure.UART_Parity = UART_Parity_No ;
    UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;//输入输出模式
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None; 	
    UART_Init(UARTx, &UART_InitStructure);
    UART_Cmd(UARTx, ENABLE);  //UART 模块使能

    UART_ClearITPendingBit(UARTx, 0xff); 
    UART_ClearITPendingBit(  UART1, UART_IT_RXIEN);
    UART_ITConfig( UART1, UART_IT_RXIEN, ENABLE );	
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9;   //uart1_tx  pa9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 推免复用输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;  //uart1_rx  pa10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //上拉输入   
    GPIO_Init(GPIOA, &GPIO_InitStructure); 
}
//********************* UART2 设置 *********************//
void Uart2Init(u32 BaudRate)
{

	  UART_InitTypeDef       UART_InitStructure;  
    GPIO_InitTypeDef  GPIO_InitStructure;   

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_UART2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_Pin_2 | GPIO_Pin_3, GPIO_AF_UART_2345); //PA9、PA10复用为串口1
    
    UART_InitStructure.UART_BaudRate =  BaudRate; //波特率
    UART_InitStructure.UART_WordLength = UART_WordLength_8b;//数据位
    UART_InitStructure.UART_StopBits = UART_StopBits_1;//停止位
    UART_InitStructure.UART_Parity = UART_Parity_No ;
    UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;//输入输出模式
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None; 	
    UART_Init(UART2, &UART_InitStructure);
    UART_Cmd(UART2, ENABLE);  //UART 模块使能

    UART_ClearITPendingBit(UART2, 0xff); 
    UART_ClearITPendingBit(  UART2, UART_IT_RXIEN);
    UART_ITConfig( UART2, UART_IT_RXIEN, ENABLE );	
		
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2;   //uart1_tx  pa9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 推免复用输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;  //uart1_rx  pa10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //上拉输入   
    GPIO_Init(GPIOA, &GPIO_InitStructure); 
		
		
}

void send2_data(uint8_t data)
{
	while((UART2->CSR &0x1) == 0);
	UART2->TDR = data; 
}
/********************************************************************************************************
**函数信息 ：uart_send(UART_TypeDef* UARTx,char c)                    
**功能描述 ：串口发送字节
**输入参数 ：UART_TypeDef* UARTx ，选择UART1、UART2、UART3
**输入参数 ：char c ,串口发送的字节
**输出参数 ：无
********************************************************************************************************/

void uart_send(UART_TypeDef* UARTx,char c)
{
	unsigned int i  = 20000;
 	
	
	UART_SendData(UARTx,(uint16_t)c);  
	
	while(1)
	{
        
        if(UART_GetFlagStatus(UARTx, UART_IT_TXIEN))
        {
            //UART_GetITStatus(UARTx, UART_IT_TXIEN);
            break;
        }
        if(i-- == 0)
        {
            break;
        }
	}
}

void SenStr(UART_TypeDef* UARTx,char *str)
{
	while(*str != '\0'){
		uart_send(UARTx,*str);
		str++;
	}
}
unsigned char HexToChar(unsigned char bChar)
{
	if((bChar>=0x30)&&(bChar<=0x39))
	{
		bChar -= 0x30;
	}
	else if((bChar>=0x41)&&(bChar<=0x46)) // Capital
	{
		bChar -= 0x37;
	}
	else if((bChar>=0x61)&&(bChar<=0x66)) //littlecase
	{
		bChar -= 0x57;
	}
	else 
	{
		bChar = 0xff;
	}
	return bChar;
}
/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting, vector table location and External memory
  *         configuration.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
	//uint32_t  i;

	RemapVtorTable();//ROM disable!!!
	
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
	
	AI_Responder_enable_BOOT();
	RCC->AHB1ENR |= 1<<13;//bkp clk,enable sram

	SystemClk_HSEInit(RCC_PLLMul_20);//启动PLL时钟，12MHz*20=240MHz

	RCC->CFGR &= ~(0x07 << 13);
	RCC->CFGR |= (0x04 << 13); //PCLK2 MAX=120Mhz

	RCC->CFGR &= ~(0x07 << 10);
	RCC->CFGR |= (0x05 << 10); //PCLK2 MAX=60Mhz

}

/*****************************END OF FILE****/
