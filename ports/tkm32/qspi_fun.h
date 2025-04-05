//#ifndef __QSPI_FUN_H__
//#define __QSPI_FUN_H__
#ifndef MICROPY_INCLUDED_TKM32_QSPI_FUN_H_H
#define MICROPY_INCLUDED_TKM32_QSPI_FUN_H_H

#include "HAL_conf.h"
#include "string.h"
//----------------------------------------------------------------------------
//SDRAM 载入新程序时空间分配 (前32K做ROM程序全局变量及数据空间地址)
//有效帧头(2) + 程序地址(4) + 程序长度(4) + 程序数据(n) + 校验和(4) + 数据结尾(2)

#define SDRAM_APPBUF_REGION     (TK499SDRAM_BASE + SDRAM_BOOT_DATA_SIZE)

#define SDRAM_APP_OFFSET        0xA
#define SDRAM_APP_CHECKSIZE     0x4
#define SDRAM_APP_TAILSIZE      0x2
#define SDRAM_APP_CHECK_DIS     0x12346789

//SDRAM 运行IAP序时空间分配 (前32K做IAP程序全局变量及数据空间地址,后面空间任意分配)

#define uartInfoSTART           0x0101
#define uartInfoSTART_SDRAM     0x0202
#define uartInfoSTOP            0x8080

#define TK499SDRAM_BASE         0x70000000
#define TK499SDRAM_SIZE         0x0800000
#define SDRAM_BOOT_DATA_SIZE    DEBUF_APP_OFFSET

//for qspi space allocation
#define QSPI_FLASH_APP_DATA     0x400

#define PRINFT_APP_ENABLE 	0

#define QSPI_PSR_DEBUG 0
#define QSPI_PSR_VALUE 2

#define QSPI_DMA_DEBUG  0
#define QSPI_QUAL_DEBUG 0
#define QSPI_QUAL_READ 0
#define ROM_DEBUG 1

#if ROM_DEBUG != 0
#define QSPI_FLASH_SAVE_APP_ADD 		0x10000
#define SDRAM_FILE_SYS_ADD 					0x70020000
#define SDRAM_UART_REC_APP_ADD 			0x70020000
#define SDRAM_APP_JUMP_ADD 					0x70020000
#define QSPI_FLASH_ALL_SIZE         0x0200000
#else
#define QSPI_FLASH_SAVE_APP_ADD 		0x00000
#define SDRAM_FILE_SYS_ADD 					0x70008000
#define SDRAM_UART_REC_APP_ADD 			0x70008000
#define SDRAM_APP_JUMP_ADD 					0x70008000
//#define QSPI_FLASH_ALL_SIZE         0x0800000
#endif
//----------------------------------------------------------------------------
#define   HSKE_SPI1_TX   0x2
#define   HSKE_SPI1_RX   0x3
#define   HSKE_SPI2_TX   0x4
#define   HSKE_SPI2_RX   0x5

// 4001_3000 ~ 4001_33FF      // APB2

#define QSPI_TXREG        *(__IO uint32_t*)(QSPI_BASE)
#define QSPI_RXREG        *(__IO uint32_t*)(QSPI_BASE+0x4)
#define QSPI_CSTAT        *(__IO uint32_t*)(QSPI_BASE+0x8)
#define QSPI_INTSTAT      *(__IO uint32_t*)(QSPI_BASE+0xc)
#define QSPI_INTEN        *(__IO uint32_t*)(QSPI_BASE+0x10)
#define QSPI_INTCLR       *(__IO uint32_t*)(QSPI_BASE+0x14)
#define QSPI_GCTL         *(__IO uint32_t*)(QSPI_BASE+0x18)
#define QSPI_CCTL         *(__IO uint32_t*)(QSPI_BASE+0x1c)
#define QSPI_SPBRG        *(__IO uint32_t*)(QSPI_BASE+0x20)
#define QSPI_RXDNR        *(__IO uint32_t*)(QSPI_BASE+0x24)
#define QSPI_SCSR         *(__IO uint32_t*)(QSPI_BASE+0x28)
#define QSPI_SCSR         *(__IO uint32_t*)(QSPI_BASE+0x28)
#define QSPI_MODE         *(__IO uint32_t*)(QSPI_BASE+0x2c)

// 4000_3800 ~ 4000_3BFF      // APB1

#define SPI2_TXREG        *(__IO uint32_t*)(SPI2_BASE)
#define SPI2_RXREG        *(__IO uint32_t*)(SPI2_BASE+0x4)
#define SPI2_CSTAT        *(__IO uint32_t*)(SPI2_BASE+0x8)
#define SPI2_INTSTAT      *(__IO uint32_t*)(SPI2_BASE+0xc)
#define SPI2_INTEN        *(__IO uint32_t*)(SPI2_BASE+0x10)
#define SPI2_INTCLR       *(__IO uint32_t*)(SPI2_BASE+0x14)
#define SPI2_GCTL         *(__IO uint32_t*)(SPI2_BASE+0x18)
#define SPI2_CCTL         *(__IO uint32_t*)(SPI2_BASE+0x1c)
#define SPI2_SPBRG        *(__IO uint32_t*)(SPI2_BASE+0x20)
#define SPI2_RXDNR        *(__IO uint32_t*)(SPI2_BASE+0x24)
#define SPI2_SCSR         *(__IO uint32_t*)(SPI2_BASE+0x28)
#define SPI2_EXTLEN       *(__IO uint32_t*)(SPI2_BASE+0x2c)

//Read
#define RD_Com		0x03
#define RD_Adr		0x03
#define FastRD_Com	0x0b
#define FastRD_Dual_Com	0x3b
#define FastRD_Quad_Com	0x6b
#define FastRD_DualIo_Com	0xbb
#define FastRD_QuadIo_Com	0xeb
#define WordRD_QuadIo_Com	0xeb
#define FastRD_Adr	0x03
#define RDID_Com	0x9f
#define RDMANUIDQIO_Com	0x94
#define RDID_Adr	0x0

//Write Control
#define WREN_Com	0x06
#define WREN_Adr	0x00
#define WRDI_Com	0x04
#define WRDI_Adr	0x00

#define WRITE_ENABLE	0x06

//Erase
#define SE_Com		0x20

#define SE_Adr		0x03
#define BE32K_Com	0x52
#define BE64K_Com	0xd8
#define BE_Adr		0x00
#define CE_Com		0x60

#define ERASE_CMD		0x20

//Program
#define	PP_Com		0x02
#define	PP_Com_Quad	0x32
#define PP_Adr		0x03
#define	PP_CMD		0x02

//Status Register
#define RDSR_Com	0x05
#define RDSR_Adr	0x00
#define WRSR1_Com	0x01
#define WRSR2_Com	0x31
#define WRSR3_Com	0x11
#define WRSR_Adr	0x00

//Identification
#define ManuID		0xef
#define MemType		0x40
#define MemCap		0x18
#define W25Q128JVID 0x18

#define   APB2CLK   72000000


//INTSTAT
#define     RXMatch_Clr     0x0f
#define     RXOERR_Clr      0x17
#define     UnderRun_Clr    0x1b
#define     RXINT_Clr       0x1d
#define     TXINT_Clr       0x1e

//INTEN
#define     RXMatchEN       0x10
#define     RXOERREN        0x08
#define     UnderRunEN      0x4
#define     RXIEN           0x2
#define     TXIEN           0x1


//GCTL
//#define     Data_8bit       0xcff
//#define     Data_16bit      0x100
//#define     Data_32bit      0x200
#define     Data_8bit       0x7ff
#define     Data_32bit      0x800

//#define     CS_Low		0xf7f
//#define     CS_High		0x80
#define     CS_Low		0xFE
#define     CS_High		0x01

#define     CSC_SEL     0x40
//#define     DMA_EN     0x20
#define     DMA_EN     0x200
#define     DMA_DIS    0xdff
#define     RXEN        0x10
#define     TXEN        0x8
//#define     TXDIS           0xf17
#define     TXDIS           0xff7
#define     RXDIS           0xfef


#define     RXDMA_EN    0x800
#define     TXDMA_EN    0x400
#define     TXDMA_Dis   0xfbff
#define     RXDMA_Dis   0xf7ff
#define     MM          0x4
#define     INTEN      0x2
#define     QSPIEN       0x1

//cctl
#define   CKPL_Low    0x0
#define   CKPL_High   0x2
#define   CKPH_Sam1Tran2_Edge 0x1
#define   CKPH_Sam2Tran1_Edge  0x0

#define   Len_8bit      0x8
#define   Len_7bit      0x7

#define   MSB_First     0xb
#define   LSB_First     0x4

#define   Txtlf         0x80
#define   Rxtlf         0x20
#define   Dis_Txtlf     0xFFFFFF3F
#define   Dis_Rxtlf     0xFFFFFF9F

#define   SRAM_BASE2    SRAM_BASE
#define   SRAM_BASE1    SRAM_BASE
#define   SRAM_BASE0    SRAM_BASE
//----------------------------------------------------------------------------
//void QSPI_FunCnofig(uint8_t mode);
//void QSPI_FunSendbyte(uint8_t data);
//void QSPI_FunSendGroup(uint8_t* pData,uint32_t len);
//void QSPI_FunNSS(uint8_t cs);
//void QSPI_FunDmaRxConfig(uint32_t adress,uint32_t len);
//void QSPI_FunDmaTxConfig(uint32_t adress,uint32_t len);
//void QSPI_FunRW(uint8_t rw);
//void QFLASH_FunReadGroup(uint32_t add,uint8_t *pBuf,uint32_t len);
//void QFLASH_FunWriteCmd(uint8_t cmd);
//void QFLASH_FunCheckStatus(void);
//void QFLASH_FunEraseAll(void);
//void QspiFlashRead(uint32_t qspiAdd, uint8_t *puladdress, uint32_t len);
//void QspiRxNData(uint8_t cmd, uint32_t addr, uint8_t *pBuf, uint16_t len);

//void QFLASH_FunProgram(uint32_t add,const uint8_t *pBuf,uint32_t len);
void QFLASH_FunErase(uint32_t add,uint8_t eType);

void qspi_init(void);
void QFLASH_FunReadId(void);
int32_t qspi_bdev_ioctl(uint32_t op, uint32_t arg);
void w25qxx_read(uint8_t *pBuf, uint32_t addr, uint16_t datlen);
void w25qxx_write(uint8_t *pBuf, uint32_t WriteAddr, uint16_t datlen);

#endif

