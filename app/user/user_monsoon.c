#include "user_monsoon.h"
#include "app_board_monsoon.h"
#include "user_key.h"
#include "driver/hw_timer.h"
#include "xlink.h"
#include "xlink_datapoint.h"
#include "user_rtc.h"
#include "user_smartconfig.h"
#include "user_apconfig.h"
#include "app_test.h"
#include "user_task.h"
#include "user_indicator.h"
#include "app_util.h"

#define	PRODUCT_TYPE				"EXOTerraMonsoon"
#define	MONSOON_PRODUCT_ID			"160fa2b88b1903e9160fa2b88b19a401"
#define	MONSOON_PRODUCT_KEY			"99f737092e5f01b7f01fbcaab1f47966"

#define	MONSOON_PROPERTY			"M1"

#define	STATUS_INDEX				10
#define	KEYACTION_INDEX				11
#define	POWER_INDEX					12
#define	POWERON_TMR_INDEX			13
#define	CUSTOM_ACTIONS_INDEX		14
#define	TIMER1_INDEX				15

#define	USER_KEY_NUM				1
#define	USER_KEY_LONG_PRESS_CNT		200

/* PWM 50Hz */
#define	CTRL_SWON_PERIOD			8		//ms
#define	CTRL_SWOFF_PERIOD			12
#define	CTRL_HWON_PERIOD			8000	//us
#define	CTRL_HWOFF_PERIOD			12000	//us

//Rx=(R*10)/(R+10)
//(3.3*1024*Rx)/(24+Rx) -> (33*1024*R)/(34*R+240)
//悬空  
//-10℃	43K		(8.11K)		3.3*1024*8.11/(R+8.11)	- R24K->853
//25℃ 	10K   	(5K)		3.3*1024*5/(R+5) 		- R24K->582
//92℃ 	1.2K	(1.07K)		3.3*1024*1.07/(R+1.07) 	- R24K->144 
//64℃ 	2.7K	(2.13K)		3.3*1024*2.13/(R+2.13) 	- R24K->275	
#define	NTC_MAX						854
#define	NTC_MIN						150
#define	NTC_RESTORE					280
#define	NTC_DELAY_MASK				0x07

#define	ERROR_NONE					0
#define	ERROR_NTC_NOT_CONNECTED		1
#define	ERROR_NTC_TEMPERATURE_OVER	2

//1-120:second 121-123:3-5min other:0
#define	TURNON_MAX_PERIOD			300
#define	TURNON_DEFAULT_PERIOD		5

typedef enum {
	TIMER_DISABLED,
	TIMER_ENABLED,
	TIMER_INVALID
} timer_error_t;

LOCAL void user_monsoon_swon(void *arg);
LOCAL void user_monsoon_swoff(void *arg);
LOCAL void user_monsoon_hwon();
LOCAL void user_monsoon_hwoff();
LOCAL void user_monsoon_open();
LOCAL void user_monsoon_close();
LOCAL void user_monsoon_process(void *arg);
LOCAL void user_monsoon_datapoint_init();
LOCAL uint16_t user_monsoon_get_poweron_period(uint8_t duration);

LOCAL void user_monsoon_default_config();
LOCAL void user_monsoon_save_config();
LOCAL void user_monsoon_para_init();
LOCAL void user_monsoon_key_init();
LOCAL void user_monsoon_datapoint_init();
LOCAL void user_monsoon_init();
LOCAL void user_monsoon_process(void *arg);
LOCAL void user_monsoon_datapoint_changed_cb();

LOCAL void user_monsoon_pre_smartconfig() ;
LOCAL void user_monsoon_post_smartconfig();
LOCAL void user_monsoon_pre_apconfig();
LOCAL void user_monsoon_post_apconfig();

LOCAL monsoon_config_t monsoon_config;
LOCAL monsoon_para_t monsoon_para = {
	.ptimers = monsoon_config.timers
};
LOCAL const task_impl_t apc_impl = newTaskImpl(user_monsoon_pre_apconfig, user_monsoon_post_apconfig);
LOCAL const task_impl_t sc_impl = newTaskImpl(user_monsoon_pre_smartconfig, user_monsoon_post_smartconfig);
user_device_t user_dev_monsoon = {
	.product = PRODUCT_TYPE,
	.product_id = MONSOON_PRODUCT_ID,
	.product_key = MONSOON_PRODUCT_KEY,
	.fw_version = FW_VERSION,

	.key_io_num = KEY_IO_NUM,
	.test_led1_num = LEDR_IO_NUM,
	.test_led2_num = LEDG_IO_NUM,

	.board_init = app_board_monsoon_init,
	.default_config = user_monsoon_default_config,
	.save_config = user_monsoon_save_config,
	.para_init = user_monsoon_para_init,
	.key_init = user_monsoon_key_init,
	.datapoint_init = user_monsoon_datapoint_init,
	.init = user_monsoon_init,
	.process = user_monsoon_process,
	.dp_changed_cb = user_monsoon_datapoint_changed_cb,
	.para = &monsoon_para.super,
	.pconfig = &monsoon_config.super
};

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;
LOCAL os_timer_t monsoon_ctrl_timer;
LOCAL os_timer_t monsoon_power_timer;

LOCAL void ESPFUNC user_monsoon_ledg_toggle() {
	ledg_toggle();
}

LOCAL void ESPFUNC user_monsoon_ledr_toggle() {
	ledr_toggle();
}

LOCAL void ESPFUNC user_monsoon_setzone(int16_t zone) {
	if (app_util_zone_isvalid(zone)) {
		monsoon_config.super.zone = zone;
		user_monsoon_save_config();
		user_device_enable_snsubscribe(&user_dev_monsoon);
	}
}

LOCAL void ESPFUNC user_monsoon_pre_smartconfig() {
	xlink_tcp_disconnect();
	xlink_enable_subscription();
	monsoon_para.status = 0;
	user_indicator_start(SMARTCONFIG_FLASH_PERIOD, 0, user_monsoon_ledg_toggle);
	ledr_off();
}

LOCAL void ESPFUNC user_monsoon_post_smartconfig() {
	user_indicator_stop();
	ledr_on();
	ledg_off();
}

LOCAL void ESPFUNC user_monsoon_pre_apconfig() {
	xlink_tcp_disconnect();
	wifi_set_opmode_current(SOFTAP_MODE);
	monsoon_para.status = 0;
	user_indicator_start(APCONFIG_FLASH_PERIOD, 0, user_monsoon_ledg_toggle);
	ledr_off();
}

LOCAL void ESPFUNC user_monsoon_post_apconfig() {
	user_indicator_stop();
	ledr_on();
	ledg_off();
}

LOCAL timer_error_t ESPFUNC user_monsoon_check_timer(monsoon_timer_t *ptmr) {
	if (ptmr == NULL || ptmr->timer > 1439 || ptmr->duration == 0 || ptmr->duration > 123) {
		return TIMER_INVALID;
	}
	if (ptmr->enable) {
		return TIMER_ENABLED;
	}
	return TIMER_DISABLED;
}

LOCAL void ESPFUNC user_monsoon_alerm1() {
	user_indicator_start(1500, 0, user_monsoon_ledr_toggle);
}

LOCAL void ESPFUNC user_monsoon_alerm2() {
	user_indicator_start(500, 0, user_monsoon_ledr_toggle);
}

LOCAL void ESPFUNC user_monsoon_get_status() {
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	LOCAL uint8_t flag1 = 0;
	LOCAL uint8_t flag2 = 0;
	LOCAL uint8_t flag3 = 0;
	bool flag = false;
	uint16_t value = system_adc_read();
	uint16_t min = value;
	uint16_t max = value;
	uint8_t i = 0;
	uint16_t sum = value;
	for(i = 0; i < 9; i++) {
		value = system_adc_read();
		if(min > value) {
			min = value;
		}
		if(max < value) {
			max = value;
		}
		sum += value;
	}
	value = (sum-min-max)/8;

	flag1 <<= 1;
	flag2 <<= 1;
	flag3 <<= 1;
	if(value > NTC_MAX) {
		flag1 |= 0x01;
	} 
	if(value < NTC_MIN) {
		flag2 |= 0x01;
	}
	if(value > NTC_RESTORE) {
		flag3 |= 0x01;
	}
	// app_logd("NTC adc value is %d, status is: %d    %d    %d    %d\n", value, flag1, flag2, flag3, user_monsoon.status);
	switch(monsoon_para.status) {
		case 0:
			if((flag1&NTC_DELAY_MASK) == NTC_DELAY_MASK) {
				monsoon_para.status = 1;
				user_monsoon_alerm1();
				flag = true;
			} else if((flag2&NTC_DELAY_MASK) == NTC_DELAY_MASK) {
				monsoon_para.status = 2;
				user_monsoon_alerm2();
				user_monsoon_close();
				flag = true;
			}
			break;
		case 1:
			if((flag1&NTC_DELAY_MASK) == 0x00) {
				if((flag2&NTC_DELAY_MASK) == NTC_DELAY_MASK) {
					monsoon_para.status = 2;
					user_monsoon_alerm2();
					user_monsoon_close();
					flag = true;
				} else {
					monsoon_para.status = 0;
					user_indicator_stop();
					ledr_on();
					flag = true;
				}
			}
			break;
		case 2:
			if((flag2&NTC_DELAY_MASK) == 0x00) {
				if((flag1&NTC_DELAY_MASK) == NTC_DELAY_MASK) {
					monsoon_para.status = 1;
					user_monsoon_alerm1();
					flag = true;
				} else if ((flag3&NTC_DELAY_MASK) == NTC_DELAY_MASK) {
					monsoon_para.status = 0;
					user_indicator_stop();
					ledr_on();
					flag = true;
				}
			}
			break;
		default:
			monsoon_para.status = 0;
			user_indicator_stop();
			ledr_on();
			flag = true;
			break;
	}
	if(flag) {
		user_device_update_dpall();
	}
}

LOCAL void ESPFUNC user_monsoon_default_config() {
	os_memset((uint8_t *)&monsoon_config, 0, sizeof(monsoon_config));
	monsoon_config.key_action = TURNON_DEFAULT_PERIOD;
	monsoon_config.super.saved_flag = CONFIG_DEFAULT_FLAG;
}

LOCAL void ESPFUNC user_monsoon_save_config() {
	//测试模式下不改变保存的参数
	if (app_test_status()) {
		return;
	}
	monsoon_config.super.saved_flag = CONFIG_SAVED_FLAG;
	xlink_write_user_para((uint8_t *)&monsoon_config, sizeof(monsoon_config));
}

LOCAL void ESPFUNC user_monsoon_para_init() {
	xlink_read_user_para((uint8_t *)&monsoon_config, sizeof(monsoon_config));
	if (monsoon_config.super.saved_flag != CONFIG_SAVED_FLAG) {
		user_monsoon_default_config();
	}
	if (monsoon_config.key_action == 0 || monsoon_config.key_action > 123) {
		monsoon_config.key_action = TURNON_DEFAULT_PERIOD;
	}
}

/**
 * key functions...
 */

LOCAL void ESPFUNC user_monsoon_key_short_press_cb() {
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	if(monsoon_para.status != 0) {
		return;
	}
	if (monsoon_para.power.onoff) {
		monsoon_para.power.value = 0;
		monsoon_para.power_shadow.value = 0;
		monsoon_para.poweron_tmr = 0;
		user_monsoon_close();
	} else {
		monsoon_para.power.onoff = 1;
		// user_monsoon.power.duration = 0;
		if(monsoon_config.key_action == 0 || monsoon_config.key_action > 123) {
			monsoon_config.key_action = TURNON_DEFAULT_PERIOD;
		}
		monsoon_para.power.duration = monsoon_config.key_action;
		monsoon_para.power_shadow.value = monsoon_para.power.value;
		monsoon_para.poweron_tmr = user_monsoon_get_poweron_period(monsoon_para.power_shadow.duration);
		user_monsoon_open();
	}
	user_device_update_dpall();
}

LOCAL void ESPFUNC user_monsoon_key_long_press_cb() {
	if (app_test_status()) {					//测试模式
		return;
	}
	if (user_smartconfig_instance_status()) {
		user_smartconfig_instance_stop();
		user_apconfig_instance_start(&apc_impl, APCONFIG_TIMEOUT, user_dev_monsoon.apssid, user_monsoon_setzone);
	} else if (user_apconfig_instance_status()) {
		return;
	} else {
		user_smartconfig_instance_start(&sc_impl, SMARTCONFIG_TIEMOUT);
	}
}

LOCAL void ESPFUNC user_monsoon_key_cont_press_cb() {
	
}

LOCAL void ESPFUNC user_monsoon_key_release_cb() {
	
}

LOCAL void ESPFUNC user_monsoon_key_init() {
	pkeys[0] = user_key_init_single(KEY_IO_NUM,
									KEY_IO_FUNC,
									KEY_IO_MUX,
									user_monsoon_key_short_press_cb,
									user_monsoon_key_long_press_cb,
									user_monsoon_key_cont_press_cb,
									user_monsoon_key_release_cb);
	pkeys[0]->long_count = KEY_LONG_PRESS_COUNT;
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list(&key_list);
}

/* key functions end */

LOCAL void ESPFUNC user_monsoon_datapoint_init() {
	uint8_t i;
	// xlink_datapoint_init_string(PROPERTY_INDEX, MONSOON_PROPERTY, os_strlen(MONSOON_PROPERTY));
	// xlink_datapoint_init_int16(ZONE_INDEX, &monsoon_config.super.zone);
	// xlink_datapoint_init_float(LONGITUDE_INDEX, &monsoon_config.super.longitude);
	// xlink_datapoint_init_float(LATITUDE_INDEX, &monsoon_config.super.latitude);
	// xlink_datapoint_init_string(DATETIME_INDEX, monsoon_para.super.datetime, os_strlen(monsoon_para.super.datetime));

	xlink_datapoint_init_byte(STATUS_INDEX, &monsoon_para.status);
	xlink_datapoint_init_byte(KEYACTION_INDEX, &monsoon_config.key_action);
	xlink_datapoint_init_byte(POWER_INDEX, &monsoon_para.power.value);
	xlink_datapoint_init_uint16(POWERON_TMR_INDEX, &monsoon_para.poweron_tmr);
	xlink_datapoint_init_binary(CUSTOM_ACTIONS_INDEX, &monsoon_config.custom_actions[0], sizeof(monsoon_config.custom_actions));
	for (i = 0; i < TIMER_COUNT_MAX; i++) {
		xlink_datapoint_init_uint32(TIMER1_INDEX+i, &monsoon_para.ptimers[i].value);
	}

	// xlink_datapoint_init_byte(SNSUB_ENABLE_INDEX, &monsoon_para.super.sn_subscribe_enable);
}

LOCAL void ESPFUNC user_monsoon_init() {
	ledr_on();
}

LOCAL void ESPFUNC user_monsoon_process(void *arg) {
	user_monsoon_get_status();
	if (user_rtc_is_synchronized() == false) {
		return;
	}
	if (monsoon_para.status != 0) {
		return;
	}
	bool flag = false;
	monsoon_timer_t *ptmr = monsoon_para.ptimers;
	uint8_t sec = monsoon_para.super.second;
	if (sec == 0) {
		uint8_t i;
		uint16_t ct = monsoon_para.super.hour * 60u + monsoon_para.super.minute;
		monsoon_power_t power;
		for (i = 0; i < TIMER_COUNT_MAX; i++) {
			if (user_monsoon_check_timer(ptmr) == TIMER_ENABLED && ct == ptmr->timer) {
				//once
				if (ptmr->repeat == 0) {
					ptmr->enable = false;
					monsoon_para.power.onoff = 1;
					monsoon_para.power.duration = ptmr->duration;
					monsoon_para.power_shadow.value = monsoon_para.power.value;
					monsoon_para.poweron_tmr = user_monsoon_get_poweron_period(monsoon_para.power_shadow.duration);
					user_monsoon_open();
					flag = true;
				} else if ((ptmr->repeat&(1<<monsoon_para.super.week)) != 0) {
					monsoon_para.power.onoff = 1;
					monsoon_para.power.duration = ptmr->duration;
					monsoon_para.power_shadow.value = monsoon_para.power.value;
					monsoon_para.poweron_tmr = user_monsoon_get_poweron_period(monsoon_para.power_shadow.duration);
					user_monsoon_open();
					flag = true;
				}
			}
			ptmr++;
		}
	}
	if (flag) {
		user_device_update_dpall();
	}
}

LOCAL void ESPFUNC user_monsoon_power_process(void *arg) {
	if (monsoon_para.poweron_tmr < 1) {
		monsoon_para.poweron_tmr = 0;
		monsoon_para.power_shadow.value = 0;
		monsoon_para.power.value = 0;
		user_monsoon_close();
	} else {
		monsoon_para.poweron_tmr--;
	}
	user_device_update_dpall();
}

LOCAL void ESPFUNC user_monsoon_datapoint_changed_cb() {
	if (monsoon_para.status == 0) {
		if (monsoon_para.power_shadow.value != monsoon_para.power.value) {
			monsoon_para.power_shadow.value = monsoon_para.power.value;
			monsoon_para.poweron_tmr = user_monsoon_get_poweron_period(monsoon_para.power_shadow.duration);
			if (monsoon_para.power_shadow.onoff == 0) {
				user_monsoon_close();
			} else {
				user_monsoon_open();
			}
		}
	} else {
		monsoon_para.power.value = 0;
		monsoon_para.power_shadow.value = 0;
		monsoon_para.poweron_tmr = 0;
		user_monsoon_close();
	}
	user_device_update_dpchanged();
	user_monsoon_save_config();
}

/**
 * firmware - v5
 * 1-120 	second
 * 121-123	3-5min
 * */
LOCAL uint16_t ESPFUNC user_monsoon_get_poweron_period(uint8_t duration) {
	if (duration <= 120) {
		return duration;
	}
	if (duration <= 123) {
		return (duration-118)*60;
	}
	return 0;
}
// LOCAL uint16_t ESPFUNC user_monsoon_get_poweron_period(uint8_t duration) {
// 	if (duration <= 120) {
// 		return duration;
// 	}
// 	if (duration == 121) {
// 		return 180;
// 	}
// 	if (duration == 122) {
// 		return 240;
// 	}
// 	if (duration == 123) {
// 		return 300;
// 	}
// 	if (duration == 124) {
// 		return 360;
// 	}
// 	if (duration == 125) {
// 		return 480;
// 	}
// 	if (duration == 126) {
// 		return 600;
// 	}
// 	if (duration == 127) {
// 		return 900;
// 	}
// 	return 0;
// }

LOCAL void ESPFUNC user_monsoon_swon(void *arg) {
	GPIO_OUTPUT_SET(CTRL_IO_NUM, 1);
	os_timer_disarm(&monsoon_ctrl_timer);
	os_timer_setfn(&monsoon_ctrl_timer, user_monsoon_swoff, NULL);
	os_timer_arm(&monsoon_ctrl_timer, CTRL_SWON_PERIOD, 0);
}

LOCAL void ESPFUNC user_monsoon_swoff(void *arg) {
	GPIO_OUTPUT_SET(CTRL_IO_NUM, 0);
	os_timer_disarm(&monsoon_ctrl_timer);
	os_timer_setfn(&monsoon_ctrl_timer, user_monsoon_swon, NULL);
	os_timer_arm(&monsoon_ctrl_timer, CTRL_SWOFF_PERIOD, 0);
}

LOCAL void user_monsoon_hwon() {
	hw_timer_disarm();
	hw_timer_set_func(user_monsoon_hwoff);
	hw_timer_init(FRC1_SOURCE, 0);
	hw_timer_arm(CTRL_HWON_PERIOD);
	GPIO_OUTPUT_SET(CTRL_IO_NUM, 1);
}

LOCAL void user_monsoon_hwoff() {
	hw_timer_disarm();
	hw_timer_set_func(user_monsoon_hwon);
	hw_timer_init(FRC1_SOURCE, 0);
	hw_timer_arm(CTRL_HWOFF_PERIOD);
	GPIO_OUTPUT_SET(CTRL_IO_NUM, 0);
}

LOCAL void ESPFUNC user_monsoon_on() {
	// AC Pump sw_timer
	// user_monsoon_swon(NULL);

	// AC Pump hw_timer
	user_monsoon_hwon();

	// DC Pump
	// GPIO_OUTPUT_SET(CTRL_IO_NUM, 1);
}

LOCAL void ESPFUNC user_monsoon_off() {
	// AC Pump use sw_timer
	// os_timer_disarm(&monsoon_ctrl_timer);
	// GPIO_OUTPUT_SET(CTRL_IO_NUM, 0);

	// AC Pump user hw_timer
	hw_timer_disarm();
	GPIO_OUTPUT_SET(CTRL_IO_NUM, 0);

	// DC Pump
	// GPIO_OUTPUT_SET(CTRL_IO_NUM, 0);
}

LOCAL void ESPFUNC user_monsoon_open() {
	if(monsoon_para.status != 0) {
		return;
	}
	os_timer_disarm(&monsoon_power_timer);

	if(monsoon_para.power_shadow.duration > 0) {
		user_monsoon_on();
		os_timer_setfn(&monsoon_power_timer, user_monsoon_power_process, NULL);
		os_timer_arm(&monsoon_power_timer, 1000, 1);
	}
}

LOCAL void ESPFUNC user_monsoon_close() {
	os_timer_disarm(&monsoon_power_timer);

	user_monsoon_off();
}

