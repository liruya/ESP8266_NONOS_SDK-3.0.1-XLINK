#ifndef	__XLINK_UPGRADE_H__
#define	__XLINK_UPGRADE_H__

#include "app_common.h"
#include "xlink_sdk.h"
#include "ets_sys.h"

#define XLINK_UPGRADE_URL_MAX_LENGTH	256

#define	XUPGRADE_STATE_IDLE				0
#define	XUPGRADE_STATE_UPGRADING		1
#define	XUPGRADE_STATE_SUCCESS			2
#define	XUPGRADE_ERRCODE_URL_INVALID	-1
#define	XUPGRADE_ERRCODE_URL_OVERLEN	-2
#define	XUPGRADE_ERRCODE_FW_INVALID		-3
#define	XUPGRADE_ERRCODE_IP_INVALID		-4
#define	XUPGRADE_ERRCODE_UPGRADING		-5
#define	XUPGRADE_ERRCODE_START_FAIL		-6
#define	XUPGRADE_ERRCODE_UPGRADE_FAIL	-7

typedef struct{
	uint8_t ipaddr[72];
	uint16_t port;
	uint8_t path[128];
} xlink_upgrade_info_t;

extern	int8_t	upgrade_state;
extern 	void xlink_upgrade_start(const char *url);
extern	void xlink_upgrade_report(int8_t state);

#endif