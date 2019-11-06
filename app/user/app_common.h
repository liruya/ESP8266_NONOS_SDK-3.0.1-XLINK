#ifndef	__APP_COMMON_H__
#define	__APP_COMMON_H__

#include "user_interface.h"
#include "osapi.h"
#include "mem.h"

#include "gpio.h"
#include "driver/gpio16.h"

#define	USE_TX_DEBUG
#define	LOG_DISABLED	0
#define	LOG_LEVEL_E		1			//error
#define	LOG_LEVEL_W		2			//warning
#define	LOG_LEVEL_D		3			//debug
#define	LOG_LEVEL_I		4			//info
#define	LOG_LEVEL_V		5			//verbose
#define	LOG_LEVEL		LOG_LEVEL_V

#define	app_logv(format, ...)
#define	app_logi(format, ...)
#define	app_logd(format, ...)
#define	app_logw(format, ...)
#define	app_loge(format, ...)
#if		(defined(LOG_LEVEL)) && (LOG_LEVEL != LOG_DISABLED)

#define	app_log(format, ...)	os_printf("\n[%s]->[%s] @%4d: " format "\n\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
//logv
#if		LOG_LEVEL >= LOG_LEVEL_V
#undef	app_logv
#define	app_logv	app_log
#endif
//logi
#if		LOG_LEVEL >= LOG_LEVEL_I
#undef	app_logi
#define	app_logi	app_log
#endif
//logd
#if		LOG_LEVEL >= LOG_LEVEL_D
#undef	app_logd
#define	app_logd	app_log
#endif
//logd
#if		LOG_LEVEL >= LOG_LEVEL_W
#undef	app_logw
#define	app_logw	app_log
#endif
//loge
#if		LOG_LEVEL >= LOG_LEVEL_E
#undef	app_loge
#define	app_loge	app_log
#endif

#endif

// #ifdef	USE_TX_DEBUG
// #define	app_printf(format, ...)	os_printf("\n[%s]->[%s] @%4d: " format "\n\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #else
// #define	app_printf(format, ...)
// #endif

#define	XFUNCTION				ICACHE_FLASH_ATTR
#define	ESPFUNC					ICACHE_FLASH_ATTR

#define	GPIO_OUTPUT_GET(pin)	((GPIO_REG_READ(GPIO_OUT_ADDRESS)>>(pin))&BIT0)
#define	GPIO16_OUTPUT_GET()		(READ_PERI_REG(RTC_GPIO_OUT)&(uint32)0x00000001)

#define	gpio_high(pin)			if (pin == 16) {\
									gpio16_output_set(1);\
								} else {\
									GPIO_OUTPUT_SET(pin, 1);\
								}
#define	gpio_low(pin)			if (pin == 16) {\
									gpio16_output_set(0);\
								} else {\
									GPIO_OUTPUT_SET(pin, 0);\
								}
#define	gpio_toggle(pin)		if (pin == 16) {\
									gpio16_output_set(!GPIO16_OUTPUT_GET());\
								} else {\
									GPIO_OUTPUT_SET(pin, !GPIO_OUTPUT_GET(pin));\
								}								
// #define	gpio_high(pin)			(GPIO_OUTPUT_SET(pin, 1))
// #define	gpio_low(pin)			(GPIO_OUTPUT_SET(pin, 0))
// #define	gpio_toggle(pin)		(GPIO_OUTPUT_SET(pin, !GPIO_OUTPUT_GET(pin)))

#endif