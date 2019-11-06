#ifndef	__XLINK_UDP_SERVER_H__
#define	__XLINK_UDP_SERVER_H__

#include "app_common.h"
#include "xlink.h"

#define	UDP_LOCAL_PORT		10000
#define	UDP_REMOTE_PORT		6000

//#define	UDP_LOCAL_PORT		5987
//#define	UDP_REMOTE_PORT		6000

extern void xlink_udp_server_init();
extern uint32_t xlink_udp_send( xlink_addr_t *addr, uint8_t *pdata, uint16_t len );

#endif