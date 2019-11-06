#ifndef	__XLINK_TCP_CLIENT_H__
#define	__XLINK_TCP_CLIENT_H__

#include "app_common.h"

#define XLINK_DOMAIN    	"mqtt.xlink.cn"
//#define XLINK_DOMAIN    	"cm2.xlink.cn"
//#define XLINK_DOMAIN    	"static.xlink.cn"

//#define SSL_ENABLE

#define	TCP_LOCAL_PORT		8089
#ifdef	SSL_ENABLE
#define	TCP_REMOTE_PORT		1884
#else
#define	TCP_REMOTE_PORT		1883
#endif

extern uint32_t xlink_tcp_send(uint8_t *data, uint16_t datalen);
extern void xlink_tcp_client_init();
extern void xlink_tcp_disconnect();

#endif
