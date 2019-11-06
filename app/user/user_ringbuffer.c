#include "user_ringbuffer.h"
#include "mem.h"

void ESPFUNC user_rb_init(ringbuf_t *p_rb, uint8_t *pbuf, uint32_t size) {
	if(p_rb == NULL || pbuf == NULL || size == 0) {
		return;
	}
	p_rb->pbuf = pbuf;
	p_rb->size = size;
	p_rb->head = 0;
	p_rb->tail = 0;
	p_rb->lock = false;
	os_memset(p_rb->pbuf, 0, p_rb->size);
}

uint32_t ESPFUNC user_rb_unread_size(ringbuf_t *p_rb) {
	if (p_rb == NULL || p_rb->pbuf == NULL || p_rb->size == 0) {
		return 0;
	}
	if(p_rb->head >= p_rb->tail) {
		return p_rb->head - p_rb->tail;
	}
	return p_rb->size + p_rb->head - p_rb->tail;
}

void ESPFUNC user_rb_reset(ringbuf_t *p_rb) {
	if (p_rb == NULL) {
		return;
	}
	p_rb->head = 0;
	p_rb->tail = 0;
	p_rb->lock = false;
	os_memset(p_rb->pbuf, 0, p_rb->size);
}

uint32_t ESPFUNC user_rb_put(ringbuf_t *p_rb, uint8_t *src, uint32_t len) {
	uint32_t i = 0;
	uint32_t sz;
	uint32_t rz;
	if (p_rb == NULL || src == NULL) {
		return 0;
	}
	while (p_rb->lock) {
		i++;
		if (i > 50000) {
			return 0;
		}
	}
	p_rb->lock = true;
	if(p_rb->head >= p_rb->tail) {
		sz = p_rb->size + p_rb->tail - p_rb->head;
		rz = p_rb->size - p_rb->head;
		if(sz > len) {
			sz = len;
		}
		if(rz >= sz) {
			os_memcpy(p_rb->pbuf + p_rb->head, src, sz);
		} else {
			os_memcpy(p_rb->pbuf + p_rb->head, src, rz);
			os_memcpy(p_rb->pbuf, src + rz, sz - rz);
		}
	} else {
		sz = p_rb->tail - p_rb->head;
		if(sz > len) {
			sz = len;
		}
		os_memcpy(p_rb->pbuf + p_rb->head, src, sz);
	}
	p_rb->head += sz;
	if(p_rb->head >= p_rb->size) {
		p_rb->head -= p_rb->size;
	}
	p_rb->lock = false;
	return sz;
}

uint32_t ESPFUNC user_rb_get(ringbuf_t *p_rb, uint8_t *des, uint32_t len) {
	uint32_t i = 0;
	uint32_t sz;
	uint32_t rz;
	if (p_rb == NULL || des == NULL) {
		return 0;
	}
	while (p_rb->lock) {
		i++;
		if (i > 50000) {
			return 0;
		}
	}
	p_rb->lock = true;
	if(p_rb->head >= p_rb->tail) {
		sz = p_rb->head - p_rb->tail;
		if(sz > len) {
			sz = len;
		}
		os_memcpy(des, p_rb->pbuf + p_rb->tail, sz);
	} else {
		sz = p_rb->size + p_rb->head - p_rb->tail;
		rz = p_rb->size - p_rb->tail;
		if(sz > len) {
			sz = len;
		}
		if(rz >= sz) {
			os_memcpy(des, p_rb->pbuf + p_rb->tail, sz);
		} else {
			os_memcpy(des, p_rb->pbuf + p_rb->tail, rz);
			os_memcpy(des + rz, p_rb->pbuf, sz - rz);
		}
	}
	p_rb->tail += sz;
	if(p_rb->tail >= p_rb->size) {
		p_rb->tail -= p_rb->size;
	}
	p_rb->lock = false;
	return sz;
}
