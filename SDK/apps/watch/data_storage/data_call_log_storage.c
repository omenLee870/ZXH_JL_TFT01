/**
 * @file data_call_log_storage.c
 * @brief 通话记录存储实现
 */
#include "app_config.h"
#include "sys_time.h"
#include "rtc.h"
#include "timestamp.h"
#include "data_storage.h"

#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[CALL-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".call_log_storage.data.bss")
#pragma data_seg(".call_log_storage.data")
#pragma const_seg(".call_log_storage.text.const")
#pragma code_seg(".call_log_storage.text")
#endif

#if TCFG_DATA_STORAGE_ENABLE

/**********************
 *  STATIC PROTOTYPES
 **********************/
static small_file_call_log_t curr_call_log;


void small_file_call_log_set_name(char *name, int name_len)
{
    int len;
    if (!name || !name_len) {
        return;
    }

    len = (name_len < CALL_LOG_NAME_LEN) ? name_len : CALL_LOG_NAME_LEN;
    memcpy(curr_call_log.name, name, len);
    curr_call_log.name[len - 1] = '\0';
}

void small_file_call_log_set_number(char *number, int number_len)
{
    int len;
    if (!number || !number_len) {
        return;
    }

    memset(curr_call_log.number, 0, CALL_LOG_NUMBER_LEN);
    len = (number_len < CALL_LOG_NUMBER_LEN) ? number_len : CALL_LOG_NUMBER_LEN;
    memcpy(curr_call_log.number, number, len);
    curr_call_log.number[len - 1] = '\0';
}

void small_file_call_log_set_date(void)
{
    struct sys_time time;
    rtc_read_time(&time);
    curr_call_log.utc_time = timestamp_mytime_2_utc_sec(&time);
}

u32 small_file_call_log_get_date(void)
{
    return curr_call_log.utc_time;
}

void small_file_call_log_set_type(enum CALL_TYPE type)
{
    curr_call_log.type = type;
}

void small_file_call_log_set_sel(enum CALL_SEL call_sel)
{
    curr_call_log.sel = call_sel;
}

int small_file_call_log_save(void)
{
#if TCFG_DATA_STORAGE_VM_ENABLE
    /*通话记录使用一个vm id, 这里额外处理*/
    u32 id = small_file_get_id_by_index(F_TYPE_CALL_LOG, 0);
    u8 *tmp_buf = NULL;
    int ret = true;

    if (id != 0) {
        u32 cur_file_size = small_file_get_size_by_id(F_TYPE_CALL_LOG, id);
        tmp_buf = zalloc(cur_file_size + SMALL_FILE_CALL_LOG_SIZE);
        ret = small_file_read(F_TYPE_CALL_LOG, id, 0, tmp_buf, cur_file_size);
        if (ret != cur_file_size) {
            log_error("<%s> line:%d", __func__, __LINE__);
            ret = false;
            goto __end;
        }

        memcpy(tmp_buf + cur_file_size, &curr_call_log, SMALL_FILE_CALL_LOG_SIZE);

        ret = small_file_write(F_TYPE_CALL_LOG, &id, 0, tmp_buf, (cur_file_size + SMALL_FILE_CALL_LOG_SIZE), (cur_file_size + SMALL_FILE_CALL_LOG_SIZE));
        memset(&curr_call_log, 0, SMALL_FILE_CALL_LOG_SIZE);
        if (ret != (cur_file_size + SMALL_FILE_CALL_LOG_SIZE)) {
            log_error("<%s> line:%d", __func__, __LINE__);
            ret = false;
            goto __end;
        }
    } else {
        ret = small_file_write(F_TYPE_CALL_LOG, &id, 0, &curr_call_log, SMALL_FILE_CALL_LOG_SIZE, SMALL_FILE_CALL_LOG_SIZE);
        memset(&curr_call_log, 0, SMALL_FILE_CALL_LOG_SIZE);
        if (ret != SMALL_FILE_CALL_LOG_SIZE) {
            log_error("<%s> line:%d", __func__, __LINE__);
            ret = false;
            goto __end;
        }
    }

__end:
    if (tmp_buf) {
        free(tmp_buf);
    }
    return ret;
#else
    u32 id = 0;
    u32 ret = small_file_write(F_TYPE_CALL_LOG, &id, 0, &curr_call_log, SMALL_FILE_CALL_LOG_SIZE, SMALL_FILE_CALL_LOG_SIZE);
    memset(&curr_call_log, 0, SMALL_FILE_CALL_LOG_SIZE);
    if (ret != SMALL_FILE_CALL_LOG_SIZE) {
        return false;
    }
    return true;
#endif
}


int ui_small_file_call_log_read_by_index(small_file_call_log_t *call_log, int index)
{
#if TCFG_DATA_STORAGE_VM_ENABLE
    /*通话记录使用一个vm id, 这里额外处理*/
    int ret;
    if (!call_log) {
        return false;
    }
    u32 id = small_file_get_id_by_index(F_TYPE_CALL_LOG, 0);
    if (!id) {
        return false;
    }

    u32 size = small_file_get_size_by_id(F_TYPE_CALL_LOG, id);
    u32 count = size / SMALL_FILE_CALL_LOG_SIZE;

    if (index + 1 > count) {
        return false;
    }

    ret = small_file_read(F_TYPE_CALL_LOG, id, index * SMALL_FILE_CALL_LOG_SIZE, call_log, SMALL_FILE_CALL_LOG_SIZE);

    if (ret == SMALL_FILE_CALL_LOG_SIZE) {
        return true;
    }
    log_error("<%s> line:%d", __func__, __LINE__);
    return false;

#else
    int ret;
    if (!call_log) {
        return false;
    }
    u32 id = small_file_get_id_by_index(F_TYPE_CALL_LOG, index);
    if (!id) {
        return false;
    }

    ret =  small_file_read(F_TYPE_CALL_LOG, id, 0, call_log, sizeof(small_file_call_log_t));
    if (ret == sizeof(small_file_call_log_t)) {
        return true;
    }
    return false;
#endif
}


int ui_small_file_call_log_get_count(void)
{
#if TCFG_DATA_STORAGE_VM_ENABLE
    /*通话记录使用一个vm id, 这里额外处理*/
    u32 id = small_file_get_id_by_index(F_TYPE_CALL_LOG, 0);
    if (!id) {
        return 0;
    }
    u32 size = small_file_get_size_by_id(F_TYPE_CALL_LOG, id);
    return size / SMALL_FILE_CALL_LOG_SIZE;
#else
    return small_file_get_count(F_TYPE_CALL_LOG);
#endif
}



/************************************************
 *                  测试用例
 ***********************************************/
#if 0
void watch_data_call_log_test(void)
{
    printf("%s %d\n", __FUNCTION__, __LINE__);
    small_file_call_log_t call_log;
    int ret;
    char name[CALL_LOG_DATE_LEN] = "张三李四";
    char number[CALL_LOG_NUMBER_LEN] = "123456789123";

    for (int i = 0; i < 10; i++) {
        small_file_call_log_set_name(name, sizeof(name));
        small_file_call_log_set_number(number, sizeof(number));
        small_file_call_log_set_date();
        small_file_call_log_set_type(1);
        small_file_call_log_set_sel(1);
        small_file_call_log_save();
        printf("count %d\n", ui_small_file_call_log_get_count());
        void os_time_dly(int tick);
        os_time_dly(100);
    }

    for (int i = 0; i < 10; i++) {
        ret = ui_small_file_call_log_read_by_index(&call_log, i);
        printf("i:%d ret:%d", i, ret);
        printf("call_log.date:%d", call_log.utc_time);
        printf("call_log.name:%s", call_log.name);
        printf("call_log.number:%s", call_log.number);
    }

    char name_1[CALL_LOG_DATE_LEN] = {0};
    char number_1[CALL_LOG_NUMBER_LEN] = "98 654 21";
    for (int i = 0; i < 5; i++) {
        small_file_call_log_set_name(name_1, sizeof(name_1));
        small_file_call_log_set_number(number_1, sizeof(number_1));
        small_file_call_log_set_date();
        small_file_call_log_set_type(1);
        small_file_call_log_set_sel(1);
        small_file_call_log_save();
        printf("count %d\n", ui_small_file_call_log_get_count());
        void os_time_dly(int tick);
        os_time_dly(100);
    }

    for (int i = 0; i < 10; i++) {
        ret = ui_small_file_call_log_read_by_index(&call_log, i);
        printf("i:%d ret:%d", i, ret);
        printf("call_log.date:%d", call_log.utc_time);
        printf("call_log.name:%s", call_log.name);
        printf("call_log.number:%s", call_log.number);
    }

    printf("%s %d\n", __FUNCTION__, __LINE__);

    ASSERT(ret);
}
#endif

#else /* if TCFG_DATA_STORAGE_ENABLE */

void small_file_call_log_set_sel(enum CALL_SEL call_sel)
{
}

int ui_small_file_call_log_read_by_index(small_file_call_log_t *call_log, int index)
{
    return 0;
}
int ui_small_file_call_log_get_count(void)
{
    return 0;
}

void small_file_call_log_set_type(enum CALL_TYPE type)
{
}


#endif /* if TCFG_DATA_STORAGE_ENABLE */

