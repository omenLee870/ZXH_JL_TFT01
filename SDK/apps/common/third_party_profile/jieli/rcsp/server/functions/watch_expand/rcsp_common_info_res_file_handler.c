#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_watch_expand.data.bss")
#pragma data_seg(".rcsp_watch_expand.data")
#pragma const_seg(".rcsp_watch_expand.text.const")
#pragma code_seg(".rcsp_watch_expand.text")
#endif

#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp.h"
#include "rcsp_event.h"
#include "ble_rcsp_server.h"
#include "rcsp_setting_opt.h"
#include "rcsp_manage.h"
#include "rcsp_watch_info.h"
#include "utils/generic/ascii.h"

#if CONFIG_APP_UI_ENABLE && (defined TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
#include "ui_bg_manage.h"
#endif


#if (RCSP_MODE )
#define LOG_TAG_CONST       APP
#define LOG_TAG     		"[SMARBOX-COMMON_INFO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CHAR_ENABLE
#include "debug.h"


#define SCREEN_SAVER_FIELD_STRING           "VIE"
#define SCREEN_SAVER_CUSTOM_FIELD_STRING    "VIE_CST"
#define CSBG_FILE_FIELD_STRING  "csbg"
#define CSBG_FILE_PATH          "storage/virfat_flash/C/"
#define RCSP_WALLPAPER_NAME_MAX_LEN         9 // "/csbg_xxx"



struct res_info {
    u32 dev_headle;
    u32 file_cluster;
    u16 crc16;

    u16 path_len;
    u8 path_data[0];
    /* data */
};

static int calculate_file_crc(u8 *file_path, u16 *file_crc)
{
    int result = 0;
    FILE *file = NULL;

    u8 *data = malloc(256);
    if (NULL == data) {
        result = -1;
        goto __calculate_file_crc_err;
    }

    file = fopen((const char *)file_path, "r");
    if (NULL == file) {
        result = -2;

        goto __calculate_file_crc_err;
    }
    u32 file_len = flen(file);
    u16 temp_crc = 0;

    for (u32 crc_offset = 0; crc_offset < file_len;) {
        wdt_clear();
        u32 crc_len = (file_len - crc_offset) > 256 ? 256 : (file_len - crc_offset);
        fseek(file, crc_offset, SEEK_SET);
        if (crc_len != fread(data, crc_len, 1, file)) {
            log_error("err : read fail, %s, %d\n", file_path, crc_offset);
            result = -3;
            goto __calculate_file_crc_err;
        }
        // putchar('h');
        temp_crc = CRC16_with_initval(data, crc_len, temp_crc);
        crc_offset += crc_len;
    }
    *file_crc = temp_crc;
    log_debug("%s[line:%d, file_crc:%d, temp_crc:%d]\n", __func__, __LINE__, *file_crc, temp_crc);

__calculate_file_crc_err:
    if (file) {
        fclose(file);
    }
    if (data) {
        free(data);
        data = NULL;
    }
    if (result) {
        log_error("%s[line:%d result:%d]\n", __func__, __LINE__, result);

    }
    return result;
}

int file_put_buf(FILE *file)
{
    int result = 0;

    u8 *data = malloc(256);
    if (NULL == data) {
        result = -1;
        goto __end;
    }

    u32 file_len = flen(file);

    for (u32 crc_offset = 0; crc_offset < file_len;) {
        wdt_clear();
        u32 crc_len = (file_len - crc_offset) > 256 ? 256 : (file_len - crc_offset);
        fseek(file, crc_offset, SEEK_SET);
        if (crc_len != fread(data, crc_len, 1, file)) {
            log_error("err : read fail\n");
            result = -3;
            goto __end;
        }
        printf("file_len:%d offset:%d", file_len, crc_offset);
        put_buf(data, crc_len);
        crc_offset += crc_len;
    }

__end:
    return result;
}



//墙纸 获取当前墙纸信息
int rcsp_common_info_get_cur_wallpaper_info(u8 *data, u16 *offset, u16 buf_len)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    char *wallpaper_name = NULL;
    u8 wallpaper_name_len;
    char *wallpaper_path = NULL;
    struct res_info *wallpaper_info = NULL;

#if (defined TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
    wallpaper_name = app_csbg_get_cur_csbg_name(CSBG_TYPE_WALLPAPER);
#endif
    if (strlen(wallpaper_name) == 0 || strlen(wallpaper_name) >= RCSP_WALLPAPER_NAME_MAX_LEN) {
        log_error("The picture naming format is wrong");
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
    }
    wallpaper_name_len = strlen(wallpaper_name) + 1;

    u8 size = sizeof(struct res_info) + wallpaper_name_len * sizeof(u8);
    wallpaper_info = zalloc(size);
    if (!wallpaper_info) {
        log_error("%s zalloc err", __func__);
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
    }

    WRITE_BIG_U32(&wallpaper_info->dev_headle, RCSPDevMapFLASH);
    WRITE_BIG_U32(&wallpaper_info->file_cluster, 0);
#if (defined TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
    wallpaper_path = csbg_get_cur_path(CSBG_TYPE_WALLPAPER);
#endif
    if (strlen(wallpaper_path) == 0) {
        log_error("wallpaper_path is null");
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
    }
    calculate_file_crc((u8 *)wallpaper_path, &wallpaper_info->crc16);
    wallpaper_info->crc16 = READ_BIG_U16(&wallpaper_info->crc16);
    WRITE_BIG_U16(&wallpaper_info->path_len, wallpaper_name_len);
    wallpaper_info->path_data[0] = '/';
    strncpy((char *)&wallpaper_info->path_data[1], wallpaper_name, wallpaper_name_len);  //比如传"/csbg_001"
    put_buf(wallpaper_info->path_data, wallpaper_name_len);
    memcpy(data, wallpaper_info, size);
    *offset += size;

__end:
    if (wallpaper_info) {
        free(wallpaper_info);
    }
    return ret;
}

//墙纸 删除墙纸
void rcsp_common_info_del_wallpaper(FILE *file)
{
    int ret = false;
    char res_name[RCSP_WALLPAPER_NAME_MAX_LEN] = {0};
    fget_name(file, (u8 *)res_name, sizeof(res_name));
    ASCII_ToLower(res_name, sizeof(res_name));
    if (!strncmp(res_name, CSBG_FILE_FIELD_STRING, strlen(CSBG_FILE_FIELD_STRING))) {
#if defined(TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
        ret = csbg_del_item(res_name, CSBG_TYPE_WALLPAPER);
#endif
        if (ret == false) {
            log_error("%s del error", __func__);
        } else {
            log_info("%s del succ", __func__);
        }
    }
}


//设置资源样式
int common_info_set_cur_res_info(u8 *app_data, u16 app_data_len)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    int res_type = app_data[0];
    char res_name[RCSP_WALLPAPER_NAME_MAX_LEN] = {0};
    switch (res_type) {
    case RCSP_SCREEN_BOX_FUNC_SCREEN_SAVER_CODE:
        ret = JL_PRO_STATUS_FAIL;
        break;
    case RCSP_SCREEN_BOX_FUNC_BOOT_ANIMATION_CODE:
        ret = JL_PRO_STATUS_FAIL;
        break;
    case RCSP_SCREEN_BOX_FUNC_WALLPAPER_CODE:
        if (app_data[1 + sizeof(struct res_info)] != '/') {
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
        }

        for (int i = 1 + sizeof(struct res_info), j = 0; i < app_data_len; i++) {
            if (app_data[i] == '.') {
                break;
            }
            if (j >= RCSP_WALLPAPER_NAME_MAX_LEN) {
                log_error("<%s> res name error", __func__);
                goto __end;
            }
            res_name[j++] = app_data[i];
        }

        if (strncmp(&res_name[1], CSBG_FILE_FIELD_STRING, strlen(CSBG_FILE_FIELD_STRING))) {
            log_error("%s Do not match field", __func__);
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
        }
        log_info("%s res_name:%s", __func__, res_name);

#if (defined TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
        csbg_add_item(&res_name[1], CSBG_TYPE_WALLPAPER);
        app_set_csbg_by_name(&res_name[1], CSBG_TYPE_WALLPAPER);
#endif
        break;
    default:
        log_error("%s %d err", __func__, res_type);
        ret = JL_PRO_STATUS_FAIL;
        break;
    }

__end:
    return ret;
}


int common_info_convert_screen_saver_res_name(char *name_buf, int name_buf_len, const char *matching_fields_string)
{

    u8 matching_fields_string_len = strlen(matching_fields_string);
    u8 name_len = strlen(name_buf);
    char num[3] = {0};
    u8 num_len;
    u8 res_name_len = strlen(SCREEN_SAVER_FIELD_STRING);

    if (strncmp(name_buf, matching_fields_string, matching_fields_string_len)) {
        goto __end;
    }

    num_len = name_len - matching_fields_string_len;
    if (num_len > sizeof(num)) {
        goto __end;
    }

    if ((res_name_len + num_len) > name_buf_len) {
        goto __end;
    }

    memcpy(num, &name_buf[matching_fields_string_len], num_len);
    memset(name_buf, 0, name_buf_len);
    memcpy(name_buf, SCREEN_SAVER_FIELD_STRING, res_name_len);
    memcpy(&name_buf[res_name_len], num, num_len);
__end:
    return 0;
}




//屏幕保护程序(屏保)
int common_info_get_cur_screen_saver_info(u8 *data, u16 *offset, u16 buf_len)
{
    extern char *watch_get_cur_path();

    int ret = JL_PRO_STATUS_SUCCESS;
    struct res_info *screen_saver_info = NULL;
    char *res_name = "/"SCREEN_SAVER_FIELD_STRING;
    char *watch_name = watch_get_cur_path();
    char index_str[3] = {0};
    u8 index;

    if (!watch_name) {
        log_error("%s don't get watch name", __func__);
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
    }

    //协议规定从VIE1开始
    index = atoi(&watch_name[strlen("/WATCH")]) + 1;
    itoa(index, index_str, 10);



    u8 patch_len = strlen(res_name) + strlen(index_str);
    u8 size = sizeof(struct res_info) + patch_len;

    screen_saver_info = zalloc(size);
    if (!screen_saver_info) {
        log_error("%s zalloc err", __func__);
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
    }

    WRITE_BIG_U32(&screen_saver_info->dev_headle, RCSPDevMapFLASH);
    WRITE_BIG_U32(&screen_saver_info->file_cluster, 0);
    WRITE_BIG_U16(&screen_saver_info->crc16, 0x1122); //crc实际没有用到，目前先随机赋值
    WRITE_BIG_U16(&screen_saver_info->path_len, patch_len);
    memcpy(&screen_saver_info->path_data[0], res_name, strlen(res_name));
    memcpy(&screen_saver_info->path_data[strlen(res_name)], index_str, strlen(index_str));
    memcpy(data, screen_saver_info, size);
    *offset += size;

__end:
    if (screen_saver_info) {
        free(screen_saver_info);
    }
    return ret;
}

#endif


