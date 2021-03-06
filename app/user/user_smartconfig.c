#include "user_smartconfig.h"

typedef struct {
	task_t super;
	os_timer_t timer;
} smartconfig_task_t;

LOCAL bool user_smartconfig_start(task_t **task);
LOCAL bool user_smartconfig_stop(task_t **task);
LOCAL void user_smartconfig_timeout_cb();

LOCAL smartconfig_task_t *sc_task;
LOCAL const task_vtable_t sc_vtable = newTaskVTable(user_smartconfig_start, user_smartconfig_stop, user_smartconfig_timeout_cb);

LOCAL void ESPFUNC user_smartconfig_done(sc_status status, void *pdata) {
	switch (status) {
		case SC_STATUS_WAIT:
			app_logd("SC_STATUS_WAIT\n");
			break;
		case SC_STATUS_FIND_CHANNEL:
			app_logd("SC_STATUS_FIND_CHANNEL\n");
			break;
		case SC_STATUS_GETTING_SSID_PSWD:
			app_logd("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = (sc_type *) pdata;
			if (*type == SC_TYPE_ESPTOUCH) {
				app_logd("SC_TYPE:SC_TYPE_ESPTOUCH\n");
			} else {
				app_logd("SC_TYPE:SC_TYPE_AIRKISS\n");
			}
			break;
		case SC_STATUS_LINK:
			app_logd("SC_STATUS_LINK\n");
			wifi_station_disconnect();
			struct station_config *sta_conf =  pdata;
			wifi_station_set_config(sta_conf);
			wifi_station_connect();
			break;
		case SC_STATUS_LINK_OVER:
			app_logd("SC_STATUS_LINK_OVER\n");
			if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
				uint8 phone_ip[4] = { 0 };
				os_memcpy(phone_ip, (uint8*) pdata, 4);
				app_logd("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
						phone_ip[2], phone_ip[3]);
			} else {
				//SC_TYPE_AIRKISS - support airkiss v2.0
			}
			user_task_stop((task_t **) &sc_task);
			break;
	}
}

LOCAL bool ESPFUNC user_smartconfig_start(task_t **task) {
	app_logd("smartconfig start... %d  *%d  **%d", task, *task, **task);
	if (task == NULL || *task == NULL) {
		return false;
	}
	smartconfig_task_t *ptask = (smartconfig_task_t *) (*task);
	wifi_station_disconnect();
	smartconfig_stop();
	wifi_set_opmode(STATION_MODE);
	smartconfig_set_type(SC_TYPE_ESPTOUCH);
	smartconfig_start(user_smartconfig_done);
	return true;
}

LOCAL void ESPFUNC user_smartconfig_stop_cb(void *arg) {
	task_t **task = arg;
	if (task == NULL || *task == NULL) {
		return;
	}
	os_free(*task);
	*task = NULL;
}

LOCAL bool ESPFUNC user_smartconfig_stop(task_t **task) {
	app_logd("smartconfig stop... %d  *%d  **%d", task, *task, **task);
	if (task == NULL || *task == NULL) {
		return false;
	}
	smartconfig_stop();

	smartconfig_task_t *ptask = (smartconfig_task_t *) (*task);
	os_timer_disarm(&ptask->timer);
	os_timer_setfn(&ptask->timer, user_smartconfig_stop_cb, task);
	os_timer_arm(&ptask->timer, 10, 0);
	return true;
}

LOCAL void ESPFUNC user_smartconfig_timeout_cb() {
	app_logd("smartconfig timeout...");
	wifi_set_opmode(STATION_MODE);
	wifi_station_connect();
}

void ESPFUNC user_smartconfig_instance_start(const task_impl_t *impl, const uint32_t timeout) {
    if (sc_task != NULL) {
        app_loge("smartconfig start failed -> already started...");
        return;
    }
    sc_task = os_zalloc(sizeof(smartconfig_task_t));
    if (sc_task == NULL) {
        app_loge("smartconfig start failed -> malloc sc_task failed...");
        return;
    }
	app_logd("smartconfig create... %d", sc_task);
    sc_task->super.vtable = &sc_vtable;
    sc_task->super.impl = impl;
    sc_task->super.timeout = timeout;
    user_task_start((task_t **) &sc_task);
}

void ESPFUNC user_smartconfig_instance_stop() {
    if (sc_task != NULL) {
        user_task_stop((task_t **) &sc_task);
		app_logd("sc_task -> %d", sc_task);
    }
}

bool ESPFUNC user_smartconfig_instance_status() {
    return (sc_task != NULL);
}
