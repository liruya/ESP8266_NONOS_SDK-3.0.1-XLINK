#ifndef	__USER_RINGBUFFER_H__
#define	__USER_RINGBUFFER_H__

#include "osapi.h"
#include "app_common.h"

typedef struct _ringbuf	ringbuf_t;

struct _ringbuf {
	uint8_t *pbuf;
	uint32_t head;
	uint32_t tail;
	uint32_t size;
	bool lock;
};

extern void user_rb_init(ringbuf_t *p_rb, uint8_t *pbuf, uint32_t size);
extern void user_rb_reset(ringbuf_t *p_rb);
extern uint32_t user_rb_unread_size(ringbuf_t *p_rb);
extern uint32_t user_rb_put(ringbuf_t *p_rb, uint8_t *src, uint32_t len);
extern uint32_t user_rb_get(ringbuf_t *p_rb, uint8_t *des, uint32_t len);

#endif