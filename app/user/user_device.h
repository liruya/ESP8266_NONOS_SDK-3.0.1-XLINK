#ifndef	__USER_DEVICE_H__
#define	__USER_DEVICE_H__

#include "app_common.h"
#include "user_indicator.h"
#include "user_smartconfig.h"
#include "user_apconfig.h"

// #define	USE_LOCATION

#define SPI_FLASH_SECTOR_SIZE       4096
#define	DEVICE_PROPERTY_SECTOR		0x100
#define	PROPERTY_SIZE				128

#define	PROPERTY_INDEX			0			//属性索引
#define	ZONE_INDEX				1			//时区索引
#define	DATETIME_INDEX			4			//设备日期时间索引,字符串格式
#define	SYNC_DATETIME_INDEX		5			//同步时钟索引 本地模式使用
#define	DAYTIME_START_INDEX		6			//白天开始时间
#define	DAYTIME_END_INDEX		7			//白天结束时间
#define	CLOUDZONE_INDEX			195			//云平台时区索引
#define	RSSI_INDEX				196			//信号强度索引
#define	UPGRADE_STATE_INDEX		197			//设备升级状态索引
#define	LOCAL_PSW_INDEX			198			//设备本地密码索引
#define	SNSUB_ENABLE_INDEX		199			//允许SN订阅索引

#ifdef	USE_LOCATION
#define	LONGITUDE_INDEX			2			//设备位置经度索引
#define	LATITUDE_INDEX			3			//设备位置纬度索引
#define	GIS_ENABLE_INDEX		191			//使能根据地理位置获取日出日落时间
#define	GIS_SUNRISE_INDEX		192			//根据地理位置计算的日出时间
#define	GIS_SUNSET_INDEX		193			//根据地理位置计算的日落时间
#define	GIS_VALID_INDEX			194			//地理位置功能是否有效
#endif

#define	KEY_LONG_PRESS_COUNT	200			//长按键周期

#define	CONFIG_SAVED_FLAG		0x55		//设备配置保存标志
#define	CONFIG_DEFAULT_FLAG		0xFF	

#define	SMARTCONFIG_TIEMOUT			60000
#define	SMARTCONFIG_FLASH_PERIOD	500
#define	APCONFIG_TIMEOUT			120000
#define	APCONFIG_FLASH_PERIOD		1500

typedef struct _user_device		user_device_t;
typedef struct _device_para		device_para_t;
typedef struct _device_config	device_config_t;

struct _user_device {
	const char * const product;
	const char * const product_id;
	const char * const product_key;
	const uint16_t fw_version;

	uint8_t	mac[6];
	uint8_t	sn[32];
	char	apssid[32];

	const uint8_t key_io_num;
	const uint8_t test_led1_num;
	const uint8_t test_led2_num;

	// key_list_t	key_list;

	void (*const board_init)();
	void (*const save_config)();
	void (*const para_init)();
	void (*const key_init)();
	void (*const datapoint_init)();
	void (*const init)();
	void (*const process)(void *);
	void (*const dp_changed_cb)();

	device_para_t * const	para;
	device_config_t	* const	pconfig;
	uint8_t property[PROPERTY_SIZE];
};

struct _device_para {
	uint16_t 	year;					//设备日期时间,从云平台日期时间转换
	uint8_t		month;
	uint8_t		day;
	int8_t		week;
	int8_t		hour;
	int8_t		minute;
	uint8_t		second;
	char		datetime[32];			//设备日期时间字符串格式

#ifdef	USE_LOCATION
	bool		gis_valid;
	uint16_t	gis_sunrise;
	uint16_t	gis_sunset;
#endif

	bool		sn_subscribe_enable;	//允许通过SN订阅设备
};

struct _device_config {
	uint16_t saved_flag;

	int16_t zone;

	uint16_t daytime_start;
	uint16_t daytime_end;

	uint32_t local_psw;

#ifdef	USE_LOCATION
	float longitude;
	float latitude;
	bool gis_enable;
#endif

};

extern bool connected_local;

extern bool user_device_psw_isvalid(user_device_t*);
extern void user_device_init(user_device_t *);

extern bool user_device_poweron_check(user_device_t *);
extern void user_device_set_cloud_zone(int16_t);

extern void user_device_update_dpall();
extern void user_device_update_dpchanged();

extern void user_device_enable_snsubscribe(user_device_t *);

#endif