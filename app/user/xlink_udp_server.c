#include "xlink_udp_server.h"

LOCAL struct espconn xlink_udp_server;

LOCAL void xlink_udp_recv_cb(void *arg, char *pdata, unsigned short len);

void ESPFUNC xlink_udp_server_init() {
	xlink_udp_server.type = ESPCONN_UDP;
	xlink_udp_server.state = ESPCONN_NONE;
	xlink_udp_server.proto.udp = (esp_udp *) os_zalloc(sizeof(esp_udp));
	xlink_udp_server.reverse = NULL;
	xlink_udp_server.proto.udp->local_port = UDP_LOCAL_PORT;
	xlink_udp_server.proto.udp->remote_port = UDP_REMOTE_PORT;
	xlink_udp_server.proto.udp->remote_ip[0] = 0x00;
	xlink_udp_server.proto.udp->remote_ip[1] = 0x00;
	xlink_udp_server.proto.udp->remote_ip[2] = 0x00;
	xlink_udp_server.proto.udp->remote_ip[3] = 0x00;
	espconn_regist_recvcb(&xlink_udp_server, xlink_udp_recv_cb);
	espconn_create(&xlink_udp_server);
}

LOCAL void ESPFUNC xlink_udp_recv_cb(void *arg, char *pdata, unsigned short len) {
	if(pdata == NULL || len == 0) {
		return;
	}
	xlink_addr_t addr;
	xlink_addr_t *paddr = &addr;
	ip_address_t ip_temp;
	ip_temp.ip = 0;
	remot_info *premote = NULL;
	if(espconn_get_connection_info(&xlink_udp_server, &premote, 0) != ESPCONN_OK) {
		return;
	}
	os_memcpy(xlink_udp_server.proto.udp->remote_ip, premote->remote_ip, 4);
	xlink_udp_server.proto.udp->remote_port = premote->remote_port;

	ip_temp.bit.byte0 = xlink_udp_server.proto.udp->remote_ip[0];
	ip_temp.bit.byte1 = xlink_udp_server.proto.udp->remote_ip[1];
	ip_temp.bit.byte2 = xlink_udp_server.proto.udp->remote_ip[2];
	ip_temp.bit.byte3 = xlink_udp_server.proto.udp->remote_ip[3];
	xlink_memset(&addr, 0, sizeof(xlink_addr_t));
	addr.ip = ip_temp.ip;
	addr.port = xlink_udp_server.proto.udp->remote_port;
	xlink_receive_udp_data((const xlink_uint8 **)&pdata, len, (const xlink_addr_t **)&paddr);
}

uint32_t ESPFUNC xlink_udp_send(xlink_addr_t *addr, uint8_t *pdata, uint16_t len) {
	ip_address_t ip_addr;
	ip_addr.ip = addr->ip;
	xlink_udp_server.proto.udp->remote_port = addr->port;
	xlink_udp_server.proto.udp->remote_ip[0] = ip_addr.bit.byte0;
	xlink_udp_server.proto.udp->remote_ip[1] = ip_addr.bit.byte1;
	xlink_udp_server.proto.udp->remote_ip[2] = ip_addr.bit.byte2;
	xlink_udp_server.proto.udp->remote_ip[3] = ip_addr.bit.byte3;
	espconn_send(&xlink_udp_server, pdata, len);
	return len;
}

uint32_t ESPFUNC xlink_udp_sent(uint8_t *pdata, uint16_t len) {
	espconn_send(&xlink_udp_server, pdata, len);
	return len;
}
