/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * console.c - Used to send console data to client
 */

#include <stdbool.h>
#include <string.h>

/*FreeRtos includes*/
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/semphr.h"
#include "console.h"
#include "anop.h"
#include "mod_wifllink.h"

#if 1

static UDPPacket messageToPrint;
static bool messageSendingIsPending = false;
static xSemaphoreHandle synch = NULL;

static const char bufferFullMsg[] = "<F>\n";
static bool isInit;

static void addBufferFullMarker();

/**
 * Send the data to the client
 * returns TRUE if successful otherwise FALSE
 */
static bool consoleSendMessage(void)
{
	if(anopSendLogStr(messageToPrint.data,messageToPrint.size,2))
	// if (crtpSendPacket(&messageToPrint) == pdTRUE)
	{
		messageToPrint.size = 0;
		messageSendingIsPending = false;
	}
	else
	{
		return false;
	}

	return true;
}

void consoleInit()
{
  if (isInit)
    return;
  messageToPrint.size = 0;
  vSemaphoreCreateBinary(synch);
  messageSendingIsPending = false;

  isInit = true;
}

bool consoleTest(void)
{
  return isInit;
}

/* 输入一个字符到console缓冲区*/
int consolePutchar(int ch)
{
  bool isInInterrupt = false; //bool isInInterrupt = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;

  if (!isInit) {
    return 0;
  }

  if (isInInterrupt) {
    return consolePutcharFromISR(ch);
  }

  if (xSemaphoreTake(synch, portMAX_DELAY) == pdTRUE)
  {
    // Try to send if we already have a pending message
    if (messageSendingIsPending) 
    {
      consoleSendMessage();
    }

    if (! messageSendingIsPending) 
    {
      if (messageToPrint.size < ANO_MAX_DATA_SIZE-4)
      {
        messageToPrint.data[messageToPrint.size] = (unsigned char)ch;
        messageToPrint.size++;
      }

      if (ch == '\n' || messageToPrint.size >= ANO_MAX_DATA_SIZE-4)
      {
        //if (crtpGetFreeTxQueuePackets() == 1) //获取队列发送个数
        {
          addBufferFullMarker();	
        }
        messageSendingIsPending = true;
        consoleSendMessage();
      }
    }
    xSemaphoreGive(synch);
  }

  return (unsigned char)ch;
}

/* 中断方式输入一个字符到console缓冲区*/
int consolePutcharFromISR(int ch) {
  BaseType_t higherPriorityTaskWoken;

  if (xSemaphoreTakeFromISR(synch, &higherPriorityTaskWoken) == pdTRUE) {
    if (messageToPrint.size < ANO_MAX_DATA_SIZE-4)
    {
      messageToPrint.data[messageToPrint.size] = (unsigned char)ch;
      messageToPrint.size++;
    }
    xSemaphoreGiveFromISR(synch, &higherPriorityTaskWoken);
  }

  return ch;
}
/* 输入一个字符串到console缓冲区*/
int consolePuts(char *str)
{
  int ret = 0;

  while(*str)
    ret |= consolePutchar(*str++);

  return ret;
}

void consoleFlush(void)
{
  if (xSemaphoreTake(synch, portMAX_DELAY) == pdTRUE)
  {
    consoleSendMessage();
    xSemaphoreGive(synch);
  }
}


static int findMarkerStart()
{
  int start = messageToPrint.size;
  
  // If last char is new line, rewind one char since the marker contains a new line.
  if (start > 0 && messageToPrint.data[start - 1] == '\n')
  {
    start -= 1;
  }

  return start;
}

static void addBufferFullMarker()
{
  // Try to add the marker after the message if it fits in the buffer, otherwise overwrite the end of the message 
  int endMarker = findMarkerStart() + sizeof(bufferFullMsg);
  if (endMarker >= (ANO_MAX_DATA_SIZE-4)) 
  {
    endMarker = ANO_MAX_DATA_SIZE-4;
  }

  int startMarker = endMarker - sizeof(bufferFullMsg);
  memcpy(&messageToPrint.data[startMarker], bufferFullMsg, sizeof(bufferFullMsg));
  messageToPrint.size = startMarker + sizeof(bufferFullMsg);
}

#endif
