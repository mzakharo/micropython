
#ifndef MICROPY_INCLUDED_TKM32_RTC_H
#define MICROPY_INCLUDED_TKM32_RTC_H

#include "py/obj.h"


typedef struct
{
    uint16_t Year;           /*!< Year value */
    uint8_t Month;          /*!< Month value */
    uint8_t Date;            /*!< Day value */
    uint8_t DayOfWeek;      /*!< Day of week value */
    uint8_t Hour;           /*!< Hour value */
    uint8_t Minute;         /*!< Minute value */
    uint8_t Second;         /*!< Second value */
} RTC_DateTypeDef;


void rtc_init(void);
uint32_t rtc_get_seconds(void);
void RTC_GetData(RTC_DateTypeDef *sDate);

extern const mp_obj_type_t pyb_rtc_type;

#endif // MICROPY_INCLUDED_TKM32_RTC_H
