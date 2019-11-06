#include "xlink_datapoint.h"
#include "xlink.h"

#define DP_BUFFER_SIZE		2048

LOCAL uint8_t dp_buffer[DP_BUFFER_SIZE];

LOCAL datapoint_t datapoints[DATAPOINT_MAX_NUM];
xlink_onDatapointChanged_callback_t xlink_onDatapointChanged_cb;

LOCAL bool ESPFUNC xlink_datapoint_check(datapoint_t *pdp) {
	if (pdp == NULL || pdp->pdata == NULL) {
		return false;
	}
	uint16_t len = pdp->length;
	switch (pdp->type) {
		case DP_TYPE_BYTE:
			if (len == 1) {
				return true;
			}
			break;
		case DP_TYPE_INT16:
		case DP_TYPE_UINT16:
			if (len == 2) {
				return true;
			}
			break;
		case DP_TYPE_INT32:
		case DP_TYPE_UINT32:
		case DP_TYPE_FLOAT:
			if (len == 4) {
				return true;
			}
			break;
		case DP_TYPE_INT64:
		case DP_TYPE_UINT64:
		case DP_TYPE_DOUBLE:
			if (len == 8) {
				return true;
			}
			break;
		case DP_TYPE_STRING:
			if (len <= DATAPOINT_STR_MAX_LEN) {
				return true;
			}
			break;
		case DP_TYPE_BINARY:
			if (len <= DATAPOINT_BIN_MAX_LEN) {
				return true;
			}
			break;
		default:
			break;
	}
	return false;
}

void ESPFUNC xlink_datapoint_init_byte(uint8_t idx, uint8_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_BYTE;
	datapoints[idx].length = 1;
	datapoints[idx].pdata = pdata;
}

void ESPFUNC xlink_datapoint_init_int16(uint8_t idx, int16_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_INT16;
	datapoints[idx].length = 2;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_uint16(uint8_t idx, uint16_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_UINT16;
	datapoints[idx].length = 2;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_int32(uint8_t idx, int32_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_INT32;
	datapoints[idx].length = 4;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_uint32(uint8_t idx, uint32_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_UINT32;
	datapoints[idx].length = 4;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_int64(uint8_t idx, sint64_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_INT64;
	datapoints[idx].length = 8;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_uint64(uint8_t idx, uint64_t *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_UINT64;
	datapoints[idx].length = 8;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_float(uint8_t idx, float *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_FLOAT;
	datapoints[idx].length = 4;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_double(uint8_t idx, double *pdata) {
	if(idx >= DATAPOINT_MAX_NUM) {
		return;
	}
	datapoints[idx].type = DP_TYPE_DOUBLE;
	datapoints[idx].length = 8;
	datapoints[idx].pdata = (uint8_t *) pdata;
}

void ESPFUNC xlink_datapoint_init_string(uint8_t idx, uint8_t *pdata, uint8_t len) {
	if(idx >= DATAPOINT_MAX_NUM || len > DATAPOINT_STR_MAX_LEN) {
		return;
	}
	datapoints[idx].type = DP_TYPE_STRING;
	datapoints[idx].length = len;
	datapoints[idx].pdata = pdata;
}

void ESPFUNC xlink_datapoint_init_binary(uint8_t idx, uint8_t *pdata, uint8_t len) {
	if(idx >= DATAPOINT_MAX_NUM || len > DATAPOINT_BIN_MAX_LEN) {
		return;
	}
	datapoints[idx].type = DP_TYPE_BINARY;
	datapoints[idx].length = len;
	datapoints[idx].pdata = pdata;
}

void ESPFUNC xlink_datapoint_set_changed(uint8_t idx) {
	if (idx < DATAPOINT_MAX_NUM) {
		if (xlink_datapoint_check(&datapoints[idx])) {
			datapoints[idx].changed = true;
		}
	}
}

uint16_t ESPFUNC xlink_probe_datapoints_to_array(uint8_t *dp_idx, uint8_t dp_length, uint8_t *pdata) {
	if(pdata == NULL) {
		return 0;
	}
	uint8_t i, j, k;
	uint8_t idx = 0;
	datapoint_t *pdp = NULL;
	for(i = 0; i < dp_length; i++) {
		j = dp_idx[i];
		if(j < DATAPOINT_MAX_NUM) {
			pdp = &datapoints[j];
			if(xlink_datapoint_check(pdp)) {
				pdata[idx++] = j;
				pdata[idx++] = pdp->array[1];
				pdata[idx++] = pdp->array[0];
				switch(pdp->type) {
					case DP_TYPE_BYTE:
					case DP_TYPE_INT16:
					case DP_TYPE_UINT16:
					case DP_TYPE_INT32:
					case DP_TYPE_UINT32:
					case DP_TYPE_INT64:
					case DP_TYPE_UINT64:
					case DP_TYPE_FLOAT:
					case DP_TYPE_DOUBLE:
						for (k = pdp->length; k > 0; k--) {
							pdata[idx++] = pdp->pdata[k-1];
						}
						break;
					case DP_TYPE_STRING:
					case DP_TYPE_BINARY:
						os_memcpy(pdata + idx, pdp->pdata, pdp->length);
						idx += pdp->length;
						break;
					default:
						break;
				}
			}
		}
	}
	return idx;
}

LOCAL uint16_t ESPFUNC xlink_datapoints_to_array(uint8_t *pdata, bool only_changed) {
	if(pdata == NULL) {
		return 0;
	}
	uint8_t i, j;
	uint16_t idx = 0;
	datapoint_t *pdp = NULL;
	for (i = 0; i < DATAPOINT_MAX_NUM; i++) {
		pdp = &datapoints[i];
		if(xlink_datapoint_check(pdp) == false) {
			continue;
		}
		if(only_changed && pdp->changed == false) {
			continue;
		}
		app_logv("idx: %d  typ: %d  len: %d", i, pdp->type, pdp->length);
		pdp->changed = false;
		pdata[idx++] = i;
		pdata[idx++] = pdp->array[1];
		pdata[idx++] = pdp->array[0];
		switch (pdp->type) {
			case DP_TYPE_BYTE:
			case DP_TYPE_INT16:
			case DP_TYPE_UINT16:
			case DP_TYPE_INT32:
			case DP_TYPE_UINT32:
			case DP_TYPE_INT64:
			case DP_TYPE_UINT64:
			case DP_TYPE_FLOAT:
			case DP_TYPE_DOUBLE:
				for (j = pdp->length; j > 0; j--) {
					pdata[idx++] = pdp->pdata[j-1];
				}
				break;
			case DP_TYPE_STRING:
			case DP_TYPE_BINARY:
				os_memcpy(pdata + idx, pdp->pdata, pdp->length);
				idx += pdp->length;
				break;
			default:
				break;
		}
	}
	app_logd("upload data length: %d", idx);
	return idx;
}

uint16_t ESPFUNC xlink_datapoints_all_to_array(uint8_t *pdata) {
	return xlink_datapoints_to_array(pdata, false);
	// if(pdata == NULL) {
	// 	return 0;
	// }
	// uint8_t i, j;
	// uint16_t idx = 0;
	// datapoint_t *pdp = NULL;
	// for (i = 0; i < DATAPOINT_MAX_NUM; i++) {
	// 	pdp = &datapoints[i];
	// 	if (xlink_datapoint_check(pdp)) {
	// 		pdata[idx++] = i;
	// 		pdata[idx++] = pdp->array[1];
	// 		pdata[idx++] = pdp->array[0];
	// 		switch (pdp->type) {
	// 			case DP_TYPE_BYTE:
	// 			case DP_TYPE_INT16:
	// 			case DP_TYPE_UINT16:
	// 			case DP_TYPE_INT32:
	// 			case DP_TYPE_UINT32:
	// 			case DP_TYPE_INT64:
	// 			case DP_TYPE_UINT64:
	// 			case DP_TYPE_FLOAT:
	// 			case DP_TYPE_DOUBLE:
	// 				for (j = pdp->length; j > 0; j--) {
	// 					pdata[idx++] = pdp->pdata[j-1];
	// 				}
	// 				break;
	// 			case DP_TYPE_STRING:
	// 			case DP_TYPE_BINARY:
	// 				os_memcpy(pdata + idx, pdp->pdata, pdp->length);
	// 				idx += pdp->length;
	// 				break;
	// 			default:
	// 				break;
	// 		}
	// 	}
	// }
	// app_logd("upload data length: %d", idx);
	// return idx;
}

uint16_t ESPFUNC xlink_datapoints_changed_to_array(uint8_t *pdata) {
	return xlink_datapoints_to_array(pdata, true);
	// if(pdata == NULL) {
	// 	return 0;
	// }
	// uint8_t i, j;
	// uint16_t idx = 0;
	// datapoint_t *pdp = NULL;
	// for (i = 0; i < DATAPOINT_MAX_NUM; i++) {
	// 	pdp = &datapoints[i];
	// 	if (xlink_datapoint_check(pdp) && pdp->changed) {
	// 		app_logv("idx: %d  typ: %d  len: %d", i, pdp->type, pdp->length);
	// 		pdp->changed = false;
	// 		pdata[idx++] = i;
	// 		pdata[idx++] = pdp->array[1];
	// 		pdata[idx++] = pdp->array[0];
	// 		switch (pdp->type) {
	// 			case DP_TYPE_BYTE:
	// 			case DP_TYPE_INT16:
	// 			case DP_TYPE_UINT16:
	// 			case DP_TYPE_INT32:
	// 			case DP_TYPE_UINT32:
	// 			case DP_TYPE_INT64:
	// 			case DP_TYPE_UINT64:
	// 			case DP_TYPE_FLOAT:
	// 			case DP_TYPE_DOUBLE:
	// 				for (j = pdp->length; j > 0; j--) {
	// 					pdata[idx++] = pdp->pdata[j-1];
	// 				}
	// 				break;
	// 			case DP_TYPE_STRING:
	// 			case DP_TYPE_BINARY:
	// 				os_memcpy(pdata + idx, pdp->pdata, pdp->length);
	// 				idx += pdp->length;
	// 				break;
	// 			default:
	// 				break;
	// 		}
	// 	}
	// }
	// app_logd("upload data length: %d", idx);
	// return idx;
}

void ESPFUNC xlink_array_to_datapoints(const uint8_t *pdata, uint16_t data_length) {
	if (pdata == NULL || data_length < 4) {
		return;
	}
	bool flag = false;
	uint16_t i;
	uint8_t j;
	uint8_t idx;
	uint8_t type;
	uint16_t len;
	datapoint_t *pdp = NULL;
	for (i = 0; i < data_length;) {
		idx = pdata[i];
		if(idx >= DATAPOINT_MAX_NUM || data_length - i < 4) {
			break;
		}
		type = pdata[i+1]>>4;
		len = ((pdata[i+1]&0x0F)<<8)|pdata[i+2];
		pdp = &datapoints[idx];
		if (xlink_datapoint_check(pdp) == false || type != pdp->type) {
			break;
		}
		pdp->changed = true;
		switch (type) {
			case DP_TYPE_BYTE:
			case DP_TYPE_INT16:
			case DP_TYPE_UINT16:
			case DP_TYPE_INT32:
			case DP_TYPE_UINT32:
			case DP_TYPE_INT64:
			case DP_TYPE_UINT64:
			case DP_TYPE_FLOAT:
			case DP_TYPE_DOUBLE:
				if (len == pdp->length) {
					for (j = 0; j < len; j++) {
						pdp->pdata[j] = pdata[i+2+len-j];
					}
					flag = true;
				}
				i += 3 + len;
				break;
			case DP_TYPE_STRING:
				if (len > 0 && len <= DATAPOINT_STR_MAX_LEN) {
					pdp->length = len;
					os_memcpy(pdp->pdata, &pdata[3+i], len);
					flag = true;
				}
				i += 3 + len;
				break;
			case DP_TYPE_BINARY:
				if (len > 0 && len <= DATAPOINT_BIN_MAX_LEN) {
					pdp->length = len;
					os_memcpy(pdp->pdata, &pdata[3+i], len);
					flag = true;
				}
				i += 3 + len;
				break;
			default:
				break;
		}
	}
	if (flag && xlink_onDatapointChanged_cb != NULL) {
		xlink_onDatapointChanged_cb();
	}
}

void ESPFUNC xlink_datapoint_update_all() {
	uint8_t *pdata = dp_buffer;
	uint16_t dp_length = xlink_datapoints_all_to_array(dp_buffer);
	xlink_update_datapoint_with_alarm((const uint8_t **)&pdata, dp_length);
}

void ESPFUNC xlink_datapoint_update_changed() {
	uint8_t *pdata = dp_buffer;
	uint16_t dp_length = xlink_datapoints_changed_to_array(dp_buffer);
	xlink_update_datapoint_with_alarm((const uint8_t **)&pdata, dp_length);
}

void ESPFUNC xlink_setOnDatapointChangedCallback(xlink_onDatapointChanged_callback_t callback) {
	xlink_onDatapointChanged_cb = callback;
}
