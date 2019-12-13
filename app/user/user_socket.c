#include "user_socket.h"
#include "app_board_socket.h"
#include "user_key.h"
#include "xlink.h"
#include "xlink_datapoint.h"
#include "user_rtc.h"
#include "user_uart.h"
#include "user_apconfig.h"
#include "user_smartconfig.h"
#include "app_test.h"
#include "user_task.h"
#include "app_util.h"

#define	PRODUCT_TYPE				"EXOTerraSocket"
#define SOCKET_PRODUCT_ID			"160fa2b95aac03e9160fa2b95aac5e01"
#define	SOCKET_PRODUCT_KEY			"dae24f80da82b6f1d55d06781e8b94e6"

#define	INDEX_SWITCH_COUNT_MAX		10
#define	INDEX_SWITCH_COUNT			12
#define	INDEX_POWER					13
#define	INDEX_MODE					14
#define	INDEX_TIMER1				15

#define	INDEX_S1_AVAILABLE			40
#define	INDEX_S1_TYPE				41
#define	INDEX_S1_VALUE				42
#define	INDEX_SV1_TYPE				43
#define	INDEX_SV1_NOTIFY_EN			44

#define	INDEX_S2_AVAILABLE			50
#define	INDEX_S2_TYPE				51
#define	INDEX_S2_VALUE				52
#define	INDEX_SV2_TYPE				53
#define	INDEX_SV2_NOTIFY_EN			54

#define	INDEX_SV1_DAY_THRESHOLD		56
#define	INDEX_SV1_NIGHT_THRESHOLD	57

#define	INDEX_SV2_DAY_THRESHOLD		58
#define	INDEX_SV2_NIGHT_THRESHOLD	59

#define	INDEX_SV1_THRD_LOWER		60
#define	INDEX_SV1_THRD_UPPER		61
#define	INDEX_S1_LOSS_FLAG			62
#define	INDEX_S1_OVER_FLAG			63

#define	INDEX_SV2_THRD_LOWER		64
#define	INDEX_SV2_THRD_UPPER		65
#define	INDEX_S2_LOSS_FLAG			66
#define	INDEX_S2_OVER_FLAG			67

//报警模板
#define	ALARM_TMP_FORMAT			"{\
\"xn\":{\
\"typ\":\"%s\",\
\"val\":\"%d.%d %s\"\
}\
}"

#define	SOCKET_PROPERTY				"S1"

#define	USER_KEY_NUM					1
#define USER_KEY_LONG_TIME_SMARTCONFIG	200

#define	SWITCH_SAVED_FLAG				0x12345678

#define FRAME_HEADER	0x68
#define CMD_SET			0x00
#define CMD_GET			0x01

#define	NOTIFY_MASK		0x80

#define	LINKAGE_LOCK_PERIOD		30000		//lock linkage after manual turnon/turnoff
#define	LINKAGE_LOCKON_PERIOD	900000		//lock socket for 15min after turnon

typedef enum timer_error{ TIMER_DISABLED, TIMER_ENABLED, TIMER_INVALID } timer_error_t;

LOCAL void user_socket_detect_sensor(void *arg);
LOCAL void user_socket_decode_sensor(uint8_t *pbuf, uint8_t len);
LOCAL bool user_socket_linkage_process();
LOCAL void reptile_temperature_linkage_process();

LOCAL void user_socket_default_config();
LOCAL void user_socket_save_config();
LOCAL void user_socket_para_init();
LOCAL void user_socket_key_init();
LOCAL void user_socket_datapoint_init();
LOCAL void user_socket_init();
LOCAL void user_socket_process(void *arg);
LOCAL void user_socket_datapoint_changed_cb();

LOCAL void user_socket_pre_smartconfig() ;
LOCAL void user_socket_post_smartconfig();
LOCAL void user_socket_pre_apconfig();
LOCAL void user_socket_post_apconfig();

LOCAL socket_config_t socket_config;
LOCAL socket_para_t socket_para = {
	.switch_count_max = SWITCH_COUNT_MAX,
	.p_sensor_args = &socket_config.sensor_args,
	.p_timer = socket_config.socket_timer
};
LOCAL const task_impl_t apc_impl = newTaskImpl(user_socket_pre_apconfig, user_socket_post_apconfig);
LOCAL const task_impl_t sc_impl = newTaskImpl(user_socket_pre_smartconfig, user_socket_post_smartconfig);

user_device_t user_dev_socket = {
	.product = PRODUCT_TYPE,
	.product_id = SOCKET_PRODUCT_ID,
	.product_key = SOCKET_PRODUCT_KEY,
	.fw_version = FW_VERSION,

	.key_io_num = KEY_IO_NUM,
	.test_led1_num = LEDR_IO_NUM,
	.test_led2_num = LEDG_IO_NUM,

	.board_init = app_board_socket_init,
	.default_config = user_socket_default_config,
	.save_config = user_socket_save_config,
	.para_init = user_socket_para_init,
	.key_init = user_socket_key_init,
	.datapoint_init = user_socket_datapoint_init,
	.init = user_socket_init,
	.process = user_socket_process,
	.dp_changed_cb = user_socket_datapoint_changed_cb,
	.para = &socket_para.super,
	.pconfig = &socket_config.super
};

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;
LOCAL bool m_sensor_detected;
LOCAL os_timer_t socket_proc_tmr;
LOCAL os_timer_t sensor_detect_tmr;
LOCAL os_timer_t linkage_lockon_tmr;
LOCAL os_timer_t linkage_lock_tmr;
LOCAL volatile bool mLock;				//手动开关后锁定联动标志
LOCAL volatile bool mLockon;			//联动打开后锁定标志

LOCAL char s1_loss_alerm[DATAPOINT_STR_MAX_LEN];
LOCAL char s1_over_alerm[DATAPOINT_STR_MAX_LEN];
LOCAL char s2_loss_alerm[DATAPOINT_STR_MAX_LEN];
LOCAL char s2_over_alerm[DATAPOINT_STR_MAX_LEN];

LOCAL void ESPFUNC user_socket_linkage_lockon_action(void *arg) {
	mLockon = false;
	app_logd("unlockon");
}

LOCAL void ESPFUNC user_socket_linkage_lockon_start() {
	os_timer_disarm(&linkage_lockon_tmr);
	os_timer_setfn(&linkage_lockon_tmr, user_socket_linkage_lockon_action, NULL);
	mLockon = true;
	os_timer_arm(&linkage_lockon_tmr, LINKAGE_LOCKON_PERIOD, 0);
	app_logd("lockon");
}

LOCAL void ESPFUNC user_socket_linkage_lock_action(void *arg) {
	mLock = false;
	app_logd("unlock");
}

LOCAL void ESPFUNC user_socket_linkage_lock_start() {
	os_timer_disarm(&linkage_lock_tmr);
	os_timer_setfn(&linkage_lock_tmr, user_socket_linkage_lock_action, NULL);
	mLock = true;
	os_timer_arm(&linkage_lock_tmr, LINKAGE_LOCK_PERIOD, 0);
	app_logd("lock");
}

/**
 * off->on return true
 * on->on return false
 * */
LOCAL bool ESPFUNC user_socket_turnon() {
	app_logd("power: %d relay: %d", socket_para.power, relay_status());
	if (relay_status() == false) {
		relay_on();
		ledr_off();
		ledb_on();
		socket_config.switch_flag = SWITCH_SAVED_FLAG;
		socket_config.switch_count++;
		xlink_datapoint_set_changed(INDEX_SWITCH_COUNT);
		return true;
	}
	return false;
}

/**
 * on->off return true
 * off->off return false
 * */
LOCAL bool ESPFUNC user_socket_turnoff() {
	app_logd("power: %d relay: %d", socket_para.power, relay_status());
	if (relay_status()) {
		relay_off();
		ledr_on();
		ledb_off();
		return true;
	}
	return false;
}

LOCAL bool ESPFUNC user_socket_turnon_manual() {
	if (user_socket_turnon()) {		//手动开,锁定联动动作
		user_socket_linkage_lock_start();
		return true;
	}
	return false;
}

LOCAL bool ESPFUNC user_socket_turnon_linkage() {
	if (user_socket_turnon()) {		//联动开,联动锁定打开周期
		user_socket_linkage_lockon_start();
		return true;
	}
	return false;
}

LOCAL bool ESPFUNC user_socket_turnoff_manual() {
	if (user_socket_turnoff()) {	//手动关,锁定联动动作
		user_socket_linkage_lock_start();
		return true;
	}
	return false;
}

LOCAL bool ESPFUNC user_socket_turnoff_linkage() {
	return user_socket_turnoff();
}

LOCAL void ESPFUNC user_socket_ledg_toggle() {
	ledg_toggle();
}

LOCAL void ESPFUNC user_socket_setzone(int16_t zone) {
	if (app_util_zone_isvalid(zone)) {
		socket_config.super.zone = zone;
		user_socket_save_config();
		user_device_enable_snsubscribe(&user_dev_socket);
	}
}

LOCAL void ESPFUNC user_socket_pre_smartconfig() {
	xlink_tcp_disconnect();
	ledr_off();
	ledb_off();
	xlink_enable_subscription();
	user_indicator_start(SMARTCONFIG_FLASH_PERIOD, 0, user_socket_ledg_toggle);
}

LOCAL void ESPFUNC user_socket_post_smartconfig() {
	user_indicator_stop();
	ledg_off();
	if (relay_status()) {
		ledb_on();
	} else {
		ledr_on();
	}
}

LOCAL void ESPFUNC user_socket_pre_apconfig() {
	xlink_tcp_disconnect();
	ledr_off();
	ledb_off();
	wifi_set_opmode_current(SOFTAP_MODE);
	user_indicator_start(APCONFIG_FLASH_PERIOD, 0, user_socket_ledg_toggle);
}

LOCAL void ESPFUNC user_socket_post_apconfig() {
	user_indicator_stop();
	ledg_off();
	if (relay_status()) {
		ledb_on();
	} else {
		ledr_on();
	}
}

LOCAL timer_error_t ESPFUNC user_socket_check_timer(socket_timer_t *p_timer) {
	if (p_timer == NULL) {
		return TIMER_INVALID;
	}
	if (p_timer->enable > 1 
		|| p_timer->action > ACTION_TURNON_DURATION 
		|| p_timer->hour > 23 || p_timer->minute > 59 || p_timer->second > 59
		|| p_timer->end_hour > 23 || p_timer->end_minute > 59 || p_timer->end_second > 59) {
		return TIMER_INVALID;
	}
	if (p_timer->enable) {
		return TIMER_ENABLED;
	}
	return TIMER_DISABLED;
}

LOCAL void ESPFUNC user_socket_reset_timer(socket_timer_t *p_timer) {
	if (p_timer == NULL) {
		return;
	}
	os_memset(p_timer, 0xFF, sizeof(socket_timer_t));
}

LOCAL void ESPFUNC user_socket_update_timers() {
	uint8_t i;
	uint8_t cnt = SOCKET_TIMER_MAX;
	for(i = 0; i < SOCKET_TIMER_MAX; i++) {
		if(user_socket_check_timer(&socket_config.socket_timer[i]) == TIMER_INVALID) {
			cnt = i;
			break;
		}
	}
	for(i = cnt; i < SOCKET_TIMER_MAX; i++) {
		os_memset(&socket_config.socket_timer[i], 0xFF, sizeof(socket_timer_t));
	}
}

LOCAL void ESPFUNC user_socket_default_config() {
	uint8_t i;
	xlink_read_user_para((uint8_t *)&socket_config, sizeof(socket_config));
	uint32_t sw_flag = socket_config.switch_flag;
	uint32_t sw_count = socket_config.switch_count;
	os_memset((uint8_t *)&socket_config, 0, sizeof(socket_config));
	socket_config.super.saved_flag = CONFIG_DEFAULT_FLAG;
	if(sw_flag == SWITCH_SAVED_FLAG && sw_count <= SWITCH_COUNT_MAX) {
		socket_config.switch_count = sw_count;
	} else {
		socket_config.switch_count = 0;
	}
	socket_config.switch_flag = SWITCH_SAVED_FLAG;
	socket_config.mode = MODE_TIMER;
	for (i = 0; i < SOCKET_TIMER_MAX; i++) {
		os_memset(&socket_config.socket_timer[i], 0xFF, sizeof(socket_timer_t));
	}
}

LOCAL void ESPFUNC user_socket_save_config() {
	//测试模式下不改变保存的参数
	if (app_test_status()) {
		return;
	}
	socket_config.super.saved_flag = CONFIG_SAVED_FLAG;
	xlink_write_user_para((uint8_t *)&socket_config, sizeof(socket_config));
}

LOCAL void ESPFUNC user_socket_para_init() {
	uint8_t i;
	xlink_read_user_para((uint8_t *)&socket_config, sizeof(socket_config));
	if (socket_config.super.saved_flag != CONFIG_SAVED_FLAG) {
		user_socket_default_config();
	}
	if(socket_config.switch_flag != SWITCH_SAVED_FLAG || socket_config.switch_count > SWITCH_COUNT_MAX) {
		socket_config.switch_flag = SWITCH_SAVED_FLAG;
		socket_config.switch_count = 0;
	}
	user_socket_update_timers();
	for (i = 0; i < SOCKET_TIMER_MAX; i++) {
		socket_config.socket_timer[i].repeat &= 0x7F;
	}
}

LOCAL void ESPFUNC user_socket_key_short_press_cb() {	
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	
	socket_para.power = !socket_para.power;
	if (socket_para.power) {
		if (user_socket_turnon_manual()) {
			user_socket_save_config();
		}
	} else {
		user_socket_turnoff_manual();
	}
	xlink_datapoint_set_changed(INDEX_POWER);
	user_device_update_dpchanged();
	// user_device_update_dpall();
}

LOCAL void ESPFUNC user_socket_key_long_press_cb() {
	if (app_test_status()) {				//测试模式
		return;	
	}
	if (user_smartconfig_instance_status()) {
		user_smartconfig_instance_stop();
		user_apconfig_instance_start(&apc_impl, APCONFIG_TIMEOUT, user_dev_socket.apssid, user_socket_setzone);
	} else if (user_apconfig_instance_status()) {
		return;
	} else {
		user_smartconfig_instance_start(&sc_impl, SMARTCONFIG_TIEMOUT);
	}
}

LOCAL void ESPFUNC user_socket_key_cont_press_cb() {
}

LOCAL void ESPFUNC user_socket_key_release_cb() {
}

LOCAL void ESPFUNC user_socket_key_init() {
	pkeys[0] = user_key_init_single(KEY_IO_NUM,
								  	KEY_IO_FUNC,
									KEY_IO_MUX,
									user_socket_key_short_press_cb,
									user_socket_key_long_press_cb,
									user_socket_key_cont_press_cb,
									user_socket_key_release_cb);
	pkeys[0]->long_count = KEY_LONG_PRESS_COUNT;
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list(&key_list);
}

LOCAL void ESPFUNC user_socket_init() {
	ledr_on();
	uart0_init(BAUDRATE_9600, 16);
	uart0_set_rx_cb(user_socket_decode_sensor);
	uart_enable_isr();
	os_timer_disarm(&sensor_detect_tmr);
	os_timer_setfn(&sensor_detect_tmr, user_socket_detect_sensor, NULL);
	os_timer_arm(&sensor_detect_tmr, 32, 1);
}

LOCAL void ESPFUNC user_socket_process(void *arg) {
	if (user_smartconfig_instance_status() || user_apconfig_instance_status()) {
		return;
	}
	if (socket_config.mode == MODE_SENSOR1 || socket_config.mode == MODE_SENSOR2) {
		user_socket_linkage_process();
		return;
	}
	if (user_rtc_is_synchronized() == false) {
		return;
	}
	uint8_t hour = socket_para.super.hour;
	uint8_t minute = socket_para.super.minute;
	uint8_t second = socket_para.super.second;

	uint8_t i;
	bool flag = false;
	bool save = false;
	bool action = false;
	socket_timer_t *p;
	user_socket_update_timers();
	for (i = 0; i < SOCKET_TIMER_MAX; i++) {
		p = &socket_config.socket_timer[i];
		if (user_socket_check_timer(p) == TIMER_ENABLED) {
			if (p->hour == hour && p->minute == minute && p->second == second) {
				if (p->repeat == 0) {
					action = (p->action > ACTION_TURNOFF ? true : false);
					flag = true;
					if (p->action == ACTION_TURNON_DURATION) {
						p->repeat = 0x80; 
					} else {
						p->enable = 0;
						xlink_datapoint_set_changed(INDEX_TIMER1+i);
						save = true;
					}
				} else if ((p->repeat&(1<<socket_para.super.week)) != 0) {
					action = (p->action > ACTION_TURNOFF ? true : false);
					flag = true;
					if (p->action == ACTION_TURNON_DURATION) {
						p->repeat |= 0x80; 
					}
				}
			}
			if (p->action == ACTION_TURNON_DURATION && p->end_hour == hour && p->end_minute == minute && p->end_second == second) { 
				if (p->repeat == 0x80) {
					p->repeat = 0;
					p->enable = false;
					action = false;
					flag = true;
					xlink_datapoint_set_changed(INDEX_TIMER1+i);
					save = true;
				// } else if ((p->repeat&(1<<socket_para.super.week)) != 0) {
				} else if (p->repeat > 0x80) {
					p->repeat &= 0x7F;
					action = false;
					flag = true;
				}
			}
		}
	}
	if (flag) {
		socket_para.power = action;
		if(action) {
			if (user_socket_turnon_manual()) {
				save = true;
			}
		} else {
			user_socket_turnoff_manual();
		}
		if(save) {
			user_socket_save_config();
		}
		xlink_datapoint_set_changed(INDEX_POWER);
		user_device_update_dpchanged();
		// user_device_update_dpall();
	}
}

LOCAL void ESPFUNC sensor1_linkage_process(uint16_t range) {
	LOCAL uint8_t overCount = 0;
	LOCAL uint8_t overRecoverCount = 0;
	LOCAL uint8_t lossCount = 0;
	LOCAL uint8_t lossRecoverCount = 0;
	LOCAL bool overFlag = false;
	LOCAL bool lossFlag = false;
	uint8_t month = socket_para.super.month;
	if (month < 1 || month > 12) {
		return;
	}
	uint8_t hour = socket_para.super.hour;
	if (hour > 23) {
		return;
	}

	uint16_t ct = socket_para.super.hour*60 + socket_para.super.minute;
	sensor_args_t *pargs = socket_para.p_sensor_args;
	uint16_t daytime_start = socket_config.super.daytime_start;
	uint16_t daytime_end = socket_config.super.daytime_end;
#ifdef	USE_LOCATION
	if (socket_para.super.gis_valid) {
		daytime_start = socket_para.super.gis_sunrise;
		daytime_end = socket_para.super.gis_sunset;
	}
#endif

	int32_t target;
	if (ct >= daytime_start && ct <= daytime_end) {
		target = pargs->s1DayThreshold;
	} else {
		target = pargs->s1NightThreshold;
	}
	int32_t max = (target+range)*10;
	int32_t min = (target-range)*10;
	int32_t temp = socket_para.sensor1_value;
	/* check sensor1 over flag */
	if (overFlag == false) {
		if (temp > max) {
			overCount++;
			if (overCount > 60) {
				overFlag = true;
				overCount = 0;

				app_logd("sensor1 overflag: 1");
			}
		} else {
			overCount = 0;
		}
	} else {
		if (temp < max) {
			overCount++;
			if (overCount > 60) {
				overFlag = false;
				overCount = 0;

				app_logd("sensor1 overflag: 0");
			}
		} else {
			overCount = 0;
		}
	}

	/* check sensor1 loss flag */
	if (lossFlag == false) {
		if (temp < min) {
			lossCount++;
			if (lossCount > 60) {
				lossFlag = true;
				lossCount = 0;

				app_logd("sensor1 lossflag: 1");
			}
		} else {
			lossCount = 0;
		}
	} else {
		if (temp > min) {
			lossCount++;
			if (lossCount > 60) {
				lossFlag = false;
				lossCount = 0;

				app_logd("sensor1 lossflag: 0");
			}
		} else {
			lossCount = 0;
		}
	}

	if (mLock) {
		return;
	}

	if (overFlag) {
		if (user_socket_turnoff_linkage()) {
			socket_para.power = false;
			xlink_datapoint_set_changed(INDEX_POWER);
			user_device_update_dpchanged();
			// user_device_update_dpall();
		}
		return;
	}
	if (lossFlag && !mLockon) {
		if (user_socket_turnon_linkage()) {
			socket_para.power = true;
			user_socket_save_config();
			xlink_datapoint_set_changed(INDEX_POWER);
			user_device_update_dpchanged();
			// user_device_update_dpall();
		}
	}
}

LOCAL void ESPFUNC sensor2_linkage_process(uint16_t range) {
	LOCAL uint8_t overCount = 0;
	LOCAL uint8_t overRecoverCount = 0;
	LOCAL uint8_t lossCount = 0;
	LOCAL uint8_t lossRecoverCount = 0;
	LOCAL bool overFlag = false;
	LOCAL bool lossFlag = false;
	uint8_t month = socket_para.super.month;
	if (month < 1 || month > 12) {
		return;
	}
	uint8_t hour = socket_para.super.hour;
	if (hour > 23) {
		return;
	}

	uint16_t ct = socket_para.super.hour*60 + socket_para.super.minute;
	sensor_args_t *pargs = socket_para.p_sensor_args;
	int32_t target;
	if (ct > socket_config.super.daytime_start && ct <= socket_config.super.daytime_end) {
		target = pargs->s2DayThreshold;
	} else {
		target = pargs->s2NightThreshold;
	}
	int32_t max = (target+range)*10;
	int32_t min = (target-range)*10;
	int32_t temp = socket_para.sensor1_value;
	/* check sensor2 over flag */
	if (overFlag == false) {
		if (temp > max) {
			overCount++;
			if (overCount > 60) {
				overFlag = true;
				overCount = 0;

				app_logd("sensor2 overflag: 1");
			}
		} else {
			overCount = 0;
		}
	} else {
		if (temp < max) {
			overCount++;
			if (overCount > 60) {
				overFlag = false;
				overCount = 0;

				app_logd("sensor2 overflag: 0");
			}
		} else {
			overCount = 0;
		}
	}

	/* check sensor2 loss flag */
	if (lossFlag == false) {
		if (temp < min) {
			lossCount++;
			if (lossCount > 60) {
				lossFlag = true;
				lossCount = 0;

				app_logd("sensor2 lossflag: 1");
			}
		} else {
			lossCount = 0;
		}
	} else {
		if (temp > min) {
			lossCount++;
			if (lossCount > 60) {
				lossFlag = false;
				lossCount = 0;

				app_logd("sensor2 lossflag: 0");
			}
		} else {
			lossCount = 0;
		}
	}

	if (mLock) {
		return;
	}

	if (overFlag) {
		if (user_socket_turnoff_linkage()) {
			socket_para.power = false;
			xlink_datapoint_set_changed(INDEX_POWER);
			user_device_update_dpchanged();
			// user_device_update_dpall();
		}
		return;
	}
	if (lossFlag && !mLockon) {
		if (user_socket_turnon_linkage()) {
			socket_para.power = true;
			user_socket_save_config();
			xlink_datapoint_set_changed(INDEX_POWER);
			user_device_update_dpchanged();
			// user_device_update_dpall();
		}
	}
}

LOCAL bool user_socket_linkage_process() {
	if (user_sensor_args_check(socket_para.p_sensor_args, socket_para.sensor1_type, socket_para.sensor2_type) == false) {
		return false;
	}
	if (socket_para.sensor1_available == true && socket_config.mode == MODE_SENSOR1) {
		switch (socket_para.sensor1_type) {
			case SENSOR_REPTILE_TEMPERATURE:
				sensor1_linkage_process(2);
				break;
		
			default:
				break;
		}
		return true;
	}
	if (socket_para.sensor2_available == true && socket_config.mode == MODE_SENSOR2) {
		switch (socket_para.sensor2_type) {
			case SENSOR_REPTILE_HUMIDITY:
				sensor2_linkage_process(5);
				break;
		
			default:
				break;
		}
		return true;
	}
	return false;
}

// LOCAL void ESPFUNC reptile_temperature_linkage_process() {
// 	LOCAL uint8_t overCount = 0;
// 	LOCAL uint8_t overRecoverCount = 0;
// 	LOCAL uint8_t lossCount = 0;
// 	LOCAL uint8_t lossRecoverCount = 0;
// 	LOCAL bool overFlag = false;
// 	LOCAL bool lossFlag = false;
// 	uint8_t month = socket_para.super.month;
// 	if (month < 1 || month > 12) {
// 		return;
// 	}
// 	uint8_t hour = socket_para.super.hour;
// 	if (hour > 23) {
// 		return;
// 	}
// 	sensor_args_t *pargs = socket_para.p_sensor_args;
// 	uint8_t target = pargs->s1args[(month-1)*12+hour/2];
// 	uint8_t max = (target+2)*10;
// 	uint8_t min = (target-2)*10;
// 	uint8_t temp = socket_para.sensor1_value;
// 	/* check temperature over flag */
// 	if (overFlag == false) {
// 		if (temp > max) {
// 			overCount++;
// 			if (overCount > 60) {
// 				overFlag = true;
// 				overCount = 0;

// 				app_logd("overflag: 1");
// 			}
// 		} else {
// 			overCount = 0;
// 		}
// 	} else {
// 		if (temp < max) {
// 			overCount++;
// 			if (overCount > 60) {
// 				overFlag = false;
// 				overCount = 0;

// 				app_logd("overflag: 0");
// 			}
// 		} else {
// 			overCount = 0;
// 		}
// 	}

// 	/* check temperature loss flag */
// 	if (lossFlag == false) {
// 		if (temp < min) {
// 			lossCount++;
// 			if (lossCount > 60) {
// 				lossFlag = true;
// 				lossCount = 0;

// 				app_logd("lossflag: 1");
// 			}
// 		} else {
// 			lossCount = 0;
// 		}
// 	} else {
// 		if (temp > min) {
// 			lossCount++;
// 			if (lossCount > 60) {
// 				lossFlag = false;
// 				lossCount = 0;

// 				app_logd("lossflag: 0");
// 			}
// 		} else {
// 			lossCount = 0;
// 		}
// 	}

// 	if (mLock) {
// 		return;
// 	}

// 	if (overFlag) {
// 		if (user_socket_turnoff_linkage()) {
// 			socket_para.power = false;
// 			user_device_update_dpall();
// 		}
// 		return;
// 	}
// 	if (lossFlag && !mLockon) {
// 		if (user_socket_turnon_linkage()) {
// 			socket_para.power = true;
// 			user_socket_save_config();
// 			user_device_update_dpall();
// 		}
// 	}
// }

LOCAL void ESPFUNC user_socket_datapoint_init() {
	uint8_t i;
	xlink_datapoint_init_uint32(INDEX_SWITCH_COUNT_MAX, (uint32_t *) &socket_para.switch_count_max);
	
	/* socket */
	xlink_datapoint_init_uint32(INDEX_SWITCH_COUNT, &socket_config.switch_count);
	xlink_datapoint_init_byte(INDEX_POWER, &socket_para.power);
	xlink_datapoint_init_byte(INDEX_MODE, &socket_config.mode);
	for(i = 0; i < SOCKET_TIMER_MAX; i++) {
		xlink_datapoint_init_binary(INDEX_TIMER1+i, (uint8_t *) &socket_config.socket_timer[i], sizeof(socket_timer_t));
	}

	xlink_datapoint_init_byte(INDEX_S1_AVAILABLE, &socket_para.sensor1_available);
	xlink_datapoint_init_byte(INDEX_S1_TYPE, &socket_para.sensor1_type);
	xlink_datapoint_init_int32(INDEX_S1_VALUE, &socket_para.sensor1_value);
	xlink_datapoint_init_byte(INDEX_SV1_TYPE, &socket_para.p_sensor_args->s1type);
	xlink_datapoint_init_byte(INDEX_SV1_NOTIFY_EN, &socket_para.p_sensor_args->s1NotifyEnable);
	// xlink_datapoint_init_byte(45, &socket_para.p_sensor_args->s1LinkageEnable);
	// uint8_t *pargs = socket_para.p_sensor_args->s1args;
	// for(i = 0; i < 4; i++) {
	// 	xlink_datapoint_init_binary(46+i, pargs+(DATAPOINT_BIN_MAX_LEN*i), DATAPOINT_BIN_MAX_LEN);
	// }
	
	xlink_datapoint_init_byte(INDEX_S2_AVAILABLE, &socket_para.sensor2_available);
	xlink_datapoint_init_byte(INDEX_S2_TYPE, &socket_para.sensor2_type);
	xlink_datapoint_init_int32(INDEX_S2_VALUE, &socket_para.sensor2_value);
	xlink_datapoint_init_byte(INDEX_SV2_TYPE, &socket_para.p_sensor_args->s2type);
	xlink_datapoint_init_byte(INDEX_SV2_NOTIFY_EN, &socket_para.p_sensor_args->s2NotifyEnable);
	// xlink_datapoint_init_binary(55, &socket_para.p_sensor_args->s2args[0], DATAPOINT_BIN_MAX_LEN);
	
	xlink_datapoint_init_int32(INDEX_SV1_DAY_THRESHOLD, &socket_para.p_sensor_args->s1DayThreshold);
	xlink_datapoint_init_int32(INDEX_SV1_NIGHT_THRESHOLD, &socket_para.p_sensor_args->s1NightThreshold);
	xlink_datapoint_init_int32(INDEX_SV2_DAY_THRESHOLD, &socket_para.p_sensor_args->s2DayThreshold);
	xlink_datapoint_init_int32(INDEX_SV2_NIGHT_THRESHOLD, &socket_para.p_sensor_args->s2NightThreshold);

	xlink_datapoint_init_int32(INDEX_SV1_THRD_LOWER, &socket_para.p_sensor_args->s1ThrdLower);
	xlink_datapoint_init_int32(INDEX_SV1_THRD_UPPER, &socket_para.p_sensor_args->s1ThrdUpper);
	xlink_datapoint_init_byte(INDEX_S1_LOSS_FLAG, &socket_para.s1_loss_flag);
	xlink_datapoint_init_byte(INDEX_S1_OVER_FLAG, &socket_para.s1_over_flag);
	xlink_datapoint_init_int32(INDEX_SV2_THRD_LOWER, &socket_para.p_sensor_args->s2ThrdLower);
	xlink_datapoint_init_int32(INDEX_SV2_THRD_UPPER, &socket_para.p_sensor_args->s2ThrdUpper);
	xlink_datapoint_init_byte(INDEX_S2_LOSS_FLAG, &socket_para.s2_loss_flag);
	xlink_datapoint_init_byte(INDEX_S2_OVER_FLAG, &socket_para.s2_over_flag);
}

LOCAL void ESPFUNC user_socket_refresh_sensor_status() {
	if (socket_para.sensor1_available == false ||
		user_sensor_args_check(&socket_config.sensor_args, socket_para.sensor1_type, socket_para.sensor2_type) == false) {
		return;
	}
	if (socket_para.sensor1_value < socket_config.sensor_args.s1ThrdLower*10) {
		socket_para.s1_loss_flag = 0x01;
	} else {
		socket_para.s1_loss_flag = 0x00;
	}
	if (socket_para.sensor1_value > socket_config.sensor_args.s1ThrdUpper*10) {
		socket_para.s1_over_flag = 0x01;
	} else {
		socket_para.s1_over_flag = 0x00;
	}
	if (socket_config.sensor_args.s1NotifyEnable) {
		socket_para.s1_loss_flag |= NOTIFY_MASK;
		socket_para.s1_over_flag |= NOTIFY_MASK;
	}

	if (socket_para.sensor2_available) {
		if (socket_para.sensor2_value < socket_config.sensor_args.s2ThrdLower*10) {
			socket_para.s2_loss_flag = 0x01;
		} else {
			socket_para.s2_loss_flag = 0x00;
		}
		if (socket_para.sensor2_value > socket_config.sensor_args.s2ThrdUpper*10) {
			socket_para.s2_over_flag = 0x01;
		} else {
			socket_para.s2_over_flag = 0x00;
		}
		if (socket_config.sensor_args.s2NotifyEnable) {
			socket_para.s2_loss_flag |= NOTIFY_MASK;
			socket_para.s2_over_flag |= NOTIFY_MASK;
		}
	}
	xlink_datapoint_set_changed(INDEX_S1_LOSS_FLAG);
	xlink_datapoint_set_changed(INDEX_S1_OVER_FLAG);
	xlink_datapoint_set_changed(INDEX_S2_LOSS_FLAG);
	xlink_datapoint_set_changed(INDEX_S2_OVER_FLAG);
}

LOCAL void ESPFUNC user_socket_datapoint_changed_cb() {
	if (xlink_datapoint_ischanged(SYNC_DATETIME_INDEX)) {
		user_rtc_set_synchronized(true);
		app_logd("%d-%d-%d %d:%d:%d week-%d zone-%d", datetime.year, datetime.month, datetime.day, datetime.hour, datetime.min, datetime.second, datetime.week, datetime.zone);
	}
	if (socket_para.power) {
		user_socket_turnon_manual();
	} else {
		user_socket_turnoff_manual();
	}
	user_socket_refresh_sensor_status();
	user_device_update_dpchanged();
	user_socket_save_config();
}

LOCAL void ESPFUNC user_socket_detect_sensor(void *arg) {
	LOCAL uint8_t trg;
	LOCAL uint8_t cont;
	uint8_t rd = GPIO_INPUT_GET(DETECT_IO_NUM) ^ 0x01 ;
	trg = rd & (rd ^ cont);
	cont = rd;
	bool detect = trg ^ cont;
	if (m_sensor_detected == true && detect == false) {
		m_sensor_detected = false;
		socket_para.sensor1_available = false;
		socket_para.sensor2_available = false;
		socket_para.s1_loss_flag = 0;
		socket_para.s1_over_flag = 0;
		socket_para.s2_loss_flag = 0;
		socket_para.s2_over_flag = 0;
		socket_config.mode = MODE_TIMER;
		user_socket_save_config();

		xlink_datapoint_set_changed(INDEX_S1_AVAILABLE);
		xlink_datapoint_set_changed(INDEX_S2_AVAILABLE);
		xlink_datapoint_set_changed(INDEX_S1_LOSS_FLAG);
		xlink_datapoint_set_changed(INDEX_S1_OVER_FLAG);
		xlink_datapoint_set_changed(INDEX_S2_LOSS_FLAG);
		xlink_datapoint_set_changed(INDEX_S2_OVER_FLAG);
		xlink_datapoint_set_changed(INDEX_MODE);
		user_device_update_dpchanged();
		// user_device_update_dpall();
		app_logd("sensor removed...");
	} else if (m_sensor_detected == false && detect == true) {
		m_sensor_detected = true;
		app_logd("sensor detected...");
	}
}

/**
 * get:	FRM_HDR CMD_GET XOR
 * rsp: FRM_HDR CMD_GET type1 value1(4) {type2 value2(4)} XOR
 */
LOCAL void ESPFUNC user_socket_decode_sensor(uint8_t *pbuf, uint8_t len) {
	if(pbuf == NULL || (len != 8 && len != 13) || pbuf[0] != FRAME_HEADER || pbuf[1] != CMD_GET) {
		return;
	}
	uint8_t i;
	uint8_t xor = 0;
	for (i = 0; i < len; i++) {
		xor ^= pbuf[i];
	}
	if(xor != 0) {
		return;
	}
	bool available1 = true;
	bool available2 = false;
	uint8_t type1 = pbuf[2];
	int32_t value1 = (pbuf[6]<<24)|(pbuf[5]<<16)|(pbuf[4]<<8)|pbuf[3];
	uint8_t type2 = 0;
	int32_t value2 = 0;
	if (len == 13) {
		available2 = true;
		type2 = pbuf[7];
		value2 = (pbuf[11]<<24)|(pbuf[10]<<16)|(pbuf[9]<<8)|pbuf[8];
	}
	bool changed = false;
	if (socket_para.sensor1_available != available1 || socket_para.sensor2_available != available2) {
		socket_para.sensor1_available = available1;		
		socket_para.sensor2_available = available2;		
		changed = true;
	}
	if (socket_para.sensor1_type != type1 || socket_para.sensor1_value != value1) {
		socket_para.sensor1_type = type1;
		socket_para.sensor1_value = value1;
		changed = true;
	}
	if (available2 && (socket_para.sensor2_type != type2 || socket_para.sensor2_value != value2)) {
		socket_para.sensor2_type = type2;
		socket_para.sensor2_value = value2;
		changed = true;
	}
	if (changed) {
		user_socket_refresh_sensor_status();
		xlink_datapoint_set_changed(INDEX_S1_AVAILABLE);
		xlink_datapoint_set_changed(INDEX_S1_TYPE);
		xlink_datapoint_set_changed(INDEX_S1_VALUE);
		xlink_datapoint_set_changed(INDEX_S2_AVAILABLE);
		xlink_datapoint_set_changed(INDEX_S2_TYPE);
		xlink_datapoint_set_changed(INDEX_S2_VALUE);
		user_device_update_dpchanged();
		// user_device_update_dpall();
	}
}
