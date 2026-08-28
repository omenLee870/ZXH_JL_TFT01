/**
 * @file data_weater_storage.c
 * @brief 电话本存储相关
 */
#include "app_config.h"
#include "flashdb.h"
#include "fdb_low_lvl.h"
#include "data_storage.h"
#include "data_weather_storage.h"

#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[WEATER-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".weater_storage.bss")
#pragma data_seg(".weater_storage.data")
#pragma const_seg(".weater_storage.text.const")
#pragma code_seg(".weater_storage.text")
#endif

#if TCFG_DATA_STORAGE_ENABLE
int ui_small_file_weather_get_count(void)
{
    return small_file_get_count(F_TYPE_WEATHER);
}


int ui_small_file_weather_get_size_by_index(int index)
{
    return small_file_get_size_by_index(F_TYPE_WEATHER, index);
}

int ui_small_file_weather_read_by_index(void *buf, u32 len, int index)
{
    int ret;
    if (!buf) {
        return FALSE;
    }

    u32 id = small_file_get_id_by_index(F_TYPE_WEATHER, index);
    if (!id) {
        return FALSE;
    }

    ret =  small_file_read(F_TYPE_WEATHER, id, 0, buf, len);
    if (ret == len) {
        return TRUE;
    }

    return FALSE;
}
int ui_small_file_weather_get_singel_info(struct weather_single_info *info, int index)
{
    ASSERT(info);
    u32 id = small_file_get_id_by_index(F_TYPE_WEATHER, index);
    if (!id) {
        return FALSE;
    }
    int file_size = small_file_get_size_by_index(F_TYPE_WEATHER, index);
    int offset = file_size - 9;
    if (file_size < 9) {
        return FALSE;
    }
    u8 rbuf[2] = {0};
    int rlen = 2;
    int ret =  small_file_read(F_TYPE_WEATHER, id, offset, rbuf, rlen);
    if (ret == rlen) {
        info->weather = rbuf[0];
        info->temperature = (s8)rbuf[1];
        printf("%s info->weather:%d tmep:%d", __func__, info->weather, info->temperature);
        return TRUE;
    }
    return FALSE;

}


/************************************************
 *                  测试用例
 ***********************************************/
#if 0

u8 test_weather_info_buf[] = {
    0x06, 0xE5, 0xB9, 0xBF, 0xE4, 0xB8, 0x9C, 0x06, 0xE7, 0x8F, 0xA0, 0xE6, 0xB5, 0xB7, 0x03, 0x18,
    0x40, 0x05, 0x06, 0x2D, 0x52, 0xE2, 0x4C,
};

void watch_data_weather_test(void)
{
    u32 create_id = 0;
    small_file_write(F_TYPE_WEATHER, &create_id, 0, &test_weather_info_buf, sizeof(test_weather_info_buf), sizeof(test_weather_info_buf));
    printf("<%s> create_id:%d", __func__, create_id);

    create_id = 0;
    small_file_write(F_TYPE_WEATHER, &create_id, 0, &test_weather_info_buf, sizeof(test_weather_info_buf), sizeof(test_weather_info_buf));
    printf("<%s> create_id:%d", __func__, create_id);

    int file_count = ui_small_file_weather_get_count();
    int file_size;
    u8 *read_weather_info_buf;
    for (int i = 0; i < file_count; i++) {
        file_size = ui_small_file_weather_get_size_by_index(i);
        read_weather_info_buf = zalloc(file_size);
        printf("i:%d file_size:%d", i, file_size);
        ui_small_file_weather_read_by_index(read_weather_info_buf, file_size, i);
        put_buf(read_weather_info_buf, file_size);
        free(read_weather_info_buf);
    }
}

#endif

#else /* if TCFG_DATA_STORAGE_ENABLE */

int ui_small_file_weather_get_count(void)
{
    return 0;
}
int ui_small_file_weather_get_size_by_index(int index)
{
    return 0;
}
int ui_small_file_weather_read_by_index(void *buf, u32 len, int index)
{
    return 0;
}

#endif /* if TCFG_DATA_STORAGE_ENABLE */


