
/* Define to prevent recursive inclusion -------------------------------------*/
// #ifndef __JPEG_UTILS_H
// #define __JPEG_UTILS_H
#ifndef MICROPY_INCLUDED_STM32_JPEG_UTILS_H
#define MICROPY_INCLUDED_STM32_JPEG_UTILS_H

#include "py/obj.h"
/* Includes ------------------------------------------------------------------*/
#include "jpeg_utils_conf.h"

#if MICROPY_ENABLE_JPEG_UTILS
//根据自己的需求,修改JPEG_BYTES_PER_PIXEL的值
#define JPEG_BYTES_PER_PIXEL 2		//输出像素大小(单位:字节)
									//2:RGB565
									//3:RGB888
									//4:ARGB8888
															
									
//////////////////////////////////////////////////////////////////////////////////  									
#if	JPEG_BYTES_PER_PIXEL>2
#define JPEG_RED_OFFSET      16		//Offset of the RED color in a pixel        
#define JPEG_GREEN_OFFSET    8		//Offset of the GREEN color in a pixel          
#define JPEG_BLUE_OFFSET     0		//Offset of the BLUE color in a pixel        
#define JPEG_ALPHA_OFFSET    24 	//Offset of the Transparency Alpha in a pixel
#endif


//函数返回值定义
#define JPEG_OK		0X00	//正常
#define JPEG_ERROR	0X01	//错误

//JPEG MCU到RGB转换需要使用的部分参数结构体
typedef struct __JPEG_MCU_RGB_ConvertorTypeDef
{
	uint32_t ColorSpace;
	uint32_t ChromaSubsampling; 
	uint32_t ImageWidth;
	uint32_t ImageHeight;
	uint32_t ImageSize_Bytes; 
	uint32_t LineOffset; 
	uint32_t H_factor;
	uint32_t V_factor; 
	uint32_t WidthExtend;
	uint32_t ScaledWidth; 
	uint32_t MCU_Total_Nb; 
}JPEG_MCU_RGB_ConvertorTypeDef;


#define YCBCR_420_BLOCK_SIZE		384				//YCbCr 4:2:0 MCU : 4 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr   
#define YCBCR_422_BLOCK_SIZE 		256				//YCbCr 4:2:2 MCU : 2 8x8 blocks of Y + 1 8x8 block of Cb + 1 8x8 block of Cr   
#define YCBCR_444_BLOCK_SIZE 		192				//YCbCr 4:4:4 MCU : 1 8x8 block of Y + 1 8x8 block of Cb + 1 8x8 block of Cr   
#define GRAY_444_BLOCK_SIZE 		64				//GrayScale MCU : 1 8x8 block of Y 
#define CMYK_444_BLOCK_SIZE 		256				//CMYK MCU : 1 8x8 blocks of Cyan + 1 8x8 block Magenta + 1 8x8 block of Yellow and 1 8x8 block of BlacK


typedef uint32_t (* JPEG_YCbCrToRGB_Convert_Function)(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);


uint32_t JPEG_MCU_YCbCr420_ARGB_ConvertBlocks(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);
uint32_t JPEG_MCU_YCbCr422_ARGB_ConvertBlocks(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);
uint32_t JPEG_MCU_YCbCr444_ARGB_ConvertBlocks(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);
uint32_t JPEG_MCU_Gray_ARGB_ConvertBlocks(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);
uint32_t JPEG_MCU_YCCK_ARGB_ConvertBlocks(uint8_t *pInBuffer,uint8_t *pOutBuffer,uint32_t BlockIndex,uint32_t DataCount);
uint8_t JPEG_GetDecodeColorConvertFunc(JPEG_ConfTypeDef *pJpegInfo, JPEG_YCbCrToRGB_Convert_Function *pFunction, uint32_t *ImageNbMCUs);

#endif

#endif /* __JPEG_UTILS_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

