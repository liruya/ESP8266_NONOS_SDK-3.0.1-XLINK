#include "xlink.h"
#include "user_rtc.h"

#define RECV_BUFFER_SIZE	2048

#define	PAIR_PERIOD			0				//0为无限时间
#define	SUBSCRIBE_PERIOD	150

#define	OTA_IDENTIFY_1		1
#define	OTA_IDENTIFY_2		2
#define	IDENTIFY			0

#define	SYNC_CLOUDTIME_RETRYCNT	3
#define	SYNC_CLOUDTIME_TIMEOUT	10000
#define	SYNC_CLOUDTIME_PERIOD	45000

bool connected_cloud;
bool server_reject;

LOCAL uint8_t recv_buffer[RECV_BUFFER_SIZE];
LOCAL struct xlink_sdk_instance g_xlink_sdk_instance;
LOCAL struct xlink_sdk_instance *p_xlink_sdk_instance;
LOCAL uint16_t msgid;
LOCAL os_timer_t sync_timer;
LOCAL os_timer_t retry_timer;
LOCAL uint8_t retry_count;				//同步云端时间重试次数

LOCAL uint8_t test_data[512];
LOCAL uint16_t test_data_len;

void xlink_enable_pair();

void XLINK_FUNCTION xlink_init(user_device_t *pdev) {
	if (pdev == NULL) {
		app_loge("xlink init faile -> null dev");
		return;
	}
	uint16_t version = xlink_read_version();
	app_logd("saved version: %d", version);
	if (version > pdev->fw_version) {
		xlink_write_version(pdev->fw_version);
	}
	os_memset(&g_xlink_sdk_instance, 0, sizeof(g_xlink_sdk_instance));
	g_xlink_sdk_instance.dev_pid = (uint8_t *) pdev->product_id;
	g_xlink_sdk_instance.dev_pkey = (uint8_t *) pdev->product_key;
	os_memcpy(g_xlink_sdk_instance.dev_name, "xlink_dev", sizeof("xlink_dev"));
	g_xlink_sdk_instance.dev_mac_length = 6;
	wifi_get_macaddr(STATION_IF, (uint8_t *) g_xlink_sdk_instance.dev_mac);
	g_xlink_sdk_instance.cloud_rec_buffer = recv_buffer;
	g_xlink_sdk_instance.cloud_rec_buffer_length = sizeof(recv_buffer);
	g_xlink_sdk_instance.dev_firmware_version = pdev->fw_version;
	g_xlink_sdk_instance.cloud_enable = 1;
	g_xlink_sdk_instance.local_enable = 1;
	g_xlink_sdk_instance.log_enable = 0;
	g_xlink_sdk_instance.log_level = 3;
	g_xlink_sdk_instance.identify = IDENTIFY;
	// g_xlink_sdk_instance.identify = (pdev->fw_version&0x01) == 0x01 ? OTA_IDENTIFY_1 : OTA_IDENTIFY_2;
	g_xlink_sdk_instance.dev_sn = pdev->sn;
	g_xlink_sdk_instance.dev_sn_length = os_strlen(pdev->sn);

	p_xlink_sdk_instance = &g_xlink_sdk_instance;
	xlink_sdk_init(&p_xlink_sdk_instance);

	xlink_enable_pair();
	xlink_setOnDatapointChangedCallback(pdev->dp_changed_cb);

	app_logd("Xlink SDK Version: %d" , SDK_MASTER_VERSION);
	app_logd("Firmware Version: %d", p_xlink_sdk_instance->dev_firmware_version);
}

void XLINK_FUNCTION xlink_deinit() {
	if (p_xlink_sdk_instance != NULL) {
		xlink_sdk_uninit(&p_xlink_sdk_instance);
		p_xlink_sdk_instance = NULL;
	}
}

bool XLINK_FUNCTION xlink_check_ip(const char *ip, uint8_t *ipaddr)
{
	uint8_t dot = 0;
	bool first_num_got = false;
	char first_num;
	int16_t val = -1;
	char temp;

	while(*ip != '\0') {
		temp = *ip++;
		if (temp >= '0' && temp <= '9') {
			if (first_num_got == false) {
				first_num_got = true;
				first_num = temp;
			} else if (first_num == '0') {
				return false;
			}
			if (val < 0) {
				val = 0;
			}
			val = val * 10 + (temp - '0');
			if (val > 255) {
				return false;
			}
			ipaddr[dot] = val;
		} else if (temp == '.') {
			/* .前面必须为 数字且在 0 ~ 255 之间  */
			if (val < 0 || val > 255) {
				return false;
			}
			dot++;
			if (dot > 3) {
				return false;
			}
			val = -1;
			first_num_got = false;
		} else {
			return false;
		}
	}
	return dot == 3 && val >= 0 && val <= 255;
}

int32_t XLINK_FUNCTION xlink_report_cloud_log(xlink_uint8 log_level, xlink_uint8 **data, xlink_uint32 len) {
	xlink_report_log(&p_xlink_sdk_instance, log_level, data, len);
}

uint32_t XLINK_FUNCTION xlink_get_deviceid() {
	return xlink_get_device_id(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_reset() {
	xlink_sdk_reset(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_connect_cloud() {
	xlink_sdk_connect_cloud(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_disconnect_cloud() {
	xlink_sdk_disconnect_cloud(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_enable_pair() {
	xlink_disable_local_pairing(&p_xlink_sdk_instance);
	xlink_enable_local_pairing(&p_xlink_sdk_instance, PAIR_PERIOD);
}

void XLINK_FUNCTION xlink_disable_pair() {
	xlink_disable_local_pairing(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_enable_subscription() {
	xlink_disable_local_subscription(&p_xlink_sdk_instance);
	xlink_enable_local_subscription(&p_xlink_sdk_instance, SUBSCRIBE_PERIOD);
}

void XLINK_FUNCTION xlink_disable_subscription() {
	xlink_disable_local_subscription(&p_xlink_sdk_instance);
}

int32_t XLINK_FUNCTION xlink_post_event(xlink_sdk_event_t **event) {
	msgid++;
	return xlink_request_event(&p_xlink_sdk_instance, &msgid, event);
}

int32_t XLINK_FUNCTION xlink_receive_tcp_data(const xlink_uint8 **data, xlink_int32 datalength) {
	return xlink_receive_data(&p_xlink_sdk_instance, data, datalength, NULL, 1);
}

int32_t XLINK_FUNCTION xlink_receive_udp_data(const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr) {
	return xlink_receive_data(&p_xlink_sdk_instance, data, datalength, addr, 0);
}

void XLINK_FUNCTION xlink_process() {
	xlink_sdk_process(&p_xlink_sdk_instance);
}

void XLINK_FUNCTION xlink_report_version() {
	uint16_t previous_version = xlink_read_version();
	uint16_t current_version = p_xlink_sdk_instance->dev_firmware_version;
	
	if (previous_version < current_version) {
		xlink_sdk_event_t event;
		event.enum_event_type_t = EVENT_TYPE_REPORT_OTA_UPGRADE_RESULT;
		event.event_struct_t.ota_report_upgrade_result_package_t.from = FROM_TYEP_CLOUD;
		event.event_struct_t.ota_report_upgrade_result_package_t.result = 0;	//0:success 1:fail
		event.event_struct_t.ota_report_upgrade_result_package_t.firmware_type = FIRMWARE_TYEP_WIFI;
		event.event_struct_t.ota_report_upgrade_result_package_t.mod = 0;
		event.event_struct_t.ota_report_upgrade_result_package_t.current_version = current_version;
		event.event_struct_t.ota_report_upgrade_result_package_t.original_version = previous_version;
		event.event_struct_t.ota_report_upgrade_result_package_t.identify = p_xlink_sdk_instance->identify;
		event.event_struct_t.ota_report_upgrade_result_package_t.task_id_length = 0;
		event.event_struct_t.ota_report_upgrade_result_package_t.task_id_str = "";
		xlink_sdk_event_t *pevent = &event;
		int ret = xlink_post_event(&pevent);
		if (ret == 0) {
			xlink_write_version(current_version);
		}
	} else {
		xlink_upgrade_version_package_t package = {0};
		package.identify = p_xlink_sdk_instance->identify;
		package.firmware_type = FIRMWARE_TYEP_WIFI;
		package.mod = FIRMWARE_TYEP_WIFI;
		package.version = p_xlink_sdk_instance->dev_firmware_version;
		xlink_update_current_verison(&p_xlink_sdk_instance, 1, &package);
	}
}

// void XLINK_FUNCTION xlink_report_version() {
// 	uint16_t previous_version = xlink_read_version();
// 	uint16_t current_version = p_xlink_sdk_instance->dev_firmware_version;
// 	app_logd("previous_version: %d  current_version: %d", previous_version, current_version);
// 	xlink_upgrade_version_package_t package = {0};
// 	if(previous_version < current_version) {
// 		xlink_sdk_event_t event;
// 		xlink_sdk_event_t *pevent;
// 		event.enum_event_type_t = EVENT_TYPE_UPGRADE_COMPLETE;
// 		event.event_struct_t.upgrade_complete_t.current_version = current_version;
// 		event.event_struct_t.upgrade_complete_t.last_version = previous_version;
// 		event.event_struct_t.upgrade_complete_t.status = 1;
// 		event.event_struct_t.upgrade_complete_t.flag = 0x80;
// 		pevent = &event;
// 		int32_t ret = xlink_post_event(&pevent);
// 		if (ret == 0) {
// 			xlink_write_version(current_version);
// 		}
// 	} else {
// 		package.identify = p_xlink_sdk_instance->identify;
// 		package.firmware_type = FIRMWARE_TYEP_WIFI;
// 		package.mod = FIRMWARE_TYEP_WIFI;
// 		package.version = p_xlink_sdk_instance->dev_firmware_version;
// 		xlink_update_current_verison(&p_xlink_sdk_instance, 1, &package);
// 	}
// }

void XLINK_FUNCTION xlink_sync_cloud_time() {
	xlink_sdk_event_t event;
	xlink_sdk_event_t *pevent = &event;
	event.enum_event_type_t = EVENT_TYPE_REQ_DATETIME;
	xlink_post_event(&pevent);
}

void XLINK_FUNCTION xlink_sync_cloud_timeout_cb(void *arg) {
	retry_count++;
	app_logd("retry count: %d", retry_count);
	if (retry_count >= SYNC_CLOUDTIME_RETRYCNT) {
		os_timer_disarm(&retry_timer);
		retry_count = 0;
		xlink_tcp_disconnect();
	} else {
		xlink_sync_cloud_time();
	}
}

void XLINK_FUNCTION xlink_get_cloud_time(void *arg) {
	retry_count = 0;
	xlink_sync_cloud_time();
	os_timer_disarm(&retry_timer);
	os_timer_setfn(&retry_timer, xlink_sync_cloud_timeout_cb, NULL);
	os_timer_arm(&retry_timer, SYNC_CLOUDTIME_TIMEOUT, 1);
}

int32_t XLINK_FUNCTION xlink_update_datapoint_with_alarm(const xlink_uint8 **data, xlink_int32 datamaxlength) {
	msgid++;
	return xlink_update_datapoint(&p_xlink_sdk_instance, &msgid, data, datamaxlength, 1);
}

int32_t XLINK_FUNCTION xlink_update_datapoint_no_alarm(const xlink_uint8 **data, xlink_int32 datamaxlength) {
	msgid++;
	return xlink_update_datapoint(&p_xlink_sdk_instance, &msgid, data, datamaxlength, 0);
}

/**
 *	Xlink sdk callback 
 **/

void XLINK_FUNCTION xlink_event_cb(struct xlink_sdk_instance **sdk_instance, const xlink_sdk_event_t **event_t) {
	xlink_sdk_event_t *pevent = (xlink_sdk_event_t *) *event_t;
	xlink_enum_sdk_status_t status;
	xlink_datetime_t *pdatetime;
	xlink_upgrade_t upgrade;
	xlink_ota_task_check_ack_package_t package;
	int16_t disv;
	uint8_t url[XLINK_UPGRADE_URL_MAX_LENGTH];
	switch (pevent->enum_event_type_t) {
		case EVENT_TYPE_STATUS:
			status = pevent->event_struct_t.status.status;
			if (status == EVENT_DISCONNECTED_SERVER) {
				connected_cloud = false;
				xlink_tcp_disconnect();   
				app_logd("xlink cloud disconnected...");
			} else if (status == EVENT_CONNECTED_SERVER) {
				connected_cloud = true;
				server_reject = false;
				xlink_report_version();
				xlink_datapoint_update_all();
				xlink_get_cloud_time(NULL);
				app_logd("xlink cloud connected...");
			} else if (status == EVENT_SERVER_REJECT_DEVICE_REQUEST) {
				connected_cloud = false;
				server_reject = true;
				xlink_tcp_disconnect();
				app_logd("xlink cloud server reject connection...");
			}
			break;
		case EVENT_TYPE_REQUEST_CB:

			break;
		case EVENT_TYPE_REQ_DATETIME_CB:
			os_timer_disarm(&retry_timer);
			os_timer_disarm(&sync_timer);
			os_timer_setfn(&sync_timer, xlink_get_cloud_time, NULL);
			os_timer_arm(&sync_timer, SYNC_CLOUDTIME_PERIOD, 0);
			pdatetime = &pevent->event_struct_t.datetime_t;
			user_device_set_cloud_zone(pdatetime->zone);
			user_rtc_sync_cloud_cb(pdatetime);
			app_logd("sync cloud time: %d-%02d-%02d %02d:%02d:%02d week-%d zone-%d", pdatetime->year,
																					 pdatetime->month,
																					 pdatetime->day,
																					 pdatetime->hour,
																					 pdatetime->min,
																					 pdatetime->second,
																					 pdatetime->week,
																					 pdatetime->zone);
			break;
		case EVENT_TYPE_UPGRADE_CB:
			upgrade = pevent->event_struct_t.upgrade_t;
			if (upgrade.url_length == 0 || upgrade.url == NULL) {
				xlink_upgrade_report(XUPGRADE_ERRCODE_URL_INVALID);
				return;
			}
			app_logd("identify: %d firmware version: %d url:%s", p_xlink_sdk_instance->identify, upgrade.firmware_version, upgrade.url);
			// if(p_xlink_sdk_instance->identify == 0) {
				if (upgrade.firmware_version > p_xlink_sdk_instance->dev_firmware_version) {
					disv = upgrade.firmware_version - p_xlink_sdk_instance->dev_firmware_version;
					if (disv%2 == 0) {
						xlink_upgrade_report(XUPGRADE_ERRCODE_FW_INVALID);
						app_logd("upgrade task invalid target version...");
						return;
					}
					if(upgrade.url_length < XLINK_UPGRADE_URL_MAX_LENGTH) {
						os_memset(url, 0, XLINK_UPGRADE_URL_MAX_LENGTH);
						os_memcpy(url, upgrade.url, upgrade.url_length);
						xlink_upgrade_start(url);
					} else {
						xlink_upgrade_report(XUPGRADE_ERRCODE_URL_OVERLEN);
						app_logd("url link too long...");
					}
				} else {
					xlink_report_version();
					xlink_upgrade_report(XUPGRADE_ERRCODE_FW_INVALID);
				}
			// }
			break;
		case EVENT_TYPE_UPGRADE_COMPLETE:
			app_logd("upgrade complete...");
			break;
		case EVENT_TYPE_NOTIFY:
			app_logd("notify type: %d  %s", pevent->event_struct_t.notify_t.from_type, pevent->event_struct_t.notify_t.message);
			break;
		case EVENT_TYPE_PRODUCTION_TEST:
			app_logd("product test event- %d", pevent->event_struct_t.pdct_cb_t.pdct_event_t);
			break;
		// case EVENT_TYPE_SERVER_ASK_DEVICE_OTA:
		// 	app_logd("ask to check ota...");
		// 	if (pevent->event_struct_t.ask_device_ota_package_t.firmware_type == FIRMWARE_TYEP_WIFI) {
		// 		xlink_ota_check(pevent->event_struct_t.ask_device_ota_package_t.from);
		// 	}
		// 	break;
		// case EVENT_TYPE_CHECK_OTA_TASK_CB:
		// 	app_logd("check ota task cb...");
		// 	package = pevent->event_struct_t.ota_task_check_ack_package_t;
		// 	if (package.code == OTA_CHECK_SUCCESS 
		// 		&& package.firmware_type == FIRMWARE_TYEP_WIFI 
		// 		&& package.current_version == p_xlink_sdk_instance->dev_firmware_version 
		// 		&& package.target_version > package.current_version
		// 		&& package.target_url_length < XLINK_UPGRADE_URL_MAX_LENGTH
		// 		&& package.from == FROM_TYEP_CLOUD) {
		// 		disv = package.target_version - package.current_version;
		// 		if(disv%2 == 0) {
		// 			app_logd("upgrade task invalid target version...");
		// 			return;
		// 		}
		// 		os_memset(url, 0, XLINK_UPGRADE_URL_MAX_LENGTH);
		// 		os_memcpy(url, package.target_url, package.target_url_length);
		// 		app_logd("%s", url);
		// 		if((p_xlink_sdk_instance->identify == OTA_IDENTIFY_1 && package.identify == OTA_IDENTIFY_2)
		// 			|| (p_xlink_sdk_instance->identify == OTA_IDENTIFY_2 && package.identify == OTA_IDENTIFY_1)) {
		// 			xlink_upgrade_start(url);
		// 		}
		// 	}
		// 	break;
		default:
			break;
	}
}

xlink_int32 XLINK_FUNCTION xlink_send_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength,
        const xlink_addr_t **addr_t, xlink_uint8 flag) {
	xlink_uint8 *pdata = (xlink_uint8 *) *data;
	if (flag) {
		return xlink_tcp_send(pdata, datalength);
	} else {
		xlink_addr_t *paddr = (xlink_addr_t *) *addr_t;
		return xlink_udp_send(paddr, pdata, datalength);
	}
	return datalength;
}

xlink_uint32 XLINK_FUNCTION xlink_get_ticktime_ms_cb(struct xlink_sdk_instance **sdk_instance) {
    return (system_get_time()/1000);
}

xlink_int32 XLINK_FUNCTION xlink_set_datapoint_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength) {
	xlink_uint8 *pdata = (xlink_uint8 *) *data;
	xlink_array_to_datapoints(pdata, datalength);
	return datalength;
}

xlink_int32 XLINK_FUNCTION xlink_get_datapoint_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength) {
	xlink_uint8 *data = (xlink_uint8 *) *buffer;
	return xlink_datapoints_all_to_array(data);
}

xlink_int32 XLINK_FUNCTION xlink_write_flash_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength) {
	uint8_t *pdata = (uint8_t *) *data;
	return xlink_write_config(pdata, datalength);
}

xlink_int32 XLINK_FUNCTION xlink_read_flash_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength) {
	uint8_t *pdata = (uint8_t *) *buffer;
	return xlink_read_config(pdata, datamaxlength);
}

xlink_int32 XLINK_FUNCTION xlink_get_rssi_cb(struct xlink_sdk_instance **sdk_instance, xlink_uint16 *result, xlink_int16 *rssi, xlink_uint16 *AP_STA) {
	xlink_int32 ret = -1;
	uint8_t wifimode, wifirssi;
	wifimode = wifi_get_opmode();
	if(wifimode == STATION_MODE) {
		wifimode = 0;
	} else {
		wifimode = 1;
	}
	wifirssi = wifi_station_get_rssi();
	if(wifirssi == 31) {
		return -1;
	}
	*result = ret;
	*rssi = wifirssi;
	*AP_STA = wifimode;
	return ret;
}

xlink_int32 XLINK_FUNCTION xlink_probe_datapoint_cb(struct xlink_sdk_instance **sdk_instance, const xlink_uint8 **dp_idx, xlink_uint8 dp_idx_length, xlink_uint8 **buffer, xlink_int32 datamaxlength) {
	xlink_uint8 *dp_index = (xlink_uint8 *) *dp_idx;
	xlink_uint8 *data = (xlink_uint8 *) *buffer;
	return xlink_probe_datapoints_to_array(dp_index, dp_idx_length, data);
}

xlink_int32 XLINK_FUNCTION xlink_get_custom_test_data_cb(struct xlink_sdk_instance **sdk_instance,  xlink_uint16 *result,xlink_uint8 **data, xlink_int32 datamaxlength) {
	app_logd("result: %d  %d: ", &result, datamaxlength);
	// xlink_uint8 *buffer = (xlink_uint8 *) *data;
	// return xlink_datapoints_to_array(buffer);
	// // if (test_data_len < datamaxlength) {

	// // } else {

	// // }
	return 0;
}
/*  xlink callback  */


int32_t XLINK_FUNCTION xlink_test_start() {
	return xlink_production_test_start(&p_xlink_sdk_instance);
}

int32_t XLINK_FUNCTION xlink_test_end() {
	uint8_t *data = &test_data[0];
	return xlink_production_test_end(&p_xlink_sdk_instance, &data, sizeof(test_data));
}

int32_t XLINK_FUNCTION xlink_test_enable() {
	return xlink_enable_production_test(&p_xlink_sdk_instance, true);
}

int32_t XLINK_FUNCTION xlink_test_disable() {
	return xlink_enable_production_test(&p_xlink_sdk_instance, false);
}

/**
 * OTA Function
 *
 * */
// LOCAL void XLINK_FUNCTION xlink_ota_check(xlink_ota_from_type_t type) {
// 	uint32_t identify;
// 	xlink_sdk_event_t event;
// 	xlink_sdk_event_t *pevent;

// 	if (p_xlink_sdk_instance->identify == OTA_IDENTIFY_1) {
// 		identify = OTA_IDENTIFY_2;
// 	} else if (p_xlink_sdk_instance->identify == OTA_IDENTIFY_2) {
// 		identify = OTA_IDENTIFY_1;
// 	} else {
// 		return;
// 	}
	
// 	event.enum_event_type_t = EVENT_TYPE_CHECK_OTA_TASK;
// 	event.event_struct_t.ota_task_check_package_t.from = type;
// 	event.event_struct_t.ota_task_check_package_t.firmware_type = FIRMWARE_TYEP_WIFI;
// 	event.event_struct_t.ota_task_check_package_t.version = p_xlink_sdk_instance->dev_firmware_version;
// 	event.event_struct_t.ota_task_check_package_t.identify = identify;
// 	pevent = &event;
// 	xlink_post_event(&pevent);
// }

// LOCAL void XLINK_FUNCTION xlink_ota_report_result(xlink_ota_report_upgrade_result_package_t *result) {
// 	xlink_sdk_event_t event;
// 	xlink_sdk_event_t *pevent = &event;
	
// 	event.enum_event_type_t = EVENT_TYPE_REPORT_OTA_UPGRADE_RESULT;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.from = result->from;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.firmware_type = result->firmware_type;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.identify = result->identify;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.current_version = result->current_version;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.original_version = result->original_version;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.mod = result->mod;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.result = result->result;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.task_id_length = result->task_id_length;
// 	event.event_struct_t.ota_report_upgrade_result_package_t.task_id_str = result->task_id_str;
// 	pevent = &event;
// 	xlink_post_event(&pevent);
// } 
