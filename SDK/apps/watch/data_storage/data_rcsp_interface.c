#include "app_config.h"
#include "includes.h"
#include "typedef.h"
#include "file_simple_transfer.h"
#include "rcsp_config.h"
#include "data_storage.h"
#include "app_config.h"

#define LOG_TAG_CONST       RCSP_ADAPTOR
#define LOG_TAG     		"[RCSP-ADAPTOR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"




#if (RCSP_MODE && TCFG_DEV_MANAGER_ENABLE && JL_RCSP_SIMPLE_TRANSFER)
#define HUAWEI_MUSIC_NOTIC_FILT 1


//*----------------------------------------------------------------------------*/
/**@brief   获取当前类型小文件列表
  @param    无
  @return   数量*4
  @note
 */
/*----------------------------------------------------------------------------*/
static int simple_trans_get_id_table_len(u8 file_type)
{
#if TCFG_DATA_STORAGE_ENABLE
    return small_file_get_id_table_len(file_type);
#else
    return 0;
#endif
}

static int simple_trans_get_id_table(u8 file_type, u8 *table_data, u16 data_len)
{
#if TCFG_DATA_STORAGE_ENABLE
    return small_file_get_id_table(file_type, table_data, data_len);
#else
    return 0;
#endif
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 读取小文件数据
 *
 * @Return 读取的字节数
 */
/* ------------------------------------------------------------------------------------*/
static int simple_trans_read_file_by_id(u8 file_type, u16 id, u32 data_offset, u8 *data, u16 data_len)
{
#if TCFG_DATA_STORAGE_ENABLE
    int rlen;
    rlen = small_file_read(file_type, id, data_offset, data, data_len);
    log_info_hexdump(data, rlen);
    log_info("%s file_type:%d id:%d data_offset:%d data_len:%d rlen:%d\n", __FUNCTION__, file_type, id, data_offset, data_len, rlen);
    return rlen;
#else
    return 0;
#endif
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 写入小文件数据
 *
 * @Return 0:成功 -1:失败
 */
/* ------------------------------------------------------------------------------------*/
static int simple_trans_insert_file_by_id(u8 file_type, u16 *id, u32 data_offset, u8 *data, u16 data_len, u16 file_total_size)
{
    log_info("<%s> file_type:%d, id:%x, data_offset:%x, data_len:%x, file_total_size:%x\n", __func__, file_type, *id, data_offset, data_len, file_total_size);
    log_info_hexdump(data, data_len);
#if HUAWEI_MUSIC_NOTIC_FILT
    if (strstr((char *)data + 10, "com.huawei.mediacontroller") != NULL) {
        return 0;
    }
#endif
#if TCFG_DATA_STORAGE_ENABLE
    //TODO 判断这个文件剩余空间是否足够
    //
    //
    u32 creat_id;
    creat_id = *id;

    int ret  = small_file_write(file_type, &creat_id, data_offset, data, data_len, file_total_size);

    if (ret != data_len) {
        *id = 0;
    } else {
        *id = (u16)creat_id;
    }

    log_info("<%s> creat id:%d", __func__, *id);
    if (ret != data_len) {
        log_error("<%s> fail:%d %d", __func__, ret, data_len);
        return -1;
    }
    //TODO ui页面显示
#if TCFG_UI_ENABLE_SMARTWIN|| TCFG_UI_ENABLE_NOTICE
    if (file_type == F_TYPE_MESSAGE) {
        //灵动岛
        UI_MSG_POST("smartwin_status:message=%4", 1);
        //消息通知
        if (ui_small_file_message_get_count() < 10) {
            UI_MSG_POST("message_status:event=%4", 1);
        } else {
            UI_MSG_POST("message_status:event=%4", 3);
        }
    }
#endif

#ifdef CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE
#if TCFG_UI_MSG_NOTICE
    if (ui_small_file_message_get_count() < 10) {
        UI_MSG_POST("message_status:event=%4", 1);
    } else {
        UI_MSG_POST("message_status:event=%4", 3);
    }
#endif
#endif
    return 0;
#endif

    return -1;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 更新小文件数据
 *
 * @Return 0:成功 -1:失败
 */
/* ------------------------------------------------------------------------------------*/
static int simple_trans_update_file_by_id(u8 file_type, u16 id, u32 data_offset, u8 *data, u16 data_len, u16 file_total_size)
{
    log_info("<%s> id:%d, data_offset:%d, data_len:%d, file_total_size:%d", __func__, id, data_offset, data_len, file_total_size);
    log_info_hexdump(data, data_len);
#if TCFG_DATA_STORAGE_ENABLE
    //TODO 判断这个文件剩余空间是否足够
    u32 creat_id;
    creat_id = id;
    int ret  = small_file_write(file_type, &creat_id, data_offset, data, data_len, file_total_size);
    if (ret != data_len) {
        return -1;
    }
    return 0;
    //TODO ui页面显示
#else
    return -1;
#endif
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 删除小文件数据
 *
 * @Return 0:成功 -1:失败
 */
/* ------------------------------------------------------------------------------------*/
static int simple_trans_delete_file_by_id(u8 file_type, u16 id)
{
    log_info("<%s> file_type:%d, id:%d", __func__, file_type, id);

#if TCFG_DATA_STORAGE_ENABLE
    int ret;
    ret = small_file_delete_by_id(file_type, id);
    if (ret == false) {
        return -1;
    }
#if TCFG_UI_MSG_NOTICE ||TCFG_UI_ENABLE_NOTICE
    if (file_type == F_TYPE_MESSAGE) {
        //消息通知
        UI_MSG_POST("message_status:event=%4", 2);
    }
#endif
    return 0;
#else
    return -1;
#endif
}

static const rcsp_simple_trans_opt g_test_trans_opt = {
    .get_id_table_len = simple_trans_get_id_table_len,
    .get_id_table = simple_trans_get_id_table,
    .read_file_by_id = simple_trans_read_file_by_id,
    .insert_file_by_id = simple_trans_insert_file_by_id,
    .update_file_by_id = simple_trans_update_file_by_id,
    .delete_file_by_id = simple_trans_delete_file_by_id,
};

int test_simple_trans(void)
{
    rcsp_register_file_simple_transfer_interface((rcsp_simple_trans_opt *)&g_test_trans_opt);
    return 0;
}

late_initcall(test_simple_trans);

#endif

#if TCFG_DATA_STORAGE_ENABLE
#if TRANS_ANCS_EN

static small_file_message_t *ancs_message_temp;
void notice_set_info_from_ancs(void *name, void *data, u16 len)
{
    if (!ancs_message_temp) {
        ancs_message_temp = zalloc(SMALL_FILE_MESSAGE_SIZE);
    }

    if (!ancs_message_temp) {
        log_error("ancs_message_temp is NULL !!!");
        return;
    }
    message_set_info_from_ancs(ancs_message_temp, name, data, len);
}

void notice_add_info_from_ancs()
{
    if (!ancs_message_temp) {
        log_error("ancs_message_temp must be valid !!!");
        return;
    }

    message_add_info_from_ancs(ancs_message_temp);
    if (ancs_message_temp) {
        free(ancs_message_temp);
        ancs_message_temp = NULL;
    }

#if TCFG_UI_ENABLE_SMARTWIN|| TCFG_UI_ENABLE_NOTICE
    //灵动岛
    UI_MSG_POST("smartwin_status:message=%4", 1);
    //消息通知
    if (ui_small_file_message_get_count() < 10) {
        UI_MSG_POST("message_status:event=%4", 1);
    } else {
        UI_MSG_POST("message_status:event=%4", 3);
    }
#endif
}

void notice_remove_info_from_ancs(u32 uid)
{
    if (ui_small_file_message_get_count() == 0) {
        log_warn("message null!!!");
        return;
    }

    log_debug("<%s> uid:%x", __func__, uid);
    small_file_delete_by_id(F_TYPE_MESSAGE, uid);
#if TCFG_UI_ENABLE_NOTICE
    UI_MSG_POST("message_status:event=%4", 2);
#endif
}

#endif //#if TRANS_ANCS_EN
#endif //#if TCFG_DATA_STORAGE_ENABLE

