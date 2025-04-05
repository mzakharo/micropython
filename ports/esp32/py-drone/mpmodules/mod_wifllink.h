
/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	mod_wifllink.h
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/4/21
	* Description			:	
******************************************************************************/
#ifndef MICROPY_INCLUDED_ESP32_MOD_WIFILINK_H
#define MICROPY_INCLUDED_ESP32_MOD_WIFILINK_H


#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include <stdbool.h>
#include <stdint.h>
#include "anop.h"

#define WIFI_RX_TX_PACKET_SIZE   (128)
typedef struct
{
  uint8_t size;
  uint8_t data[WIFI_RX_TX_PACKET_SIZE];
} UDPPacket;
//-------------------------------------------------------------------------------------------------------
extern const mp_obj_type_t drone_wifilink_type;

bool wifiGetDataBlocking(UDPPacket *in);
bool wifiSendData(uint32_t size, uint8_t *data);
bool getUDPConnectedStatus(void);

#endif // __VIDEO_H__
