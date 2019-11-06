#ifndef	__USER_MONSOON_H__
#define	__USER_MONSOON_H__

#include "app_common.h"
#include "xlink.h"
#include "user_device.h"

// #define	MONSOON_FIRMWARE_VERSION	5

#define	TIMER_COUNT_MAX		24

#define	CUSTOM_COUNT		8

typedef union {
	struct {
		unsigned duration : 7;
		unsigned onoff : 1;
	};
	uint8_t value;
} monsoon_power_t;

typedef union {
	struct {
		unsigned timer : 16;
		unsigned duration : 7;
		unsigned : 1;
		unsigned repeat : 7;
		unsigned enable : 1;
	};
	uint32_t value;
} monsoon_timer_t;

typedef struct {
	device_config_t	super;
	
	uint8_t 		key_action;
	uint8_t 		custom_actions[CUSTOM_COUNT];
	monsoon_timer_t timers[TIMER_COUNT_MAX];
} monsoon_config_t;

typedef struct {
	device_para_t super;

	uint8_t status;
	monsoon_power_t power;
	monsoon_power_t power_shadow;
	uint16_t poweron_tmr;
	monsoon_timer_t *const ptimers;
} monsoon_para_t;

extern user_device_t user_dev_monsoon;

#endif