#ifndef __RCSP_SPORT_RAISE_WRIST_H__
#define __RCSP_SPORT_RAISE_WRIST_H__

#include "typedef.h"

void sport_info_raise_wrist_attr_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

u32 sport_info_raise_wrist_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset);

#endif
