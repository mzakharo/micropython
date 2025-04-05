#include "qspi_fun.h"

#include <stdio.h>

#include "py/obj.h"
#include "py/mperrno.h"
#include "irq.h"
#include "led.h"
#include "storage.h"

static void QSPI_FunRW(uint8_t rw)//send 8bit clk at switch to received
{
  QSPI->GCTL &= ~0x18;
  if(rw == 0)
  { 
    QSPI->GCTL |= 1<<3; //使能发送
  }
  else
  {
    QSPI->GCTL |= 1<<4;//使能接收
  }
}
void qspi_init(void)//config
{
  RCC->APB2ENR |= 0x1<<24;//QSPI enable
  RCC->AHB1ENR |= 0x8;//GPIOD enable;
  GPIOD->AFRH &= 0xf000000f;
  GPIOD->AFRH |= 0x06666660;//AF6

  GPIOD->CRH &= 0xf000000f;
  GPIOD->CRH |= 0x0bbbbbb0;
  
	//QSPI->CCTL = 0x39;
	QSPI->CCTL = 0x19;//mode0
	QSPI->SPBRG = 2;  //速度，注意，在240MHz下，这个要调小一点，要不太快，FLASH芯片受不了
	QSPI->SCSR = 0;  //选中设备
	QSPI->RXDNR = 1;

	QSPI->MODE = 0xC;//标准spi模式
	QSPI->GCTL &= 0x7ff; //清除配置

	QSPI->GCTL |= 1;  //使能spi
	QSPI_FunRW(0);
}

static void QSPI_FunSendbyte(uint8_t data)//byte send
{
	QSPI->GCTL |= TXEN; //使能发送
	QSPI->TXREG = data;
 while(!(QSPI->CSTAT & 0x1));
}

static uint8_t QSPI_FunRecbyte(void)//byte send
{
  QSPI->RXDNR = 1;
	QSPI_FunRW(1);
	
  while(!(QSPI->CSTAT & 0x2));
  return QSPI->RXREG ;
}
//
static void QSPI_FunNSS(uint8_t cs)//qspi chip select
{
  if(cs == 0)
  {
    QSPI->SCSR = 0; //选中设备
  }
  else
  {
    QSPI->SCSR = 1;
  }
}

static void QFLASH_FunWriteCmd(uint8_t cmd)
{
  QSPI_FunNSS(0);
  QSPI_FunSendbyte( cmd);
  QSPI_FunNSS(1);
}
static void QspiRxNData(uint8_t cmd, uint32_t addr, uint8_t *pBuf, uint16_t len)
{
	uint8_t addr0,addr1,addr2;
	uint16_t rxlen = len;

	addr &= 0x00ffffff; 
	addr0 = ((uint8_t)(addr>>16))&0xff;
	addr1 = ((uint8_t)(addr>>8))&0xff;
	addr2 = ((uint8_t)addr)&0xff;
	
	QSPI->SCSR &= 0xFE; 
	
	QSPI_FunSendbyte(cmd); //SPDAT = PP;
	QSPI_FunSendbyte(addr0);
	QSPI_FunSendbyte(addr1);
	QSPI_FunSendbyte(addr2);
	QSPI->GCTL &= TXDIS; //禁止发送，清空发送寄存器
	QSPI->RXDNR = len;  //配置接收字节数
	QSPI->GCTL |= RXEN; //使能接收
	while(rxlen--){
		while(!(QSPI->CSTAT & 0x02));
		*pBuf = QSPI->RXREG;
		 pBuf++;
	}
	QSPI->GCTL&=RXDIS;
	QSPI->SCSR |= 0x01; //CS=1
}

//--------------------------------------------------------------------
static void qspi_WriteCmd(uint8_t cmd)
{
  QSPI->SCSR = 0;
  QSPI_FunSendbyte(cmd);
  QSPI->SCSR = 1; //CS=1
}

static void QFLASH_FunCheckStatus(void)
{
  uint8_t tempData;
  while(1)
  {
    QSPI_FunRW(0);
    QSPI_FunNSS(0);
    QSPI_FunSendbyte( RDSR_Com);
    tempData = QSPI_FunRecbyte();
    QSPI_FunNSS(1);
    if((tempData & 0x1) == 0)
    {
      QSPI_FunRW(0);
      break;
    }
  }
}

//读取flash iD
void QFLASH_FunReadId(void)
{
	uint8_t ReadID[2] = {0};

	QspiRxNData(0x90, 0x00, ReadID, 2);
}
void w25qxx_read(uint8_t *pBuf, uint32_t addr, uint16_t datlen)
{
	QspiRxNData(RD_Com, addr, pBuf, datlen);
}

static void W25QXX_Write_Page(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
	uint16_t i;  

	qspi_WriteCmd(WRITE_ENABLE);

	QSPI->SCSR = 0;   //使能器件

	QSPI_FunSendbyte(PP_CMD);   //发送写页命令   

	QSPI_FunSendbyte((uint8_t)((WriteAddr)>>16)); //发送24bit地址    
	QSPI_FunSendbyte((uint8_t)((WriteAddr)>>8));   
	QSPI_FunSendbyte((uint8_t)WriteAddr);   
	for(i=0;i<NumByteToWrite;i++)QSPI_FunSendbyte(pBuffer[i]);//循环写数  
	
	QSPI->SCSR = 1;                            //取消片选  
	qspi_WriteCmd(WRDI_Com);
	QFLASH_FunCheckStatus();   				    //等待擦除完成
} 

static void W25QXX_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)   
{
	uint32_t pageremain;
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{	   
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	}
}
uint8_t W25QXX_BUFFER[4096];
void w25qxx_write(uint8_t *pBuf, uint32_t WriteAddr, uint16_t datlen)
{
	uint32_t secpos;
	uint32_t secoff;
	uint32_t secremain;	   
	uint32_t i;
	uint8_t * W25QXX_BUF;
	W25QXX_BUF=W25QXX_BUFFER;   

	secpos=WriteAddr>>12;//扇区地址  
	secoff=WriteAddr & 0xFFF;//在扇区内的偏移
	
	secremain=4096-secoff;//扇区剩余空间大小   

	if(datlen<=secremain) secremain=datlen;//不大于4096个字节
	while(1) 
	{	
		w25qxx_read(W25QXX_BUF,secpos<<12,4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(W25QXX_BUF[secoff+i]!=0XFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			QFLASH_FunErase(secpos<<12,0);//eType 0:SE_Com (Erase 4KB), 1:BE64K_Com
			for(i=0;i<secremain;i++)
			{
				W25QXX_BUF[i+secoff]=pBuf[i];	  
			}
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos<<12,4096);//写入整个扇区
		}else W25QXX_Write_NoCheck(pBuf,WriteAddr,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(datlen==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuf+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移	   
		   	datlen-=secremain;				//字节数递减
			if(datlen>4096)secremain=4096;	//下一个扇区还是写不完
			else secremain=datlen;			//下一个扇区可以写完了
		}	 
	}	 
}

void QFLASH_FunErase(uint32_t add,uint8_t eType)//eType 0:SE_Com (Erase 4KB), 1:BE64K_Com
{
  uint8_t addr0,addr1,addr2;
  add = add&0x00ffffff; 
  addr0 = (uint8_t)(add>>16);
  addr1 = (uint8_t)(add>>8);
  addr2 = (uint8_t)(add);

  QFLASH_FunWriteCmd(WREN_Com);// Write Enable

  QSPI_FunNSS(0);
  
  if(eType == 0)
  {
    QSPI_FunSendbyte(SE_Com);//Erase 4KB
  }
  else
  {
    QSPI_FunSendbyte(BE64K_Com);//Erase 64KB
  }
  
  QSPI_FunSendbyte(addr0);
  QSPI_FunSendbyte(addr1);
  QSPI_FunSendbyte(addr2);

  QSPI_FunNSS(1);
  QFLASH_FunWriteCmd(WRDI_Com); 
  QFLASH_FunCheckStatus();
}

int32_t qspi_bdev_ioctl(uint32_t op, uint32_t arg) {
	switch (op) {
		case BDEV_IOCTL_INIT:
			qspi_init();
			QFLASH_FunReadId();
			return 0;

		case BDEV_IOCTL_IRQ_HANDLER:

			return 0;

		case BDEV_IOCTL_SYNC:

			return 0;

		case BDEV_IOCTL_BLOCK_ERASE: {  //整片擦除芯片

			return 0;
		}
	}
	return -MP_EINVAL;
}



