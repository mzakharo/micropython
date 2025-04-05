
#include <stdio.h>

#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "extint.h"
#include "rtc.h"
#include "irq.h"

/// \moduleref pyb
/// \class RTC - real time clock
///
/// The RTC is and independent clock that keeps track of the date
/// and time.
///
/// Example usage:
///
///     rtc = pyb.RTC()
///     rtc.datetime((2021, 3, 2, 15, 18, 0))
///     print(rtc.datetime())
/******************************************************************************
 DEFINE TYPES
 

 ******************************************************************************/
STATIC bool rtc_is_init = false;
 
STATIC uint8_t weekday = 2;//实际不准
STATIC uint16_t SubSeconds = 2;//p秒实际不准

typedef struct {
    mp_obj_base_t base;
} pyb_rtc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_rtc_obj_t pyb_rtc_obj = { {&pyb_rtc_type} };

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
#define LSE_DELAY 2000

#define START_YEAR 			(2000)
#define START_MONTH 		(1)
#define START_DATA 			(1)

#define SECOND_DAY    	(86400)    //60*60*24
#define SECOND_HOUR    	(3600)    //60*60
#define SECOND_MIN    	(60)    //60
STATIC void RTC_SetData(RTC_DateTypeDef *sDate);
void rtc_init(void)
{
	if(rtc_is_init){
		return;
	}
	uint32_t overTime;
	uint32_t rtc_dat = RCC->BDCR;
	if(rtc_dat & (1<<15)){
		rtc_is_init = 1;
		RTC_DateTypeDef tDate;

		RTC_GetData(&tDate);
		RTC_SetData(&tDate);
		return;
	}
	
	RCC->APB1ENR |= (1<<29); //enable PWREN
	RCC->APB1ENR |= (1<<9); //enable BKPEN
	PWR->CR |= (1<<8); //Enable DBP 关闭写保护

	RCC->BDCR |= (1<<16);  //BDRST
	RCC->BDCR &= ~(1<<16);  //BDRST
	mp_hal_delay_us(200);
	
	RCC->BDCR |= 0x8101U;  //LSE作为RTC时钟
	mp_hal_delay_us(200);

	overTime = 0;
	while(!(RCC->BDCR & 0x02)){
			overTime++;
			mp_hal_delay_us(1);
			if(overTime > LSE_DELAY){

				break;
			}
	}

	overTime = 0;
	while(!(RTC->CRL & 0x20)){  //RTOFF
			overTime++;
			mp_hal_delay_ms(1);
			if(overTime > LSE_DELAY){

				break;
			}
	}
	
	RTC->CRL |= (1<<4); //进入配置模式
	
	RTC->CRL &= ~(1<<3); 
	mp_hal_delay_ms(1);

	RTC->PRLH = 0;
	RTC->PRLL = 0x7FFF;
	RTC->CRL &= ~(1<<4); //退出配置模式
	
	PWR->CR &= ~(1<<8); //写保护

	overTime = 0;
	while(!(RTC->CRL & 0x20)){  //RTOFF
			overTime++;
			mp_hal_delay_ms(1);
			if(overTime > LSE_DELAY){
				break;
			}
	}

	rtc_is_init = 1;
	
}
//---------------------------------------------------------------------------

const uint16_t mon_yday[][13] =
{
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

//判断一个年份是否为闰年，是就返回1，不是就返回0
inline bool isLeapYear(uint16_t year)
{
    return( (year%4 == 0 && year%100 != 0) || (year%400 == 0) );
}

//获取一年的天数
inline uint16_t getDaysForYear(uint16_t year)
{
    return (isLeapYear(year)?366:365);
}

//根据秒数计算日期
void RTC_GetData(RTC_DateTypeDef *sDate)
{
	uint32_t Second  = 0;
	uint16_t overTime = 0;
	while(!(RTC->CRL & 0x08)){
			overTime++;
			mp_hal_delay_us(1);
			if(overTime > 5000){
				break;
			}
	}
	Second = (uint32_t)((RTC->CNTH << 16)|RTC->CNTL);

	uint16_t days = (uint16_t)(Second / SECOND_DAY);  //计算计数的天数
	uint16_t curYear = START_YEAR;
	uint16_t leftDays = days;

    //calc year
    uint16_t daysCurYear = getDaysForYear(curYear);  //计算年天数
    while (leftDays >= daysCurYear)  //过一年了
    {
        leftDays -= daysCurYear;
        curYear++;
        daysCurYear = getDaysForYear(curYear);
    }
		sDate->Year = curYear;

    //calc month and Date
    bool isLeepYear = isLeapYear(curYear);
    for (uint8_t i = 1; i < 13; i++)
    {
			if (leftDays < mon_yday[isLeepYear][i])
			{
				sDate->Month = i;
				sDate->Date = leftDays - mon_yday[isLeepYear][i-1] + 1;
				break;
			}
    }
	uint32_t leftSeconds = Second % SECOND_DAY;
	sDate->Hour = (uint8_t)(leftSeconds / SECOND_HOUR);
	sDate->Minute = (uint8_t)((leftSeconds % SECOND_HOUR) / SECOND_MIN);
	sDate->Second = (uint8_t)(leftSeconds % SECOND_MIN);
}

STATIC void RTC_SetData(RTC_DateTypeDef *sDate)
{
	
	uint16_t curYear = sDate->Year;
	uint16_t CurData = 0;  //天数
	uint32_t overTime;
	uint32_t Second = 0;
	
	while (curYear > START_YEAR)  //过一年了
	{
		curYear--;
		CurData += getDaysForYear(curYear);
	}
	//calc month and Date
	bool isLeepYear = isLeapYear(sDate->Year);
	CurData += mon_yday[isLeepYear][sDate->Month - 1];

	CurData += sDate->Date-1;
	Second = CurData * SECOND_DAY;
	Second += (sDate->Hour * SECOND_HOUR)+ (sDate->Minute * SECOND_MIN);
	Second += sDate->Second;

	RCC->APB1ENR |= (1<<29); //enable PWREN
	RCC->APB1ENR |= (1<<9); //enable BKPEN
	PWR->CR |= (1<<8); //Enable DBP 关闭写保护

	RCC->BDCR |= (1<<16);  //BDRST
	RCC->BDCR &= ~(1<<16);  //BDRST
	mp_hal_delay_us(200);
	
	RCC->BDCR |= 0x8101U;  //LSE作为RTC时钟
	mp_hal_delay_us(200);

	overTime = 0;
	while(!(RCC->BDCR & 0x02)){
			overTime++;
			mp_hal_delay_us(1);
			if(overTime > LSE_DELAY){

				break;
			}
	}

	overTime = 0;
	while(!(RTC->CRL & 0x20)){  //RTOFF
			overTime++;
			mp_hal_delay_ms(1);
			if(overTime > LSE_DELAY){

				break;
			}
	}
	
	RTC->CRL |= (1<<4); //进入配置模式
	
	RTC->CRL &= ~(1<<3); 
	mp_hal_delay_ms(1);

	RTC->PRLH = 0;
	RTC->PRLL = 0x7FFF;
	
		RTC->CNTH = (uint16_t)(Second>>16);
	RTC->CNTL = (uint16_t)(Second & 0x0000FFFF);
	
	RTC->CRL &= ~(1<<4); //退出配置模式
	
	PWR->CR &= ~(1<<8); //写保护

	overTime = 0;
	while(!(RTC->CRL & 0x20)){  //RTOFF
			overTime++;
			mp_hal_delay_ms(1);
			if(overTime > LSE_DELAY){
				break;
			}
	}

}

uint64_t mp_hal_time_ns(void) {
    uint64_t ns = 0;
    return ns;
}

/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void rtc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_printf(print, "RTC()");
}


STATIC mp_obj_t rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_rtc_obj;
}
// force rtc to re-initialise
mp_obj_t pyb_rtc_init(mp_obj_t self_in) {

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_rtc_init_obj, pyb_rtc_init);

STATIC mp_obj_t rtc_datetime(size_t n_args, const mp_obj_t *args)
{
	if(n_args == 1) // get date and time
	{
		RTC_DateTypeDef tDate;
		RTC_GetData(&tDate);

		mp_obj_t tuple[8] = 
		{
			mp_obj_new_int(tDate.Year),
			mp_obj_new_int(tDate.Month),
			mp_obj_new_int(tDate.Date),
			mp_obj_new_int(weekday),
			mp_obj_new_int(tDate.Hour),
			mp_obj_new_int(tDate.Minute),
			mp_obj_new_int(tDate.Second),
			mp_obj_new_int(SubSeconds),
    };
		return mp_obj_new_tuple(8, tuple);
	}else            // set date and time
	{
			mp_obj_t *items;
			mp_obj_get_array_fixed_n(args[1], 8, &items);
			RTC_DateTypeDef tDate;
			tDate.Year   	= mp_obj_get_int(items[0]);
			tDate.Month  	= mp_obj_get_int(items[1]);
			tDate.Date    = mp_obj_get_int(items[2]);
			weekday				= mp_obj_get_int(items[3]);
			tDate.Hour		= mp_obj_get_int(items[4]);
			tDate.Minute  = mp_obj_get_int(items[5]);
			tDate.Second 	= mp_obj_get_int(items[6]);
			RTC_SetData(&tDate);
	}

	return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtc_datetime_obj, 1, 2, rtc_datetime);


STATIC const mp_rom_map_elem_t rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&rtc_datetime_obj) },
		{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_rtc_init_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rtc_locals_dict, rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .print = rtc_print,
    .make_new = rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&rtc_locals_dict,
};

