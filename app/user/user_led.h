#ifndef	__USER_LED_H__
#define	__USER_LED_H__

#include "app_common.h"
#include "xlink.h"

// #define LED_FIRMWARE_VERSION	1

#define LED_CHANNEL_COUNT		5					// 1 ~ 6
#define CHN1_NAME				"ColdWhite"
#define CHN2_NAME				"Red"
#define CHN3_NAME				"Blue"
#define CHN4_NAME				"Purple"
#define CHN5_NAME				"WarmWhite"
// #define CHN1_NAME				"WarmWhite"
// #define CHN2_NAME				"Purple"
// #define CHN3_NAME				"Blue"
// #define CHN4_NAME				"Red"
// #define CHN5_NAME				"ColdWhite"
#define NIGHT_CHANNEL			0

#if	NIGHT_CHANNEL >= LED_CHANNEL_COUNT
#error "NIGHT_CHANNEL must be smaller than CHANNEL_COUNT."
#endif

#define	CUSTOM_COUNT			4

#define POINT_COUNT_MIN			4
#define POINT_COUNT_MAX			12

#define	MODE_MANUAL				0
#define MODE_AUTO				1
#define	MODE_PRO				2

#define	PROFILE_COUNT_MAX		12

typedef struct {
		uint8_t count;
		uint16_t timers[POINT_COUNT_MAX];
		uint8_t brights[POINT_COUNT_MAX*LED_CHANNEL_COUNT];
} profile_t;

typedef struct {
	device_config_t super;

	unsigned last_mode : 2;									//上次的模式 Auto/Pro
	unsigned state : 2;										//Off->All->Blue->WiFi
	unsigned all_bright : 10;								//All Bright   全亮亮度
	unsigned blue_bright : 10;								//Blue Bright  夜光亮度
	unsigned reserved : 8;

	uint8_t mode;											//模式 Manual / Auto / Pro

	/* Manual Mode */
	bool power;												//手动模式 开/关
	uint16_t bright[LED_CHANNEL_COUNT];						//通道亮度
	uint8_t custom_bright[CUSTOM_COUNT][LED_CHANNEL_COUNT];	//自定义配光

	/* Auto Mode */
	bool gis_enable;
	uint16_t sunrise;										//日出时间
	uint8_t sunrise_ramp;
	uint8_t day_bright[LED_CHANNEL_COUNT];					//白天亮度
	uint16_t sunset;										//日落时间
	uint8_t sunset_ramp;
	uint8_t night_bright[LED_CHANNEL_COUNT];				//夜晚亮度
	bool turnoff_enabled;									//晚上关灯使能
	uint16_t turnoff_time;									//晚上关灯时间

	// /* Pro Mode */
	profile_t profile0;
	profile_t profiles[PROFILE_COUNT_MAX];
	uint8_t select_profile;
} led_config_t;

typedef struct {
	device_para_t 	super;
	const uint8_t 	channel_count;
	const char*		channel_names[LED_CHANNEL_COUNT];

	uint16_t 		gis_sunrise;
	uint16_t 		gis_sunset;
	bool 			gis_valid;

	uint32_t 		current_bright[LED_CHANNEL_COUNT];
	uint32_t 		target_bright[LED_CHANNEL_COUNT];
	bool 			day_rise;
	bool 			night_rise;
} led_para_t;

extern user_device_t user_dev_led;

#endif