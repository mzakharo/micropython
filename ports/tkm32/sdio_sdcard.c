
#include "string.h"  
#include <stdio.h>

#include "py/mperrno.h"
#include "py/mphal.h"
#include "genhdr/pins.h"
#include "pendsv.h"
#include "sdio_sdcard.h"

#include "sys.h"	
#if MICROPY_HW_ENABLE_SDCARD

static uint8_t CardType=SDIO_STD_CAPACITY_SD_CARD_V1_1;		//SD卡类型（默认为1.x卡）
static uint32_t CSD_Tab[4],CID_Tab[4],RCA=0;					//SD卡CSD,CID以及相对地址(RCA)数据
static uint8_t DeviceMode=SD_DMA_MODE;		   				//工作模式,注意,工作模式必须通过SD_SetDeviceMode,后才算数.这里只是定义一个默认的模式(SD_DMA_MODE)
static uint8_t StopCondition=0; 								//是否发送停止传输标志位,DMA多块读写的时候用到  
volatile SD_Error TransferError=SD_OK;					//数据传输错误标志,DMA读写时使用	    
volatile uint8_t TransferEnd=0;								//传输结束标志,DMA读写时使用
SD_CardInfo SDCardInfo;									//SD卡信息

//SD_ReadDisk/SD_WriteDisk函数专用buf,当这两个函数的数据缓存区地址不是4字节对齐的时候,
//需要用到该数组,确保数据缓存区地址是4字节对齐的.
__attribute__ ((aligned (4))) uint8_t SDIO_DATA_BUFFER[512];						  
 
//初始化SD卡
//返回值:错误代码;(0,无错误)
SD_Error SD_Init(void)
{
	SD_Error errorstatus=SD_OK;	  
	uint8_t clkdiv=0;
	//SDIO IO口初始化   	 
	  
// 	MY_NVIC_Init(0,0,SDIO_IRQn,2);		//SDIO中断配置
  errorstatus = SD_PowerON();			//SD卡上电
	
 	if(errorstatus==SD_OK)
  {
    errorstatus=SD_InitializeCards();			//初始化SD卡				
  }
  if(errorstatus==SD_OK)
  {
    errorstatus=SD_GetCardInfo(&SDCardInfo);	//获取卡信息
  }
  
  if(errorstatus==SD_OK)
  {
    errorstatus=SD_SelectDeselect((uint32_t)(SDCardInfo.RCA<<16));//选中SD卡   
  }
  
  if(errorstatus==SD_OK)
  {
    errorstatus=SD_EnableWideBusOperation(1);	//4位宽度,如果是MMC卡,则不能用4位模式 
  }
  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"errorstatus2 = %x \r\n",errorstatus));
  if((errorstatus==SD_OK)||(SDIO_MULTIMEDIA_CARD==CardType))
	{  		    
		if(SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1||SDCardInfo.CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)
		{
			clkdiv=SDIO_TRANSFER_CLK_DIV+1;	//V1.1/V2.0卡，设置最高48/4=12Mhz
		}
    else 
    {
      clkdiv=SDIO_TRANSFER_CLK_DIV;	//SDHC等其他卡，设置最高48/2=24Mhz
			//clkdiv=SDIO_TRANSFER_CLK_DIV+4;	//SDHC等其他卡，设置最高48/2=24Mhz
    }
		SDIO_Clock_Set(clkdiv);	//设置时钟频率,SDIO时钟计算公式:SDIO_CK时钟=SDIOCLK/[clkdiv+2];其中,SDIOCLK固定为48Mhz 
		//errorstatus=SD_SetDeviceMode(SD_DMA_MODE);	//设置为DMA模式
		errorstatus=SD_SetDeviceMode(SD_POLLING_MODE);//设置为查询模式
 	}
	
	//printf("CardType:%d\r\n",SDCardInfo.CardType);

	return errorstatus;		 
}
//SDIO时钟初始化设置
//clkdiv:时钟分频系数
//CK时钟=SDIOCLK/[clkdiv+2];(SDIOCLK时钟固定为48Mhz)
void SDIO_Clock_Set(uint8_t clkdiv)
{
	uint32_t tmpreg=TEST_SDIOx->MMC_CTRL; 
  tmpreg&=0XFFFFFFC7; 
 	tmpreg|=clkdiv<<3; 
	TEST_SDIOx->MMC_CARDSEL = 0xC0;   //enable module, enable mmcclk
	TEST_SDIOx->MMC_CTRL=tmpreg;

 //TEST_SDIOx->MMC_CTRL |= 0x1<<10;//20191107改动，屏蔽 
} 
//SDIO发送命令函数
//cmdindex:命令索引,低六位有效
//waitrsp:期待的相应.00/10,无响应;01,短响应;11,长响应
//arg:参数

void SDIO_Send_Cmd(uint8_t cmdindex,uint8_t waitrsp,uint32_t arg)
{			
//  int i;
//  int n;
//	uint32_t tmpreg;
//	TEST_SDIOx->ARG=arg;
//	tmpreg=TEST_SDIOx->CMD; 
//	tmpreg&=0XFFFFF800;		//清除index和waitrsp
//	tmpreg|=cmdindex&0X3F;	//设置新的index			 
//	tmpreg|=waitrsp<<6;		//设置新的wait rsp 
//	tmpreg|=0<<8;			//无等待
//  	tmpreg|=1<<10;			//命令通道状态机使能
//	TEST_SDIOx->CMD=tmpreg;
  

  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"SDIO_Send_Cmd = %2d( %2d , %x )\r\n",cmdindex,waitrsp,arg));
  TEST_SDIOx->CMD_BUF[4] = 0x40 | cmdindex;  
  TEST_SDIOx->CMD_BUF[3] = ((arg&0xff000000)>>24);
  TEST_SDIOx->CMD_BUF[2] = ((arg&0xff0000)>>16);
  TEST_SDIOx->CMD_BUF[1] = ((arg&0xff00)>>8);
  TEST_SDIOx->CMD_BUF[0] = (arg&0xff);
	TEST_SDIOx->CLR_MMC_INT |= 0;
  TEST_SDIOx->MMC_IO = 0x04;  //2019 auto only command transfer TEST_SDIOx->MMC_IO |= 0x04;
  while(!(TEST_SDIOx->CLR_MMC_INT & 0x1)){}

	if(TEST_SDIOx->CLR_MMC_INT & 0x1){
		TEST_SDIOx->CLR_MMC_INT |= 0x1;   //write 1 clear interrup
  }
  if(waitrsp == 0x1)
  {
//    n = 1000;while(n--);
    TEST_SDIOx->MMC_IO = 0x8c;//2019 TEST_SDIOx->MMC_IO = 0x0c;
//    while(!(TEST_SDIOx->CLR_MMC_INT & 0x1));
//    TEST_SDIOx->CLR_MMC_INT = 0x1;   //write 1 clear interrup
  }
  else if(waitrsp == 0x3)
  {
    TEST_SDIOx->MMC_IO = 0x9c;//2019 TEST_SDIOx->MMC_IO = 0x1c;
//    while(!(TEST_SDIOx->CLR_MMC_INT & 0x1));
//    TEST_SDIOx->CLR_MMC_INT = 0x1;   //write 1 clear interrup
  }
  else
  {
//    i = 100;while(i--);
  }

//  TEST_SDIOx->MMC_IO = 0x20;
//  while(!(TEST_SDIOx->CLR_MMC_INT & 0x1));
//  TEST_SDIOx->CLR_MMC_INT = 0x1;   //write 1 clear interrup

}
//SDIO发送数据配置函数
//datatimeout:超时时间设置
//datalen:传输数据长度,低25位有效,必须为块大小的整数倍
//blksize:块大小.实际大小为:2^blksize字节
//dir:数据传输方向:0,控制器到卡;1,卡到控制器;
void SDIO_Send_Data_Cfg(uint32_t datatimeout,uint32_t datalen,uint8_t blksize,uint8_t dir)
{
	uint32_t tmpreg,tmpreg1,tmpreg2=0;
//	TEST_SDIOx->DTIMER=datatimeout;
//  TEST_SDIOx->DLEN=datalen&0X1FFFFFF;	//低25位有效
//	tmpreg=TEST_SDIOx->DCTRL; 
//	tmpreg&=0xFFFFFF08;		//清除之前的设置.
//	tmpreg|=blksize<<4;		//设置块大小
//	tmpreg|=0<<2;			//块数据传输
//	tmpreg|=(dir&0X01)<<1;	//方向控制
//	tmpreg|=1<<0;			//数据传输使能,DPSM状态机
//	TEST_SDIOx->DCTRL=tmpreg;		
  tmpreg = TEST_SDIOx->MMC_IO_MBCTL;
  tmpreg1 = TEST_SDIOx->MMC_IO;
  tmpreg &= ~((0x3<<4)|(03<<0));
  if(datatimeout<100)
  {
    TEST_SDIOx->MMC_TIMEOUTCNT = datatimeout;
    tmpreg |= 0x0<<4;
  }
  else if(datatimeout<10000)
  {
    TEST_SDIOx->MMC_TIMEOUTCNT = datatimeout/100;
    tmpreg |= 0x1<<4;
  }
  else if(datatimeout<1000000)
  {
    TEST_SDIOx->MMC_TIMEOUTCNT = datatimeout/10000;
    tmpreg |= 0x2<<4;
  }
  else
  {
    TEST_SDIOx->MMC_TIMEOUTCNT = datatimeout/1000000;
    tmpreg |= 0x3<<4;
  }
  TEST_SDIOx->MMC_BYTECNTL = datalen&0x1FFFFFF;	;
  TEST_SDIOx->MMC_BLOCKCNT = blksize;
  if(dir == 0)
  {
    tmpreg |= 1<<1;
    tmpreg1 |= 1<<1;
    tmpreg2 |= 1<<11;
  }
  else
  { 
    tmpreg &= ~(1<<1);
    tmpreg1 &= ~(1<<1);
    tmpreg2 &= ~(1<<11);
  }

  TEST_SDIOx->MMC_IO_MBCTL = tmpreg;
  
  TEST_SDIOx->MMC_IO = tmpreg1;  
  TEST_SDIOx->BUF_CTL = tmpreg2;
//  TEST_SDIOx->BUF_CTL |= 0x20<<2;
//  TEST_SDIOx->MMC_IO          |= 0x1;
}  

//卡上电
//查询所有SDIO接口上的卡设备,并查询其电压和配置时钟
//返回值:错误代码;(0,无错误)
SD_Error SD_PowerON(void)
{
 	uint8_t i=0;
	SD_Error errorstatus=SD_OK;
	uint32_t response=0,count=0,validvoltage=0;
	uint32_t SDType=SD_STD_CAPACITY;
	//配置CLKCR寄存器 
//	TEST_SDIOx->CLKCR=0;				//清空CLKCR之前的设置
//	TEST_SDIOx->CLKCR|=0<<9;			//非省电模式
//	TEST_SDIOx->CLKCR|=0<<10;			//关闭旁路,CK根据分频设置输出
//	TEST_SDIOx->CLKCR|=0<<11;			//1位数据宽度
//	TEST_SDIOx->CLKCR|=0<<13;			//SDIOCLK上升沿产生SDIOCK
//	TEST_SDIOx->CLKCR|=0<<14;			//关闭硬件流控制    
//	SDIO_Clock_Set(SDIO_INIT_CLK_DIV);//设置时钟频率(初始化的时候,不能超过400Khz)			 
// 	TEST_SDIOx->POWER=0X03;			//上电状态,开启卡时钟    
//  	TEST_SDIOx->CLKCR|=1<<8;			//SDIOCK使能   
  for(i=0;i<74;i++)
	{
		SDIO_Send_Cmd(SD_CMD_GO_IDLE_STATE,0,0);//发送CMD0进入IDLE STAGE模式命令.												  
		errorstatus=CmdError();
		if(errorstatus==SD_OK)break;
 	}
 	if(errorstatus)return errorstatus;//返回错误状态

	SDIO_Send_Cmd(SDIO_SEND_IF_COND,1,SD_CHECK_PATTERN);//发送CMD8,短响应,检查SD卡接口特性.
 														//arg[11:8]:01,支持电压范围,2.7~3.6V
														//arg[7:0]:默认0XAA
														//返回响应7
  	errorstatus=CmdResp7Error();						//等待R7响应
 	if(errorstatus==SD_OK) 								//R7响应正常
	{
		CardType=SDIO_STD_CAPACITY_SD_CARD_V2_0;		//SD 2.0卡
		SDType=SD_HIGH_CAPACITY;			   			//高容量卡
	}
	SDIO_Send_Cmd(SD_CMD_APP_CMD,1,0);					//发送CMD55,短响应	 
	errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 		 	//等待R1响应   
	if(errorstatus==SD_OK)//SD2.0/SD 1.1,否则为MMC卡
	{																  
		//SD卡,发送ACMD41 SD_APP_OP_COND,参数为:0x80100000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   
			SDIO_Send_Cmd(SD_CMD_APP_CMD,1,0);				//发送CMD55,短响应	 
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD); 	 	//等待R1响应   
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误
			SDIO_Send_Cmd(SD_CMD_SD_APP_OP_COND,1,SD_VOLTAGE_WINDOW_SD|SDType);//发送ACMD41,短响应	 
			errorstatus=CmdResp3Error(); 					//等待R3响应   
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误  
//			response=TEST_SDIOx->RESP1;			   				//得到响应
      response=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];	
			validvoltage=(((response>>31)==1)?1:0);			//判断SD卡上电是否完成
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}
		if(response&=SD_HIGH_CAPACITY)
		{
			CardType=SDIO_HIGH_CAPACITY_SD_CARD;
		}
 	}else//MMC卡
	{
		//MMC卡,发送CMD1 SDIO_SEND_OP_COND,参数为:0x80FF8000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{	   										   				   
			SDIO_Send_Cmd(SD_CMD_SEND_OP_COND,1,SD_VOLTAGE_WINDOW_MMC);//发送CMD1,短响应	 
			errorstatus=CmdResp3Error(); 					//等待R3响应   
 			if(errorstatus!=SD_OK)return errorstatus;   	//响应错误  
			response=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];			   				//得到响应
			validvoltage=(((response>>31)==1)?1:0);
			count++;
		}
		if(count>=SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 			    
		CardType=SDIO_MULTIMEDIA_CARD;	  
  	}  
  	return(errorstatus);		
}
//SD卡 Power OFF
//返回值:错误代码;(0,无错误)
SD_Error SD_PowerOFF(void)
{
//  	TEST_SDIOx->POWER&=~(3<<0);//SDIO电源关闭,时钟停止	
  TEST_SDIOx->MMC_CARDSEL &= ~(0x3<<6);//SDIO电源关闭,时钟停止	
	return SD_OK;		  
}   
//初始化所有的卡,并让卡进入就绪状态
//返回值:错误代码
SD_Error SD_InitializeCards(void)
{
  int i;
 	SD_Error errorstatus=SD_OK;
	uint16_t rca = 0x01;
// 	if((TEST_SDIOx->POWER&0X03)==0)return SD_REQUEST_NOT_APPLICABLE;//检查电源状态,确保为上电状态
  if(((TEST_SDIOx->MMC_CARDSEL>>6)&0X03)==0)return SD_REQUEST_NOT_APPLICABLE;//检查电源状态,确保为上电状态
 	if(SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//非SECURE_DIGITAL_IO_CARD
	{
		SDIO_Send_Cmd(SD_CMD_ALL_SEND_CID,3,0);			//发送CMD2,取得CID,长响应	 
		errorstatus=CmdResp2Error(); 					//等待R2响应   
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误	
//    CID_Tab[0]=TEST_SDIOx->RESP1;    
//    CID_Tab[1]=TEST_SDIOx->RESP2;
//		CID_Tab[2]=TEST_SDIOx->RESP3;
//		CID_Tab[3]=TEST_SDIOx->RESP4;
    
    for(i = 0;i<4;i++)
    {
      if(i == 0)
      {
        CID_Tab[3 -i ]= TEST_SDIOx->CMD_BUF[2+4*i]<<24  |TEST_SDIOx->CMD_BUF[1+4*i]<<16  |TEST_SDIOx->CMD_BUF[0+4*i]<<8  | TEST_SDIOx->CMD_BUF[0+4*i];
      }
      else
      {
        CID_Tab[3 -i ]= TEST_SDIOx->CMD_BUF[2+4*i]<<24  |TEST_SDIOx->CMD_BUF[1+4*i]<<16  |TEST_SDIOx->CMD_BUF[0+4*i]<<8  | TEST_SDIOx->CMD_BUF[4*i-1];
      }
    }
	}
	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_SECURE_DIGITAL_IO_COMBO_CARD==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))//判断卡类型
	{
		SDIO_Send_Cmd(SD_CMD_SET_REL_ADDR,1,0);			//发送CMD3,短响应 
		errorstatus=CmdResp6Error(SD_CMD_SET_REL_ADDR,&rca);//等待R6响应 
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误		    
	}
  if (SDIO_MULTIMEDIA_CARD==CardType)
  {
    SDIO_Send_Cmd(SD_CMD_SET_REL_ADDR,1,(uint32_t)(rca<<16));//发送CMD3,短响应 	   
    errorstatus=CmdResp2Error(); 					        //等待R2响应
    if(errorstatus!=SD_OK)return errorstatus;   	//响应错误
  }
	if (SDIO_SECURE_DIGITAL_IO_CARD!=CardType)			//非SECURE_DIGITAL_IO_CARD
	{
		RCA = rca;
		SDIO_Send_Cmd(SD_CMD_SEND_CSD,3,(uint32_t)(rca<<16));//发送CMD9+卡RCA,取得CSD,长响应 	   
		errorstatus=CmdResp2Error(); 					//等待R2响应   
		if(errorstatus!=SD_OK)return errorstatus;   	//响应错误		    

    
    for(i = 0;i<4;i++)
    {
      if(i == 0)
      {
        CSD_Tab[3 -i ]= TEST_SDIOx->CMD_BUF[2+4*i]<<24  |TEST_SDIOx->CMD_BUF[1+4*i]<<16  |TEST_SDIOx->CMD_BUF[0+4*i]<<8  | TEST_SDIOx->CMD_BUF[0+4*i];
      }
      else
      {
        CSD_Tab[3 -i ]= TEST_SDIOx->CMD_BUF[2+4*i]<<24  |TEST_SDIOx->CMD_BUF[1+4*i]<<16  |TEST_SDIOx->CMD_BUF[0+4*i]<<8  | TEST_SDIOx->CMD_BUF[4*i-1];
      }
    }   

//    CSD_Tab[0]= TEST_SDIOx->CMD_BUF[0]<<24  |TEST_SDIOx->CMD_BUF[1]<<16  |TEST_SDIOx->CMD_BUF[2]<<8  | TEST_SDIOx->CMD_BUF[3];
//		CSD_Tab[1]= TEST_SDIOx->CMD_BUF[4]<<24  |TEST_SDIOx->CMD_BUF[5]<<16  |TEST_SDIOx->CMD_BUF[6]<<8  | TEST_SDIOx->CMD_BUF[7];
//		CSD_Tab[2]= TEST_SDIOx->CMD_BUF[8]<<24 |TEST_SDIOx->CMD_BUF[9]<<16 |TEST_SDIOx->CMD_BUF[10]<<8  | TEST_SDIOx->CMD_BUF[11];
//		CSD_Tab[3]= TEST_SDIOx->CMD_BUF[12]<<24 |TEST_SDIOx->CMD_BUF[13]<<16 |TEST_SDIOx->CMD_BUF[14]<<8 | TEST_SDIOx->CMD_BUF[15];			    
	}
	return SD_OK;//卡初始化成功
} 
//得到卡信息
//cardinfo:卡信息存储区
//返回值:错误状态
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo)
{
 	SD_Error errorstatus=SD_OK;
	uint8_t tmp=0;	   
	cardinfo->CardType=(uint8_t)CardType; 				//卡类型
	cardinfo->RCA=(uint16_t)RCA;							//卡RCA值
	tmp=(uint8_t)((CSD_Tab[0]&0xFF000000)>>24);
	cardinfo->SD_csd.CSDStruct=(tmp&0xC0)>>6;		//CSD结构
	cardinfo->SD_csd.SysSpecVersion=(tmp&0x3C)>>2;	//2.0协议还没定义这部分(为保留),应该是后续协议定义的
	cardinfo->SD_csd.Reserved1=tmp&0x03;			//2个保留位  
	tmp=(uint8_t)((CSD_Tab[0]&0x00FF0000)>>16);			//第1个字节
	cardinfo->SD_csd.TAAC=tmp;				   		//数据读时间1
	tmp=(uint8_t)((CSD_Tab[0]&0x0000FF00)>>8);	  		//第2个字节
	cardinfo->SD_csd.NSAC=tmp;		  				//数据读时间2
	tmp=(uint8_t)(CSD_Tab[0]&0x000000FF);				//第3个字节
	cardinfo->SD_csd.MaxBusClkFrec=tmp;		  		//传输速度	   
	tmp=(uint8_t)((CSD_Tab[1]&0xFF000000)>>24);			//第4个字节
	cardinfo->SD_csd.CardComdClasses=tmp<<4;    	//卡指令类高四位
	tmp=(uint8_t)((CSD_Tab[1]&0x00FF0000)>>16);	 		//第5个字节
	cardinfo->SD_csd.CardComdClasses|=(tmp&0xF0)>>4;//卡指令类低四位
	cardinfo->SD_csd.RdBlockLen=tmp&0x0F;	    	//最大读取数据长度
	tmp=(uint8_t)((CSD_Tab[1]&0x0000FF00)>>8);			//第6个字节
	cardinfo->SD_csd.PartBlockRead=(tmp&0x80)>>7;	//允许分块读
	cardinfo->SD_csd.WrBlockMisalign=(tmp&0x40)>>6;	//写块错位
	cardinfo->SD_csd.RdBlockMisalign=(tmp&0x20)>>5;	//读块错位
	cardinfo->SD_csd.DSRImpl=(tmp&0x10)>>4;
	cardinfo->SD_csd.Reserved2=0; 					//保留
 	if((CardType==SDIO_STD_CAPACITY_SD_CARD_V1_1)||(CardType==SDIO_STD_CAPACITY_SD_CARD_V2_0)||(SDIO_MULTIMEDIA_CARD==CardType))//标准1.1/2.0卡/MMC卡
	{
		cardinfo->SD_csd.DeviceSize=(tmp&0x03)<<10;	//C_SIZE(12位)
	 	tmp=(uint8_t)(CSD_Tab[1]&0x000000FF); 			//第7个字节	
		cardinfo->SD_csd.DeviceSize|=(tmp)<<2;
 		tmp=(uint8_t)((CSD_Tab[2]&0xFF000000)>>24);		//第8个字节	
		cardinfo->SD_csd.DeviceSize|=(tmp&0xC0)>>6;
 		cardinfo->SD_csd.MaxRdCurrentVDDMin=(tmp&0x38)>>3;
		cardinfo->SD_csd.MaxRdCurrentVDDMax=(tmp&0x07);
 		tmp=(uint8_t)((CSD_Tab[2]&0x00FF0000)>>16);		//第9个字节	
		cardinfo->SD_csd.MaxWrCurrentVDDMin=(tmp&0xE0)>>5;
		cardinfo->SD_csd.MaxWrCurrentVDDMax=(tmp&0x1C)>>2;
		cardinfo->SD_csd.DeviceSizeMul=(tmp&0x03)<<1;//C_SIZE_MULT
 		tmp=(uint8_t)((CSD_Tab[2]&0x0000FF00)>>8);	  	//第10个字节	
		cardinfo->SD_csd.DeviceSizeMul|=(tmp&0x80)>>7;
 		cardinfo->CardCapacity=(cardinfo->SD_csd.DeviceSize+1);//计算卡容量
		cardinfo->CardCapacity*=(1<<(cardinfo->SD_csd.DeviceSizeMul+2));
		cardinfo->CardBlockSize=1<<(cardinfo->SD_csd.RdBlockLen);//块大小
		cardinfo->CardCapacity*=cardinfo->CardBlockSize;
	}else if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//高容量卡
	{
 		tmp=(uint8_t)(CSD_Tab[1]&0x000000FF); 		//第7个字节	
		cardinfo->SD_csd.DeviceSize=(tmp&0x3F)<<16;//C_SIZE
 		tmp=(uint8_t)((CSD_Tab[2]&0xFF000000)>>24); 	//第8个字节	
 		cardinfo->SD_csd.DeviceSize|=(tmp<<8);
 		tmp=(uint8_t)((CSD_Tab[2]&0x00FF0000)>>16);	//第9个字节	
 		cardinfo->SD_csd.DeviceSize|=(tmp);
 		tmp=(uint8_t)((CSD_Tab[2]&0x0000FF00)>>8); 	//第10个字节	
 		cardinfo->CardCapacity=(long long)(cardinfo->SD_csd.DeviceSize+1)*512*1024;//计算卡容量
		cardinfo->CardBlockSize=512; 			//块大小固定为512字节
	}	  
	cardinfo->SD_csd.EraseGrSize=(tmp&0x40)>>6;
	cardinfo->SD_csd.EraseGrMul=(tmp&0x3F)<<1;	   
	tmp=(uint8_t)(CSD_Tab[2]&0x000000FF);			//第11个字节	
	cardinfo->SD_csd.EraseGrMul|=(tmp&0x80)>>7;
	cardinfo->SD_csd.WrProtectGrSize=(tmp&0x7F);
 	tmp=(uint8_t)((CSD_Tab[3]&0xFF000000)>>24);		//第12个字节	
	cardinfo->SD_csd.WrProtectGrEnable=(tmp&0x80)>>7;
	cardinfo->SD_csd.ManDeflECC=(tmp&0x60)>>5;
	cardinfo->SD_csd.WrSpeedFact=(tmp&0x1C)>>2;
	cardinfo->SD_csd.MaxWrBlockLen=(tmp&0x03)<<2;	 
	tmp=(uint8_t)((CSD_Tab[3]&0x00FF0000)>>16);		//第13个字节
	cardinfo->SD_csd.MaxWrBlockLen|=(tmp&0xC0)>>6;
	cardinfo->SD_csd.WriteBlockPaPartial=(tmp&0x20)>>5;
	cardinfo->SD_csd.Reserved3=0;
	cardinfo->SD_csd.ContentProtectAppli=(tmp&0x01);  
	tmp=(uint8_t)((CSD_Tab[3]&0x0000FF00)>>8);		//第14个字节
	cardinfo->SD_csd.FileFormatGrouop=(tmp&0x80)>>7;
	cardinfo->SD_csd.CopyFlag=(tmp&0x40)>>6;
	cardinfo->SD_csd.PermWrProtect=(tmp&0x20)>>5;
	cardinfo->SD_csd.TempWrProtect=(tmp&0x10)>>4;
	cardinfo->SD_csd.FileFormat=(tmp&0x0C)>>2;
	cardinfo->SD_csd.ECC=(tmp&0x03);  
	tmp=(uint8_t)(CSD_Tab[3]&0x000000FF);			//第15个字节
	cardinfo->SD_csd.CSD_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_csd.Reserved4=1;		 
	tmp=(uint8_t)((CID_Tab[0]&0xFF000000)>>24);		//第0个字节
	cardinfo->SD_cid.ManufacturerID=tmp;		    
	tmp=(uint8_t)((CID_Tab[0]&0x00FF0000)>>16);		//第1个字节
	cardinfo->SD_cid.OEM_AppliID=tmp<<8;	  
	tmp=(uint8_t)((CID_Tab[0]&0x000000FF00)>>8);		//第2个字节
	cardinfo->SD_cid.OEM_AppliID|=tmp;	    
	tmp=(uint8_t)(CID_Tab[0]&0x000000FF);			//第3个字节	
	cardinfo->SD_cid.ProdName1=tmp<<24;				  
	tmp=(uint8_t)((CID_Tab[1]&0xFF000000)>>24); 		//第4个字节
	cardinfo->SD_cid.ProdName1|=tmp<<16;	  
	tmp=(uint8_t)((CID_Tab[1]&0x00FF0000)>>16);	   	//第5个字节
	cardinfo->SD_cid.ProdName1|=tmp<<8;		 
	tmp=(uint8_t)((CID_Tab[1]&0x0000FF00)>>8);		//第6个字节
	cardinfo->SD_cid.ProdName1|=tmp;		   
	tmp=(uint8_t)(CID_Tab[1]&0x000000FF);	  		//第7个字节
	cardinfo->SD_cid.ProdName2=tmp;			  
	tmp=(uint8_t)((CID_Tab[2]&0xFF000000)>>24); 		//第8个字节
	cardinfo->SD_cid.ProdRev=tmp;		 
	tmp=(uint8_t)((CID_Tab[2]&0x00FF0000)>>16);		//第9个字节
	cardinfo->SD_cid.ProdSN=tmp<<24;	   
	tmp=(uint8_t)((CID_Tab[2]&0x0000FF00)>>8); 		//第10个字节
	cardinfo->SD_cid.ProdSN|=tmp<<16;	   
	tmp=(uint8_t)(CID_Tab[2]&0x000000FF);   			//第11个字节
	cardinfo->SD_cid.ProdSN|=tmp<<8;		   
	tmp=(uint8_t)((CID_Tab[3]&0xFF000000)>>24); 		//第12个字节
	cardinfo->SD_cid.ProdSN|=tmp;			     
	tmp=(uint8_t)((CID_Tab[3]&0x00FF0000)>>16);	 	//第13个字节
	cardinfo->SD_cid.Reserved1|=(tmp&0xF0)>>4;
	cardinfo->SD_cid.ManufactDate=(tmp&0x0F)<<8;    
	tmp=(uint8_t)((CID_Tab[3]&0x0000FF00)>>8);		//第14个字节
	cardinfo->SD_cid.ManufactDate|=tmp;		 	  
	tmp=(uint8_t)(CID_Tab[3]&0x000000FF);			//第15个字节
	cardinfo->SD_cid.CID_CRC=(tmp&0xFE)>>1;
	cardinfo->SD_cid.Reserved2=1;	 
	
	return errorstatus;
}
//设置SDIO总线宽度(MMC卡不支持4bit模式)
//wmode:位宽模式.0,1位数据宽度;1,4位数据宽度;2,8位数据宽度
//返回值:SD卡错误状态
SD_Error SD_EnableWideBusOperation(uint32_t wmode)
{
  	SD_Error errorstatus=SD_OK;
	uint16_t clkcr=0;
  	if(SDIO_MULTIMEDIA_CARD==CardType)return SD_UNSUPPORTED_FEATURE;//MMC卡不支持
 	else if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
	{
		if(wmode>=2)return SD_UNSUPPORTED_FEATURE;//不支持8位模式
 		else   
		{
			errorstatus=SDEnWideBus(wmode);
      
 			if(SD_OK==errorstatus)
			{
//				clkcr=TEST_SDIOx->CLKCR;		//读取CLKCR的值
//				clkcr&=~(3<<11);		//清除之前的位宽设置    
//				clkcr|=(uint16_t)wmode<<11;	//1位/4位总线宽度 
//				clkcr|=0<<14;			//不开启硬件流控制
//				TEST_SDIOx->CLKCR=clkcr;		//重新设置CLKCR值 
        clkcr = TEST_SDIOx->MMC_CTRL;
        clkcr &= ~(1<<7);
        clkcr|=(uint16_t)wmode<<7;
        clkcr|=(uint16_t)1<<8;
        TEST_SDIOx->MMC_CTRL = clkcr;
			}
		}  
	} 
	return errorstatus; 
}
//设置SD卡工作模式
//Mode:
//返回值:错误状态
SD_Error SD_SetDeviceMode(uint32_t Mode)
{
	SD_Error errorstatus = SD_OK;
 	if((Mode==SD_DMA_MODE)||(Mode==SD_POLLING_MODE))DeviceMode=Mode;
	else errorstatus=SD_INVALID_PARAMETER;
	return errorstatus;	    
}
//选卡
//发送CMD7,选择相对地址(rca)为addr的卡,取消其他卡.如果为0,则都不选择.
//addr:卡的RCA地址
SD_Error SD_SelectDeselect(uint32_t addr)
{
 	SDIO_Send_Cmd(SD_CMD_SEL_DESEL_CARD,1,addr);	//发送CMD7,选择卡,短响应	 	   
   	return CmdResp1Error(SD_CMD_SEL_DESEL_CARD);	  
}  
//SD卡读取单个/多个块 
//buf:读数据缓存区
//addr:读取地址
//blksize:块大小
//nblks:要读取的块数,1,表示读取单个块
//返回值:错误状态 
int ob_cnt = 0;
SD_Error SD_ReadBlocks(uint8_t *buf,long long addr,uint16_t blksize,uint32_t nblks)
{
  SD_Error errorstatus=SD_OK; 
  uint32_t tempNblk;
	
	uint32_t timeout=SDIO_DATATIMEOUT;  
	uint32_t *tempbuff=(uint32_t*)buf;	//转换为uint32_t指针 
//    TEST_SDIOx->DCTRL=0x0;			//数据控制寄存器清零(关DMA)   
  ob_cnt = 0;
  

//  TEST_SDIOx->BUF_CTL |= 1<<15;
//  TEST_SDIOx->BUF_CTL &= ~(1<<15);
  
  
  TEST_SDIOx->BUF_CTL &= ~(1<<14);
	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)//大容量卡
	{
		blksize=512;
		addr>>=9;
	}    

	SDIO_Send_Cmd(SD_CMD_SET_BLOCKLEN,1,blksize);			//发送CMD16+设置数据长度为blksize,短响应 	   
	errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);			//等待R1响应   
	if(errorstatus!=SD_OK)return errorstatus;   			//响应错误
	SDIO_Send_Data_Cfg(SD_DATATIMEOUT,nblks*blksize,blksize,1);	//nblks*blksize,块大小恒为512,卡到控制器	 
 
  
	if(nblks>1)												//多块读  
	{									    
		SDIO_Send_Cmd(SD_CMD_READ_MULT_BLOCK,1,addr);		//发送CMD18+从addr地址出读取数据,短响应 	   
		errorstatus=CmdResp1Error(SD_CMD_READ_MULT_BLOCK);	//等待R1响应   
		if(errorstatus!=SD_OK)return errorstatus;   		//响应错误	 
	}else													//单块读
	{ 

		SDIO_Send_Cmd(SD_CMD_READ_SINGLE_BLOCK,1,addr);		//发送CMD17+从addr地址出读取数据,短响应 	   
		errorstatus=CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);//等待R1响应   
		if(errorstatus!=SD_OK)return errorstatus;   		//响应错误	 
	}
  
  
	if(DeviceMode==SD_POLLING_MODE)//
	{ 
		INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!)
		TEST_SDIOx->MMC_BLOCKCNT = nblks;
    TEST_SDIOx->MMC_IO          = 0x02;  //2019 receive TEST_SDIOx->MMC_IO          = 0x2;
		ob_cnt = 0;
    tempNblk = nblks;
		while(tempNblk--)
		{
//			TEST_SDIOx->BUF_CTL |= 1<<15;

			TEST_SDIOx->MMC_IO          |= 0x1;//2019 TEST_SDIOx->MMC_IO          |= 0x1

			while(!(TEST_SDIOx->CLR_MMC_INT&((1<<2)|(1<<6)|(1<<1))))//CRC/超时/完成(标志)
			{
				if(!(TEST_SDIOx->BUF_CTL&(1<<1)))	//FIFO里面,还存在可用数据
				{
					*(uint32_t*)tempbuff=TEST_SDIOx->DATA_BUF0;	//循环读取数据
					tempbuff++;
					ob_cnt++;
					timeout=0X7FFFFF;
				} 
				if(timeout == 0)
				{
					return SD_DATA_TIMEOUT;
				}
				timeout--;
			}
			
			if(TEST_SDIOx->CLR_MMC_INT&(1<<6))		//数据超时错误
			{					
				TEST_SDIOx->CLR_MMC_INT = 1<<6; 		//清错误标志
				return SD_DATA_TIMEOUT;
			}else if(TEST_SDIOx->CLR_MMC_INT&(1<<2))	//数据块CRC错误
			{
				TEST_SDIOx->CLR_MMC_INT = 1<<2; 		//清错误标志
				return SD_DATA_CRC_FAIL;		   
			}
			
			while(!(TEST_SDIOx->BUF_CTL&(1<<1)))	//FIFO里面,还存在可用数据
			{
				*(uint32_t*)tempbuff=TEST_SDIOx->DATA_BUF0;	//循环读取数据
				tempbuff++;
				ob_cnt++;
			}  
	  }

    if(nblks>1)
    {
      if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
      {
        //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"\r\n123\r\n"));
        SDIO_Send_Cmd(SD_CMD_STOP_TRANSMISSION,1,0);		//发送CMD12+结束传输 	   
        errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//等待R1响应   
        if(errorstatus!=SD_OK)return errorstatus;	 
      }
    }
		INTX_ENABLE();//开启总中断
//		TEST_SDIOx->ICR = 0X5FF;	 		//清除所有标记 
    TEST_SDIOx->CLR_MMC_INT = 0xFF;
	}
  else if(DeviceMode==SD_DMA_MODE)
	{
		TransferError=SD_OK;
		if(nblks>1)StopCondition=1;	//多块读,需要发送停止传输指令 
		else StopCondition=0;		//单块读,不需要发送停止传输指令 
		TransferEnd=0;				//传输结束标置位，在中断服务置1
    
//		TEST_SDIOx->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<5)|(1<<9);	//配置需要的中断 
//		TEST_SDIOx->DCTRL|=1<<3;		 						//SDIO DMA使能 
    TEST_SDIOx->BUF_CTL |= (1<<14);
		SD_DMA_Config((uint32_t*)buf,nblks*blksize,0); 
    
//		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//等待传输完成 
    
		if(timeout==0)return SD_DATA_TIMEOUT;//超时
		while((TransferEnd==0)&&(TransferError==SD_OK)); 
		if(TransferError!=SD_OK)errorstatus=TransferError;  	 
	}
	return errorstatus;
}	
 		    																  
//SD卡写单个/多个块 
//buf:数据缓存区
//addr:写地址
//blksize:块大小
//nblks:要读取的块数,1,表示读取单个块
//返回值:错误状态			
extern uint32_t regTemp[] ;
SD_Error SD_WriteBlocks(uint8_t *buf,long long addr,uint16_t blksize,uint32_t nblks)
{
	SD_Error errorstatus = SD_OK;
	uint8_t  cardstate=0;
	uint32_t timeout=0,bytestransferred=0;
	uint32_t cardstatus=0;
	uint32_t tlen=nblks*blksize;						//总长度(字节)
	uint32_t *tempbuff=(uint32_t*)buf;					

 	if(buf==NULL)return SD_INVALID_PARAMETER;	//参数错误   
//  	TEST_SDIOx->DCTRL=0x0;							//数据控制寄存器清零(关DMA)  
  TEST_SDIOx->BUF_CTL &= ~(1<<14); 

 	if(CardType==SDIO_HIGH_CAPACITY_SD_CARD)	//大容量卡
	{
		blksize=512;
		addr>>=9;
	}    
    
	SDIO_Send_Cmd(SD_CMD_SET_BLOCKLEN,1,blksize);			//发送CMD16+设置数据长度为blksize,短响应 	   
	errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);			//等待R1响应   
	if(errorstatus!=SD_OK)return errorstatus;   			//响应错误  
	if(nblks>1)												//多块写
	{									     
		if(nblks*blksize>SD_MAX_DATA_LENGTH)return SD_INVALID_PARAMETER;   
     	if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
    	{
			//提高性能
	 	   	SDIO_Send_Cmd(SD_CMD_APP_CMD,1,(uint32_t)RCA<<16);	//发送ACMD55,短响应 	   
			errorstatus=CmdResp1Error(SD_CMD_APP_CMD);		//等待R1响应   		   
			if(errorstatus!=SD_OK)return errorstatus;				    
	 	   	SDIO_Send_Cmd(SD_CMD_SET_BLOCK_COUNT,1,nblks);	//发送CMD23,设置块数量,短响应 	   
			errorstatus=CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);//等待R1响应   		   
			if(errorstatus!=SD_OK)return errorstatus;				    
		} 
		SDIO_Send_Cmd(SD_CMD_WRITE_MULT_BLOCK,1,addr);		//发送CMD25,多块写指令,短响应 	   
		errorstatus=CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);	//等待R1响应    
	}else													//单块写		
	{ 
		SDIO_Send_Cmd(SD_CMD_SEND_STATUS,1,(uint32_t)RCA<<16);	//发送CMD13,查询卡的状态,短响应 	   
		errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);		//等待R1响应   		   
		if(errorstatus!=SD_OK)return errorstatus;
		cardstatus=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];													  
		timeout=SD_DATATIMEOUT;
		while(((cardstatus&0x00000100)==0)&&(timeout>0)) 	//检查READY_FOR_DATA位是否置位
		{
			timeout--;
			SDIO_Send_Cmd(SD_CMD_SEND_STATUS,1,(uint32_t)RCA<<16);//发送CMD13,查询卡的状态,短响应 	   
			errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//等待R1响应   		   
			if(errorstatus!=SD_OK)return errorstatus;				    
			cardstatus=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];												  
		}
		if(timeout==0)return SD_ERROR;  
    
		SDIO_Send_Cmd(SD_CMD_WRITE_SINGLE_BLOCK,1,addr);	//发送CMD24,写单块指令,短响应 	   
		errorstatus=CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);//等待R1响应   	
	}	   
	if(errorstatus!=SD_OK)return errorstatus;   	   
 	SDIO_Send_Data_Cfg(SD_DATATIMEOUT,nblks*blksize,9,0);	//blksize,块大小恒为512字节,控制器到卡	  
	timeout=SDIO_DATATIMEOUT; 
	if(DeviceMode==SD_POLLING_MODE)
	{
    
		INTX_DISABLE();//关闭总中断(POLLING模式,严禁中断打断SDIO读写操作!!!) 
    timeout=0X7FFFFF;
    
    TEST_SDIOx->MMC_IO          = 0x00;//2019 TEST_SDIOx->MMC_IO          = 0x0;
    TEST_SDIOx->BUF_CTL         = 0x8800;    //disable dma, write sd card
		TEST_SDIOx->MMC_BLOCKCNT = nblks;
		bytestransferred = 0;
		
	#if 1
		#if 1
    while(!(TEST_SDIOx->CLR_MMC_INT&((1<<2)|(1<<6)|(1<<1))))//CRC/超时/完成(标志)
    {
      if(!(TEST_SDIOx->BUF_CTL&(1<<0)))							//send buffer full
			{
				if((tlen-bytestransferred) > 0)
				{
          TEST_SDIOx->DATA_BUF0 = *(uint32_t*)tempbuff++;
          bytestransferred+=4;

				}
        if(tlen == bytestransferred)
        {
					TEST_SDIOx->MMC_IO   = 0x1; //2019  TEST_SDIOx->MMC_IO   = 0x1;
          break;
        }

				timeout=0X3FFFFFFF;	//写数据溢出时间
			}
			else
			{
				if(timeout==0)return SD_DATA_TIMEOUT; 
				timeout--;
			}
    }
		#endif
		
      
//    TEST_SDIOx->MMC_IO          = 0x1;      //write data, auto transfer   
    
		if(TEST_SDIOx->CLR_MMC_INT&(1<<6))		//数据超时错误
		{										   
			TEST_SDIOx->CLR_MMC_INT = 1<<6; 		//清错误标志
			return SD_DATA_TIMEOUT;
		}else if(TEST_SDIOx->CLR_MMC_INT&(1<<2))	//数据块CRC错误
		{
			TEST_SDIOx->CLR_MMC_INT = 1<<2; 		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}
		if(TEST_SDIOx->CLR_MMC_INT&(1<<6))		//数据超时错误
		{										   
			TEST_SDIOx->CLR_MMC_INT = 1<<6; 		//清错误标志
			return SD_DATA_TIMEOUT;
		}else if(TEST_SDIOx->CLR_MMC_INT&(1<<2))	//数据块CRC错误
		{
			TEST_SDIOx->CLR_MMC_INT = 1<<2; 		//清错误标志
			return SD_DATA_CRC_FAIL;		   
		}
		
		while(!(TEST_SDIOx->BUF_CTL&(1<<1)));
		TEST_SDIOx->CLR_MMC_INT |= 0;

//		TEST_SDIOx->MMC_IO          = 0x0;
		TEST_SDIOx->BUF_CTL         = 0x8800;    //disable dma, write sd card
    #else
//		while(1)//CRC/超时/完成(标志)
    {
      if(TEST_SDIOx->BUF_CTL&(1<<1))							//send buffer empty
			{			
				for(i = 0;i < tlen; )
				{

					while(!(TEST_SDIOx->BUF_CTL&(1<<0)))
					{

						TEST_SDIOx->DATA_BUF0 = *(uint32_t*)tempbuff++;
						i += 4;
						if(i == tlen)break;
					}
					TEST_SDIOx->MMC_IO   = 0x1; 
				}
//				TEST_SDIOx->MMC_IO_MBCTL &= ~0x1;
				if(TEST_SDIOx->CLR_MMC_INT&(1<<6))		//数据超时错误
				{										   
					TEST_SDIOx->CLR_MMC_INT = 1<<6; 		//清错误标志
					return SD_DATA_TIMEOUT;
				}else if(TEST_SDIOx->CLR_MMC_INT&(1<<2))	//数据块CRC错误
				{
					TEST_SDIOx->CLR_MMC_INT = 1<<2; 		//清错误标志
					return SD_DATA_CRC_FAIL;		   
				}
				
				while(!(TEST_SDIOx->BUF_CTL&(1<<1)));
				TEST_SDIOx->CLR_MMC_INT |= 0;
				i = 1000;while(i--);
				TEST_SDIOx->MMC_IO          = 0x0;
				TEST_SDIOx->BUF_CTL         = 0x8800;    //disable dma, write sd card
				timeout=0X3FFFFFFF;	//写数据溢出时间
			}
    }
		
    #endif
    
    if(nblks>1)
    {
      if((SDIO_STD_CAPACITY_SD_CARD_V1_1==CardType)||(SDIO_STD_CAPACITY_SD_CARD_V2_0==CardType)||(SDIO_HIGH_CAPACITY_SD_CARD==CardType))
      {
        SDIO_Send_Cmd(SD_CMD_STOP_TRANSMISSION,1,0);		//发送CMD12+结束传输 	   
        errorstatus=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);//等待R1响应   
        if(errorstatus!=SD_OK)return errorstatus;	 
      }
    }

		INTX_ENABLE();				//开启总中断
//		TEST_SDIOx->ICR=0X5FF;	 		//清除所有标记	  
    TEST_SDIOx->CLR_MMC_INT = 0xFF;
	}else if(DeviceMode==SD_DMA_MODE)
	{
//   		TransferError=SD_OK;
//		if(nblks>1)StopCondition=1;	//多块写,需要发送停止传输指令 
//		else StopCondition=0;		//单块写,不需要发送停止传输指令  
//		TransferEnd=0;				//传输结束标置位，在中断服务置1
//		TEST_SDIOx->MASK|=(1<<1)|(1<<3)|(1<<8)|(1<<4)|(1<<9);	//配置产生数据接收完成中断
//		SD_DMA_Config((uint32_t*)buf,nblks*blksize,1);		//SDIO DMA配置
// 	 	TEST_SDIOx->DCTRL|=1<<3;								//SDIO DMA使能.  
//    TEST_SDIOx->BUF_CTL |= (1<<14);
// 		while(((DMA2->LISR&(1<<27))==RESET)&&timeout)timeout--;//等待传输完成 
//		if(timeout==0)
//		{
//  			SD_Init();	 					//重新初始化SD卡,可以解决写入死机的问题
//			return SD_DATA_TIMEOUT;			//超时	 
// 		}
//		timeout=SDIO_DATATIMEOUT;
//		while((TransferEnd==0)&&(TransferError==SD_OK)&&timeout)timeout--;
// 		if(timeout==0)return SD_DATA_TIMEOUT;			//超时	 
//  		if(TransferError!=SD_OK)return TransferError;
  }  
//  TEST_SDIOx->ICR=0X5FF;	 		//清除所有标记
  TEST_SDIOx->CLR_MMC_INT = 0xFF;
 	errorstatus=IsCardProgramming(&cardstate);
 	while((errorstatus==SD_OK)&&((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)))
	{
		errorstatus=IsCardProgramming(&cardstate);
	}   
	return errorstatus;
} 
//SDIO中断服务函数		  
void SDIO_IRQHandler(void) 
{											
 	SD_ProcessIRQSrc();//处理所有SDIO相关中断
}	 																    
//SDIO中断处理函数
//处理SDIO传输过程中的各种中断事务
//返回值:错误代码
SD_Error SD_ProcessIRQSrc(void)
{
//	if(TEST_SDIOx->STA&(1<<8))//接收完成中断
  if(TEST_SDIOx->CLR_MMC_INT & 0x01)
	{	 
		if (StopCondition==1)
		{
			SDIO_Send_Cmd(SD_CMD_STOP_TRANSMISSION,1,0);		//发送CMD12,结束传输 	   
			TransferError=CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		}else TransferError = SD_OK;	
// 		TEST_SDIOx->ICR|=1<<8;//清除完成中断标记
    TEST_SDIOx->CLR_MMC_INT = 1<<0;
    
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
    TEST_SDIOx->MMC_INT_MASK &= ~((1<<0)|(1<<1)|(1<<6)|(1<<7));
 		TransferEnd = 1;
		return(TransferError);
	}
// 	if(TEST_SDIOx->STA&(1<<1))//数据CRC错误
  if(TEST_SDIOx->CLR_MMC_INT&(1<<2))
	{
//		TEST_SDIOx->ICR|=1<<1;//清除中断标记
    TEST_SDIOx->CLR_MMC_INT = 1<<2;
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
    TEST_SDIOx->MMC_INT_MASK &= ~((1<<0)|(1<<1)|(1<<6)|(1<<7));
	    TransferError = SD_DATA_CRC_FAIL;
	    return(SD_DATA_CRC_FAIL);
	}
// 	if(TEST_SDIOx->STA&(1<<3))//数据超时错误
  if(TEST_SDIOx->CLR_MMC_INT&(1<<6))
	{
//		TEST_SDIOx->ICR|=1<<3;//清除中断标记
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
    TEST_SDIOx->MMC_INT_MASK &= ~((1<<0)|(1<<1)|(1<<6)|(1<<7));
	    TransferError = SD_DATA_TIMEOUT;
	    return(SD_DATA_TIMEOUT);
	}
//  	if(TEST_SDIOx->STA&(1<<5))//FIFO上溢错误
//	{
//		TEST_SDIOx->ICR|=1<<5;//清除中断标记
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
//	    TransferError = SD_RX_OVERRUN;
//	    return(SD_RX_OVERRUN);
//	}
//   	if(TEST_SDIOx->STA&(1<<4))//FIFO下溢错误
//	{
//		TEST_SDIOx->ICR|=1<<4;//清除中断标记
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
//	    TransferError = SD_TX_UNDERRUN;
//	    return(SD_TX_UNDERRUN);
//	}
//	if(TEST_SDIOx->STA&(1<<9))//起始位错误
//	{
//		TEST_SDIOx->ICR|=1<<9;//清除中断标记
//		TEST_SDIOx->MASK&=~((1<<1)|(1<<3)|(1<<8)|(1<<14)|(1<<15)|(1<<4)|(1<<5)|(1<<9));//关闭相关中断
//	    TransferError = SD_START_BIT_ERR;
//	    return(SD_START_BIT_ERR);
//	}
	return(SD_OK);
}
  
//检查CMD0的执行状态
//返回值:sd卡错误码
SD_Error CmdError(void)
{
	SD_Error errorstatus = SD_OK;
	uint32_t timeout=SDIO_CMD0TIMEOUT;	   
	while(timeout--)
	{
//		if(TEST_SDIOx->STA&(1<<7))break;	//命令已发送(无需响应)	 
    if(((TEST_SDIOx->MMC_IO>>2)&0x3) == 0)break;
	}	    
	if(timeout==0)return SD_CMD_RSP_TIMEOUT;  
//	TEST_SDIOx->ICR=0X5FF;				//清除标记
  TEST_SDIOx->CLR_MMC_INT = 0xFF;
	return errorstatus;
}	 
//检查R7响应的错误状态
//返回值:sd卡错误码
SD_Error CmdResp7Error(void)
{
	SD_Error errorstatus=SD_OK;
	uint32_t status;
	uint32_t timeout=SDIO_CMD0TIMEOUT;
 	while(timeout--)
	{
//		status=TEST_SDIOx->STA;
//		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
    status = TEST_SDIOx->CLR_MMC_INT ;
    if(status&((1<<7)|(1<<6)|(1<<0)))break;   
	}
 	if((timeout==0)||(status&(1<<6)))	//响应超时
	{																				    
		errorstatus=SD_CMD_RSP_TIMEOUT;	//当前卡不是2.0兼容卡,或者不支持设定的电压范围
//		TEST_SDIOx->ICR|=1<<2;				//清除命令响应超时标志
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
		return errorstatus;
	}	 
	if(status&1<<0)						//成功接收到响应
	{								   
		errorstatus=SD_OK;
//		TEST_SDIOx->ICR|=1<<6;				//清除响应标志
    TEST_SDIOx->CLR_MMC_INT = 1<<0;
 	}
	return errorstatus;
}	   
//检查R1响应的错误状态
//cmd:当前命令
//返回值:sd卡错误码
SD_Error CmdResp1Error(uint8_t cmd)
{	  
  uint32_t status; 
  uint32_t response;
	while(1)
	{
//		status=TEST_SDIOx->STA;
//		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)
    status = TEST_SDIOx->CLR_MMC_INT ;
    if(status&((1<<7)|(1<<6)|(1<<0)))break;   
	} 

  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp1 = %2d\r\n",TEST_SDIOx->CMD_BUF[4]&0x3F));
	if(status&(1<<6))					//响应超时
	{																				    
// 		TEST_SDIOx->ICR=1<<2;					//清除命令响应超时标志
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp1Error = %2d(SD_CMD_RSP_TIMEOUT)\r\n",cmd));
		return SD_CMD_RSP_TIMEOUT;
	}	
 	if(status&(1<<7))					//CRC错误
	{																				    
// 		TEST_SDIOx->ICR=1<<0;					//清除标志
    TEST_SDIOx->CLR_MMC_INT = 1<<7;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp1Error = %2d(SD_CMD_CRC_FAIL)\r\n",cmd));
		return SD_CMD_CRC_FAIL;
	}		
  TEST_SDIOx->CLR_MMC_INT = 0xFF;
  
  if((TEST_SDIOx->CMD_BUF[4]&0x3F) != cmd)
  {
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp1Error = %2d(SD_ILLEGAL_CMD)\r\n",cmd));
    return SD_ILLEGAL_CMD;//命令不匹配 
  }
//	if(TEST_SDIOx->RESPCMD!=cmd)return SD_ILLEGAL_CMD;//命令不匹配 
  response=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];	
//  TEST_SDIOx->ICR=0X5FF;	 				//清除标记
 	//UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp1Error = %2d(%d)\r\n",cmd,response&SD_OCR_ERRORBITS));
	return (SD_Error)(response&SD_OCR_ERRORBITS);//返回卡响应
}
//检查R3响应的错误状态
//返回值:错误状态
SD_Error CmdResp3Error(void)
{
	uint32_t status;		
  
 	while(1)
	{
//		status=TEST_SDIOx->STA;
//		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
    status = TEST_SDIOx->CLR_MMC_INT ;
    if(status&((1<<7)|(1<<6)|(1<<0)))break;   
	}

 	if(status&(1<<6))					//响应超时
	{											 
//		TEST_SDIOx->ICR|=1<<2;				//清除命令响应超时标志
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp3Error = SD_CMD_RSP_TIMEOUT\r\n"));  
		return SD_CMD_RSP_TIMEOUT;
	}	 
//   	TEST_SDIOx->ICR=0X5FF;	 				//清除标记
  TEST_SDIOx->CLR_MMC_INT = 0xFF;
  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp3Error = SD_OK\r\n"));  
 	return SD_OK;								  
}
//检查R2响应的错误状态
//返回值:错误状态
SD_Error CmdResp2Error(void)
{
	SD_Error errorstatus=SD_OK;
	uint32_t status;
	uint32_t timeout=SDIO_CMD0TIMEOUT;
   
 	while(timeout--)
	{
//		status=TEST_SDIOx->STA;
//		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
    status = TEST_SDIOx->CLR_MMC_INT ;
    if(status&((1<<7)|(1<<6)|(1<<0)))break;   
	}

  	if((timeout==0)||(status&(1<<6)))	//响应超时
	{																				    
		errorstatus=SD_CMD_RSP_TIMEOUT; 
//		TEST_SDIOx->ICR|=1<<2;				//清除命令响应超时标志
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp2Error = SD_CMD_RSP_TIMEOUT\r\n")); 
		return errorstatus;
	}	 
	if(status&(1<<7))						//CRC错误
	{								   
		errorstatus=SD_CMD_CRC_FAIL;
//		TEST_SDIOx->ICR|=1<<0;				//清除响应标志
    TEST_SDIOx->CLR_MMC_INT = 1<<7;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp2Error = SD_CMD_CRC_FAIL\r\n")); 
 	}
  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp2Error = SD_OK\r\n")); 
//	TEST_SDIOx->ICR=0X5FF;	 				//清除标记
  TEST_SDIOx->CLR_MMC_INT = 0xff;
 	return errorstatus;								    		 
} 
//检查R6响应的错误状态
//cmd:之前发送的命令
//prca:卡返回的RCA地址
//返回值:错误状态
SD_Error CmdResp6Error(uint8_t cmd,uint16_t*prca)
{
	SD_Error errorstatus=SD_OK;
	uint32_t status;					    
	uint32_t rspr1;
 	while(1)
	{
//		status=TEST_SDIOx->STA;
//		if(status&((1<<0)|(1<<2)|(1<<6)))break;//CRC错误/命令响应超时/已经收到响应(CRC校验成功)	
    status = TEST_SDIOx->CLR_MMC_INT ;
    if(status&((1<<7)|(1<<6)|(1<<0)))break;   
	}

	if(status&(1<<6))					//响应超时
	{
      
// 		TEST_SDIOx->ICR|=1<<2;				//清除命令响应超时标志
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_CMD_RSP_TIMEOUT\r\n")); 
		return SD_CMD_RSP_TIMEOUT;
	}	 	 
	if(status&(1<<7))						//CRC错误
	{								   
//		TEST_SDIOx->ICR|=1<<0;				//清除响应标志
    TEST_SDIOx->CLR_MMC_INT = 1<<7;
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_CMD_CRC_FAIL\r\n")); 
 		return SD_CMD_CRC_FAIL;
	}
//if(TEST_SDIOx->RESPCMD!=cmd)					//判断是否响应cmd命令 TEST_SDIOx->CMD_BUF4&0x3F
  if((TEST_SDIOx->CMD_BUF[4]&0x3F)!=cmd)
	{
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_ILLEGAL_CMD\r\n")); 
 		return SD_ILLEGAL_CMD; 		
	}	    
//	TEST_SDIOx->ICR=0X5FF;	 				//清除所有标记
  TEST_SDIOx->CLR_MMC_INT = 0xFF;
	rspr1=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];						//得到响应 	 
	if(SD_ALLZERO==(rspr1&(SD_R6_GENERAL_UNKNOWN_ERROR|SD_R6_ILLEGAL_CMD|SD_R6_COM_CRC_FAILED)))
	{
		*prca=(uint16_t)(rspr1>>16);			//右移16位得到,rca
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_OK(%d)\r\n",errorstatus)); 
		return errorstatus;
	}
  if(rspr1&SD_R6_GENERAL_UNKNOWN_ERROR)
  {
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_R6_GENERAL_UNKNOWN_ERROR\r\n")); 
    return SD_GENERAL_UNKNOWN_ERROR;
  }
  if(rspr1&SD_R6_ILLEGAL_CMD)
  {
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_ILLEGAL_CMD\r\n")); 
    return SD_ILLEGAL_CMD;
  }
  if(rspr1&SD_R6_COM_CRC_FAILED)
  {
    //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_R6_COM_CRC_FAILED\r\n")); 
    return SD_COM_CRC_FAILED;
  }
  //UartSendGroup((uint8_t*)printBuf, sprintf(printBuf,"CmdResp6Error = SD_OK\r\n")); 
	return errorstatus;
}

//SDIO使能宽总线模式
//enx:0,不使能;1,使能;
//返回值:错误状态
SD_Error SDEnWideBus(uint8_t enx)
{
	SD_Error errorstatus = SD_OK;
  uint32_t response;
 	//uint32_t scr[2]={0,0};
	uint8_t arg=0X00;
	if(enx)arg=0X02;
	else arg=0X00;
// 	if(TEST_SDIOx->RESP1&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//SD卡处于LOCKED状态		  
  response = TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];	 
  if(response&SD_CARD_LOCKED)return SD_LOCK_UNLOCK_FAILED;//SD卡处于LOCKED状态	
	
// 	errorstatus=FindSCR(RCA,scr);						//得到SCR寄存器数据20190504,暂时注释掉，因为兼容性不好
  
 	if(errorstatus!=SD_OK)return errorstatus;
//	if((scr[1]&SD_WIDE_BUS_SUPPORT)!=SD_ALLZERO)		//支持宽总线 tao
	{
	 	SDIO_Send_Cmd(SD_CMD_APP_CMD,1,(uint32_t)RCA<<16);	//发送CMD55+RCA,短响应			
    
	 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
	 	if(errorstatus!=SD_OK)return errorstatus; 
    
	 	SDIO_Send_Cmd(SD_CMD_APP_SD_SET_BUSWIDTH,1,arg);//发送ACMD6,短响应,参数:10,4位;00,1位.											  
		errorstatus=CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
		return errorstatus;
	}
  //else return SD_REQUEST_NOT_APPLICABLE;				//不支持宽总线设置 	 
}												   
//检查卡是否正在执行写操作
//pstatus:当前状态.
//返回值:错误代码
SD_Error IsCardProgramming(uint8_t *pstatus)
{
 	vu32 respR1 = 0, status = 0; 
  while(!(TEST_SDIOx->BUF_CTL & 0x02));
  SDIO_Send_Cmd(SD_CMD_SEND_STATUS,1,(uint32_t)RCA<<16);		//发送CMD13 	   
//  	status=TEST_SDIOx->STA;
//	while(!(status&((1<<0)|(1<<6)|(1<<2))))status=TEST_SDIOx->STA;//等待操作完成
  
  status = TEST_SDIOx->CLR_MMC_INT ;
  while(!(status&((1<<7)|(1<<6)|(1<<0))))status = TEST_SDIOx->CLR_MMC_INT ;   
  
   	if(status&(1<<7))			//CRC检测失败
	{
//		TEST_SDIOx->ICR|=1<<0;		//清除错误标记
    TEST_SDIOx->CLR_MMC_INT = 1<<7;
		return SD_CMD_CRC_FAIL;
	}
   	if(status&(1<<6))			//命令超时 
	{
//		TEST_SDIOx->ICR|=1<<2;		//清除错误标记
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
		return SD_CMD_RSP_TIMEOUT;
	}
// 	if(TEST_SDIOx->RESPCMD!=SD_CMD_SEND_STATUS)return SD_ILLEGAL_CMD;
  if((TEST_SDIOx->CMD_BUF[4]&0x3F)!=SD_CMD_SEND_STATUS)return SD_ILLEGAL_CMD;
//	TEST_SDIOx->ICR=0X5FF;	 		//清除所有标记
  TEST_SDIOx->CLR_MMC_INT = 0xff;
	respR1=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];	
	*pstatus=(uint8_t)((respR1>>9)&0x0000000F);
	return SD_OK;
}
//读取当前卡状态
//pcardstatus:卡状态
//返回值:错误代码
SD_Error SD_SendStatus(uint32_t *pcardstatus)
{
	SD_Error errorstatus = SD_OK;
	if(pcardstatus==NULL)
	{
		errorstatus=SD_INVALID_PARAMETER;
		return errorstatus;
	}
 	SDIO_Send_Cmd(SD_CMD_SEND_STATUS,1,RCA<<16);	//发送CMD13,短响应		 
	errorstatus=CmdResp1Error(SD_CMD_SEND_STATUS);	//查询响应状态 
	if(errorstatus!=SD_OK)return errorstatus;
	*pcardstatus=TEST_SDIOx->CMD_BUF[3]<<24  |TEST_SDIOx->CMD_BUF[2]<<16  |TEST_SDIOx->CMD_BUF[1]<<8  | TEST_SDIOx->CMD_BUF[0];	//读取响应值
	return errorstatus;
} 
//返回SD卡的状态
//返回值:SD卡状态
SDCardState SD_GetState(void)
{
	uint32_t resp1=0;
	if(SD_SendStatus(&resp1)!=SD_OK)return SD_CARD_ERROR;
	else return (SDCardState)((resp1>>9) & 0x0F);
}


//查找SD卡的SCR寄存器值
//rca:卡相对地址
//pscr:数据缓存区(存储SCR内容)
//返回值:错误状态		   
SD_Error FindSCR(uint16_t rca,uint32_t *pscr)
{
	uint32_t index = 0; 
	SD_Error errorstatus = SD_OK;
	uint8_t tempscr[8]={0,0};  
 	SDIO_Send_Cmd(SD_CMD_SET_BLOCKLEN,1,8);			//发送CMD16,短响应,设置Block Size为8字节											  
 	errorstatus=CmdResp1Error(SD_CMD_SET_BLOCKLEN);
 	if(errorstatus!=SD_OK)return errorstatus;	    
  	SDIO_Send_Cmd(SD_CMD_APP_CMD,1,(uint32_t)rca<<16);	//发送CMD55,短响应 									  
 	errorstatus=CmdResp1Error(SD_CMD_APP_CMD);
 	if(errorstatus!=SD_OK)return errorstatus;
	SDIO_Send_Data_Cfg(SD_DATATIMEOUT,8,3,1);		//8个字节长度,block为8字节,SD卡到SDIO.
   	SDIO_Send_Cmd(SD_CMD_SD_APP_SEND_SCR,1,0);		//发送ACMD51,短响应,参数为0											  
 	errorstatus=CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);
 	if(errorstatus!=SD_OK)return errorstatus;							   
// 	while(!(TEST_SDIOx->STA&(SDIO_FLAG_RXOVERR|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DBCKEND|SDIO_FLAG_STBITERR)))
  
  for( index = 0; index < 8;index ++ )
  {
    #if 0
			*(tempscr+index)=TEST_SDIOx->CMD_BUF[index];	//读取buf
    #else
      *(tempscr+index)=TEST_SDIOx->DATA_BUF0;	//读取buf
    #endif
			index++;
  }

 	if(TEST_SDIOx->CLR_MMC_INT&(1<<6))		//接收数据超时
	{										 
// 		TEST_SDIOx->ICR|=1<<3;		//清除标记
    TEST_SDIOx->CLR_MMC_INT = 1<<6;
		return SD_DATA_TIMEOUT;
	}
	else if(TEST_SDIOx->CLR_MMC_INT&(1<<7))	//已发送/接收的数据块CRC校验错误
	{
// 		TEST_SDIOx->ICR|=1<<1;		//清除标记
    TEST_SDIOx->CLR_MMC_INT = 1<<7;
		return SD_DATA_CRC_FAIL;   
	}
//	else if(TEST_SDIOx->STA&(1<<5))	//接收FIFO溢出
//	{
// 		TEST_SDIOx->ICR|=1<<5;		//清除标记
//		return SD_RX_OVERRUN;   	   
//	}
//	else if(TEST_SDIOx->STA&(1<<9))	//起始位检测错误
//	{
// 		TEST_SDIOx->ICR|=1<<9;		//清除标记
//		return SD_START_BIT_ERR;    
//	}
//   	TEST_SDIOx->ICR=0X5FF;	 		//清除标记	 
  TEST_SDIOx->CLR_MMC_INT = 0xff;
	//把数据顺序按8位为单位倒过来.   	
	*(pscr+1)=((tempscr[0])<<24)|((tempscr[1])<<8)|((tempscr[2])>>8)|((tempscr[3])>>24);
	*(pscr)=((tempscr[7])<<24)|((tempscr[6])<<8)|((tempscr[5])>>8)|((tempscr[4])>>24);
 	return errorstatus;
} 
//配置SDIO DMA  
//mbuf:存储器地址
//bufsize:传输数据量
//dir:方向;1,存储器-->SDIO(写数据);0,SDIO-->存储器(读数据);
void SD_DMA_Config(uint32_t*mbuf,uint32_t bufsize,uint8_t dir)
{		 
// 	uint32_t tmpreg=0;//重新设置
//	while(DMA2_Stream3->CR&0X01);	//等待DMA可配置 
//	DMA2->LIFCR|=0X3D<<22;			//清空之前该stream3上的所有中断标志
//	
//	
//	DMA2_Stream3->PAR=(uint32_t)&TEST_SDIOx->FIFO;	//DMA2 外设地址
//	DMA2_Stream3->M0AR=(uint32_t)mbuf; 	//DMA2,存储器0地址;	 
//	DMA2_Stream3->NDTR=0; 			//DMA2,传输数据量0,外设流控制 
//	tmpreg|=dir<<6;		//数据传输方向控制
//	tmpreg|=0<<8;		//非循环模式(即使用普通模式)
//	tmpreg|=0<<9;		//外设非增量模式
//	tmpreg|=1<<10;		//存储器增量模式
//	tmpreg|=2<<11;		//外设数据长度:32位
//	tmpreg|=2<<13;		//存储器数据长度:32位
//	tmpreg|=3<<16;		//最高优先级
//	tmpreg|=1<<21;		//外设突发4次传输
//	tmpreg|=1<<23;		//存储器突发4次传输
//	tmpreg|=(uint32_t)4<<25;	//通道选择
//	DMA2_Stream3->CR=tmpreg; 
//	
//	tmpreg=DMA2_Stream3->FCR;
//	tmpreg&=0XFFFFFFF8;	//清除DMDIS和FTH
//	tmpreg|=1<<2;		//FIFO使能
//	tmpreg|=3<<0;		//全FIFO
//	DMA2_Stream3->FCR=tmpreg;
//	DMA2_Stream3->CR|=1<<5;	//外设流控制 
//	DMA2_Stream3->CR|=1<<0;	//开启DMA传输 
}   
//读SD卡
//buf:读数据缓存区
//sector:扇区地址
//cnt:扇区个数	
//返回值:错误状态;0,正常;其他,错误代码;				

uint8_t SD_ReadDisk(uint8_t*buf,uint32_t sector,uint32_t cnt)
{

	#if 1
	uint8_t sta=SD_OK;
	long long lsector=sector;
	uint32_t temp = 1;
	uint32_t n;
//	if(CardType != SDIO_STD_CAPACITY_SD_CARD_V1_1)//2020.12.14 对各类卡全执行，不挑卡
	{
		temp = 1<<9;
		lsector <<= 9;
	}
	for(n= 0;n < cnt ;n++)
	{
    
		sta = SD_ReadBlocks(buf,lsector,512,1);	//单个/多个sector  
		lsector += temp;
		buf += 512;
	}
  
	return sta;
	#endif
}
//写SD卡
//buf:写数据缓存区
//sector:扇区地址
//cnt:扇区个数	
//返回值:错误状态;0,正常;其他,错误代码;	
uint8_t SD_WriteDisk(uint8_t*buf,uint32_t sector,uint32_t cnt)
{

	#if 1
	uint8_t sta=SD_OK;
	uint32_t temp = 1;
	uint32_t n;
	long long lsector=sector;
	if(CardType != SDIO_STD_CAPACITY_SD_CARD_V1_1)
	{	
		temp = 1<<9;
		lsector <<= 9;
	}
	for(n=0;n<cnt;n++)
	{
		memcpy(SDIO_DATA_BUFFER,buf,512);
		sta=SD_WriteBlocks(SDIO_DATA_BUFFER,lsector,512,1);//单个sector的写操作
		lsector += temp;
		buf+=512;
	} 

	return sta;
	#endif
}
#endif // MICROPY_HW_ENABLE_SDCARD || MICROPY_HW_ENABLE_MMCARD






