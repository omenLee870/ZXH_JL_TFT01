#include "app_config.h"
#include "asm/cpu.h"
#include "device/device.h"
#include "flashdb.h"
#include "data_storage.h"
#include "os/os_api.h"


#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[NULL-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".storage.data.bss")
#pragma data_seg(".storage.data")
#pragma const_seg(".storage.text.const")
#pragma code_seg(".storage.text")
#endif

#if !TCFG_DATA_STORAGE_ENABLE


int small_file_get_id_table_len(u8 small_file_type)
{
    return 0;
}

int small_file_get_id_table(u8 small_file_type, u8 *table_data, u16 data_len)
{
    return 0;
}

int small_file_delete_by_id(u8 small_file_type, u32 id)
{
    return 0;
}

int small_file_del_all(void)
{
    return false;
}

int small_file_read(u8 small_file_type, u32 id, u32 offset, void *buf, u32 len)
{
    return 0;
}


u32 small_file_write(u8 small_file_type, u32 *id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    return 0;
}

int small_file_update_by_id(u8 small_file_type, u32 id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    return 0;
}

u32 small_file_get_size_by_index(u8 small_file_type, int index)
{
    return 0;
}

u32 small_file_get_size_by_id(u8 small_file_type, int id)
{
    return 0;
}

u32 small_file_get_id_by_index(u8 small_file_type, int index)
{
    return 0;
}

int small_file_get_count(u8 small_file_type)
{
    return 0;
}

int data_small_file_init(void)
{
    return false;;
}


#endif /* if TCFG_DATA_STORAGE_ENABLE */

