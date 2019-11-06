#ifndef	__XLINK_DATAPOINT_H__
#define	__XLINK_DATAPOINT_H__

#include "app_common.h"

#define DATAPOINT_MAX_NUM		200
#define	DATAPOINT_BIN_MAX_LEN	64
#define DATAPOINT_STR_MAX_LEN	64

#define	DP_READ_ONLY		1
#define	DP_READ_WRITE		0

#define DP_TYPE_BYTE		0
#define DP_TYPE_INT16		1
#define DP_TYPE_UINT16		2
#define DP_TYPE_INT32		3
#define DP_TYPE_UINT32		4
#define DP_TYPE_INT64		5
#define DP_TYPE_UINT64		6
#define DP_TYPE_FLOAT		7
#define DP_TYPE_DOUBLE		8
#define DP_TYPE_STRING		9
#define DP_TYPE_BINARY		10

typedef enum {
	DP_BYTE = 0x00,
	DP_INT16 = 0x10,
	DP_UINT16 = 0x20,
	DP_INT32 = 0x30,
	DP_UINT32 = 0x40,
	DP_INT64 = 0x50,
	DP_UINT64 = 0x60,
	DP_FLOAT = 0x70,
	DP_DOUBLE = 0x80,
	DP_STRING = 0x90,
	DP_BINARY = 0XA0
} enum_DataPoint_t;

typedef struct {
	union {
		struct {
			unsigned length : 12;
			unsigned type : 4;
			unsigned read_only : 1;
			unsigned fixed_length : 7;
			unsigned max_length : 7;
			unsigned : 1;
		};
		uint8_t array[4];
	};
	uint8_t *pdata;
	bool changed;
} datapoint_t;

typedef void (* xlink_onDatapointChanged_callback_t)();

extern void xlink_datapoint_init_byte(uint8_t idx, uint8_t *pdata);
extern void xlink_datapoint_init_int16(uint8_t idx, int16_t *pdata);
extern void xlink_datapoint_init_uint16(uint8_t idx, uint16_t *pdata);
extern void xlink_datapoint_init_int32(uint8_t idx, int32_t *pdata);
extern void xlink_datapoint_init_uint32(uint8_t idx, uint32_t *pdata);
extern void xlink_datapoint_init_int64(uint8_t idx, sint64_t *pdata);
extern void xlink_datapoint_init_uint64(uint8_t idx, uint64_t *pdata);
extern void xlink_datapoint_init_float(uint8_t idx, float *pdata);
extern void xlink_datapoint_init_double(uint8_t idx, double *pdata);
extern void xlink_datapoint_init_string(uint8_t idx, uint8_t *pdata, uint8_t len);
extern void xlink_datapoint_init_binary(uint8_t idx, uint8_t *pdata, uint8_t len);
extern void xlink_datapoint_set_changed(uint8_t idx);
extern void xlink_datapoint_update_all();
extern void xlink_datapoint_update_changed();

extern void xlink_array_to_datapoints(const uint8_t *pdata, uint16_t data_length);
extern uint16_t xlink_datapoints_all_to_array(uint8_t *pdata);
extern uint16_t xlink_probe_datapoints_to_array(uint8_t *dp_idx, uint8_t dp_length, uint8_t *pdata);
extern void xlink_setOnDatapointChangedCallback(xlink_onDatapointChanged_callback_t callback);

#endif