#include "user_rtc.h"
#include "app_util.h"
#include "xlink_tcp_client.h"

xlink_datetime_t datetime;
LOCAL os_timer_t tmr_1sec;

LOCAL bool isSynchronized;

const uint8_t MONTH[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

LOCAL void user_rtc_1s_cb(void *arg);

void ESPFUNC user_rtc_init() {
	os_memset(&datetime, 0, sizeof(datetime));
	datetime.year = 2000;
	datetime.month = 1;
	datetime.day = 1;

	os_timer_disarm(&tmr_1sec);
	os_timer_setfn(&tmr_1sec, user_rtc_1s_cb, NULL);
	os_timer_arm(&tmr_1sec, 1000, 1);
}

void ESPFUNC user_rtc_set_synchronized() {
	isSynchronized = true;
}

bool ESPFUNC user_rtc_is_synchronized() {
	return isSynchronized;
}

void ESPFUNC user_rtc_sync_cloud_cb(xlink_datetime_t *pdatetime) {
	if (pdatetime == NULL) {
		return;
	}
	os_timer_disarm(&tmr_1sec);
	datetime.year = pdatetime->year;
	datetime.month = pdatetime->month;
	datetime.day = pdatetime->day;
	datetime.week = pdatetime->week;
	datetime.hour = pdatetime->hour;
	datetime.min = pdatetime->min;
	datetime.second = pdatetime->second;
	datetime.zone = pdatetime->zone;
	os_timer_arm(&tmr_1sec, 1000, 1);
	isSynchronized = true;
}

LOCAL void ESPFUNC user_rtc_1s_cb(void *arg) {
	uint8_t month_days;
	datetime.second++;
	if (datetime.second > 59) {
		datetime.second = 0;
		datetime.min++;
		if (datetime.min > 59) {
			datetime.min = 0;
			datetime.hour++;
			if (datetime.hour > 23) {
				datetime.hour = 0;
				datetime.week++;
				if(datetime.week > 6) {
					datetime.week = 0;
				}
				datetime.day++;
				month_days = MONTH[datetime.month-1];
				if (datetime.month == 2 && app_util_is_leap_year(datetime.year)) {
					month_days += 1;
				}
				if (datetime.day > month_days) {
					datetime.day = 1;
					datetime.month++;
					if (datetime.month > 12) {
						datetime.month = 1;
						datetime.year++;
					}
				}
			}
		}
	}
}

uint16_t ESPFUNC user_rtc_get_year() {
	return datetime.year;
}

uint8_t ESPFUNC user_rtc_get_month() {
	return datetime.month;
}

uint8_t ESPFUNC user_rtc_get_day() {
	return datetime.day;
}

uint8_t ESPFUNC user_rtc_get_week() {
	return datetime.week;
}

uint8_t ESPFUNC user_rtc_get_hour() {
	return datetime.hour;
}

uint8_t ESPFUNC user_rtc_get_minute() {
	return datetime.min;
}

uint8_t ESPFUNC user_rtc_get_second() {
	return datetime.second;
}

int16_t ESPFUNC user_rtc_get_zone() {
	return datetime.zone;
}
