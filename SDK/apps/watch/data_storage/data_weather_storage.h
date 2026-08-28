#ifndef __WEATHER_H__
#define __WEATHER_H__

#include "generic/typedef.h"

/**********************
 *      TYPEDEFS
 **********************/
struct __WEATHER_INFO {
    u8 province_name_len;
    u8 *province;
    u8 city_name_len;
    u8 *city;
    u8 weather;
    s8 temperature;
    u8 humidity;
    u8 wind_direction;
    u8 wind_power;
    u32 update_time;
};

struct weather_single_info {
    u8 weather;
    s8 temperature;
};
//获取简单信息的接口，不耗ram
int ui_small_file_weather_get_singel_info(struct weather_single_info *info, int index);
int ui_small_file_weather_get_count(void);
int ui_small_file_weather_get_size_by_index(int index);
int ui_small_file_weather_read_by_index(void *buf, u32 len, int index);

#endif

