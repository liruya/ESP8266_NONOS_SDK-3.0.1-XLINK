#include "user_led.h"
#include "app_board_led.h"
#include "user_key.h"
#include "xlink.h"
#include "xlink_datapoint.h"
#include "user_rtc.h"
#include "user_geography.h"
#include "user_apconfig.h"
#include "user_smartconfig.h"
#include "app_test.h"
#include "user_task.h"
#include "user_indicator.h"
#include "app_util.h"
#include "pwm.h"

#define	PRODUCT_TYPE			"EXOTerraStrip"
#define LED_PRODUCT_ID			"160fa2b85ae803e9160fa2b85ae84e01"
#define LED_PRODUCT_KEY			"83b9287195eafa8e16ba4d409aba5375"

#define	LED_PROPERTY			"Marine 500mm"

#define PWM_PERIOD			4500			//us duty max 100000
#define DUTY_GAIN			100
#define BRIGHT_MIN			0
#define BRIGHT_MAX			1000
#define BRIGHT_DELT			200
#define BRIGHT_MIN2			10
#define BRIGHT_DELT_TOUCH	10
#define BRIGHT_STEP_NORMAL	10

#define TIME_VALUE_MAX		1439

#define LED_STATE_OFF		0
#define LED_STATE_DAY		1
#define LED_STATE_NIGHT		2
#define LED_STATE_WIFI		3

#define FLASH_PERIOD		750				//ms

#define	USER_KEY_NUM					1
#define USER_KEY_LONG_TIME_NORMAL		40

#define PREVIEW_INTERVAL	50

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;

LOCAL uint32_t pwm_io_info[][3] = { {PWM1_IO_MUX, PWM1_IO_FUNC, PWM1_IO_NUM},
									{PWM2_IO_MUX, PWM2_IO_FUNC, PWM2_IO_NUM},
									{PWM3_IO_MUX, PWM3_IO_FUNC, PWM3_IO_NUM},
									{PWM4_IO_MUX, PWM4_IO_FUNC, PWM4_IO_NUM},
									{PWM5_IO_MUX, PWM5_IO_FUNC, PWM5_IO_NUM}};

LOCAL bool find_flag;
LOCAL bool prev_flag_shadow;
LOCAL bool prev_flag;
LOCAL uint16_t prev_ct;
LOCAL os_timer_t find_timer;
LOCAL os_timer_t prev_timer;

LOCAL os_timer_t ramp_timer;

LOCAL void user_led_ramp();

LOCAL void user_led_off_onShortPress();
LOCAL void user_led_day_onShortPress();
LOCAL void user_led_night_onShortPress();
LOCAL void user_led_wifi_onShortPress();

LOCAL void user_led_off_onLongPress();
LOCAL void user_led_day_onLongPress();
LOCAL void user_led_night_onLongPress();
LOCAL void user_led_wifi_onLongPress();

LOCAL void user_led_off_onContPress();
LOCAL void user_led_day_onContPress();
LOCAL void user_led_night_onContPress();
LOCAL void user_led_wifi_onContPress();

LOCAL void user_led_off_onRelease();
LOCAL void user_led_day_onRelease();
LOCAL void user_led_night_onRelease();
LOCAL void user_led_wifi_onRelease();

LOCAL void user_led_onShortPress();
LOCAL void user_led_onLongPress();
LOCAL void user_led_onContPress();
LOCAL void user_led_onRelease();

LOCAL void user_led_auto_pro_run(void *arg);
LOCAL void user_led_auto_pro_prev(void *arg);
LOCAL void user_led_turnoff_ramp();
LOCAL void user_led_turnon_ramp();
LOCAL void user_led_turnoff_direct();
LOCAL void user_led_turnmax_direct();
LOCAL void user_led_update_day_status();
LOCAL void user_led_update_night_status();
LOCAL void user_led_update_day_bright();
LOCAL void user_led_update_night_bright();
LOCAL void user_led_indicate_off();
LOCAL void user_led_indicate_day();
LOCAL void user_led_indicate_night();
LOCAL void user_led_indicate_wifi();

LOCAL void user_led_default_config();
LOCAL void user_led_save_config();
LOCAL void user_led_para_init();
LOCAL void user_led_key_init();
LOCAL void user_led_datapoint_init();
LOCAL void user_led_init();
LOCAL void user_led_process(void *arg);
LOCAL void user_led_datapoint_changed_cb();

LOCAL void user_led_pre_smartconfig() ;
LOCAL void user_led_post_smartconfig();
LOCAL void user_led_pre_apconfig();
LOCAL void user_led_post_apconfig();

LOCAL const key_function_t short_press_func[4] = {	user_led_off_onShortPress,
													user_led_day_onShortPress,
													user_led_night_onShortPress,
													user_led_wifi_onShortPress };
LOCAL const key_function_t long_press_func[4] = { 	user_led_off_onLongPress,
													user_led_day_onLongPress,
													user_led_night_onLongPress,
													user_led_wifi_onLongPress };
LOCAL const key_function_t cont_press_func[4] = { 	user_led_off_onContPress,
													user_led_day_onContPress,
													user_led_night_onContPress,
													user_led_wifi_onContPress };
LOCAL const key_function_t release_func[4] = { 	user_led_off_onRelease,
												user_led_day_onRelease,
												user_led_night_onRelease,
												user_led_wifi_onRelease };

LOCAL led_config_t led_config;
LOCAL led_para_t led_para = {
	.channel_count = LED_CHANNEL_COUNT,
	.channel_names = {CHN1_NAME, CHN2_NAME, CHN3_NAME, CHN4_NAME, CHN5_NAME}
};
LOCAL const task_impl_t apc_impl = newTaskImpl(user_led_pre_apconfig, user_led_post_apconfig);
LOCAL const task_impl_t sc_impl = newTaskImpl(user_led_pre_smartconfig, user_led_post_smartconfig);
user_device_t user_dev_led = {
	.product = PRODUCT_TYPE,
	.product_id = LED_PRODUCT_ID,
	.product_key = LED_PRODUCT_KEY,
	.fw_version = FW_VERSION,

	.key_io_num = TOUCH_IO_NUM,
	.test_led1_num = LEDR_IO_NUM,
	.test_led2_num = LEDG_IO_NUM,

	.board_init = app_board_led_init,
	.default_config = user_led_default_config,
	.save_config = user_led_save_config,
	.para_init = user_led_para_init,
	.key_init = user_led_key_init,
	.datapoint_init = user_led_datapoint_init,
	.init = user_led_init,
	.process = user_led_process,
	.dp_changed_cb = user_led_datapoint_changed_cb,
	.para = &led_para.super,
	.pconfig = &led_config.super
};

LOCAL void ESPFUNC user_led_ledg_toggle() {
	ledg_toggle();
}

LOCAL void ESPFUNC user_led_setzone(int16_t zone) {
	if (app_util_zone_isvalid(zone)) {
		led_config.super.zone = zone;
		user_led_save_config();
		user_device_enable_snsubscribe(&user_dev_led);
	}
}

LOCAL void ESPFUNC user_led_pre_smartconfig() {
	xlink_tcp_disconnect();
	xlink_enable_subscription();
	user_indicator_start(SMARTCONFIG_FLASH_PERIOD, 0, user_led_ledg_toggle);
}

LOCAL void ESPFUNC user_led_post_smartconfig() {
	user_indicator_stop();
	ledg_on();
}

LOCAL void ESPFUNC user_led_pre_apconfig() {
	xlink_tcp_disconnect();
	wifi_set_opmode_current(SOFTAP_MODE);
	user_indicator_start(APCONFIG_FLASH_PERIOD, 0, user_led_ledg_toggle);
}

LOCAL void ESPFUNC user_led_post_apconfig() {
	user_indicator_stop();
	ledg_on();
}

LOCAL bool ESPFUNC user_led_check_profile(profile_t *profile) {
	if(profile == NULL) {
		return false;
	}
	if(profile->count < POINT_COUNT_MIN || profile->count > POINT_COUNT_MAX)  {
		return false;
	}
	uint8_t i, j;
	for(i = 0; i < profile->count; i++) {
		if(profile->timers[i] > TIME_VALUE_MAX) {
			return false;
		}
		for(j = 0; j < LED_CHANNEL_COUNT; j++) {
			if(profile->brights[i*LED_CHANNEL_COUNT+j] > 100) {
				return false;
			}
		}
	}
	return true;
}

LOCAL void user_led_default_profile(profile_t *profile) {
	if(profile == NULL) {
		return;
	}
	uint8_t i;
	profile->count = 6;
	profile->timers[0] = 420;
	profile->timers[1] = 480;
	profile->timers[2] = 1020;
	profile->timers[3] = 1080;
	profile->timers[4] = 1320;
	profile->timers[5] = 1350;
	for(i = 0; i < LED_CHANNEL_COUNT; i++) {
		profile->brights[i] = 0;
		profile->brights[LED_CHANNEL_COUNT+i] = 100;
		profile->brights[2*LED_CHANNEL_COUNT+i] = 100;
		profile->brights[3*LED_CHANNEL_COUNT+i] = 0;
		profile->brights[4*LED_CHANNEL_COUNT+i] = 0;
		profile->brights[5*LED_CHANNEL_COUNT+i] = 0;
	}
	profile->brights[3*LED_CHANNEL_COUNT+NIGHT_CHANNEL] = 5;
	profile->brights[4*LED_CHANNEL_COUNT+NIGHT_CHANNEL] = 5;
}

LOCAL void ESPFUNC user_led_load_duty(uint32_t value, uint8_t chn) {
	pwm_set_duty(value * DUTY_GAIN, chn);
}

LOCAL void ESPFUNC user_led_default_config() {
	uint8_t i;
	os_memset(&led_config, 0, sizeof(led_config));
	led_config.last_mode = MODE_AUTO;
	led_config.state = LED_STATE_OFF;
	led_config.all_bright = BRIGHT_MAX;
	led_config.blue_bright = BRIGHT_MAX;

	led_config.mode = MODE_MANUAL;
	led_config.power = 0;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_config.bright[i] = BRIGHT_MAX;
	}
		
	led_config.gis_enable = false;

	led_config.sunrise = 420;					//07:00
	led_config.sunrise_ramp = 60;
	led_config.sunset = 1080;					//18:00
	led_config.sunset_ramp = 60;
	led_config.turnoff_enabled = true;
	led_config.turnoff_time = 1320;					//22:00
	for(i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_config.day_bright[i] = 100;
		led_config.night_bright[i] = 0;
	}
	led_config.night_bright[NIGHT_CHANNEL] = 5;

	user_led_default_profile(&led_config.profile0);
	for (i = 0; i < PROFILE_COUNT_MAX; i++) {
		user_led_default_profile(&led_config.profiles[i]);
	}

	led_config.super.saved_flag = CONFIG_DEFAULT_FLAG;
}

LOCAL void ESPFUNC user_led_save_config() {
	//测试模式下不改变保存的参数
	if (app_test_status()) {
		return;
	}
	led_config.super.saved_flag = CONFIG_SAVED_FLAG;
	xlink_write_user_para((uint8_t *) &led_config, sizeof(led_config));
}

LOCAL void ESPFUNC user_led_para_init() {
	uint8_t i, j;
	xlink_read_user_para((uint8_t *)&led_config, sizeof(led_config));
	if (led_config.super.saved_flag != CONFIG_SAVED_FLAG) {
		user_led_default_config();
	}
	if (led_config.mode == MODE_AUTO || led_config.mode == MODE_PRO) {
		led_config.last_mode = led_config.mode;
	} else {
		led_config.mode = MODE_MANUAL;
		if (led_config.last_mode == 0 || led_config.last_mode == 3) {
			led_config.last_mode = MODE_AUTO;
		}
	}

	if (led_config.power > 1) {
		led_config.power = 1;
	}
	if (led_config.all_bright > BRIGHT_MAX) {
		led_config.all_bright = BRIGHT_MAX;
	}
	if (led_config.blue_bright > BRIGHT_MAX) {
		led_config.blue_bright = BRIGHT_MAX;
	}
	if (led_config.all_bright < BRIGHT_MIN2) {
		led_config.all_bright = BRIGHT_MIN2;
	}
	if (led_config.blue_bright < BRIGHT_MIN2) {
		led_config.blue_bright = BRIGHT_MIN2;
	}
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		if (led_config.bright[i] > BRIGHT_MAX) {
			led_config.bright[i] = BRIGHT_MAX;
		}
	}
	
	if (led_config.gis_enable > 1)  {
		led_config.gis_enable = 0;
	}
	if (led_config.sunrise > TIME_VALUE_MAX) {
		led_config.sunrise = 0;
	}
	if (led_config.sunrise_ramp > 240) {
		led_config.sunrise_ramp = 0;
	}
	if (led_config.sunset > TIME_VALUE_MAX) {
		led_config.sunset = 0;
	}
	if (led_config.sunset_ramp > 240) {
		led_config.sunset_ramp = 0;
	}
	if (led_config.turnoff_time > TIME_VALUE_MAX) {
		led_config.turnoff_time = 0;
	}
	if (led_config.turnoff_enabled > 1) {
		led_config.turnoff_enabled = 1;
	}
	for(j = 0; j < LED_CHANNEL_COUNT; j++) {
		if(led_config.day_bright[j] > 100) {
			led_config.day_bright[j] = 100;
		}
		if(led_config.night_bright[j] > 100) {
			led_config.night_bright[j] = 100;
		}
	}

	if (led_config.profile0.count < POINT_COUNT_MIN) {
		led_config.profile0.count = POINT_COUNT_MIN;
	} else if (led_config.profile0.count > POINT_COUNT_MAX) {
		led_config.profile0.count = POINT_COUNT_MAX;
	}
	for (i = 0; i < POINT_COUNT_MAX; i++) {
		if (led_config.profile0.timers[i] > TIME_VALUE_MAX) {
			led_config.profile0.timers[i] = 0;
		}
		for (j = 0; j < LED_CHANNEL_COUNT; j++) {
			if (led_config.profile0.brights[i*j] > 100) {
				led_config.profile0.brights[i*j] = 100;
			}
		}
	}
	if(led_config.select_profile > PROFILE_COUNT_MAX) {
		led_config.select_profile = 0;
	}
}

LOCAL void ESPFUNC user_led_key_init() {
	pkeys[0] = user_key_init_single(TOUCH_IO_NUM,
							  	TOUCH_IO_FUNC,
								TOUCH_IO_MUX,
								user_led_onShortPress,
								user_led_onLongPress,
								user_led_onContPress,
								user_led_onRelease);
	if (led_config.state == LED_STATE_WIFI) {
		pkeys[0]->long_count = KEY_LONG_PRESS_COUNT;
	} else {
		pkeys[0]->long_count = USER_KEY_LONG_TIME_NORMAL;
	}
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list(&key_list);
}

LOCAL void ESPFUNC user_led_datapoint_init() {
	uint8_t i;

	xlink_datapoint_init_byte(10, (uint8_t *) &led_para.channel_count);
	xlink_datapoint_init_byte(17, &led_config.mode);
	xlink_datapoint_init_byte(18, &led_config.power);
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		xlink_datapoint_init_string(11+i, (uint8_t *) led_para.channel_names[i], os_strlen(led_para.channel_names[i]));
		xlink_datapoint_init_uint16(19+i, &led_config.bright[i]);
	}

	xlink_datapoint_init_binary(25, &led_config.custom_bright[0][0], LED_CHANNEL_COUNT);
	xlink_datapoint_init_binary(26, &led_config.custom_bright[1][0], LED_CHANNEL_COUNT);
	xlink_datapoint_init_binary(27, &led_config.custom_bright[2][0], LED_CHANNEL_COUNT);
	xlink_datapoint_init_binary(28, &led_config.custom_bright[3][0], LED_CHANNEL_COUNT);

	xlink_datapoint_init_byte(29, &led_config.gis_enable);
	xlink_datapoint_init_uint16(30, &led_para.gis_sunrise);
	xlink_datapoint_init_uint16(31, &led_para.gis_sunset);
	xlink_datapoint_init_byte(32, &led_para.gis_valid);

	xlink_datapoint_init_uint16(33, &led_config.sunrise);
	xlink_datapoint_init_byte(34, &led_config.sunrise_ramp);
	xlink_datapoint_init_binary(35, led_config.day_bright, LED_CHANNEL_COUNT);
	xlink_datapoint_init_uint16(36, &led_config.sunset);
	xlink_datapoint_init_byte(37, &led_config.sunset_ramp);
	xlink_datapoint_init_binary(38, led_config.night_bright, LED_CHANNEL_COUNT);
	xlink_datapoint_init_byte(39, &led_config.turnoff_enabled);
	xlink_datapoint_init_uint16(40, &led_config.turnoff_time);

	xlink_datapoint_init_byte(41, &led_config.profile0.count);
	xlink_datapoint_init_binary(42, (uint8_t *) led_config.profile0.timers, led_config.profile0.count*2);
	xlink_datapoint_init_binary(43, led_config.profile0.brights, led_config.profile0.count*LED_CHANNEL_COUNT);
	for(i = 0; i < PROFILE_COUNT_MAX; i++) {
		xlink_datapoint_init_byte(45+i*4, &led_config.profiles[i].count);
		xlink_datapoint_init_binary(46+i*4, (uint8_t *) led_config.profiles[i].timers, led_config.profiles[i].count*2);
		xlink_datapoint_init_binary(47+i*4, led_config.profiles[i].brights, led_config.profiles[i].count*LED_CHANNEL_COUNT);
	}
	xlink_datapoint_init_byte(92, &led_config.select_profile);

	xlink_datapoint_init_byte(93, &prev_flag_shadow);

	// xlink_datapoint_init_byte(UPGRADE_STATE_INDEX, &led_para.super.upgrade_state);
	// xlink_datapoint_init_uint32(LOCAL_PSW_INDEX, &led_config.super.local_psw);
	// xlink_datapoint_init_byte(SNSUB_ENABLE_INDEX, &led_para.super.sn_subscribe_enable);
}

LOCAL void ESPFUNC user_led_init() {
	pwm_init(PWM_PERIOD, led_para.current_bright, LED_CHANNEL_COUNT, pwm_io_info);
	pwm_start();
	switch (led_config.state) {
		case LED_STATE_OFF:
			user_led_indicate_off();
			user_led_turnoff_ramp();
			break;
		case LED_STATE_DAY:
			user_led_indicate_day();
			user_led_update_day_bright();
			break;
		case LED_STATE_NIGHT:
			user_led_indicate_night();
			user_led_update_night_bright();
			break;
		case LED_STATE_WIFI:
			user_led_indicate_wifi();
			if (led_config.mode == MODE_MANUAL) {
				if (led_config.power) {
					user_led_turnon_ramp();
				} else {
					user_led_turnoff_ramp();
				}
			}
			break;
		default:
			break;
	}
	os_timer_disarm(&ramp_timer);
	os_timer_setfn(&ramp_timer, user_led_ramp, NULL);
	os_timer_arm(&ramp_timer, 10, 1);
}

LOCAL void ESPFUNC user_led_turnoff_ramp() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_para.target_bright[i] = 0;
	}
}

LOCAL void ESPFUNC user_led_turnon_ramp() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_para.target_bright[i] = led_config.bright[i];
	}
}

LOCAL void ESPFUNC user_led_turnoff_direct() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_para.current_bright[i] = 0;
		led_para.target_bright[i] = 0;
	}
}

LOCAL void ESPFUNC user_led_turnmax_direct() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_para.current_bright[i] = BRIGHT_MAX;
		led_para.target_bright[i] = BRIGHT_MAX;
	}
}

LOCAL void ESPFUNC user_led_update_day_status() {
	if (led_config.all_bright > BRIGHT_MAX - BRIGHT_DELT) {
		led_para.day_rise = false;
	} else if (led_config.all_bright < BRIGHT_MIN + BRIGHT_DELT) {
		led_para.day_rise = true;
	}
}

LOCAL void ESPFUNC user_led_update_night_status() {
	if (led_config.blue_bright > BRIGHT_MAX - BRIGHT_DELT) {
		led_para.night_rise = false;
	} else if (led_config.blue_bright < BRIGHT_MIN + BRIGHT_DELT) {
		led_para.night_rise = true;
	}
}

LOCAL void ESPFUNC user_led_update_day_bright() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		led_para.current_bright[i] = led_config.all_bright;
		led_para.target_bright[i] = led_config.all_bright;
		led_config.bright[i] = led_config.all_bright;
	}
}

LOCAL void ESPFUNC user_led_update_night_bright() {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		if (i == NIGHT_CHANNEL) {
			led_para.current_bright[i] = led_config.blue_bright;
			led_para.target_bright[i] = led_config.blue_bright;
			led_config.bright[i] = led_config.blue_bright;
		} else {
			led_para.current_bright[i] = 0;
			led_para.target_bright[i] = 0;
			led_config.bright[i]  = 0;
		}
	}
}

LOCAL void ESPFUNC user_led_indicate_off() {
	ledr_on();
	ledg_off();
	ledb_off();
}

LOCAL void ESPFUNC user_led_indicate_day() {
	ledr_on();
	ledg_on();
	ledb_on();
}

LOCAL void ESPFUNC user_led_indicate_night() {
	ledr_off();
	ledg_off();
	ledb_on();
}

LOCAL void ESPFUNC user_led_indicate_wifi() {
	ledr_off();
	ledg_on();
	ledb_off();
}

LOCAL void ESPFUNC user_led_off_onShortPress() {
	led_config.state++;
	led_config.power = 1;
	user_led_indicate_day();
	user_led_update_day_bright();
}

LOCAL void ESPFUNC user_led_off_onLongPress() {
}

LOCAL void ESPFUNC user_led_off_onContPress() {
}

LOCAL void ESPFUNC user_led_off_onRelease() {
}

LOCAL void ESPFUNC user_led_day_onShortPress() {
	led_config.state++;
	led_config.power = 1;
	user_led_indicate_night();
	user_led_update_night_bright();
}

LOCAL void ESPFUNC user_led_day_onLongPress() {
	user_led_update_day_status();
}

LOCAL void ESPFUNC user_led_day_onContPress() {
	if (led_para.day_rise) {
		if (led_config.all_bright + BRIGHT_DELT_TOUCH <= BRIGHT_MAX) {
			led_config.all_bright += BRIGHT_DELT_TOUCH;
		} else {
			led_config.all_bright = BRIGHT_MAX;
		}
	} else {
		if (led_config.all_bright >= BRIGHT_MIN2 + BRIGHT_DELT_TOUCH) {
			led_config.all_bright -= BRIGHT_DELT_TOUCH;
		} else {
			led_config.all_bright = BRIGHT_MIN2;
		}
	}
	user_led_update_day_bright();
}

LOCAL void ESPFUNC user_led_day_onRelease() {
	user_led_save_config();
	user_device_update_dpall();
}

LOCAL void ESPFUNC user_led_night_onShortPress() {
	led_config.state++;
	if (led_config.last_mode == MODE_PRO) {
		led_config.mode = MODE_PRO;
	} else {
		led_config.mode = MODE_AUTO;
	}
	pkeys[0]->long_count = KEY_LONG_PRESS_COUNT;
	user_led_indicate_wifi();
}

LOCAL void ESPFUNC user_led_night_onLongPress() {
	user_led_update_night_status();
}

LOCAL void ESPFUNC user_led_night_onContPress() {
	if (led_para.night_rise) {
		if (led_config.blue_bright + BRIGHT_DELT_TOUCH <= BRIGHT_MAX) {
			led_config.blue_bright += BRIGHT_DELT_TOUCH;
		} else {
			led_config.blue_bright = BRIGHT_MAX;
		}
	} else {
		if (led_config.blue_bright >= BRIGHT_MIN2 + BRIGHT_DELT_TOUCH) {
			led_config.blue_bright -= BRIGHT_DELT_TOUCH;
		} else {
			led_config.blue_bright = BRIGHT_MIN2;
		}
	}
	user_led_update_night_bright();
}

LOCAL void ESPFUNC user_led_night_onRelease() {
	user_led_save_config();
	user_device_update_dpall();
}

LOCAL void ESPFUNC user_led_wifi_onShortPress() {
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	led_config.state++;
	led_config.mode = MODE_MANUAL;
	led_config.power = 0;
	pkeys[0]->long_count = USER_KEY_LONG_TIME_NORMAL;
	user_led_indicate_off();
	user_led_turnoff_direct();
}

LOCAL void ESPFUNC user_led_wifi_onLongPress() {
	if (app_test_status()) {					//测试模式
		return;
	}
	if (user_smartconfig_instance_status()) {
		user_smartconfig_instance_stop();
		user_apconfig_instance_start(&apc_impl, APCONFIG_TIMEOUT, user_dev_led.apssid, user_led_setzone);
	} else if (user_apconfig_instance_status()) {
		return;
	} else {
		user_smartconfig_instance_start(&sc_impl, SMARTCONFIG_TIEMOUT);
	}
}

LOCAL void ESPFUNC user_led_wifi_onContPress() {
}

LOCAL void ESPFUNC user_led_wifi_onRelease() {
}

LOCAL void ESPFUNC user_led_onShortPress() {
	short_press_func[led_config.state]();
	user_led_save_config();
	user_device_update_dpall();
}

LOCAL void ESPFUNC user_led_onLongPress() {
	long_press_func[led_config.state]();
}

LOCAL void ESPFUNC user_led_onContPress() {
	cont_press_func[led_config.state]();
}

LOCAL void ESPFUNC user_led_onRelease() {
	release_func[led_config.state]();
}

LOCAL void ESPFUNC user_led_ramp(void *arg) {
	uint8_t i;
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		if (led_para.current_bright[i] + BRIGHT_STEP_NORMAL < led_para.target_bright[i]) {
			led_para.current_bright[i] += BRIGHT_STEP_NORMAL;
		} else if (led_para.current_bright[i] > led_para.target_bright[i] + BRIGHT_STEP_NORMAL) {
			led_para.current_bright[i] -= BRIGHT_STEP_NORMAL;
		} else {
			led_para.current_bright[i] = led_para.target_bright[i];
		}
		user_led_load_duty(led_para.current_bright[i], i);
	}
	pwm_start();
}

LOCAL void ESPFUNC user_led_flash_cb(void *arg) {
	LOCAL uint8_t count = 0;
	count++;
	if (count&0x01) {
		user_led_turnmax_direct();
	} else {
		user_led_turnoff_direct();
	}
	if (count >= 6) {
		count = 0;
		find_flag = false;
		os_timer_disarm(&find_timer);

		switch (led_config.state) {
			case LED_STATE_OFF:
				user_led_indicate_off();
				user_led_turnoff_ramp();
				break;
			case LED_STATE_DAY:
				user_led_indicate_day();
				user_led_update_day_bright();
				break;
			case LED_STATE_NIGHT:
				user_led_indicate_night();
				user_led_update_night_bright();
				break;
			case LED_STATE_WIFI:
				user_led_indicate_wifi();
				if (led_config.mode == MODE_MANUAL) {
					if (led_config.power) {
						user_led_turnon_ramp();
					} else {
						user_led_turnoff_ramp();
					}
				}
				break;
			default:
				break;
		}
	}
}

LOCAL void ESPFUNC user_led_flash() {
	if (find_flag) {
		return;
	}
	find_flag = true;
	os_timer_disarm(&find_timer);
	os_timer_setfn(&find_timer, user_led_flash_cb, NULL);
	os_timer_arm(&find_timer, FLASH_PERIOD, 1);
}

LOCAL void ESPFUNC user_led_prev_stop() {
	prev_ct = 0;
	prev_flag = false;
	os_timer_disarm(&prev_timer);
}

LOCAL void ESPFUNC user_led_prev_start() {
	prev_ct = 0;
	prev_flag = true;
	os_timer_disarm(&prev_timer);
	os_timer_setfn(&prev_timer, user_led_auto_pro_prev, NULL);
	os_timer_arm(&prev_timer, PREVIEW_INTERVAL, 1);
}

LOCAL bool ESPFUNC user_led_update_gis() {
	LOCAL bool valid;
	LOCAL uint16_t sunrise;
	LOCAL uint16_t sunset;
	uint16_t year = led_para.super.year;
	uint8_t month = led_para.super.month;
	uint8_t day = led_para.super.day;
	valid = get_sunrise_sunset(	led_config.super.longitude,
								led_config.super.latitude,
								year,
								month,
								day,
								led_config.super.zone,
								&sunrise,
								&sunset);
	if (valid) {
		valid = led_config.gis_enable;
		if (led_para.gis_valid != valid || led_para.gis_sunrise != sunrise || led_para.gis_sunset != sunset) {
			led_para.gis_valid = valid;
			led_para.gis_sunrise = sunrise;
			led_para.gis_sunset = sunset;
			return true;
		}
	} else if (led_para.gis_valid) {
		led_para.gis_valid = false;
		return true;
	}
	return false;
}

LOCAL void ESPFUNC user_led_auto_proccess(uint16_t ct, uint8_t sec) {
	if (ct > 1439 || sec > 59) {
		return;
	}
	if (led_config.mode != MODE_AUTO || find_flag || led_config.state != LED_STATE_WIFI) {
		return;
	}
	uint8_t i, j, k;
	uint8_t count = 4;
	uint16_t st, et, duration;
	uint32_t dt;
	uint8_t dbrt;
	
	bool update = user_led_update_gis();
	uint16_t sunrise_start;
	uint16_t sunrise_end;
	uint16_t sunset_start;
	uint16_t sunset_end;
	uint16_t sunrise_ramp;
	uint16_t sunset_ramp;
	if(led_para.gis_valid) {
		sunrise_start = led_para.gis_sunrise;
		sunset_end = led_para.gis_sunset;
		sunrise_ramp = ((1440+sunset_end-sunrise_start)%1440)/10;
		sunset_ramp = sunrise_ramp;
	} else {
		sunrise_start = led_config.sunrise;
		sunset_end = led_config.sunset;
		sunrise_ramp = led_config.sunrise_ramp;
		sunset_ramp = led_config.sunset_ramp;
	}
	sunrise_end = (sunrise_start + sunrise_ramp)%1440;
	sunset_start = (1440 + sunset_end - sunset_ramp)%1440;

	uint16_t tm[6] = { 	sunrise_start,
						sunrise_end,
						sunset_start,
						sunset_end,
						led_config.turnoff_time,
						led_config.turnoff_time};
	uint8_t brt[6][LED_CHANNEL_COUNT];
	for (i = 0; i < LED_CHANNEL_COUNT; i++) {
		if (led_config.turnoff_enabled) {
			brt[0][i] = 0;
		} else {
			brt[0][i] = led_config.night_bright[i];
		}
		brt[1][i] = led_config.day_bright[i];
		brt[2][i] = led_config.day_bright[i];
		brt[3][i] = led_config.night_bright[i];
		brt[4][i] = led_config.night_bright[i];
		brt[5][i] = 0;
	}
	if (led_config.turnoff_enabled) {
		count = 6;
	}
	for (i = 0; i < count; i++) {
		j = (i + 1) % count;
		st = tm[i];
		et = tm[j];
		if (et >= st) {
			if (ct >= st && ct < et) {
				duration = et - st;
				dt = (ct - st) * 60u + sec;
			} else {
				continue;
			}
		} else {
			if (ct >= st || ct < et) {
				duration = 1440u - st + et;
				if (ct >= st) {
					dt = (ct - st) * 60u + sec;
				} else {
					dt = (1440u - st + ct) * 60u + sec;
				}
			} else {
				continue;
			}
		}
		for (k = 0; k < LED_CHANNEL_COUNT; k++) {
			if (brt[j][k] >= brt[i][k]) {
				dbrt = brt[j][k] - brt[i][k];
				led_para.target_bright[k] = brt[i][k] * 10u + dbrt * dt / (duration * 6u);
			} else {
				dbrt = brt[i][k] - brt[j][k];
				led_para.target_bright[k] = brt[i][k] * 10u - dbrt * dt / (duration * 6u);
			}
		}
	}
	if (update) {
		user_device_update_dpall();
	}
}

LOCAL void ESPFUNC user_led_pro_process(uint16_t ct, uint8_t sec) {
	if (ct > 1439 || sec > 59) {
		return;
	}
	if (led_config.mode != MODE_PRO || find_flag || led_config.state != LED_STATE_WIFI) {
		return;
	}
	profile_t *profile = &led_config.profile0;
	if (led_config.select_profile > 0 && led_config.select_profile <= PROFILE_COUNT_MAX) {
		profile = &led_config.profiles[led_config.select_profile-1];
		if(user_led_check_profile(profile) == false) {
			led_config.select_profile = 0;
			profile = &led_config.profile0;
		}
	} else if (led_config.select_profile == PROFILE_COUNT_MAX + 1) {
		uint8_t month = led_para.super.month;
		if (month >= 1 && month <= 12) {
			profile = &led_config.profiles[month-1];
		}
		if(user_led_check_profile(profile) == false) {
			led_config.select_profile = 0;
			profile = &led_config.profile0;
		}
	}

	if (profile->count < POINT_COUNT_MIN) {
		profile->count = POINT_COUNT_MIN;
	} else if (profile->count > POINT_COUNT_MAX) {
		profile->count = POINT_COUNT_MAX;
	}
	int i, j;
	int index[POINT_COUNT_MAX];
	for (i = 0; i < POINT_COUNT_MAX; i++) {
		index[i] = i;
	}
	for (i = profile->count - 1;  i > 0; i--) {
		for (j = 0; j < i; j++) {
			if (profile->timers[index[j]] > profile->timers[index[j+1]]) {
				int tmp = index[j];
				index[j] = index[j+1];
				index[j+1] = tmp;
			}
		}
	}
	int ts = profile->timers[index[0]];
	int te = profile->timers[index[profile->count-1]];
	int duration = 1440 - te + ts;
	int start = index[profile->count-1];
	int end = index[0];
	int dt;
	bool flag = false;	
	if (ct >= te) {
		dt = (ct - te)*60u + sec;
		flag = true;
	} else if (ct < ts) {
		dt = (1440 - te + ct)*60u + sec;
		flag = true;
	} else {
		for (i = 0; i < profile->count - 1; i++) {
			start = index[i];
			end = index[i+1];
			if (ct >= profile->timers[start] && ct < profile->timers[end]) {
				duration = profile->timers[end] - profile->timers[start];
				dt = (ct - profile->timers[start])*60u + sec;
				flag = true;
				break;
			}
		}
	}
	if (flag) {
		for (i = 0; i < LED_CHANNEL_COUNT; i++) {
			int dbrt;
			if (profile->brights[start*LED_CHANNEL_COUNT+i] < profile->brights[end*LED_CHANNEL_COUNT+i]) {
				dbrt = profile->brights[end*LED_CHANNEL_COUNT+i] - profile->brights[start*LED_CHANNEL_COUNT+i];
				led_para.target_bright[i] = profile->brights[start*LED_CHANNEL_COUNT+i] * 10u + dbrt * dt / (duration * 6u);
			} else {
				dbrt = profile->brights[start*LED_CHANNEL_COUNT+i] - profile->brights[end*LED_CHANNEL_COUNT+i];
				led_para.target_bright[i] = profile->brights[start*LED_CHANNEL_COUNT+i] * 10u - dbrt * dt / (duration * 6u);
			}
		}
	}
}

LOCAL void ESPFUNC user_led_process(void *arg) {
	if (user_rtc_is_synchronized() == false) {
		return;
	}
	if (prev_flag) {
		return;
	}
	uint16_t ct = led_para.super.hour * 60u + led_para.super.minute;
	uint8_t sec = led_para.super.second;
	if (led_config.mode == MODE_AUTO) {
		user_led_auto_proccess(ct, sec);
	} else if (led_config.mode == MODE_PRO) {
		user_led_pro_process(ct, sec);
	}
}

LOCAL void ESPFUNC user_led_auto_pro_prev(void *arg) {
	if (!prev_flag) {
		user_led_prev_stop();
		return;
	}
	prev_ct++;
	if (prev_ct > 1439) {
		user_led_prev_stop();
		prev_flag_shadow = false;
		user_device_update_dpall();
		return;
	}
	if (led_config.mode == MODE_AUTO) {
		user_led_auto_proccess(prev_ct, 0);
	} else if (led_config.mode == MODE_PRO) {
		user_led_pro_process(prev_ct, 0);
	}
}

void ESPFUNC user_led_datapoint_changed_cb() {
	led_config.state = LED_STATE_WIFI;
	pkeys[0]->long_count = KEY_LONG_PRESS_COUNT;
	user_led_indicate_wifi();
	if (find_flag) {
		return;
	}
	if (prev_flag_shadow && !prev_flag) {
		user_led_prev_start();
	} else if (!prev_flag_shadow && prev_flag) {
		user_led_prev_stop();
	}
	if (led_config.mode == MODE_AUTO) {
		led_config.last_mode = MODE_AUTO;
	} else if (led_config.mode == MODE_PRO) {
		led_config.last_mode = MODE_PRO;
	} else {
		led_config.mode = MODE_MANUAL;
		if (led_config.power == false) {
			user_led_turnoff_ramp();
		} else {
			user_led_turnon_ramp();
		}
	}
	user_device_update_dpchanged();
	user_led_save_config();
}
