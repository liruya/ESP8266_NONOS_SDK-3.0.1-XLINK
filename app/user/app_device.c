#include "app_device.h"
#include "xlink.h"
#include "espconn.h"
#include "user_apconfig.h"
#include "app_test.h"
#include "user_device.h"
#include "xlink_datapoint.h"

#if		PRDT_TYPE == PRDT_TYPE_EXO_STRIP

#include "app_board_led.h"
#include "user_led.h"

/**
 * APP 在Makefile中 -DAPP=$(app) 
 * app为gen_misc.sh 和 gen_misc2.sh中定义 分别用于生成user1.bin 和 user2.bin
 * */
// #if	(LED_FIRMWARE_VERSION%2 == 0 && APP == 1) || (LED_FIRMWARE_VERSION%2 != 0 && APP == 2)
// #error	"firmware version confict with user bin..."
// #endif

LOCAL user_device_t * const pdev = &user_dev_led;

#elif	PRDT_TYPE == PRDT_TYPE_EXO_SOCKET

#include "app_board_socket.h"
#include "user_socket.h"

/**
 * APP 在Makefile中 -DAPP=$(app) 
 * app为gen_misc.sh 和 gen_misc2.sh中定义 分别用于生成user1.bin 和 user2.bin
 * */
// #if	(SOCKET_FIRMWARE_VERSION%2 == 0 && APP == 1) || (SOCKET_FIRMWARE_VERSION%2 != 0 && APP == 2)
// #error	"firmware version confict with user bin..."
// #endif

LOCAL user_device_t * const pdev = &user_dev_socket;

#elif	PRDT_TYPE == PRDT_TYPE_EXO_MONSOON

#include "app_board_monsoon.h"
#include "user_monsoon.h"

/**
 * APP 在Makefile中 -DAPP=$(app) 
 * app为gen_misc.sh 和 gen_misc2.sh中定义 分别用于生成user1.bin 和 user2.bin
 * */
// #if	(MONSOON_FIRMWARE_VERSION%2 == 0 && APP == 1) || (MONSOON_FIRMWARE_VERSION%2 != 0 && APP == 2)
// #error	"firmware version confict with user bin..."
// #endif

LOCAL user_device_t * const pdev = &user_dev_monsoon;

#else
#error	"Please define PRDT_TYPE ..."
#endif

void ESPFUNC app_device_init() {
	pdev->board_init();
	if (user_device_poweron_check(pdev)) {
		xlink_reset_config();
		system_restore();

		app_test_init(pdev);
	} else {
		user_device_init(pdev);
		xlink_init(pdev);
	}
}
