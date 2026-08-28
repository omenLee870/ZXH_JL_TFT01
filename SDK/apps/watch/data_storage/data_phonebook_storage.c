/**
 * @file data_phonebook_storage.c
 * @brief 电话本存储相关
 */
#include "app_config.h"
#include "flashdb.h"
#include "fdb_low_lvl.h"
#include "data_storage.h"
#include "data_phonebook_storage.h"

#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[PBOOK-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".phonebook_storage.bss")
#pragma data_seg(".phonebook_storage.data")
#pragma const_seg(".phonebook_storage.text.const")
#pragma code_seg(".phonebook_storage.text")
#endif

#if TCFG_DATA_STORAGE_ENABLE


int ui_small_file_phonebook_get_count(void)
{
    int phonebook_count;
    u32 file_len = small_file_get_size_by_index(F_TYPE_PHONEBOOK, 0);
    phonebook_count = file_len / SMALL_FILE_PHONEBOOK_SIZE;
    return phonebook_count;
}


int ui_small_file_phonebook_read_by_index(small_file_phonebook_t *phonebook, int index)
{
    int ret;
    if (!phonebook) {
        return false;
    }
    u32 id = small_file_get_id_by_index(F_TYPE_PHONEBOOK, 0);
    if (!id) {
        return false;
    }
    int phonebook_count = ui_small_file_phonebook_get_count();
    if (index >= phonebook_count) {
        return false;
    }

    ret =  small_file_read(F_TYPE_PHONEBOOK, id, index * sizeof(struct small_file_phonebook), phonebook, sizeof(struct small_file_phonebook));

    /*jlui框架显示文字,数字需要用到结束符。避免app传过来，不带结束符。*/
    phonebook->name[PHONEBOOK_NAME_LEN - 1] = 0;
    phonebook->number[PHONEBOOK_NUMBER_LEN - 1] = 0 ;

    if (ret == sizeof(struct small_file_phonebook)) {
        return true;
    }

    return false;
}

/*电话号码除去空格*/
static int phone_number_neaten(const char *in_number, char *out_number)
{
    int out_len = 0;
    for (int i = 0; i < PHONEBOOK_NUMBER_LEN - 1; i++) {
        if (in_number[i] != ' ') {
            out_number[out_len++] = in_number[i];
        }
    }
    out_number[out_len] = 0;
    return out_len;
}

int small_file_phonebook_get_name_by_number(char *name, char *number)
{
    int phonebook_count;
    small_file_phonebook_t phonebook;
    char temp_number[PHONEBOOK_NUMBER_LEN];

    if (!name || !number) {
        return false;
    }

    phonebook_count = ui_small_file_phonebook_get_count();
    if (!phonebook_count)  {
        memcpy(name, "UNKNOW", strlen("UNKNOW") + 1);
        return false;
    }

    for (int i = 0; i < phonebook_count; i++) {
        ui_small_file_phonebook_read_by_index(&phonebook, i);
        phone_number_neaten(phonebook.number, temp_number);
        if (!strcmp(temp_number, number)) {
            memcpy(name, phonebook.name, sizeof(phonebook.name));
            return true;
        }
    }

    memcpy(name, "UNKNOW", strlen("UNKNOW") + 1);
    return false;
}



/************************************************
 *                  测试用例
 ***********************************************/
#if 0

small_file_phonebook_t test_phonebook[] = {
    {"aaaaa", "12345678123"},
    {"张三", "13452341111"},
    {"测试号码", "19507567268"},
    {"你好", "546 2563462"},
    {"ddddd", "04809234"},
    {"李四", "049 852 09348"},
};

void watch_data_phonebook_test(void)
{
    /*不带id创建*/
    /* u32 create_id = 0; */

    /*带id创建*/
    u32 create_id = 0x2198abcd;

    small_file_write(F_TYPE_PHONEBOOK, &create_id, 0, test_phonebook, sizeof(test_phonebook), sizeof(test_phonebook));
    printf("<%s> create_id:%x", __func__, create_id);

    u8 *read_phonebook_buf = zalloc(sizeof(test_phonebook));
    small_file_read(F_TYPE_PHONEBOOK, create_id, 0, read_phonebook_buf, sizeof(test_phonebook));
    put_buf(read_phonebook_buf, sizeof(test_phonebook));

    char name[PHONEBOOK_NAME_LEN] = {0};
    if (small_file_phonebook_get_name_by_number(name, "13452341111")) {
        printf("<%s> find name:%s", __func__, name);
    }

    small_file_delete_by_id(F_TYPE_PHONEBOOK, create_id);
}

#endif

#else /* if TCFG_DATA_STORAGE_ENABLE */

int ui_small_file_phonebook_get_count(void)
{
    return 0;
}
int ui_small_file_phonebook_read_by_index(small_file_phonebook_t *phonebook, int index)
{
    return 0;
}
int small_file_phonebook_get_name_by_number(char *name, char *number)
{
    return 0;
}


#endif /* if TCFG_DATA_STORAGE_ENABLE */

