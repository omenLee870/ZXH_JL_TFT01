#ifndef __RCSP_SPORT_PRESSURE_DETECTION_H__
#define __RCSP_SPORT_PRESSURE_DETECTION_H__

#include "typedef.h"
#include "app_config.h"

void sport_info_pressure_detection_attr_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

u32 sport_info_pressure_detection_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset);


#endif
