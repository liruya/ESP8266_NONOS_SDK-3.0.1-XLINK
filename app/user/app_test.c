#include "app_test.h"

#define	TEST_FLASH_PERIOD		500
#define	TEST_FLASH_COUNT		4

// #define	EXOTERRA_TEST_SSID		"exoterra_test"
// #define	EXOTERRA_TEST_PSW		"exoterra001"
#define	EXOTERRA_TEST_SSID		"TP-LINK_E152"
#define	EXOTERRA_TEST_PSW		"inledco152"

typedef struct {
	uint32_t count;
	os_timer_t led_timer;
} test_para_t;

LOCAL test_para_t *ptest = NULL;
LOCAL bool test_status;

void ESPFUNC app_test_flash_cb(void *arg) {
	if (ptest == NULL) {
		return;
	}
	user_device_t *pdev = arg;
	gpio_toggle(pdev->test_led1_num);
	gpio_toggle(pdev->test_led2_num);
	ptest->count++;
	if (ptest->count > TEST_FLASH_COUNT*2) {
		os_timer_disarm(&ptest->led_timer);
		os_free(ptest);
		ptest = NULL;
		
		gpio_high(pdev->test_led1_num);
		gpio_high(pdev->test_led2_num);
		user_device_init(pdev);
		xlink_init(pdev);
	}
}

bool ESPFUNC app_test_status() {
	return test_status;
}

void ESPFUNC app_test_init(user_device_t *pdev) {
	if (pdev == NULL) {
		return;
	}
	struct station_config config;
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_get_config(&config);
	os_memset(config.ssid, 0, sizeof(config.ssid));
	os_memset(config.password, 0, sizeof(config.password));
	os_memcpy(config.ssid, EXOTERRA_TEST_SSID, os_strlen(EXOTERRA_TEST_SSID));
	os_memcpy(config.password, EXOTERRA_TEST_PSW, os_strlen(EXOTERRA_TEST_PSW));
	
	config.bssid_set = false;
	config.threshold.rssi = -127;
	config.threshold.authmode = AUTH_WPA_WPA2_PSK;
    wifi_station_set_config_current(&config);
	wifi_station_connect();
	
	gpio_low(pdev->test_led1_num);
	gpio_high(pdev->test_led2_num);

	ptest = (test_para_t *) os_zalloc(sizeof(test_para_t));
	if (ptest == NULL) {
		app_loge("test init failed -> malloc failed");
		return;
	}
	os_timer_disarm(&ptest->led_timer);
	os_timer_setfn(&ptest->led_timer, app_test_flash_cb, pdev);
	os_timer_arm(&ptest->led_timer, TEST_FLASH_PERIOD, 1);
	test_status = true;
}
