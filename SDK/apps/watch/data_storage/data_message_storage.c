/**
 * @file data_message_storage.c
 * @brief 消息存储实现
 */
#include "app_config.h"
#include "sys_time.h"
#include "flashdb.h"
#include "data_storage.h"

#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[MSG-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".message_storage.data.bss")
#pragma data_seg(".message_storage.data")
#pragma const_seg(".message_storage.text.const")
#pragma code_seg(".message_storage.text")
#endif

#if TCFG_DATA_STORAGE_ENABLE


#define WRITE_BIG_U32(a,src)   {*((u8*)(a)+0) = (u8)((src)>>24);  *((u8*)(a)+1) = (u8)(((src)>>16)&0xff);*((u8*)(a)+2) = (u8)(((src)>>8)&0xff);*((u8*)(a)+3) = (u8)((src)&0xff);}

static u32 ancs_message_uid;


int ui_small_file_message_get_count(void)
{
    return small_file_get_count(F_TYPE_MESSAGE);
}




int small_file_message_read_by_index(small_file_message_t *message, int index)
{
    /* struct fdb_kv_iterator iterator; */
    /* fdb_kv_t cur_kv; */
    /* struct fdb_blob blob; */
    /* u32 data_size; */
    u8 *data_buf;
    /* int count = 0; */
    u8 T;
    u8 *V;
    u16 L;
    u16 offset = 0;
    u32 copy_len;

    if (!message) {
        return false;
    }



    u32 id = small_file_get_id_by_index(F_TYPE_MESSAGE, index);
    if (!id) {
        return false;
    }

    data_buf = zalloc(512);
    if (!data_buf) {
        return false;
    }


    int  ret =  small_file_read(F_TYPE_MESSAGE, id, 0, data_buf, 512);
    if (!ret) {
        ASSERT(0);
    }
    /* put_buf(data_buf, ret); */

    while (offset < ret) {
        L = (data_buf[offset] << 8) | data_buf[offset + 1];
        T = data_buf[offset + 2];
        V = &data_buf[offset + 3];
        if (T == MESSAGE_TIMESTAMP_TYPE) {
            copy_len = (L - 1) > MESSAGE_TIMESTAMP_LEN ? MESSAGE_TIMESTAMP_LEN : (L - 1);
            memcpy(&message->timestamp, V, copy_len);
            /* printf("TimeStamp:0x%x",message->timestamp); */
        } else if (T == MESSAGE_PACKAGENAME_TYPE) {
            copy_len = (L - 1) > MESSAGE_PACKAGENAME_LEN ? MESSAGE_PACKAGENAME_LEN : (L - 1);
            memset(message->packagename, 0, MESSAGE_PACKAGENAME_LEN);
            memcpy(message->packagename, V, copy_len);
            /* log_info("packagename:%s", temp_message_buf->packagename); */
        } else if (T == MESSAGE_APP_IDENTIFIER_TYPE) {
            copy_len = (L - 1) > MESSAGE_APP_IDENTIFIER_LEN ? MESSAGE_APP_IDENTIFIER_LEN : (L - 1);
            memcpy(&message->app_identifier, V, copy_len);
            /* log_info("AppIdentifier:%d", temp_message_buf->AppIdentifier); */
        } else if (T == MESSAGE_TITLE_TYPE) {
            copy_len = (L - 1) > MESSAGE_TITLE_LEN ? MESSAGE_TITLE_LEN : (L - 1);
            memset(message->title, 0, MESSAGE_TITLE_LEN);
            memcpy(message->title, V, copy_len);
            /* log_info("title:%s", temp_message_buf->title); */
        } else if (T == MESSAGE_CONTENT_TYPE) {
            copy_len = (L - 1) > MESSAGE_CONTENT_LEN ? MESSAGE_CONTENT_LEN : (L - 1);
            memset(message->content, 0, MESSAGE_CONTENT_LEN);
            memcpy(message->content, V, copy_len);
        }
        offset += L + 2;
    }
    free(data_buf);

    return true;
}

void message_set_info_from_ancs(void *info, void *name, void *data, u16 len)
{
    u32 copy_len;
    long date_value;
    char date_string[9];
    char *end;
    u32 timestamp = 0;
    small_file_message_t *p_msg = info;

    ASSERT(p_msg);
    if (!strcmp((char *)name, "UID")) {
        ancs_message_uid = *(u32 *)data;
    } else if (!strcmp((char *)name, "AppIdentifier")) {
        copy_len = len > MESSAGE_PACKAGENAME_LEN ? MESSAGE_PACKAGENAME_LEN : len;
        memset(p_msg->packagename, 0, MESSAGE_PACKAGENAME_LEN);
        memcpy(p_msg->packagename, (u8 *)data, copy_len);
        log_debug("packagename:%s", p_msg->packagename);
        if (!strcmp((char *)data, PACKAGE_NAME_SYS_MESSAGE_SEND)) {
            p_msg->app_identifier = 1;
        } else if (!strcmp((char *)data, PACKAGE_NAME_SYS_MESSAGE_RECEIVE)) {
            p_msg->app_identifier = 1;
        } else if (!strcmp((char *)data, IOS_PACKAGE_NAME_SYS_MESSAGE)) {
            p_msg->app_identifier = 1;
        } else if (!strcmp((char *)data, IOS_PACKAGE_NAME_WECHAT)) {
            p_msg->app_identifier = 2;
        } else if (!strcmp((char *)data, IOS_PACKAGE_NAME_QQ)) {
            p_msg->app_identifier = 3;
        } else if (!strcmp((char *)data, IOS_PACKAGE_NAME_DING_DING)) {
            p_msg->app_identifier = 4;
        } else {
            p_msg->app_identifier = 0;
        }
        log_debug("app_identifier:%d", p_msg->app_identifier);
    } else if (!strcmp((char *)name, "IDTitle")) {
        copy_len = len > MESSAGE_TITLE_LEN ? MESSAGE_TITLE_LEN : len;
        memset(p_msg->title, 0, MESSAGE_TITLE_LEN);
        memcpy(p_msg->title, (u8 *)data, copy_len);
        log_debug("%s %d %d\n", data, len, __LINE__);
    } else if (!strcmp((char *)name, "IDMessage")) {
        copy_len = len > MESSAGE_CONTENT_LEN ? MESSAGE_CONTENT_LEN : len;
        memset(p_msg->content, 0, MESSAGE_CONTENT_LEN);
        memcpy(p_msg->content, (u8 *)data, copy_len);
        log_debug("id_msg:%s %d %d\n", data, len, __LINE__);
    } else if (!strcmp((char *)name, "IDDate")) {
        /*格式:20240905T174158*/

        /*秒*/
        snprintf(date_string, (2 + 1), "%s", (char *)data + 13);
        date_value = strtol(date_string, &end, 10);
        log_debug("msc date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x3f);

        /*分*/
        snprintf(date_string, (2 + 1), "%s", (char *)data + 11);
        date_value = strtol(date_string, &end, 10);
        log_debug("min date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x3f) << 6;

        /*时*/
        snprintf(date_string, (2 + 1), "%s", (char *)data + 9);
        date_value = strtol(date_string, &end, 10);
        log_debug("hour date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x1f) << 12;

        /*日*/
        snprintf(date_string, (2 + 1), "%s", (char *)data + 6);
        date_value = strtol(date_string, &end, 10);
        log_debug("day date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x1f) << 17;

        /*月*/
        snprintf(date_string, (2 + 1), "%s", (char *)data + 4);
        date_value = strtol(date_string, &end, 10);
        log_debug("month date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x0f) << 21;

        /*年*/
        snprintf(date_string, (4 + 1), "%s", (char *)data);
        date_value = strtol(date_string, &end, 10) - 2010;
        log_debug("year date_string:%s date_value:%d", date_string, date_value);
        timestamp |= (date_value & 0x3f) << 26;

        WRITE_BIG_U32(&p_msg->timestamp, timestamp);
        log_debug("IDDate:%s timestamp:%x, p_msg->timestamp:%x", data, timestamp, p_msg->timestamp);
    }
}

void message_add_info_from_ancs(void *info)
{
    u32 offset = 0;
    u8 *temp_ptr = 0;
    small_file_message_t *p_msg = info;
    ASSERT(p_msg);
    u8 packagename_len = strlen((const char *)p_msg->packagename);
    u8 title_len = strlen((const char *)p_msg->title);
    u8 content_len = strlen((const char *)p_msg->content);
    /* length+type: 3 byte*/
    u32 mess_data_len = 3 + MESSAGE_TIMESTAMP_LEN + 3 + packagename_len +
                        3 + MESSAGE_APP_IDENTIFIER_LEN + 3 + title_len +
                        3 + content_len;

    temp_ptr = zalloc(mess_data_len);
    if (!temp_ptr) {
        log_error("temp_ptr malloc fail");
        return;
    }

    temp_ptr[offset] = (MESSAGE_TIMESTAMP_LEN + 1) >> 8;
    offset++;
    temp_ptr[offset] = (MESSAGE_TIMESTAMP_LEN + 1) & 0xff;
    offset++;
    temp_ptr[offset] = 0;
    offset++;
    memcpy(&temp_ptr[offset], &p_msg->timestamp, MESSAGE_TIMESTAMP_LEN);
    offset += MESSAGE_TIMESTAMP_LEN;

    temp_ptr[offset] = (packagename_len + 1) >> 8;
    offset++;
    temp_ptr[offset] = (packagename_len + 1) & 0xff;
    offset++;
    temp_ptr[offset] = 1;
    offset++;
    memcpy(&temp_ptr[offset], p_msg->packagename, packagename_len);
    offset += packagename_len;

    temp_ptr[offset] = (MESSAGE_APP_IDENTIFIER_LEN + 1) >> 8;
    offset++;
    temp_ptr[offset] = (MESSAGE_APP_IDENTIFIER_LEN + 1) & 0xff;
    offset++;
    temp_ptr[offset] = 2;
    offset++;
    temp_ptr[offset] = p_msg->app_identifier;
    offset += MESSAGE_APP_IDENTIFIER_LEN;

    temp_ptr[offset] = (title_len + 1) >> 8;
    offset++;
    temp_ptr[offset] = (title_len + 1) & 0xff;
    offset++;
    temp_ptr[offset] = 3;
    offset++;
    memcpy(&temp_ptr[offset], p_msg->title, title_len);
    offset += title_len;

    temp_ptr[offset] = (content_len + 1) >> 8;
    offset++;
    temp_ptr[offset] = (content_len + 1) & 0xff;
    offset++;
    temp_ptr[offset] = 4;
    offset++;
    memcpy(&temp_ptr[offset], p_msg->content, content_len);

    u32 file_id = ancs_message_uid;
    small_file_write(F_TYPE_MESSAGE, &file_id, 0, temp_ptr, mess_data_len, mess_data_len);
    log_debug("ancs_message_uid:%x, file_id:%x", ancs_message_uid, file_id);
    ancs_message_uid = 0;

    free(temp_ptr);

    // UI SHOW
#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
#if TCFG_UI_MSG_NOTICE
    if (ui_small_file_message_get_count() < 10) {
        UI_MSG_POST("message_status:event=%4", 1);
    } else {
        UI_MSG_POST("message_status:event=%4", 3);
    }
#endif
#endif
#if 0
    if (flash_message_count() < 10) {
        UI_MSG_POST("message_status:event=%4", 1);
    } else {
        UI_MSG_POST("message_status:event=%4", 3);
    }
#else
    /* u8 cur_task = app_get_curr_task(); */
    /* switch (cur_task) { */
    /* case APP_POWERON_TASK: */
    /* case APP_POWEROFF_TASK: */
    /* case APP_WATCH_UPDATE_TASK: */
    /* case APP_SMARTBOX_ACTION_TASK: */
    /*     break; */
    /* default: */
    /*     if (UI_WINDOW_PREEMPTION_CHECK()) { */
    /*         break; */
    /*     } */
    /*     if (get_screen_saver_status()) { */
    /*         ui_screen_recover(0); */
    /*         ui_auto_shut_down_enable(); */
    /*         UI_SHOW_WINDOW(ID_WINDOW_MESS); */
    /*     } else { */
    /*         ui_auto_shut_down_re_run(); */
    /*         if (UI_GET_WINDOW_ID() == ID_WINDOW_MESS) { */
    /*             if (flash_message_count() < 10) { */
    /*                 UI_MSG_POST("message_status:event=%4", 1); */
    /*             } else { */
    /*                 UI_MSG_POST("message_status:event=%4", 3); */
    /*             } */
    /*         } else { */
    /*             UI_HIDE_CURR_WINDOW(); */
    /*             UI_SHOW_WINDOW(ID_WINDOW_MESS); */
    /*         } */
    /*     } */
    /*     break; */
    /* } */
#endif
    // moto
    /* UI_MOTO_RUN(2); */
}


#else /* if TCFG_DATA_STORAGE_ENABLE */

void message_set_info_from_ancs(void *info, void *name, void *data, u16 len)
{
}
void message_add_info_from_ancs(void *info)
{
}

int ui_small_file_message_get_count(void)
{
    return 0;
}
int small_file_message_read_by_index(small_file_message_t *message, int index)
{
    return 0;
}

#endif /* if TCFG_DATA_STORAGE_ENABLE */

