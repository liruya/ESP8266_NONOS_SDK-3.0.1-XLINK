#ifndef	__XLINK_H__
#define	__XLINK_H__

#include "app_common.h"
#include "xlink_sdk.h"
#include "xlink_config.h"
#include "xlink_datapoint.h"
#include "xlink_upgrade.h"
#include "xlink_udp_server.h"
#include "xlink_tcp_client.h"
#include "user_device.h"

extern bool connected_cloud;

extern bool xlink_check_ip(const char *ip, uint8_t *ipaddr);

extern void		xlink_init(user_device_t *pdev);
extern void 	xlink_connect_cloud();
extern void 	xlink_disconnect_cloud();
extern void 	xlink_reset();
// extern uint32_t xlink_get_deviceid();
// extern void 	xlink_enable_pair();
// extern void 	xlink_disable_pair();
extern void 	xlink_enable_subscription();
extern void 	xlink_disable_subscription();
extern int32_t  xlink_test_start();
extern int32_t  xlink_test_end();
extern int32_t  xlink_test_enable();
extern int32_t  xlink_test_disable();
// extern int32_t 	xlink_post_event(xlink_sdk_event_t **event);
extern int32_t 	xlink_receive_tcp_data(const xlink_uint8 **data, xlink_int32 datalength);
extern int32_t  xlink_receive_udp_data(const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr);
extern int32_t 	xlink_update_datapoint_with_alarm(const xlink_uint8 **data, xlink_int32 datamaxlength);
extern int32_t 	xlink_update_datapoint_no_alarm(const xlink_uint8 **data, xlink_int32 datamaxlength);
extern void 	xlink_process();

#endif