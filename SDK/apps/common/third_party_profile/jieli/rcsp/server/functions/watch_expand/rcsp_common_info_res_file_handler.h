#ifndef __RCSP_COMMON_INFO_RES_FILE_HANDLER_H__
#define __RCSP_COMMON_INFO_RES_FILE_HANDLER_H__

#include "typedef.h"

int rcsp_common_info_get_cur_wallpaper_info(u8 *data, u16 *offset, u16 buf_len);
int common_info_set_cur_res_info(u8 *app_data, u16 app_data_len);
int common_info_get_cur_screen_saver_info(u8 *data, u16 *offset, u16 buf_len);


#endif
