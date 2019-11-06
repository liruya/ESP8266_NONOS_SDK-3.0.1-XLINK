#include "xlink_tcp_client.h"
#include "user_ringbuffer.h"
#include "xlink.h"
#include "espconn.h"
#include "user_smartconfig.h"
#include "user_apconfig.h"

#define	TCP_SEND_BUFFER_SIZE	2048

#define TCP_SEND_TIMEOUT		1000				//unit ms

#define	DNS_GET_INTERVAL		3000

LOCAL struct espconn xlink_tcp_client;
LOCAL uint8_t tcp_ringbuffer[TCP_SEND_BUFFER_SIZE];
LOCAL ringbuf_t tcp_rb_send;
LOCAL bool mTcpSending;
LOCAL os_timer_t tcp_send_timer;
LOCAL uint8_t reconn_cnt;
LOCAL bool flag_dns_discovery;
LOCAL bool isDnsFound;
LOCAL ip_addr_t remote_ip;
LOCAL os_timer_t dns_timer;
LOCAL os_timer_t loop_timer;

LOCAL void xlink_tcp_send_loop();

void ESPFUNC xlink_tcp_connect() {
	int8_t error = espconn_connect(&xlink_tcp_client);
	app_logd("connect tcp error: %d", error);
}

void ESPFUNC xlink_tcp_disconnect() {
	xlink_disconnect_cloud();
	espconn_disconnect(&xlink_tcp_client);
}

void ESPFUNC xlink_tcp_reconnect() {
	if(user_smartconfig_instance_status() || user_apconfig_instance_status) {
		reconn_cnt = 0;
		return;
	}
	reconn_cnt++;
	if(reconn_cnt > 5) {
		reconn_cnt = 0;
		isDnsFound = false;
		connected_cloud = false;
		flag_dns_discovery = false;
		xlink_tcp_disconnect();
	} else {
		xlink_tcp_connect();
	}
}

LOCAL void ESPFUNC xlink_tcp_dns_found(const char *name, ip_addr_t *ip, void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	reconn_cnt = 0;
	connected_cloud = false;
	flag_dns_discovery = false;
	os_timer_disarm(&dns_timer);
	if(ip == NULL || ip->addr == 0) {
		return;
	}
	isDnsFound = true;
	os_memcpy(xlink_tcp_client.proto.tcp->remote_ip, &ip->addr, 4);
	espconn_connect(&xlink_tcp_client);
}

LOCAL void ESPFUNC xlink_tcp_dns_check_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	espconn_gethostbyname(pespconn, XLINK_DOMAIN, &remote_ip, xlink_tcp_dns_found);
}

LOCAL void ESPFUNC xlink_tcp_start_dns(struct espconn *pespconn) {
	remote_ip.addr = 0;
	espconn_gethostbyname(pespconn, XLINK_DOMAIN, &remote_ip, xlink_tcp_dns_found);

	os_timer_disarm(&dns_timer);
	os_timer_setfn(&dns_timer, xlink_tcp_dns_check_cb, pespconn);
	os_timer_arm(&dns_timer, DNS_GET_INTERVAL, 1);
	app_logd("Start DNS...\n");
}

LOCAL void ESPFUNC xlink_tcp_func_process(void *arg) {
	if (user_smartconfig_instance_status() || 
		user_apconfig_instance_status()) {
		flag_dns_discovery = false;
		isDnsFound = false;
		reconn_cnt = 0;
		connected_cloud = false;
		return;
	}
	struct ip_info ipinfo;
	wifi_get_ip_info(STATION_IF, &ipinfo);
	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipinfo.ip.addr != 0) {
		if(flag_dns_discovery == false && isDnsFound == false) {
			flag_dns_discovery = true;
			reconn_cnt = 0;
			xlink_tcp_start_dns(&xlink_tcp_client);
		}
		if (connected_cloud) {
			xlink_tcp_send_loop();
		}
		xlink_process();
	} else {
		flag_dns_discovery = false;
		isDnsFound = false;
		reconn_cnt = 0;
		connected_cloud = false;
	}
}

LOCAL void ESPFUNC xlink_tcp_connect_cb(void *arg) {
	app_logd("tcp connected...");
	reconn_cnt = 0;
	connected_cloud = true;
	xlink_connect_cloud();
}

LOCAL void ESPFUNC xlink_tcp_disconnect_cb(void *arg) {
	app_logd("tcp disconnected...");
	isDnsFound = false;
	connected_cloud = false;
	flag_dns_discovery = false;
	reconn_cnt = 0;
}

LOCAL void ESPFUNC xlink_tcp_reconnect_cb(void *arg, sint8 errtype) {
	app_logd("tcp reconnect with error: %d", errtype);
	xlink_tcp_reconnect();
}

LOCAL void ESPFUNC xlink_tcp_recv_cb(void *arg, char *buf, uint16_t len) {
	if (buf == NULL || len == 0) {
		return;
	}
	xlink_receive_tcp_data((const xlink_uint8 **)&buf, len);
}

LOCAL void ESPFUNC xlink_tcp_sent_cb(void *arg) {
	os_timer_disarm(&tcp_send_timer);
	mTcpSending = false;
}

uint32_t ESPFUNC xlink_tcp_send(uint8_t *data, uint16_t datalen) {
	return user_rb_put(&tcp_rb_send, data, datalen);
}

LOCAL void ESPFUNC xlink_tcp_send_timeout_cb(void *arg) {
	os_timer_disarm(&tcp_send_timer);
	mTcpSending = false;
}

LOCAL uint8_t tcp_send_buffer[1460];
LOCAL void ESPFUNC xlink_tcp_send_loop() {
	uint32_t len;
	if (mTcpSending) {
		return;
	}
	len = user_rb_unread_size(&tcp_rb_send);
	if (len == 0) {
		return;
	}
	mTcpSending = true;
	memset(tcp_send_buffer, 0, sizeof(tcp_send_buffer));
	len = user_rb_get(&tcp_rb_send, tcp_send_buffer, sizeof(tcp_send_buffer));
	espconn_send(&xlink_tcp_client, tcp_send_buffer, len);
	os_timer_disarm(&tcp_send_timer);
	os_timer_setfn(&tcp_send_timer, xlink_tcp_send_timeout_cb, NULL);
	os_timer_arm(&tcp_send_timer, TCP_SEND_TIMEOUT, 0);
}

void ESPFUNC xlink_tcp_client_init() {
	if (xlink_tcp_client.proto.tcp == NULL) {
		xlink_tcp_client.proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
	}

	xlink_tcp_client.type = ESPCONN_TCP;
	xlink_tcp_client.state = ESPCONN_NONE;
	xlink_tcp_client.proto.tcp->local_port = TCP_LOCAL_PORT;
	xlink_tcp_client.proto.tcp->remote_port = TCP_REMOTE_PORT;
	espconn_regist_connectcb(&xlink_tcp_client, xlink_tcp_connect_cb);
	espconn_regist_disconcb(&xlink_tcp_client, xlink_tcp_disconnect_cb);
	espconn_regist_reconcb(&xlink_tcp_client, xlink_tcp_reconnect_cb);
	espconn_regist_recvcb(&xlink_tcp_client, xlink_tcp_recv_cb);
	espconn_regist_sentcb(&xlink_tcp_client, xlink_tcp_sent_cb);

	user_rb_init(&tcp_rb_send, tcp_ringbuffer, sizeof(tcp_ringbuffer));

	os_timer_disarm(&loop_timer);
	os_timer_setfn(&loop_timer, xlink_tcp_func_process, NULL);
	os_timer_arm(&loop_timer, 100, 1);
}