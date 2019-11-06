#ifndef	__USER_GEOGRAPHY_H__
#define	__USER_GEOGRAPHY_H__

#include <math.h>
#include "app_common.h"

extern bool get_sunrise_sunset(float glong, float glat, uint16_t year, uint8_t month, uint8_t day, int16_t zone, uint16_t *sunrise, uint16_t *sunset);

#endif