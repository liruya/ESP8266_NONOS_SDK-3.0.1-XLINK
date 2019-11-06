#ifndef	__XLINK_CONFIG_H__
#define	__XLINK_CONFIG_H__

#include "xlink_sdk.h"
#include "app_common.h"

extern int32_t	xlink_write_config(uint8_t *data, uint32_t len);
extern int32_t	xlink_read_config(uint8_t *data, uint32_t len);
extern bool 	xlink_write_version(uint16_t version);
extern uint16_t xlink_read_version();
extern bool 	xlink_write_user_para(uint8_t *data, uint32_t len);
extern bool 	xlink_read_user_para(uint8_t *data, uint32_t len);

#endif