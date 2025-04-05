/**
  ******************************************************************************
  * @file    __TFTLCD_H.h
  * @author  
  * @brief   
  ******************************************************************************
  
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LCD43M_H
#define __LCD43M_H

#ifdef __cplusplus
 extern "C" {
#endif 
#include "py/obj.h"
#include "modtftlcd.h"
extern const mp_obj_type_t tftlcd_lcd43m_type;

/* Includes ------------------------------------------------------------------*/  

#ifndef LCD43M_REG
#define LCD43M_REG	(*(volatile uint16_t *)(0x6C000000))
#define LCD43M_RAM	(*(volatile uint16_t *)(0x6C000002))
#endif

#define WRAMCMD					0X2C00
#define SETXCMD					0X2A00
#define SETYCMD					0X2B00

//扫描方向定义
#define L2R_U2D  0 		//从左到右,从上到下
#define L2R_D2U  1 		//从左到右,从下到上
#define R2L_U2D  2 		//从右到左,从上到下
#define R2L_D2U  3 		//从右到左,从下到上

#define U2D_L2R  4 		//从上到下,从左到右
#define U2D_R2L  5 		//从上到下,从右到左
#define D2U_L2R  6 		//从下到上,从左到右
#define D2U_R2L  7		//从下到上,从右到左	 

#define DFT_SCAN_DIR  L2R_U2D  //默认的扫描方向

extern Graphics_Display lcd43_glcd;

extern void lcd43m_init();
extern void LCD_Display_Dir(uint8_t dir);
extern void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);
extern void LCD_Clear(uint16_t color);
extern void LCD_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height);

extern void LCD43M_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color);
extern void LCD43M_Full(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height,uint16_t *color);
extern uint16_t LCD43M_ReadPoint(uint16_t x , uint16_t y);
extern void LCD43M_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
extern void lcd43m_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color);
extern void LCD43M_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);


/**
  * @}
  */    
#ifdef __cplusplus
}
#endif

#endif /* __TFTLCD_H */

