/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "app_common.h"
#include "app_device.h"
#include "user_smartconfig.h"
#include "xlink_udp_server.h"
#include "xlink_tcp_client.h"
#include "user_rtc.h"
#include "driver/uart.h"

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#else
#error "The flash map is not supported"
#endif

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 			0x0, 										0x1000},
    { SYSTEM_PARTITION_OTA_1,   			0x1000, 									SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   			SYSTEM_PARTITION_OTA_2_ADDR, 				SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  			SYSTEM_PARTITION_RF_CAL_ADDR, 				0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 			SYSTEM_PARTITION_PHY_DATA_ADDR, 			0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 	SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 	0x3000},
};

void ESPFUNC user_pre_init(void) {
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		app_loge("system_partition_table_regist failed");
		while(1);
	}
    app_logd("partition table registered");
}

void ESPFUNC app_print_reset_cause() {
    struct rst_info *info = system_get_rst_info();
    uint32_t reason = info->reason;
    app_log("Reset reason: %x\n", info->reason);
    if (reason == REASON_WDT_RST || reason == REASON_EXCEPTION_RST || reason == REASON_SOFT_WDT_RST) {
        if (reason == REASON_EXCEPTION_RST) {
            app_log("Fatal exception (%d):\n", info->exccause);
        }
        app_log("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n", 
                    info->epc1, info->epc2, info->epc3, info->excvaddr, info->depc);
    }
}

void ESPFUNC wifi_event_handler_cb(System_Event_t *e) {
    switch (e->event) {
        case EVENT_STAMODE_CONNECTED:
            app_logd("connected to %s", e->event_info.connected.ssid);
            connected_local = true;
            break;
        case EVENT_STAMODE_DISCONNECTED:
            app_logd("disconnected from %s reason: %d", e->event_info.disconnected.ssid, e->event_info.disconnected.reason);
            connected_local = false;
            connected_cloud = false;
            break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:

            break;
        case EVENT_STAMODE_DHCP_TIMEOUT:

            break;
        case EVENT_STAMODE_GOT_IP:
            app_logd("got ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR "", 
                    IP2STR(&e->event_info.got_ip.ip),
                    IP2STR(&e->event_info.got_ip.mask),
                    IP2STR(&e->event_info.got_ip.gw));
            break;
        case EVENT_SOFTAPMODE_STACONNECTED:

            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            break;
        default:
            break;
    }
}

void ESPFUNC app_init() {
    os_delay_us(100000);

    app_device_init();
    
    wifi_set_event_handler_cb(wifi_event_handler_cb);
    wifi_set_opmode(STATION_MODE);
    wifi_station_set_auto_connect(true);
    
    user_rtc_init();
    xlink_udp_server_init();
    xlink_tcp_client_init();

    app_print_reset_cause();
    app_log("ESP8266 SDK version: %s\n", system_get_sdk_version());
    app_log("free heap size: %d", system_get_free_heap_size());
}

void ESPFUNC user_init(void) {
    system_init_done_cb(app_init);
}
