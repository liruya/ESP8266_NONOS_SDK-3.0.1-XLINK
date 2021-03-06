#include "user_device.h"
#include "user_rtc.h"
#include "app_util.h"
#include "xlink_datapoint.h"
#include "xlink_upgrade.h"
#ifdef	USE_LOCATION
#include "user_geography.h"
#endif

#define	PSW_ENABLE_MASK	0xFF000000
#define	PSW_ENABLE_FLAG	0x55000000
#define	PSW_MASK		0x00FFFFFF
#define	PSW_MAX			999999

#define	SNSUBSCRIBE_ENABLE_PERIOD	60000

bool connected_local;

// LOCAL char property[65];
LOCAL os_timer_t proc_timer;
LOCAL os_timer_t snsub_timer;

LOCAL int16_t m_cloud_zone;
LOCAL int16_t wifi_rssi;

void ESPFUNC user_device_set_cloud_zone(int16_t zone) {
	if (m_cloud_zone != zone) {
		m_cloud_zone = zone;
		xlink_datapoint_set_changed(CLOUDZONE_INDEX);
	}
}

bool ESPFUNC user_device_psw_isvalid(user_device_t *pdev) {
	if (pdev == NULL || pdev->pconfig == NULL) {
		return false;
	}
	if ((pdev->pconfig->local_psw&PSW_ENABLE_MASK) == PSW_ENABLE_FLAG && (pdev->pconfig->local_psw&PSW_MASK) <= PSW_MAX) {
		return true;
	}
	return false;
}

LOCAL void ESPFUNC user_device_datetime_calibration(user_device_t *pdev) {
	if (pdev == NULL || pdev->para == NULL || pdev->pconfig == NULL) {
		return;
	}
	int16_t cloud_zone = user_rtc_get_zone();
	pdev->para->year = user_rtc_get_year();
	pdev->para->month = user_rtc_get_month();
	pdev->para->day = user_rtc_get_day();
	pdev->para->week = user_rtc_get_week();
	pdev->para->hour = user_rtc_get_hour();
	pdev->para->minute = user_rtc_get_minute();
	pdev->para->second = user_rtc_get_second();
	int16_t zone = pdev->pconfig->zone;
	if(app_util_zone_isvalid(cloud_zone) && app_util_zone_isvalid(zone)) {
		int16_t t0 = (cloud_zone/100)*60 + (cloud_zone%100);
		int16_t t1 = (zone/100)*60 + (zone%100);
		int16_t dt = t1 - t0;
		int8_t dh = dt/60;
		int8_t dm = dt%60;
		uint8_t month_days;
		pdev->para->minute += dm;
		if(pdev->para->minute > 59) {
			pdev->para->minute -= 60;
			pdev->para->hour++;
		} else if(pdev->para->minute < 0) {
			pdev->para->minute += 60;
			pdev->para->hour--;
		}
		pdev->para->hour += dh;
		if(pdev->para->hour > 23) {
			pdev->para->hour -= 24;
			pdev->para->week++;
			if(pdev->para->week > 6) {
				pdev->para->week = 0;
			}
			pdev->para->day++;
			month_days = MONTH[pdev->para->month-1];
			if (pdev->para->month == 2 && app_util_is_leap_year(pdev->para->year)) {
				month_days += 1;
			}
			if ( pdev->para->day > month_days ) {
				pdev->para->day = 1;
				pdev->para->month++;
				if ( pdev->para->month > 12 ) {
					pdev->para->month = 1;
					pdev->para->year++;
				}
			}
		} else if(pdev->para->hour < 0) {
			pdev->para->hour += 24;
			pdev->para->week--;
			if(pdev->para->week < 0) {
				pdev->para->week = 6;
			}
			pdev->para->day--;
			if(pdev->para->day < 1) {
				pdev->para->month--;
				if(pdev->para->month < 1) {
					pdev->para->month = 12;
					pdev->para->year--;
				}
				month_days = MONTH[pdev->para->month-1];
				if (pdev->para->month == 2 && app_util_is_leap_year(pdev->para->year)) {
					month_days += 1;
				}
				pdev->para->day = month_days;
			}
		}

		device_para_t *para = pdev->para;
		os_memset(para->datetime, 0, sizeof(para->datetime));
		os_sprintf(para->datetime, "%04d-%02d-%02d %02d:%02d:%02d", para->year, para->month, para->day, para->hour, para->minute, para->second);
		xlink_datapoint_set_changed(DATETIME_INDEX);
	}
}

#ifdef	USE_LOCATION
LOCAL bool ESPFUNC user_device_update_gis(user_device_t *pdev) {
	if (pdev == NULL || pdev->para == NULL || pdev->pconfig == NULL) {
		return false;
	}
	LOCAL bool valid;
	LOCAL uint16_t sunrise;
	LOCAL uint16_t sunset;
	uint16_t year = pdev->para->year;
	uint8_t month = pdev->para->month;
	uint8_t day = pdev->para->day;
	valid = get_sunrise_sunset(	pdev->pconfig->longitude,
								pdev->pconfig->latitude,
								year,
								month,
								day,
								pdev->pconfig->zone,
								&sunrise,
								&sunset);
	if (valid) {
		valid = pdev->pconfig->gis_enable;
		if (pdev->para->gis_valid != valid || pdev->para->gis_sunrise != sunrise || pdev->para->gis_sunset != sunset) {
			pdev->para->gis_valid = valid;
			pdev->para->gis_sunrise = sunrise;
			pdev->para->gis_sunset = sunset;
			return true;
		}
	} else if (pdev->para->gis_valid) {
		pdev->para->gis_valid = false;
		return true;
	}
	return false;
}
#endif

void ESPFUNC user_device_save_config(user_device_t *pdev) {
	if (pdev == NULL) {
		return;
	}
	pdev->save_config();
}

void ESPFUNC user_device_para_init(user_device_t *pdev) {
	if (pdev == NULL || pdev->pconfig == NULL) {
		return;
	}
	pdev->para_init();
	if (!app_util_zone_isvalid(pdev->pconfig->zone)) {
		pdev->pconfig->zone = 0;
	}
	if (pdev->pconfig->daytime_start > 1439) {
		pdev->pconfig->daytime_start = 0;
	}
	if (pdev->pconfig->daytime_end > 1439) {
		pdev->pconfig->daytime_end = 0;
	}
#ifdef	USE_LOCATION
if (!app_util_longitude_isvalid(pdev->pconfig->longitude)) {
		pdev->pconfig->longitude = 0;
	}
	if (!app_util_latitude_isvalid(pdev->pconfig->latitude)) {
		pdev->pconfig->latitude = 0;
	}
	if (pdev->pconfig->gis_enable > 1) {
		pdev->pconfig->gis_enable = false;
	}
#endif
}

void ESPFUNC user_device_key_init(user_device_t *pdev) {
	if (pdev == NULL) {
		return;
	}
	pdev->key_init();
}

void ESPFUNC user_device_datapoint_init(user_device_t *pdev) {
	if (pdev == NULL || pdev->para == NULL || pdev->pconfig == NULL) {
		app_loge("device datapoint init error");
		return;
	}
	uint8_t len = os_strlen(pdev->property);
	if (len > DATAPOINT_STR_MAX_LEN) {
		len = DATAPOINT_STR_MAX_LEN;
	}
	xlink_datapoint_init_string(PROPERTY_INDEX, pdev->property, len);
	xlink_datapoint_init_int16(ZONE_INDEX, &pdev->pconfig->zone);
	xlink_datapoint_init_string(DATETIME_INDEX, pdev->para->datetime, os_strlen(pdev->para->datetime));
	xlink_datapoint_init_binary(SYNC_DATETIME_INDEX, (uint8_t *) &datetime, sizeof(datetime));
	xlink_datapoint_init_uint16(DAYTIME_START_INDEX, &pdev->pconfig->daytime_start);
	xlink_datapoint_init_uint16(DAYTIME_END_INDEX, &pdev->pconfig->daytime_end);

#ifdef	USE_LOCATION
	xlink_datapoint_init_float(LONGITUDE_INDEX, &pdev->pconfig->longitude);
	xlink_datapoint_init_float(LATITUDE_INDEX, &pdev->pconfig->latitude);
	xlink_datapoint_init_byte(GIS_ENABLE_INDEX, &pdev->pconfig->gis_enable);
	xlink_datapoint_init_uint16(GIS_SUNRISE_INDEX, &pdev->para->gis_sunrise);
	xlink_datapoint_init_uint16(GIS_SUNSET_INDEX, &pdev->para->gis_sunset);
	xlink_datapoint_init_byte(GIS_VALID_INDEX, &pdev->para->gis_valid);
#endif

	xlink_datapoint_init_int16(CLOUDZONE_INDEX, &m_cloud_zone);
	xlink_datapoint_init_int16(RSSI_INDEX, &wifi_rssi);
	xlink_datapoint_init_byte(UPGRADE_STATE_INDEX, &upgrade_state);
	xlink_datapoint_init_uint32(LOCAL_PSW_INDEX, &pdev->pconfig->local_psw);
	xlink_datapoint_init_byte(SNSUB_ENABLE_INDEX, &pdev->para->sn_subscribe_enable);
	pdev->datapoint_init();
}

void ESPFUNC user_device_process(void *arg) {
	user_device_t *pdev = arg;
	if (pdev == NULL) {
		return;
	}
	sint8_t rssi = wifi_station_get_rssi();
	if (wifi_rssi != rssi) {
		wifi_rssi = rssi;
		xlink_datapoint_set_changed(RSSI_INDEX);
	}
	user_device_datetime_calibration(pdev);

#ifdef	USE_LOCATION
	bool update = user_device_update_gis(pdev);
	if (update) {
		xlink_datapoint_set_changed(GIS_SUNRISE_INDEX);
		xlink_datapoint_set_changed(GIS_SUNSET_INDEX);
		xlink_datapoint_set_changed(GIS_VALID_INDEX);
	}
#endif

	pdev->process(arg);

#ifdef	USE_LOCATION
	if (update) {
		user_device_update_dpchanged();
	}
#endif
}

void ESPFUNC user_device_update_dpall() {
	if (!connected_local) {
		return;
	}
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	xlink_datapoint_update_all();
}

void ESPFUNC user_device_update_dpchanged() {
	if (!connected_local) {
		return;
	}
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	xlink_datapoint_update_changed();
}

void ESPFUNC user_device_init(user_device_t *pdev) {
	if (pdev == NULL || pdev->para == NULL || pdev->pconfig == NULL) {
		app_loge("device init failed...");
		while (1);
		return;
	}
	spi_flash_read(DEVICE_PROPERTY_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) pdev->property, sizeof(pdev->property));
	device_para_t *para = pdev->para;
	os_memset(para->datetime, 0, sizeof(para->datetime));
	os_sprintf(para->datetime, "%04d-%02d-%02d %02d:%02d:%02d", para->year, para->month, para->day, para->hour, para->minute, para->second);
	if (wifi_get_macaddr(STATION_IF, pdev->mac)) {
		os_memset(pdev->sn, 0, sizeof(pdev->sn));
		os_sprintf(pdev->sn, "%02x%02x%02x%02x%02x%02x", pdev->mac[0], pdev->mac[1], pdev->mac[2], pdev->mac[3], pdev->mac[4], pdev->mac[5]);
		os_memset(pdev->apssid, 0, sizeof(pdev->apssid));
		os_sprintf(pdev->apssid, "%s_%02X%02X%02X", pdev->product, pdev->mac[3], pdev->mac[4], pdev->mac[5]);
	} else {
		app_loge("device init failed -> get mac addr failed");
		while (1);
		return;
	}
	user_device_para_init(pdev);
	user_device_datapoint_init(pdev);
	user_device_key_init(pdev);
	pdev->init();
	os_timer_disarm(&proc_timer);
	os_timer_setfn(&proc_timer, user_device_process, pdev);
	os_timer_arm(&proc_timer, 1000, 1);
}

bool ESPFUNC user_device_poweron_check(user_device_t *pdev) {
	if (pdev == NULL) {
		return false;
	}
	bool flag = false;
	uint8_t cnt = 0;

	while(cnt < 250) {
		cnt++;
		os_delay_us(10000);
		system_soft_wdt_feed();
		if (GPIO_INPUT_GET(pdev->key_io_num) == 0) {
			flag = true;
			break;
		}
	}

	if (flag) {
		cnt = 0;
		//长按4秒
		while(cnt < 200) {
			system_soft_wdt_feed();
			os_delay_us(20000);
			if(GPIO_INPUT_GET(pdev->key_io_num) == 0) {
				cnt++;
			} else {
				return false;
			}
		}
		return true;
	}
	return false;
}

/**
 * sn subscribe functions
 */

LOCAL void ESPFUNC user_device_snsubscribe_timeout(void *arg) {
	user_device_t *pdev = arg;
	if (pdev == NULL || pdev->para == NULL) {
		return;
	}
	pdev->para->sn_subscribe_enable = false;
	xlink_datapoint_update_all();
}

void ESPFUNC user_device_enable_snsubscribe(user_device_t *pdev) {
	if (pdev == NULL || pdev->para == NULL) {
		return;
	}
	pdev->para->sn_subscribe_enable = true;
	os_timer_disarm(&snsub_timer);
	os_timer_setfn(&snsub_timer, user_device_snsubscribe_timeout, pdev);
	os_timer_arm(&snsub_timer, SNSUBSCRIBE_ENABLE_PERIOD, 0);
}

/* sn subscribe functions end*/