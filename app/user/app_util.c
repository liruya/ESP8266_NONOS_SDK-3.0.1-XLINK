#include "app_util.h"

bool ESPFUNC app_util_zone_isvalid(int16_t zone) {
	if(zone > 1200 || zone < -1200 || zone%100 >= 60 || zone%100 <= -60) {
		return false;
	}
	return true;
}

bool ESPFUNC app_util_longitude_isvalid(float longitude) {
	if(longitude > 180 || longitude < -180) {
		return false;
	}
	return true;
}

bool ESPFUNC app_util_latitude_isvalid(float latitude) {
	if(latitude > 60 || latitude < -60) {
		return false;
	}
	return true;
}

bool ESPFUNC app_util_is_leap_year(uint16_t year) {
	if (year%4 != 0) {
		return false;
	}
	if (year%100 == 0 && year%400 != 0) {
		return false;
	}
	return true;
}