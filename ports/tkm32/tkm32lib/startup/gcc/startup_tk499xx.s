/**
  ******************************************************************************
  * @file      startup_tk499xx.s
  * @author    MCD Application Team
  * @brief     tk499xx Devices vector table for GCC based toolchains. 
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Branches to main in the C library (which eventually
  *				   - Enable floating-point operations
  *                  calls main()).
  *            After Reset the Cortex-M4 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 01Studio http://bbs.01studio.org/.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
    
  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section. 
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */  
.word  _sdata
/* end address for the .data section. defined in linker script */
.word  _edata
/* start address for the .bss section. defined in linker script */
.word  _sbss
/* end address for the .bss section. defined in linker script */
.word  _ebss
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called. 
 * @param  None
 * @retval : None
*/

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler: 
  ldr   sp, =_estack       /* set stack pointer */
 
/* Copy the data segment initializers from flash to SRAM */  
  movs  r1, #0
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, =_sidata
  ldr  r3, [r3, r1]
  str  r3, [r0, r1]
  adds  r1, r1, #4
    
LoopCopyDataInit:
  ldr  r0, =_sdata
  ldr  r3, =_edata
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyDataInit
  ldr  r2, =_sbss
  b  LoopFillZerobss
/* Zero fill the bss segment. */  
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4
    
LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
  bcc  FillZerobss
/* Call the clock system intitialization function.*/
  bl  SystemInit
	
/* Call static constructors */
    bl __libc_init_array
/*		
  ldr     r0, =0xe000ed88
  ldr     r1,[r0]
  orr     r1,r1,#(0x0f << 20)
  str     r1,[r0]
*/
/* Call the applications entry point.*/
  bl  main
  bx  lr    
.size  Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an 
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None     
 * @retval None       
*/
    .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
* 
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler							 /*Reset Handler*/
  .word  NMI_Handler							 /*NMI Handler*/
  .word	 HardFault_Handler						 /*Hard Fault Handler*/
  .word  MemManage_Handler						 /*MPU Fault Handler*/
  .word  BusFault_Handler						 /*Bus Fault Handler*/
  .word  UsageFault_Handler						 /*Usage Fault Handler*/
  .word  0                                       /**/
  .word  0                                       /**/
  .word  0                                       /**/
  .word  0                                       /**/
  .word  SVC_Handler							 /*SVCall Handler*/
  .word  DebugMon_Handler						 /*Debug Monitor Handler*/
  .word  0                                       /**/
  .word  PendSV_Handler							 /*PendSV Handler*/
  .word  SysTick_Handler						 /*SysTick Handler*/
  
  /* External Interrupts */
  .word  WWDG_IRQHandler                  /*WWDG 		*/    
  .word  0                                /* 			*/
  .word  TAMPER_IRQHandler                /*TAMPER  	*/   
  .word  RTC_IRQHandler                   /*RTC  		*/   
  .word  0                                /* 			*/
  .word  RCC_IRQHandler                   /*RCC 		*/   
  .word  EXTI0_IRQHandler                 /*EXTI0  		*/   
  .word  EXTI1_IRQHandler                 /*EXTI1     	*/
  .word  EXTI2_IRQHandler                 /*EXTI2     	*/
  .word  EXTI3_IRQHandler                 /*EXTI3     	*/
  .word  EXTI4_IRQHandler                 /*EXTI4    	*/
  .word  DMA1_Channel1_IRQHandler         /*DMA1 Channel 1 */
  .word  DMA1_Channel2_IRQHandler         /*DMA1 Channel 2 */
  .word  DMA1_Channel3_IRQHandler         /*DMA1 Channel 3 */
  .word  DMA1_Channel4_IRQHandler         /*DMA1 Channel 4 */
  .word  DMA1_Channel5_IRQHandler         /*DMA1 Channel 5 */
  .word  DMA1_Channel6_IRQHandler         /*DMA1 Channel 6 */
  .word  DMA1_Channel7_IRQHandler         /*DMA1 Channel 7 */
  .word  ADC1_IRQHandler                  /*ADC1           */
  .word  CAN1_IRQHandler         	  	  /*CAN1           */
  .word   0                               /*                */
  .word   0                               /*                */
  .word   0                               /*                */
  .word  EXTI9_5_IRQHandler               /*EXTI9_5         */
  .word  TIM1_BRK_IRQHandler              /*TIM1_BRK        */
  .word  TIM1_UP_IRQHandler               /*TIM1_UP         */
  .word  TIM1_TRG_COM_IRQHandler          /*TIM1_TRG_COM    */
  .word  TIM1_CC_IRQHandler               /*TIM1_CC         */      
  .word  TIM3_IRQHandler                  /*TIM3            */
  .word  TIM4_IRQHandler                  /*TIM4            */
  .word  TIM5_IRQHandler                  /*TIM5            */
  .word  TIM6_IRQHandler                  /*TIM6            */ 
  .word  TIM7_IRQHandler                  /*TIM7            */
  .word  I2C1_IRQHandler                  /*I2C1            */
  .word  I2C2_IRQHandler                  /*I2C2            */
  .word  SPI1_IRQHandler                  /*SPI1            */
  .word  SPI2_IRQHandler                  /*SPI2            */
  .word  UART1_IRQHandler                 /*UART1           */
  .word  UART2_IRQHandler                 /*UART2           */
  .word  UART3_IRQHandler                 /*UART3           */
  .word  EXTI15_10_IRQHandler             /*EXTI15_10       */     
  .word  RTCAlarm_IRQHandler              /*RTC_ALARM       */      
  .word  USBAwake_IRQHandler              /*USBAwake        */     
  .word  TIM2_BRK_IRQHandler              /*TIM2_BRK        */  
  .word  TIM2_UP_IRQHandler               /*TIM2_UP         */ 
  .word  TIM2_TRG_COM_IRQHandler          /*TIM2_TRG_COM    */
  .word  TIM2_CC_IRQHandler               /*TIM2_CC         */                    
  .word  DMA1_Channel8_IRQHandler         /*DMA1            */  
  .word  TK80_IRQHandler                  /*TK80            */           
  .word  SDIO1_IRQHandler                 /*SDIO1           */
  .word  SDIO2_IRQHandler                 /*SDIO2           */
  .word  SPI3_IRQHandler                  /*SPI3            */
  .word  UART4_IRQHandler                 /*UART4           */        
  .word  UART5_IRQHandler                 /*UART5           */     
  .word  0                                /*                */
  .word  TIM8_IRQHandler                  /*TIM8            */
  .word  DMA2_Channel1_IRQHandler         /*DMA2 Channel 1  */  
  .word  DMA2_Channel2_IRQHandler         /*DMA2 Channel 2  */  
  .word  DMA2_Channel3_IRQHandler         /*DMA2 Channel 3  */  
  .word  DMA2_Channel4_IRQHandler         /*DMA2 Channel 4  */  
  .word  DMA2_Channel5_IRQHandler         /*DMA2 Channel 5  */  
  .word  TIM9_IRQHandler                  /*TIM9            */
  .word  TIM10_IRQHandler                 /*TIM10           */
  .word  CAN2_IRQHandler                  /*CAN2            */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  USB_IRQHandler                   /*USB             */
  .word  DMA2_Channel6_IRQHandler         /*DMA2 Channel 6  */  
  .word  DMA2_Channel7_IRQHandler         /*DMA2 Channel 7  */  
  .word  DMA2_Channel8_IRQHandler         /*DMA2 Channel 8  */ 
  .word   0                               /*                */
  .word  I2C3_IRQHandler                  /*I2C3            */
  .word  I2C4_IRQHandler                  /*I2C4            */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  0                          	  /*                */
  .word  FPU_IRQHandler                   /*FPU             */
  .word  0                                /*                */
  .word  0                                /*                */
  .word  SPI4_IRQHandler                  /*SPI4            */
  .word  0                                /*                */
  .word  TOUCHPAD_IRQHandler              /* TCHPAD        */
  .word  QSPI_IRQHandler                  /*QSPI            */
  .word  LTDC_IRQHandler               	  /*LCD-TFT          */
  .word  0                                /*                */
  .word  I2S1_IRQHandler                  /*I2S1            */
                    
/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler. 
* As they are weak aliases, any function with the same name will override 
* this definition.
* 
*******************************************************************************/
   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler
  
   .weak      HardFault_Handler
   .thumb_set HardFault_Handler,Default_Handler
  
   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,Default_Handler
  
   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,Default_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,Default_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler              
  
   .weak      WWDG_IRQHandler                   
   .thumb_set WWDG_IRQHandler,Default_Handler      
                  
   .weak      TAMPER_IRQHandler            
   .thumb_set TAMPER_IRQHandler,Default_Handler
            
   .weak      RTC_IRQHandler                  
   .thumb_set RTC_IRQHandler,Default_Handler
            
   .weak      RCC_IRQHandler      
   .thumb_set RCC_IRQHandler,Default_Handler
                  
   .weak      EXTI0_IRQHandler         
   .thumb_set EXTI0_IRQHandler,Default_Handler
                  
   .weak      EXTI1_IRQHandler         
   .thumb_set EXTI1_IRQHandler,Default_Handler
                     
   .weak      EXTI2_IRQHandler         
   .thumb_set EXTI2_IRQHandler,Default_Handler 
                 
   .weak      EXTI3_IRQHandler         
   .thumb_set EXTI3_IRQHandler,Default_Handler
                        
   .weak      EXTI4_IRQHandler         
   .thumb_set EXTI4_IRQHandler,Default_Handler
                  
   .weak      DMA1_Channel1_IRQHandler               
   .thumb_set DMA1_Channel1_IRQHandler,Default_Handler
         
   .weak      DMA1_Channel2_IRQHandler               
   .thumb_set DMA1_Channel2_IRQHandler,Default_Handler
                  
   .weak      DMA1_Channel3_IRQHandler               
   .thumb_set DMA1_Channel3_IRQHandler,Default_Handler
                  
   .weak      DMA1_Channel4_IRQHandler               
   .thumb_set DMA1_Channel4_IRQHandler,Default_Handler 
                 
   .weak      DMA1_Channel5_IRQHandler              
   .thumb_set DMA1_Channel5_IRQHandler,Default_Handler
                  
   .weak      DMA1_Channel6_IRQHandler               
   .thumb_set DMA1_Channel6_IRQHandler,Default_Handler
                  
   .weak      DMA1_Channel7_IRQHandler               
   .thumb_set DMA1_Channel7_IRQHandler,Default_Handler
                  
   .weak      ADC1_IRQHandler      
   .thumb_set ADC1_IRQHandler,Default_Handler

   .weak      CAN1_IRQHandler      
   .thumb_set CAN1_IRQHandler,Default_Handler
            
   .weak      EXTI9_5_IRQHandler   
   .thumb_set EXTI9_5_IRQHandler,Default_Handler
            
   .weak      TIM1_BRK_IRQHandler            
   .thumb_set TIM1_BRK_IRQHandler,Default_Handler
            
   .weak      TIM1_UP_IRQHandler            
   .thumb_set TIM1_UP_IRQHandler,Default_Handler
      
   .weak      TIM1_TRG_COM_IRQHandler      
   .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler
      
   .weak      TIM1_CC_IRQHandler   
   .thumb_set TIM1_CC_IRQHandler,Default_Handler
                  
   .weak      TIM3_IRQHandler            
   .thumb_set TIM3_IRQHandler,Default_Handler
                  
   .weak      TIM4_IRQHandler            
   .thumb_set TIM4_IRQHandler,Default_Handler
                  
   .weak      TIM5_IRQHandler            
   .thumb_set TIM5_IRQHandler,Default_Handler

   .weak      TIM6_IRQHandler            
   .thumb_set TIM6_IRQHandler,Default_Handler

   .weak      TIM7_IRQHandler            
   .thumb_set TIM7_IRQHandler,Default_Handler
                  
   .weak      I2C1_IRQHandler   
   .thumb_set I2C1_IRQHandler,Default_Handler
                     
   .weak      I2C2_IRQHandler   
   .thumb_set I2C2_IRQHandler,Default_Handler
                  
   .weak      SPI1_IRQHandler            
   .thumb_set SPI1_IRQHandler,Default_Handler
                        
   .weak      SPI2_IRQHandler            
   .thumb_set SPI2_IRQHandler,Default_Handler
                  
   .weak      UART1_IRQHandler      
   .thumb_set UART1_IRQHandler,Default_Handler
                     
   .weak      UART2_IRQHandler      
   .thumb_set UART2_IRQHandler,Default_Handler
                                  
   .weak      UART3_IRQHandler      
   .thumb_set UART3_IRQHandler,Default_Handler                                

   .weak      EXTI15_10_IRQHandler               
   .thumb_set EXTI15_10_IRQHandler,Default_Handler
               
   .weak      RTCAlarm_IRQHandler               
   .thumb_set RTCAlarm_IRQHandler,Default_Handler
            
   .weak      USBAwake_IRQHandler         
   .thumb_set USBAwake_IRQHandler,Default_Handler

   .weak      TIM2_BRK_IRQHandler      
   .thumb_set TIM2_BRK_IRQHandler,Default_Handler                                

   .weak      TIM2_UP_IRQHandler      
   .thumb_set TIM2_UP_IRQHandler,Default_Handler                                

   .weak      TIM2_TRG_COM_IRQHandler      
   .thumb_set TIM2_TRG_COM_IRQHandler,Default_Handler                                

   .weak      TIM2_CC_IRQHandler      
   .thumb_set TIM2_CC_IRQHandler,Default_Handler                                           

   .weak      DMA1_Channel8_IRQHandler               
   .thumb_set DMA1_Channel8_IRQHandler,Default_Handler
                     
   .weak      TK80_IRQHandler            
   .thumb_set TK80_IRQHandler,Default_Handler

   .weak      SDIO1_IRQHandler            
   .thumb_set SDIO1_IRQHandler,Default_Handler
                     
   .weak      SDIO2_IRQHandler            
   .thumb_set SDIO2_IRQHandler,Default_Handler
                     
   .weak      SPI3_IRQHandler            
   .thumb_set SPI3_IRQHandler,Default_Handler

   .weak      UART4_IRQHandler      
   .thumb_set UART4_IRQHandler,Default_Handler
                     
   .weak      UART5_IRQHandler      
   .thumb_set UART5_IRQHandler,Default_Handler
                                  
   .weak      TIM8_IRQHandler      
   .thumb_set TIM8_IRQHandler,Default_Handler
                     
   .weak      DMA2_Channel1_IRQHandler               
   .thumb_set DMA2_Channel1_IRQHandler,Default_Handler
               
   .weak      DMA2_Channel2_IRQHandler               
   .thumb_set DMA2_Channel2_IRQHandler,Default_Handler
                  
   .weak      DMA2_Channel3_IRQHandler               
   .thumb_set DMA2_Channel3_IRQHandler,Default_Handler
            
   .weak      DMA2_Channel4_IRQHandler               
   .thumb_set DMA2_Channel4_IRQHandler,Default_Handler
            
   .weak      DMA2_Channel5_IRQHandler               
   .thumb_set DMA2_Channel5_IRQHandler,Default_Handler

   .weak      TIM9_IRQHandler               
   .thumb_set TIM9_IRQHandler,Default_Handler

   .weak      TIM10_IRQHandler               
   .thumb_set TIM10_IRQHandler,Default_Handler

   .weak      CAN2_IRQHandler               
   .thumb_set CAN2_IRQHandler,Default_Handler
            
   .weak      USB_IRQHandler      
   .thumb_set USB_IRQHandler,Default_Handler
                     
   .weak      DMA2_Channel6_IRQHandler               
   .thumb_set DMA2_Channel6_IRQHandler,Default_Handler

   .weak      DMA2_Channel7_IRQHandler               
   .thumb_set DMA2_Channel7_IRQHandler,Default_Handler
                  
   .weak      DMA2_Channel8_IRQHandler               
   .thumb_set DMA2_Channel8_IRQHandler,Default_Handler
                  
   .weak      I2C3_IRQHandler      
   .thumb_set I2C3_IRQHandler,Default_Handler
                        
   .weak      I2C4_IRQHandler   
   .thumb_set I2C4_IRQHandler,Default_Handler
                        
   .weak      FPU_IRQHandler                  
   .thumb_set FPU_IRQHandler,Default_Handler  

   .weak      SPI4_IRQHandler                  
   .thumb_set SPI4_IRQHandler,Default_Handler

   .weak      TOUCHPAD_IRQHandler      
   .thumb_set TOUCHPAD_IRQHandler,Default_Handler
                        
   .weak      QSPI_IRQHandler   
   .thumb_set QSPI_IRQHandler,Default_Handler
                        
   .weak      LTDC_IRQHandler                  
   .thumb_set LTDC_IRQHandler,Default_Handler  

   .weak      I2S1_IRQHandler                  
   .thumb_set I2S1_IRQHandler,Default_Handler

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/		
 
   
   
