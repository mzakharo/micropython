/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2020 Bitcraze AB
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
 */

/*
 * ledseq.c - LED sequence handler
 */

#include <stdbool.h>

#include "ledseq.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "led.h"
#include "config.h"
#include "esp_log.h"

#define LEDSEQ_CHARGE_CYCLE_TIME_500MA  1000
#define LEDSEQ_CHARGE_CYCLE_TIME_MAX    500

/* Led ledseqStep_t */
//led序列
static ledseqStep_t const *sequences[] = 
{
	seq_lowbat,
	seq_charged,
	seq_charging,
	seq_calibrated,
	seq_alive,
	seq_linkup,
};
//低电压序列
ledseqStep_t const seq_lowbat[] = 
{
	{true, LEDSEQ_WAITMS(1000)},
	{0,LEDSEQ_LOOP},
};

//传感器校准完成序列
ledseqStep_t const seq_calibrated[] = 
{
	// {true, LEDSEQ_WAITMS(M2T(50))},
	// {false, LEDSEQ_WAITMS(M2T(450))},
	
	{true, LEDSEQ_WAITMS(M2T(1000))},
	{0,LEDSEQ_LOOP},
};
//开机序列
ledseqStep_t const seq_alive[] = 
{
	{true, LEDSEQ_WAITMS(M2T(50))},
	{false, LEDSEQ_WAITMS(M2T(1950))},
	{0,LEDSEQ_LOOP},
};
//通信连接序列
ledseqStep_t const seq_linkup[] = 
{
	// {true, LEDSEQ_WAITMS(M2T(20))},
	// {false, LEDSEQ_WAITMS(20)},
	{true, LEDSEQ_WAITMS(M2T(100))},
	{0,LEDSEQ_LOOP},
};
//电池充电完成序列
ledseqStep_t const seq_charged[] = 
{
	{true, LEDSEQ_WAITMS(M2T(1000))},
	{0,LEDSEQ_LOOP},
};
//电池充电进行中序列
ledseqStep_t const seq_charging[] = 
{
	{true, LEDSEQ_WAITMS(M2T(200))},
	{false, LEDSEQ_WAITMS(M2T(800))},
	{0,LEDSEQ_LOOP},
};
#define SEQ_NUM (sizeof(sequences)/sizeof(sequences[0]))  //序列数量

static int activeSeq[LED_NUM]; /*每个LED对应的活动优先级序列*/
static int state[LED_NUM][SEQ_NUM];	/*每个LED对应的序列的当前位置*/
static xTimerHandle timer[LED_NUM];  //软件的定时器
static xSemaphoreHandle ledseqSem;	/*信号量*/
static bool isInit = false;
static bool ledseqEnabled = false;
static void runLedseq( xTimerHandle xTimer );
//初始化
void ledseqInit() {
	if(isInit) {
		return;
	}

	ledInit();

	//Initialise the sequences state
	for(int i=0; i<LED_NUM; i++) {
		activeSeq[i] = LEDSEQ_STOP;
		for(int j=0;j<SEQ_NUM;j++){
			state[i][j] = LEDSEQ_STOP;
		}
	}

	//Init the soft timers that runs the led sequences for each leds
	for(int i=0; i<LED_NUM; i++) { //创建软件定时器
		//timer[i] = xTimerCreateStatic("ledseqTimer",1000, pdFALSE, (void*)i, runLedseq, &timerBuffer[i]);
		timer[i] = xTimerCreate("ledseqTimer", M2T(1000), pdFALSE, (void*)i, runLedseq);
	}
	vSemaphoreCreateBinary(ledseqSem);	/*创建一个2值信号量*/
	
	isInit = true;
	ledseqTest();
}

void ledseqEnable(bool enable) {
  ledseqEnabled = enable;
}

bool ledseqTest(void) {
  bool status;

  status = isInit & ledTest();

  ledseqEnable(true);

  return status;
}

void ledseqSetTimes(ledseqStep_t *sequence, int onTime, int offTime)
{
	sequence[0].action = onTime;
	sequence[1].action = offTime;
}
/*获取led序列优先级*/
static int getPrio(const ledseqStep_t *seq)
{
	int prio;

	for(prio=0; prio<SEQ_NUM; prio++)
		if(sequences[prio]==seq) return prio;

	return -1; /*无效序列*/
}
// Utility functions

static void updateActive(led_t led) {
  	int prio;
	ledSet(led, false);
	activeSeq[led]=LEDSEQ_STOP;
	
	for(prio=0;prio<SEQ_NUM;prio++)
	{
		if (state[led][prio] != LEDSEQ_STOP)
		{
			activeSeq[led]=prio;
			break;
		}
	}
}

/*FreeRTOS 定时器回调函数*/
static void runLedseq( xTimerHandle xTimer )
{
	bool leave = false;
	const ledseqStep_t *step = NULL;
	led_t led = (led_t)pvTimerGetTimerID(xTimer);

	if (!ledseqEnabled) return;

	while(!leave) 
	{
		int prio = activeSeq[led];

		if (prio == LEDSEQ_STOP)
			return;

		step = &sequences[prio][state[led][prio]];

		state[led][prio]++;

		xSemaphoreTake(ledseqSem, portMAX_DELAY);
		switch(step->action)
		{
			case LEDSEQ_LOOP:
				state[led][prio] = 0;
				break;
			case LEDSEQ_STOP:
				state[led][prio] = LEDSEQ_STOP;
				updateActive(led);
				break;
			default:  /*LED定时*/
				ledSet(led, step->value);	/*定时step->value*/
				if (step->action == 0)
					break;
				xTimerChangePeriod(xTimer, step->action, 0);
				xTimerStart(xTimer, 0);
				leave=true;
				break;
		}
		xSemaphoreGive(ledseqSem);
	}
}

//运行序列
void ledseqRun(led_t led ,const ledseqStep_t *sequence) {

	int prio = getPrio(sequence);	/*获取led优先级序列*/

	if(prio<0) return;

	xSemaphoreTake(ledseqSem, portMAX_DELAY);
	state[led][prio] = 0; 
	updateActive(led);
	xSemaphoreGive(ledseqSem);

	if(activeSeq[led] == prio)	/*当前序列优先级等于活动序列优先级*/
		runLedseq(timer[led]);
}

/*停止led的sequence序列*/
void ledseqStop(led_t led, const ledseqStep_t *sequence)
{
	int prio = getPrio(sequence);

	if(prio<0) return;

	xSemaphoreTake(ledseqSem, portMAX_DELAY);
	state[led][prio] = LEDSEQ_STOP;  
	updateActive(led);
	xSemaphoreGive(ledseqSem);

	runLedseq(timer[led]);
}
void ledseqDeInit(void)
{
	int i = 0;
	for(i=0; i<LED_NUM; i++) {
		activeSeq[i] = LEDSEQ_STOP;
		for(int j=0;j<SEQ_NUM;j++){
			state[i][j] = LEDSEQ_STOP;
		}
	}
	ledseqEnable(false);
	ledDeInit();
	
	for(i=0; i<LED_NUM; i++) { 
		xTimerStop( timer[i], 0 );
	}
	vSemaphoreDelete(ledseqSem);
	
	isInit = false;
}



