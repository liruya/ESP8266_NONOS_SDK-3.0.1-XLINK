#ifndef	__USER_SOCKET_H__
#define	__USER_SOCKET_H__

#include "app_common.h"
#include "xlink.h"
#include "user_sensor.h"

// #define SOCKET_FIRMWARE_VERSION		1

#define SOCKET_TIMER_MAX			24
#define	SOKCET_TIMER_INVALID		0xFFFFFFFF

#define POWER_ON					1
#define POWER_OFF					0

#define	SWITCH_COUNT_MAX			50000

typedef union{
	struct {
		unsigned timer : 16;
		unsigned action : 8;
		unsigned repeat : 7;
		unsigned enable : 1;
	};
	uint32_t value;
} socket_timer_t;

typedef struct{
	device_config_t super;

	socket_timer_t socket_timer[SOCKET_TIMER_MAX];
	uint32_t switch_flag;
	uint32_t switch_count;
	sensor_args_t sensor_args;
} socket_config_t;

typedef struct {
	device_para_t super;

	const uint32_t switch_count_max;

	bool sensor1_available;
	uint8_t sensor1_type;
	int32_t sensor1_value;
	uint8_t s1_loss_flag;
	uint8_t s1_over_flag;
	
	bool sensor2_available;
	uint8_t sensor2_type;
	int32_t sensor2_value;
	uint8_t s2_loss_flag;
	uint8_t s2_over_flag;
	
	sensor_args_t * const p_sensor_args;

	bool power;
	socket_timer_t * const p_timer;
} socket_para_t;

extern user_device_t user_dev_socket;

#endif