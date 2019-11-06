/**
 * Modified by moore on 2018/12/13.
 */

#ifndef _XLINK_SDK_H_
#define _XLINK_SDK_H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef USE_ESP8266_SWITCH
    #define USE_ESP8266_SWITCH  0
#endif // USE_ESP8266_SWITCH

#ifndef USE_ESP32_SWITCH
    #define USE_ESP32_SWITCH	0
#endif // USE_ESP32_SWITCH

#ifndef USE_LPX30_SWITCH
    #define USE_LPX30_SWITCH	0
#endif // USE_LPX30_SWITCH

#ifndef USE_LPB120_SWITCH
    #define USE_LPB120_SWITCH	0
#endif // USE_LPB120_SWITCH

#ifndef USE_G510_SWITCH
    #define USE_G510_SWITCH    0
#endif // USE_G510_SWITCH

#if defined(WIN32)
#define XLINK_DLL_EXPORTS
#endif

#include<stdio.h>
#include<string.h>
#include <stdlib.h>

//define xlink datatype,modify by different system
#define xlink_int8   char
#define xlink_uint8  unsigned char
#define xlink_int16  short
#define xlink_uint16 unsigned short
#define xlink_int32  int
#define xlink_uint32 unsigned int
#define xlink_int64  long long
#define xlink_uint64 unsigned long long
#define xlink_double double
#define xlink_null   NULL
#define xlink_enum   enum
#define xlink_union  union
#define xlink_sizeof sizeof

#if USE_ESP8266_SWITCH

#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "c_types.h"
#include "ip_addr.h"
#include "upgrade.h"
#include "espconn.h"

#define xlink_memcpy    os_memcpy
#define xlink_memset    os_memset
#define xlink_memcmp    os_memcmp
#define xlink_strlen    os_strlen
#define xlink_sprintf   os_sprintf
#define xlink_printf    os_printf
#define xlink_snprintf  ets_snprintf
#define xlink_strstr    os_strstr
#define xlink_strcpy    os_strcpy
#define xlink_strncpy   os_strncpy
#define xlink_strcmp    os_strcmp
#define xlink_strchr    os_strchr

#elif USE_ESP32_SWITCH

#elif USE_LPX30_SWITCH

#include <hsf.h>

#define xlink_memcpy	memcpy
#define xlink_memset	memset
#define xlink_memcmp	memcmp
#define xlink_sizeof	sizeof
#define xlink_strlen	strlen
#define xlink_sprintf	sprintf
#define xlink_printf	u_printf
#define xlink_snprintf  snprintf
#define xlink_strcpy	strcpy
#define xlink_strncpy   strncpy
#define xlink_strcmp    strcmp
#define xlink_strchr    strchr
#define xlink_strstr    strstr

#elif USE_LPB120_SWITCH || defined(WIN32)

#define xlink_memcpy	memcpy
#define xlink_memset	memset
#define xlink_memcmp	memcmp
#define xlink_sizeof	sizeof
#define xlink_strlen	strlen
#define xlink_sprintf	sprintf
#define xlink_printf	printf
#define xlink_snprintf  snprintf
#define xlink_strcpy	strcpy
#define xlink_strncpy   strncpy
#define xlink_strcmp    strcmp
#define xlink_strchr    strchr
#define xlink_strstr    strstr

#endif // define

#define xlink_delay_ms

#if defined(WIN32)
    #ifdef XLINK_DLL_EXPORTS
        #define XLINK_FUNCTION __declspec(dllexport)
    #else
        #define XLINK_FUNCTION __declspec(dllimport)
    #endif
#elif USE_ESP8266_SWITCH
    #define XLINK_FUNCTION ICACHE_FLASH_ATTR
#elif USE_ESP32_SWITCH
    #define XLINK_FUNCTION
#elif USE_LPX30_SWITCH
    #define XLINK_FUNCTION
#elif USE_LPB120_SWITCH
    #define XLINK_FUNCTION
#else
    #define XLINK_FUNCTION
#endif

//define xlink_sdk
#define XLINK_DEV_NAME_MAX 16
#define XLINK_DEV_MAC_LENGTH_MIN 1
#define XLINK_DEV_MAC_LENGTH_MAX 32
#define XLINK_SYS_PARA_LENGTH (1024*3)
#define XLINK_PACKET_LENGTH_MAX 1000
//NEW
#define XLINK_LOCAL_PAIRING_SEED_MAX	16
#define XLINK_LOCAL_PAIRING_MAX_TIMES   65536

#define PRODUCTION_TEST_ENABLE  1
#define SDK_PROTOCOL_VERSION	6
#define SDK_MASTER_VERSION      6220

#define XLINK_DISCOVER_ENABLE	1
#define MQTT_VERSION 3
//NEW
//xlink addr
typedef struct xlink_addr_t {
    xlink_int32 socket;
    xlink_uint32 ip;
    xlink_uint16 port;
} xlink_addr_t;

typedef union {
	unsigned int ip;
	struct {
		unsigned char byte0 :8;
		unsigned char byte1 :8;
		unsigned char byte2 :8;
		unsigned char byte3 :8;
	}bit;
}ip_address_t;


//xlink sdk configuration struct
typedef struct xlink_sdk_instance {
    //device product id
    xlink_uint8 *dev_pid;
    //device product key
    xlink_uint8 *dev_pkey;
    //device name
    xlink_uint8 dev_name[XLINK_DEV_NAME_MAX + 1];
    //device mac
    xlink_uint8 dev_mac[XLINK_DEV_MAC_LENGTH_MAX];
    //device mac length
    xlink_uint8 dev_mac_length;
    //device firmware version
    xlink_uint16 dev_firmware_version;
    //mcu firmware version
    xlink_uint16 mcu_firmware_version;
    //enable cloud
    xlink_uint8 cloud_enable;
    //enable local
    xlink_uint8 local_enable;
    //user certification
    xlink_uint8 *certificate_id;
    //user certification length
    xlink_int16 certificate_id_length;
    //user certification key
    xlink_uint8 *certificate_key;
    //user certification key
    xlink_int16 certificate_key_length;
    //log level
    xlink_uint8 log_level;
    //log enable
    xlink_uint8 log_enable;
    //cloud rec buffer
    xlink_uint8 *cloud_rec_buffer;
    //cloud rec buffer length;
    xlink_int32 cloud_rec_buffer_length;
    //system para,user don't care
    xlink_uint8 sdk_para[XLINK_SYS_PARA_LENGTH];
	//device sn length
    xlink_uint16 dev_sn_length;
    //device sn
    xlink_uint8 *dev_sn;
    //device pingcode length
    xlink_uint16 pingcode_length;
    //device pingcode
    xlink_uint8 *pingcode;
    //device identify
    xlink_uint32 identify;
} xlink_sdk_instance_t;

//xlink sdk event enum
typedef enum {
    EVENT_TYPE_STATUS = 0,
    EVENT_TYPE_REQ_DATETIME,
    EVENT_TYPE_REQ_DATETIME_CB,
    EVENT_TYPE_UPGRADE_CB,
    EVENT_TYPE_UPGRADE_COMPLETE,
    EVENT_TYPE_REQUEST_CB,
    EVENT_TYPE_NOTIFY,
	EVENT_TYPE_PRODUCTION_TEST,
    EVENT_TYPE_SERVER_ASK_DEVICE_OTA,
    EVENT_TYPE_CHECK_OTA_TASK,
    EVENT_TYPE_CHECK_OTA_TASK_CB,
    EVENT_TYPE_REPORT_OTA_UPGRADE_RESULT
} xlink_enum_event_type_t;

typedef enum {
	EVENT_DISCONNECTED_SERVER = 0,
	EVENT_CONNECTED_SERVER,
	EVENT_SERVER_REJECT_DEVICE_REQUEST,
    EVENT_PAIRING_ENABLE,
    EVENT_PAIRING_DISABLE,
    EVENT_SUBSCRIBE_ENABLE,
    EVENT_SUBSCRIBE_DISABLE
} xlink_enum_sdk_status_t;

//xlink device connect station struct
typedef struct xlink_status {
	xlink_enum_sdk_status_t status;
} xlink_status_t;

//xlink sdk datetime struct
typedef struct xlink_datetime {
    xlink_uint16 year;
    xlink_uint8 month;
    xlink_uint8 day;
    xlink_uint8 week;
    xlink_uint8 hour;
    xlink_uint8 min;
    xlink_uint8 second;
    xlink_int16 zone;
} xlink_datetime_t;

//xlink sdk upgrade struct
typedef struct xlink_upgrade {
    xlink_int32 device_id;
    xlink_uint8 flag;
    xlink_uint16 firmware_version;
    xlink_uint32 file_size;
    xlink_uint8 *url;
    xlink_int16 url_length;
    xlink_uint8 *hash;
    xlink_int16 hash_length;
} xlink_upgrade_t;

//xlink sdk upgrade complete struct
typedef struct xlink_upgrade_complete {
    xlink_uint8 flag;
    xlink_uint8 status;
    xlink_uint16 last_version;
    xlink_uint16 current_version;
} xlink_upgrade_complete_t;

//xlink sdk request call back struct
typedef struct xlink_request_cb {
    xlink_uint16 messageid;
    xlink_uint32 value;
} xlink_request_cb_t;

//xlink notify struct
typedef struct xlink_sdk_notify {
    xlink_uint8 from_type;//1:server,2:devcie,3:app
    xlink_int32 from_id;
    xlink_uint8 *message;
    xlink_int16 message_length;
} xlink_sdk_notify_t;

#if PRODUCTION_TEST_ENABLE
typedef enum{
    EVENT_TYPE_ENTER_PDCT_TEST_SUCCESS = 0,
    EVENT_TYPE_ENTER_PDCT_TEST_FAIL,
    EVENT_TYPE_PDCT_TEST_END_SUCCESS,
    EVENT_TYPE_PDCT_TEST_END_FAIL,
} xlink_pdct_event_type;

typedef enum {
    XLINK_PRODUCTION_START_SUCCESS = 0,
    XLINK_PRODUCTION_START_FAIL_DEVIDE_OFFLINE,
    XLINK_PRODUCTION_START_FAIL_PRODUCTION_TEST_DISABLE,
    XLINK_PRODUCTION_END_SUCCESS,
    XLINK_PRODUCTION_END_FAIL
}xlink_production_event_msgtype;

typedef struct xlink_pdct_cb {
    //struct xlink_addr_t **addr_t;
//    xlink_int32 index;
//    xlink_uint16 packetid;
//    xlink_uint16 messageid;
    xlink_pdct_event_type pdct_event_t;
    xlink_production_event_msgtype result_message;
}xlink_pdct_cb_t;
#endif

/*add by kaven*/
//xlink sdk location struct
typedef struct xlink_location {
    xlink_double longitude;
    xlink_double latitude;
    xlink_uint64 timestamp;
    xlink_uint8 timestamp_flag;//if timestamp is not use,timestamp_flag must be 0
    xlink_uint16 address_length;//if address is not use,address_length must be 0
    xlink_uint8 *address;
} xlink_location_t;

//param->xlink_update_current_verison()
typedef struct xlink_upgrade_version_package{
	xlink_uint8     firmware_type;
	xlink_uint8     mod;
	xlink_uint32    identify;
	xlink_uint16    version;
}__attribute__((packed))xlink_upgrade_version_package_t;

//ota firmware type
typedef enum {
    FIRMWARE_TYEP_WIFI =1,
    FIRMWARE_TYEP_MCU,
    FIRMWARE_TYEP_SUBDEV
}xlink_ota_firmware_type_t;

//ota channel
typedef enum {
    FROM_TYEP_LOCAL = 0,
    FROM_TYEP_CLOUD = 1
}xlink_ota_from_type_t;

//ota server/app ask devcice to check ota task
typedef struct xlink_ask_device_ota_package{
    xlink_uint8     from;
    xlink_uint8     firmware_type;
}__attribute__((packed))xlink_ask_device_ota_package_t;

//ota device check server/app ota task info
typedef struct xlink_ota_task_check_package{
    xlink_uint8     from;
	xlink_uint8     firmware_type;
	xlink_uint16    version;
	xlink_uint32    identify;
}__attribute__((packed))xlink_ota_task_check_package_t;

//ota server/app return check ota task info -> code
typedef enum {
    OTA_CHECK_SUCCESS = 1,
    OTA_UNKNOWN_FIRMWARE_TYPE,
    OTA_UNKNOWN_DEVICE,
    OTA_NO_TASK
} xlink_enum_ota_check_ack_code_t;

//ota server/app return check ota task info
typedef struct xlink_ota_task_check_ack_package{
    xlink_uint8     from;
    xlink_uint8     code;
    xlink_uint8     firmware_type;
    xlink_uint16    current_version;
    xlink_uint32    identify;
    xlink_uint16    task_id_length;
    xlink_uint8*    task_id;
    xlink_uint16    target_version;
    xlink_uint16    target_md5_length;
    xlink_uint8*    target_md5;
    xlink_uint16    target_url_length;
    xlink_uint8*    target_url;
    xlink_uint32    target_size;
}__attribute__((packed))xlink_ota_task_check_ack_package_t;

//ota device upgrade ota result info
typedef struct xlink_ota_report_upgrade_result_package{
    xlink_uint8     from;
	xlink_uint8     result;
	xlink_uint8     firmware_type;
	xlink_uint8     mod;
	xlink_uint16    current_version;
	xlink_uint16    original_version;
	xlink_uint32    identify;
	xlink_uint16    task_id_length;
	xlink_uint8     *task_id_str;
}__attribute__((packed))xlink_ota_report_upgrade_result_package_t;

//xlink sdk event struct -> xlink sdk event enum
//xlink_status_t -> EVENT_TYPE_STATUS = 0,
//xlink_datetime_t -> EVENT_TYPE_REQ_DATETIME_CB,
//xlink_upgrade_t -> EVENT_TYPE_UPGRADE_CB,
//xlink_upgrade_complete_t -> EVENT_TYPE_UPGRADE_COMPLETE,
//xlink_request_cb_t -> EVENT_TYPE_REQUEST_CB,
//xlink_sdk_notify_t -> EVENT_TYPE_NOTIFY,
//xlink_pdct_cb_t -> EVENT_TYPE_PRODUCTION_TEST,
//xlink_ask_device_ota_package_t -> EVENT_TYPE_SERVER_ASK_DEVICE_OTA,
//xlink_ota_task_check_package_t -> EVENT_TYPE_CHECK_OTA_TASK,
//xlink_ota_task_check_ack_package_t -> EVENT_TYPE_CHECK_OTA_TASK_CB,
//xlink_ota_report_upgrade_result_package_t -> EVENT_TYPE_REPORT_OTA_UPGRADE_RESULT
typedef struct xlink_sdk_event {
    xlink_enum_event_type_t enum_event_type_t;
    xlink_union {
        xlink_status_t status;
        xlink_datetime_t datetime_t;
        xlink_upgrade_t upgrade_t;
        xlink_upgrade_complete_t upgrade_complete_t;
        xlink_request_cb_t request_cb_t;
        xlink_sdk_notify_t notify_t;
		xlink_ask_device_ota_package_t ask_device_ota_package_t;
		xlink_ota_task_check_package_t ota_task_check_package_t;
        xlink_ota_task_check_ack_package_t ota_task_check_ack_package_t;
		xlink_ota_report_upgrade_result_package_t ota_report_upgrade_result_package_t;
#if PRODUCTION_TEST_ENABLE
        xlink_pdct_cb_t pdct_cb_t;
#endif
    } event_struct_t;
} xlink_sdk_event_t;

//interface function
//sdk function
XLINK_FUNCTION extern xlink_int32 xlink_sdk_init(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_uninit(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_connect_cloud(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_disconnect_cloud(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern void xlink_sdk_process(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_reset(struct xlink_sdk_instance **sdk_instance);

#if defined(WIN32)

typedef struct function_callback {
	xlink_int32 (*xlink_write_flash_cb)(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
	xlink_int32 (*xlink_read_flash_cb)(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
	xlink_int32 (*xlink_send_cb)(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
	xlink_int32 (*xlink_get_datapoint_cb)(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
	xlink_int32 (*xlink_set_datapoint_cb)(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
	void (*xlink_event_cb)(struct xlink_sdk_instance **sdk_instance, const struct xlink_sdk_event **event_t);
	xlink_int32 (*xlink_get_ticktime_ms_cb)(struct xlink_sdk_instance **sdk_instance);
	xlink_int32 (*xlink_probe_datapoint_cb)(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 ** dp_idx, xlink_uint8 dp_idx_length, xlink_uint8 **buffer, xlink_int32 datamaxlength);
	xlink_int32 (*xlink_get_rssi_cb)(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *result, xlink_int16 *rssi, xlink_uint16 *AP_STA);
	xlink_int32 (*xlink_get_custom_test_data_cb)(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *result, xlink_uint8 **data, xlink_int32 datamaxlength);
}function_callback;

XLINK_FUNCTION extern xlink_int32 xlink_register_callback(struct xlink_sdk_instance **sdk_instance, struct function_callback callback);

#define xlink_write_flash_cb(...)  ((g_function_callback.xlink_write_flash_cb == xlink_null) ? (-1) : (g_function_callback.xlink_write_flash_cb(__VA_ARGS__)))
#define xlink_read_flash_cb(...)  ((g_function_callback.xlink_read_flash_cb == xlink_null) ? (-1) : (g_function_callback.xlink_read_flash_cb(__VA_ARGS__)))
#define xlink_send_cb(...)  ((g_function_callback.xlink_send_cb == xlink_null) ? (-1) : (g_function_callback.xlink_send_cb(__VA_ARGS__) ))
#define xlink_get_datapoint_cb(...)  ((g_function_callback.xlink_get_datapoint_cb == xlink_null) ? (-1) : (g_function_callback.xlink_get_datapoint_cb(__VA_ARGS__)))
#define xlink_set_datapoint_cb(...)  ((g_function_callback.xlink_set_datapoint_cb == xlink_null) ? (-1): (g_function_callback.xlink_set_datapoint_cb(__VA_ARGS__)))
#define xlink_event_cb(...)  ((g_function_callback.xlink_event_cb == xlink_null) ? (-1) : (g_function_callback.xlink_event_cb(__VA_ARGS__)))
#define xlink_get_ticktime_ms_cb(...)  ((g_function_callback.xlink_get_ticktime_ms_cb == xlink_null) ? (-1) : (g_function_callback.xlink_get_ticktime_ms_cb(__VA_ARGS__)))
#else
//sdk call back function
XLINK_FUNCTION extern xlink_int32 xlink_write_flash_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNCTION extern xlink_int32 xlink_read_flash_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNCTION extern xlink_int32 xlink_send_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_get_datapoint_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNCTION extern xlink_int32 xlink_set_datapoint_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNCTION extern void xlink_event_cb(struct xlink_sdk_instance **sdk_instance, const struct xlink_sdk_event **event_t);
XLINK_FUNCTION extern xlink_uint32 xlink_get_ticktime_ms_cb(struct xlink_sdk_instance **sdk_instance);
#endif

//sdk send data function
XLINK_FUNCTION extern xlink_int32 xlink_update_datapoint(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *messageid, const xlink_uint8 **data, xlink_int32 datamaxlength, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_request_event(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *messageid, struct xlink_sdk_event **event_t);
XLINK_FUNCTION extern xlink_int32 xlink_receive_data(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_get_device_id(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_get_device_key(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer);
XLINK_FUNCTION extern xlink_int32 xlink_report_log(struct xlink_sdk_instance** sdk_instance, xlink_uint8 log_level, xlink_uint8** data, xlink_uint32 datalength);

//upload location
XLINK_FUNCTION extern xlink_int32 xlink_upload_location_data(struct xlink_sdk_instance **sdk_instance, struct xlink_location**xlink_location, xlink_uint16 *message_id);

//production test
#if PRODUCTION_TEST_ENABLE
XLINK_FUNCTION extern xlink_int32 xlink_production_test_end(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNCTION extern xlink_int32 xlink_production_test_start(struct xlink_sdk_instance **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_enable_production_test(struct xlink_sdk_instance **sdk_instance, xlink_uint8 flag);
#endif

XLINK_FUNCTION extern xlink_int32 xlink_enable_local_pairing(struct xlink_sdk_instance **sdk_instance, xlink_uint16 time);
XLINK_FUNCTION extern xlink_int32 xlink_disable_local_pairing(struct xlink_sdk_instance **sdk_instance);

/*add by mcho*/
XLINK_FUNCTION extern xlink_int32 xlink_enable_local_subscription(struct xlink_sdk_instance **sdk_instance, xlink_uint16 time);
XLINK_FUNCTION extern xlink_int32 xlink_disable_local_subscription(struct xlink_sdk_instance **sdk_instance);

#if defined(WIN32)
#define xlink_probe_datapoint_cb(...) ((g_function_callback.xlink_probe_datapoint_cb == xlink_null )? (-1) : (g_function_callback.xlink_probe_datapoint_cb(__VA_ARGS__)))
#define xlink_get_rssi_cb(...) ((g_function_callback.xlink_get_rssi_cb == xlink_null) ? (-1) : (g_function_callback.xlink_get_rssi_cb(__VA_ARGS__)))
#define xlink_get_custom_test_data_cb(...) ((g_function_callback.xlink_get_custom_test_data_cb == xlink_null) ? (-1) : (g_function_callback.xlink_get_custom_test_data_cb(__VA_ARGS__)))
#else
XLINK_FUNCTION extern xlink_int32 xlink_probe_datapoint_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 ** dp_idx, xlink_uint8 dp_idx_length, xlink_uint8 **buffer, xlink_int32 datamaxlength) ;
XLINK_FUNCTION extern xlink_int32 xlink_get_rssi_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *result, xlink_int16 *rssi, xlink_uint16 *AP_STA);
XLINK_FUNCTION extern xlink_int32 xlink_get_custom_test_data_cb(struct xlink_sdk_instance **sdk_instance,  xlink_uint16 *result,xlink_uint8 **data, xlink_int32 datamaxlength);
#endif

XLINK_FUNCTION extern xlink_int32 xlink_get_sdk_version(struct xlink_sdk_instance** sdk_instance, xlink_uint8* buffer, xlink_uint32 buffer_len);
/*add by kaven*/

XLINK_FUNCTION extern xlink_int32 xlink_update_local_datapoint(struct xlink_sdk_instance** sdk_instance, xlink_uint16* messageid, xlink_uint8** data, xlink_int32 datalength, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_update_cloud_datapoint(struct xlink_sdk_instance** sdk_instance, xlink_uint16* messageid, xlink_uint8** data, xlink_int32 datalength, xlink_uint8 flag);

/**
 * @function xlink_update_current_verison
 * update device current version.
 * @return 0 : success, 1 : fail
 */
XLINK_FUNCTION xlink_int32 xlink_update_current_verison(struct xlink_sdk_instance** sdk_instance, xlink_uint8 id_count, void * data);

#ifdef  __cplusplus
}
#endif

#endif //_XLINK_SDK_H_
