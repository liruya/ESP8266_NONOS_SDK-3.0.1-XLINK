#ifndef	__USER_SENSOR_H__
#define	__USER_SENSOR_H__

#include "app_common.h"

/* Reptile temperature sensor */
#define	REPTILE_TEMPERATURE_THRESHOLD_MIN	10
#define	REPTILE_TEMPERATURE_THRESHOLD_MAX	40
#define	REPTILE_TEMPERATURE_PERIOD_MAX		12
#define	REPTILE_TEMPERATURE_POINT_MAX		12

#define	REPTILE_HUMIDITY_THRESHOLD_MIN		0
#define	REPTILE_HUMIDITY_THRESHOLD_MAX		100

typedef enum _sensor_type{
	SENSOR_NONE,
	SENSOR_REPTILE_TEMPERATURE,
	SENSOR_REPTILE_HUMIDITY,
	SENSOR_INVALID
}sensor_type_t;

// typedef union {
// 	struct {
// 		uint8_t		s1type;
// 		bool 		s1NotifyEnable;
// 		bool 		s1LinkageEnable;
// 		uint8_t 	s1args[256];
// 		int32_t		s1ThrdLower;
// 		int32_t		s1ThrdUpper;

// 		uint8_t 	s2type;
// 		bool 		s2NotifyEnable;
// 		uint8_t 	s2args[64];
// 		int32_t		s2ThrdLower;
// 		int32_t		s2ThrdUpper;
// 	};
// 	uint8_t array[360];
// } sensor_args_t;

typedef union {
	struct {
		uint8_t		s1type;
		bool 		s1NotifyEnable;
		int32_t		s1DayThreshold;
		int32_t		s1NightThreshold;
		int32_t		s1ThrdLower;
		int32_t		s1ThrdUpper;

		uint8_t 	s2type;
		bool 		s2NotifyEnable;
		int32_t		s2DayThreshold;
		int32_t		s2NightThreshold;
		int32_t		s2ThrdLower;
		int32_t		s2ThrdUpper;
	};
	uint8_t array[360];
} sensor_args_t;

extern bool user_sensor_args_check(sensor_args_t *parg, uint8_t s1type, uint8_t s2type); 
// extern char* user_sensor_get_type(uint8_t type);
// extern char* user_sensor_get_unit(uint8_t type);

#endif