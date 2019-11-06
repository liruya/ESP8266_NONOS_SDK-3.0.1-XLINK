#ifndef	__APP_UTIL_H__
#define	__APP_UTIL_H__

#include "app_common.h"

extern bool	app_util_zone_isvalid(int16_t zone);
extern bool	app_util_longitude_isvalid(float longitude);
extern bool	app_util_latitude_isvalid(float latitude);
extern bool app_util_is_leap_year(uint16_t year);

#endif