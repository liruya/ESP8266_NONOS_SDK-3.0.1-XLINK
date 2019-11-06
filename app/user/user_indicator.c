#include "user_indicator.h"

typedef struct {
	uint32_t period;
	uint32_t flash_cnt;
	void (* toggle)();

	uint32_t count;
	os_timer_t timer;
} indicator_t;

LOCAL indicator_t *pindicator;

LOCAL void ESPFUNC user_indicator_flash_cb(void *arg) {
	// indicator_t **ppind = arg;
	// if (ppind != NULL) {
	// 	indicator_t *pind = *ppind;
	// 	if (pind != NULL) {
	// 		if (pind->flash_cnt > 0) {
	// 			pind->count++;
	// 			if (pind->count > pind->flash_cnt*2) {
	// 				os_timer_disarm(&pind->timer);
	// 				os_free(*ppind);
	// 				*ppind = NULL;
	// 				return;
	// 			}
	// 		}
	// 		if (pindicator->toggle != NULL) {
	// 			pindicator->toggle();
	// 		}
	// 	}
	// }
	if (pindicator != NULL) {
		if (pindicator->flash_cnt > 0) {
			pindicator->count++;
			if (pindicator->count > pindicator->flash_cnt*2) {
				os_timer_disarm(&pindicator->timer);
				os_free(pindicator);
				pindicator = NULL;
				return;
			}
		}
		if (pindicator->toggle != NULL) {
			pindicator->toggle();
		}
	}
}

void ESPFUNC user_indicator_start(const uint32_t period, const uint32_t count, void (*const toggle)()) {
	user_indicator_stop();
	pindicator = os_zalloc(sizeof(indicator_t));
	if (pindicator == NULL) {
		app_loge("pindicator start failed -> malloc failed");
		return;
	}
	pindicator->period = period;
	pindicator->flash_cnt = count;
	pindicator->toggle = toggle;

	os_timer_disarm(&pindicator->timer);
	os_timer_setfn(&pindicator->timer, user_indicator_flash_cb, &pindicator);
	os_timer_arm(&pindicator->timer, pindicator->period, 1);
}

void ESPFUNC user_indicator_stop() {
	if (pindicator != NULL) {
		os_timer_disarm(&pindicator->timer);
		os_free(pindicator);
		pindicator = NULL;
	}
}
