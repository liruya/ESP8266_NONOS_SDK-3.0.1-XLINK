#include "user_sensor.h"

bool ESPFUNC user_sensor_args_check(sensor_args_t *parg, uint8_t s1type, uint8_t s2type) {
	if (parg == NULL) {
		return false;
	}
	if (parg->s1type == SENSOR_NONE || parg->s1type >= SENSOR_INVALID) {
		return false;
	}
	if (parg->s2type >= SENSOR_INVALID) {
		return false;
	}
	if (parg->s1NotifyEnable > 1) {
		return false;
	}
	if (parg->s2NotifyEnable > 1) {
		return false;
	}
	if (parg->s1type != s1type || parg->s2type != s2type) {
		return false;
	}
	if (parg->s1ThrdLower > parg->s1ThrdUpper || parg->s2ThrdLower > parg->s2ThrdUpper) {
		return false;
	}
	
	switch (parg->s1type) {
		case SENSOR_REPTILE_TEMPERATURE:
			if (parg->s1ThrdLower < REPTILE_TEMPERATURE_THRESHOLD_MIN || parg->s1ThrdUpper > REPTILE_TEMPERATURE_THRESHOLD_MAX) {
				return false;
			}
			if (parg->s1DayThreshold < REPTILE_TEMPERATURE_THRESHOLD_MIN || parg->s1DayThreshold > REPTILE_TEMPERATURE_THRESHOLD_MAX) {
				return false;
			}
			if (parg->s1NightThreshold < REPTILE_TEMPERATURE_THRESHOLD_MIN || parg->s1NightThreshold > REPTILE_TEMPERATURE_THRESHOLD_MAX) {
				return false;
			}
			break;
		default:
			return false;
	}
	switch (parg->s2type) {
		case SENSOR_REPTILE_HUMIDITY:
			if (parg->s2ThrdLower < REPTILE_HUMIDITY_THRESHOLD_MIN || parg->s2ThrdUpper > REPTILE_HUMIDITY_THRESHOLD_MAX) {
				return false;
			}
			if (parg->s2DayThreshold < REPTILE_HUMIDITY_THRESHOLD_MIN || parg->s2DayThreshold > REPTILE_HUMIDITY_THRESHOLD_MAX) {
				return false;
			}
			if (parg->s2NightThreshold < REPTILE_HUMIDITY_THRESHOLD_MIN || parg->s2NightThreshold > REPTILE_HUMIDITY_THRESHOLD_MAX) {
				return false;
			}
			break;
		default:
			break;
	}
	return true;
}

// char* ESPFUNC user_sensor_get_type(uint8_t type) {
// 	switch (type) {
// 		case SENSOR_NONE:
// 			return "None";
// 		case SENSOR_REPTILE_TEMPERATURE:
// 			return "Temperature";
// 		case SENSOR_REPTILE_HUMIDITY:
// 			return "Humidity";
// 		default:
// 			break;
// 	}
// 	return "Invalid";
// }

// char* ESPFUNC user_sensor_get_unit(uint8_t type) {
// 	switch (type) {
// 		case SENSOR_REPTILE_TEMPERATURE:
// 			return "℃";
// 		case SENSOR_REPTILE_HUMIDITY:
// 			return "%%RH";
// 		default:
// 			break;
// 	}
// 	return "";
// }

