#include "xlink_config.h"

#define SPI_FLASH_SECTOR_SIZE       4096
/*need  to define for the User Data section*/
/* 2MBytes  512KB +  512KB  0x7C */
/* 2MBytes 1024KB + 1024KB  0xFC */
#define PRIV_PARAM_START_SECTOR     0x7C

#define XLINK_CONFIG_BUFFER_SIZE		512
#define USER_PARA_BUFFER_SIZE			2048

/**
 * 必须4字节对齐
 **/ 
typedef struct {
	uint8_t auth_pwd_name_buf[XLINK_CONFIG_BUFFER_SIZE];
	uint32_t sw_version;
	uint8_t user_para_buf[USER_PARA_BUFFER_SIZE];
} xlink_para_t;

LOCAL xlink_para_t xlink_para;

LOCAL bool XLINK_FUNCTION xlink_write() {
	// spi_flash_erase_sector(PRIV_PARAM_START_SECTOR);
	// spi_flash_write(PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof(xlink_para));
	return system_param_save_with_protect(PRIV_PARAM_START_SECTOR, &xlink_para, sizeof(xlink_para_t));
}

LOCAL bool XLINK_FUNCTION xlink_read() {
	// spi_flash_read(PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof(xlink_para));
	return system_param_load(PRIV_PARAM_START_SECTOR, 0, &xlink_para, sizeof(xlink_para_t));
}

int32_t XLINK_FUNCTION xlink_write_config(uint8_t *data, uint32_t len) {
	if(len > XLINK_CONFIG_BUFFER_SIZE) {
		return -2;
	}
	os_memcpy(xlink_para.auth_pwd_name_buf, data, len);
	if (xlink_write()) {
		return len;
	}
	return -1;
}

int32_t XLINK_FUNCTION xlink_read_config(uint8_t *data, uint32_t len) {
	if(len > XLINK_CONFIG_BUFFER_SIZE) {
		return -2;
	}
	if (xlink_read()) {
		os_memcpy(data, xlink_para.auth_pwd_name_buf, len);
		return len;
	}
	return -1;
}

bool XLINK_FUNCTION xlink_write_version(uint16_t version) {
	xlink_para.sw_version = version;
	return xlink_write();
}

uint16_t XLINK_FUNCTION xlink_read_version() {
	if (xlink_read()) {
		return xlink_para.sw_version;
	}
	return 0;
}

bool XLINK_FUNCTION xlink_write_user_para(uint8_t *data, uint32_t len) {
	if(len > USER_PARA_BUFFER_SIZE) {
		return false;
	}
	os_memcpy(xlink_para.user_para_buf, data, len);
	return xlink_write();
}

bool XLINK_FUNCTION xlink_read_user_para(uint8_t *data, uint32_t len) {
	if(len > USER_PARA_BUFFER_SIZE) {
		return false;
	}
	if (xlink_read()) {
		os_memcpy(data, xlink_para.user_para_buf, len);
		return true;
	}
	return false;
}