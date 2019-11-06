#include "xlink_upgrade.h"
#include "upgrade.h"
#include "xlink_datapoint.h"
#include "xlink.h"

/**
 * 升级流程: 
 * 1. 根据传入的固件地址(static.xlink.cn/xxxxx)解析出hostname, port, filepath
 * 2. 如果hostname非ip地址,开始发现dns，发现dns后获得ip
 * 3. upgrade_server_info初始化port, ip, check_cb, check_times, url
 * 4. system_upgrade_start(&upgrade_server_info); 
 * */

#define XLINK_UPGRADE_LINK		"static.xlink.cn"

#define	REBOOT_DELAY			2000

#define PHEADBUFFER 			"GET /%s HTTP/1.1\r\n\
Host: %s:%d\r\n\
Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"
			
#define HEADER 					"GET HTTP/1.1\r\n\
Host: %d.%d.%d.%d\r\n\
Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

LOCAL os_timer_t delay_timer;

LOCAL bool processing;
LOCAL struct espconn xlink_upgrade_tcp_client;
LOCAL ip_addr_t xlink_upgrade_ip;
LOCAL struct upgrade_server_info *xlink_upgrade_server;
LOCAL xlink_upgrade_info_t xlink_upgrade_info;
int8_t	upgrade_state;

void ESPFUNC xlink_upgrade_report(int8_t state) {
	upgrade_state = state;
	xlink_datapoint_update_all();
}

LOCAL void ESPFUNC xlink_upgrade_reboot(void *arg) {
	system_upgrade_reboot();
}

LOCAL void ESPFUNC xlink_upgrade_destroy() {
	if(xlink_upgrade_server != NULL) {
		if(xlink_upgrade_server->url != NULL) {
			os_free(xlink_upgrade_server->url);
			xlink_upgrade_server->url = NULL;
		}
		if(xlink_upgrade_server->pespconn != NULL) {
			os_free(xlink_upgrade_server->pespconn);
			xlink_upgrade_server->pespconn = NULL;
		}
		os_free(xlink_upgrade_server);
		xlink_upgrade_server = NULL;
	}
}

LOCAL void ESPFUNC xlink_upgrade_response(void *arg) {
	struct upgrade_server_info *server = arg;
	processing = false;
	if (server->upgrade_flag == 1) {
		app_logd("Device OTA upgrade success...");
		xlink_upgrade_report(XUPGRADE_STATE_SUCCESS);
	} else {
		app_loge("Device OTA upgrade failed...");
		xlink_upgrade_report(XUPGRADE_ERRCODE_UPGRADE_FAIL);
	}
	xlink_upgrade_destroy();

	os_timer_disarm(&delay_timer);
	os_timer_setfn(&delay_timer, xlink_upgrade_reboot, NULL);
	os_timer_arm(&delay_timer, REBOOT_DELAY, 0);
	// system_upgrade_reboot();
}

void ESPFUNC xlink_upgrade_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	uint8_t addr[4];

	if (ipaddr == NULL || ipaddr->addr == 0) {
		app_loge("ipaddr invalid");

		processing = false;
		xlink_upgrade_report(XUPGRADE_ERRCODE_IP_INVALID);

		xlink_upgrade_destroy();
		return;
	} else {
		app_logd("ipaddr is %d.%d.%d.%d", 	*((uint8_t *)&ipaddr->addr),
												*((uint8_t *)&ipaddr->addr+1),
												*((uint8_t *)&ipaddr->addr+2),
												*((uint8_t *)&ipaddr->addr+3));
	}
	os_memset(addr, 0, sizeof(addr));

	if (ipaddr->addr != 0) {
		os_memcpy(xlink_upgrade_server->ip, &ipaddr->addr, 4);
		if (system_upgrade_start(xlink_upgrade_server) == false) {
			app_loge("upgrade start error...");

			xlink_upgrade_report(XUPGRADE_ERRCODE_UPGRADING);

			xlink_upgrade_destroy();
		} else {
			app_logd("upgrade start success...");
		}
	}
}

void ESPFUNC xlink_upgrade_start_dns(const char *hostname) {
	xlink_upgrade_ip.addr = 0;
	espconn_gethostbyname(&xlink_upgrade_tcp_client, hostname, &xlink_upgrade_ip, xlink_upgrade_dns_found);
}

bool ESPFUNC xlink_upgrade_get_info(const char *url, xlink_upgrade_info_t *pinfo) {
	if(url == NULL || os_strlen(url) >= XLINK_UPGRADE_URL_MAX_LENGTH) {
		return false;
	}
	char *p = os_strstr(url, "http://");
	int offset = 0;
	char domain[128];
	char port[8];
	if (p != NULL) {
		if(p == url)  {
			offset = os_strlen("http://");
		} else {
			return false;
		}
	}
	p = os_strchr(url + offset, '/');
	if (p == NULL) {
		return false;
	}
	os_memset(pinfo, 0, sizeof(xlink_upgrade_info_t));
	os_strcpy(pinfo->path, p+1);
	os_memset(domain, 0, sizeof(domain));
	os_memcpy(domain, url + offset, p - url - offset);
	p = os_strchr(domain, ':');
	if (p == NULL) {
		os_strcpy(port, "80");
	} else {
		*p = '\0';
		os_strcpy(port, p+1);
	}
	os_strcpy(pinfo->ipaddr, domain);
	pinfo->port = atoi(port);
	return true;
}

void ESPFUNC xlink_upgrade_start(const char *url) {
	uint8_t ipaddr[4];
	if (processing) {
		return;
	}
	
	if (xlink_upgrade_get_info(url, &xlink_upgrade_info)) {
		xlink_upgrade_destroy();

		xlink_upgrade_server = (struct upgrade_server_info *) os_zalloc(sizeof(struct upgrade_server_info));
		if (xlink_upgrade_server == NULL || 
			(xlink_upgrade_server->pespconn = (struct espconn *) os_zalloc(sizeof(struct espconn))) == NULL ||
			(xlink_upgrade_server->url = (uint8_t *) os_zalloc(512)) == NULL) {
			app_loge("Malloc upgrade_server_info failed...");

			xlink_upgrade_report(XUPGRADE_ERRCODE_START_FAIL);
			xlink_upgrade_destroy();
			return;
		}

		xlink_upgrade_server->port = xlink_upgrade_info.port;
		xlink_upgrade_server->check_cb = xlink_upgrade_response;
		xlink_upgrade_server->check_times = 60000;
		os_sprintf(xlink_upgrade_server->url, PHEADBUFFER, xlink_upgrade_info.path, xlink_upgrade_info.ipaddr, xlink_upgrade_info.port);

		app_logd("%s", xlink_upgrade_server->url);
		if(xlink_check_ip(xlink_upgrade_info.ipaddr, ipaddr)) {
			os_memset(xlink_upgrade_server->ip, 0, 4);
			os_memcpy(xlink_upgrade_server->ip, ipaddr, 4);
			app_logd("%d.%d.%d.%d", xlink_upgrade_server->ip[0], xlink_upgrade_server->ip[1], xlink_upgrade_server->ip[2], xlink_upgrade_server->ip[3]);
			if (system_upgrade_start(xlink_upgrade_server) == false) {		//已经开始升级
				app_loge("upgrade start error...");

				xlink_upgrade_report(XUPGRADE_ERRCODE_UPGRADING);
				xlink_upgrade_destroy();
				return;
			} else {														//开始升级
				app_logd("upgrade start success...");

				xlink_upgrade_report(XUPGRADE_STATE_UPGRADING);
			}
		} else {															//开始发现DNS
			xlink_upgrade_report(XUPGRADE_STATE_UPGRADING);

			xlink_upgrade_start_dns(xlink_upgrade_info.ipaddr);
		}
		processing = true;
	} else {																//无效的固件链接
		app_loge("Invalid upgrade url...");

		xlink_upgrade_report(XUPGRADE_ERRCODE_URL_INVALID);
	}
}
