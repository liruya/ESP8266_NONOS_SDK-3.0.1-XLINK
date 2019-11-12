#ifndef	__USER_RTC_H__
#define	__USER_RTC_H__

#include "app_common.h"
#include "xlink.h"

extern xlink_datetime_t datetime;
extern const uint8_t MONTH[12];

extern void user_rtc_init();
extern void user_rtc_set_synchronized();
extern bool user_rtc_is_synchronized();
extern void user_rtc_sync_cloud_cb(xlink_datetime_t *pdatetime);

extern int16_t user_rtc_get_zone();
extern uint16_t user_rtc_get_year();
extern uint8_t user_rtc_get_month();
extern uint8_t user_rtc_get_day();
extern uint8_t user_rtc_get_week();
extern uint8_t user_rtc_get_hour();
extern uint8_t user_rtc_get_minute();
extern uint8_t user_rtc_get_second();

#endif