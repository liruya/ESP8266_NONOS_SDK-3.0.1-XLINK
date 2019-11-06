#ifndef	__USER_DEVICE_H__
#define	__USER_DEVICE_H__

#include "app_common.h"
#include "user_indicator.h"
#include "user_smartconfig.h"
#include "user_apconfig.h"

#define	PROPERTY_INDEX			0			//属性索引
#define	ZONE_INDEX				1			//时区索引
#define	LONGITUDE_INDEX			2			//设备位置经度索引
#define	LATITUDE_INDEX			3			//设备位置纬度索引
#define	DATETIME_INDEX			4			//设备日期时间索引,字符串格式
#define	CLOUDZONE_INDEX			195			//云平台时区索引
#define	RSSI_INDEX				196			//信号强度索引
#define	UPGRADE_STATE_INDEX		197			//设备升级状态索引
#define	LOCAL_PSW_INDEX			198			//设备本地密码索引
#define	SNSUB_ENABLE_INDEX		199			//允许SN订阅索引

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
	const char * property;
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
	void (*const default_config)();
	void (*const save_config)();
	void (*const para_init)();
	void (*const key_init)();
	void (*const datapoint_init)();
	void (*const init)();
	void (*const process)(void *);
	void (*const dp_changed_cb)();

	device_para_t * const	para;
	device_config_t	* const	pconfig;
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

	bool		sn_subscribe_enable;	//允许通过SN订阅设备
};

struct _device_config {
	uint16_t saved_flag;

	int16_t zone;

	float longitude;
	float latitude;

	uint32_t local_psw;
};

extern bool connected_local;

extern bool user_device_psw_isvalid(user_device_t*);
extern void user_device_init(user_device_t *);

extern void user_device_restore(user_device_t *);
extern bool user_device_poweron_check(user_device_t *);
extern void user_device_set_cloud_zone(int16_t);

extern void user_device_update_dpall();
extern void user_device_update_dpchanged();

extern void user_device_enable_snsubscribe(user_device_t *);

#endif