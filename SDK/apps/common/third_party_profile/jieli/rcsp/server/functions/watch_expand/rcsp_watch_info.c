
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
#include "rcsp_common_info_res_file_handler.h"

#if CONFIG_APP_UI_ENABLE && CONFIG_WATCH_CASE_ENABLE
#include "ui_sys_param.h"
#include "ui/ui_api.h"
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

extern const int jlui_res_support_gif;
extern int get_cur_srreen_width_and_height(u16 *screen_width, u16 *screen_height);
extern int get_cur_srreen_radius_and_fill_argb(u16 *screen_radius, u32 *screen_fill_argb);

/*功能版本号*/
#define SCREEN_BRIGHTENESS_VER              0
#define WATCH_DIAL_EXT_INFO_VER				0 // 表盘扩展参数版本号
#define FUNCTION_RESOURCES_VER              0
#define DEVICE_SDK_INFO_VER                 0

enum : u8 {
    // op
    COMMON_INFO_OP_READ = 0,
    COMMON_INFO_OP_SET,
    COMMON_INFO_OP_NOTIFY,
};

enum : u8 {
    // shape
    WATCH_DIAL_EXT_SHAPE_ROUND = 1,			// 圆形
    WATCH_DIAL_EXT_SHAPE_RECTANGLE,			// 矩形
    WATCH_DIAL_EXT_SHAPE_ROUND_RECTANGLE,	// 圆角矩形
};

struct cur_device_info {
    u8 device_shape;
    u16 device_radius;
    u32 screen_fill_argb;
#if (defined TCFG_CSC_BT_APP) && TCFG_CSC_BT_APP
    u16 screen_width;
    u16 screen_height;
#endif
} __attribute__((packed));


static int common_info_screen_brighteness(u8 *data, u16 *offset, u16 buf_len, const smartbox_common_info_set_cmd_t *common_info, u8 *app_data, u16 app_data_len)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    u16 wlen = 0;
    u16 data_len = *offset;

    if (common_info->version != SCREEN_BRIGHTENESS_VER) {
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
    }
    data[wlen++] = app_data[0]; //获取version
    memcpy(&data[wlen], common_info, sizeof(smartbox_common_info_set_cmd_t));
    wlen += sizeof(smartbox_common_info_set_cmd_t);
    switch (common_info->op) {
    case COMMON_INFO_OP_READ:
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
        u8 screen_brighteness;
        screen_brighteness = get_light_level() * 100 / UI_LIGHT_LEVEL_MAX;
        data[wlen++] = screen_brighteness;
#else
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
#endif
        break;
    case COMMON_INFO_OP_SET:
#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))
        float level_f = (float)app_data[1 + sizeof(smartbox_common_info_set_cmd_t)] / 100 * UI_LIGHT_LEVEL_MAX;
        int level = (int)(level_f + 0.5);

        if (level > UI_LIGHT_LEVEL_MAX) {
            level = UI_LIGHT_LEVEL_MAX;
        }
        if (level <= UI_LIGHT_LEVEL_MIN) {
            level = UI_LIGHT_LEVEL_MIN;
        }
        set_ui_sys_param(LightLevel, level);
        ui_ajust_light(level);
        void save_ui_info_to_vm();
        save_ui_info_to_vm();
        UI_MSG_POST("ui_brightness");
#else
        ret = JL_PRO_STATUS_FAIL;
        goto __end;
#endif
        break;
    default:
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
        break;
    }

    *offset = data_len + wlen;

__end:
    return ret;
}

static int common_info_function_resources(u8 *data, u16 *offset, u16 buf_len, u8 version, const smartbox_common_info_set_cmd_t *common_info, u8 *app_data, u16 app_data_len)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    u16 wlen = 0;
    u16 data_len = *offset;

    if (common_info->version != FUNCTION_RESOURCES_VER) {
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
    }
    data[wlen++] = version;
    memcpy(&data[wlen], common_info, sizeof(smartbox_common_info_set_cmd_t));
    wlen += sizeof(smartbox_common_info_set_cmd_t);

    switch (common_info->op) {
    case COMMON_INFO_OP_READ:
        u8 function_code = app_data[1 + sizeof(smartbox_common_info_set_cmd_t)];
        log_info("function_code:%d", function_code);
        if (function_code == RCSP_SCREEN_BOX_FUNC_SCREEN_SAVER_CODE) {
#if (defined(RCSP_SCREEN_UI_BOX_SCREEN_SAVER) && RCSP_SCREEN_UI_BOX_SCREEN_SAVER)
            data[wlen++] = RCSP_SCREEN_BOX_FUNC_SCREEN_SAVER_CODE;
            ret = common_info_get_cur_screen_saver_info(&data[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen);
            if (ret != JL_PRO_STATUS_SUCCESS) {
                goto __end;
            }
#else
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
#endif
        } else if (function_code == RCSP_SCREEN_BOX_FUNC_BOOT_ANIMATION_CODE) {
#if (defined(RCSP_SCREEN_UI_BOX_BOOT_ANIMATION) && RCSP_SCREEN_UI_BOX_BOOT_ANIMATION)
            data[wlen++] = RCSP_SCREEN_BOX_FUNC_BOOT_ANIMATION_CODE;
#else
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
#endif
        } else if (function_code == RCSP_SCREEN_BOX_FUNC_WALLPAPER_CODE) {
#if defined(RCSP_SCREEN_UI_BOX_WALLPAPER) && RCSP_SCREEN_UI_BOX_WALLPAPER && \
    defined(TCFG_UI_BG_ENABLE) && TCFG_UI_BG_ENABLE
            data[wlen++] = RCSP_SCREEN_BOX_FUNC_WALLPAPER_CODE;
            ret = rcsp_common_info_get_cur_wallpaper_info(&data[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen);
            if (ret != JL_PRO_STATUS_SUCCESS) {
                goto __end;
            }
#else
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
#endif
        } else {
            ret = JL_PRO_STATUS_FAIL;
            goto __end;
        }
        break;
    case COMMON_INFO_OP_SET:
        data[wlen++] = RCSP_SCREEN_BOX_FUNC_WALLPAPER_CODE;
        ret = common_info_set_cur_res_info(&app_data[1 + sizeof(smartbox_common_info_set_cmd_t)], app_data_len - (1 + sizeof(smartbox_common_info_set_cmd_t)));
        if (ret != JL_PRO_STATUS_SUCCESS) {
            goto __end;
        }
        break;
    default:
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
        break;
    }
    *offset = data_len + wlen;

__end:
    log_info("<%s> ret:%d", __func__, ret);
    return ret;
}


static int smartbox_watch_dial_ext_info(u8 *data, u16 *offset, u16 buf_len)
{
    struct cur_device_info device_info;
    u16 screen_width = 0;
    u16 screen_height = 0;
    u8 shape = WATCH_DIAL_EXT_SHAPE_RECTANGLE;
    u16 radius = 0;
    u32 screen_fill_argb = 0;
    u16 data_len = *offset;

    if (data_len + sizeof(shape) + sizeof(radius) + sizeof(screen_fill_argb) > buf_len) {
        return JL_PRO_STATUS_FAIL;
    }

#if (TCFG_UI_ENABLE && CONFIG_JL_UI_ENABLE)
    get_cur_srreen_width_and_height(&screen_width, &screen_height);
    get_cur_srreen_radius_and_fill_argb(&radius, &screen_fill_argb);
#endif
    log_info("<%s> widht:%d height:%d", __func__, screen_width, screen_height);
    if (radius) {
        u16 limit_radius = MIN(screen_width, screen_height) / 2;
        if (radius > limit_radius) {
            log_error("radius error: %d, %d, %d \n", radius, screen_width, screen_height);
        } else {
            if ((screen_width == screen_height) && (radius == limit_radius)) {
                shape = WATCH_DIAL_EXT_SHAPE_ROUND;
                log_debug("WATCH_DIAL_EXT_SHAPE_ROUND\n");
            } else {
                shape = WATCH_DIAL_EXT_SHAPE_ROUND_RECTANGLE;
                log_debug("WATCH_DIAL_EXT_SHAPE_RECTANGLE\n");
            }
        }
    }
    device_info.device_shape = shape;
    WRITE_BIG_U16(&device_info.device_radius, radius);
    WRITE_BIG_U32(&device_info.screen_fill_argb, screen_fill_argb);
#if (defined TCFG_CSC_BT_APP) && TCFG_CSC_BT_APP
    WRITE_BIG_U16(&device_info.screen_width, screen_width);
    WRITE_BIG_U16(&device_info.screen_height, screen_height);
#endif
    memcpy(data, &device_info, sizeof(struct cur_device_info));
    data_len += sizeof(struct cur_device_info);
    log_debug("data_len:%d\n", data_len);
    *offset = data_len;

    /* log_info_hexdump(data, sizeof(struct cur_device_info)); */

    return JL_PRO_STATUS_SUCCESS;
}


static int common_info_watch_dial_ext(u8 *data, u16 *offset, u16 buf_len, u8 version, const smartbox_common_info_set_cmd_t *common_info)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    u16 wlen = 0;
    u16 data_len = *offset;

    if (common_info->version != WATCH_DIAL_EXT_INFO_VER) {
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
    }
    data[wlen++] = version;
    memcpy(&data[wlen], common_info, sizeof(smartbox_common_info_set_cmd_t));
    wlen += sizeof(smartbox_common_info_set_cmd_t);
    switch (common_info->op) {
    case COMMON_INFO_OP_READ:
        ret = smartbox_watch_dial_ext_info(&data[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen);
        log_debug_hexdump(data, wlen);
        break;
    default:
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
        break;
    }

    *offset = data_len + wlen;

__end:
    return ret;
}


static int common_info_device_sdk_info(u8 *data, u16 *offset, u16 buf_len, u8 version, const smartbox_common_info_set_cmd_t *common_info)
{
    int ret = JL_PRO_STATUS_SUCCESS;
    u16 wlen = 0;
    u16 data_len = *offset;

    if (common_info->version != DEVICE_SDK_INFO_VER) {
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
    }
    data[wlen++] = version;
    memcpy(&data[wlen], common_info, sizeof(smartbox_common_info_set_cmd_t));
    wlen += sizeof(smartbox_common_info_set_cmd_t);
    switch (common_info->op) {
    case COMMON_INFO_OP_READ:
        //project
        WRITE_BIG_U16(&data[wlen], RCSP_COMMON_INFO_CHARGING_CASE);
        wlen += sizeof(u16);
        //product
        WRITE_BIG_U16(&data[wlen], RCSP_COMMON_INFO_SCREEN_BOX);
        wlen += sizeof(u16);
        //chip
#if (defined CONFIG_CPU_BR28)
        WRITE_BIG_U16(&data[wlen], RCSP_COMMON_INFO_CHIP_BR28);
        wlen += sizeof(u16);
#else
        WRITE_BIG_U16(&data[wlen], RCSP_COMMON_INFO_CHIP_BR35);
        wlen += sizeof(u16);
#endif
        //cfg_len
        data[wlen] = 1;
        wlen += sizeof(u8);
        //configure
        if (0) {
            data[wlen] |= BIT(RCSP_DEVICE_SDK_INFO_BIT_SUPPORT_GIF);
        } else {
            data[wlen] &= ~BIT(RCSP_DEVICE_SDK_INFO_BIT_SUPPORT_GIF);
        }
        wlen += sizeof(u8);
        break;
    default:
        ret = JL_PRO_STATUS_PARAM_ERR;
        goto __end;
        break;
    }

    *offset = data_len + wlen;

__end:
    return ret;
}


u8 rcsp_common_info_set_cmd_deal(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u16 wlen = 0;
    u8 ret = JL_PRO_STATUS_SUCCESS;
    u8 version = data[0];

    u8 *resp = zalloc(TARGET_FEATURE_RESP_BUF_SIZE);
    if (resp == NULL) {
        ret = JL_PRO_STATUS_FAIL;
        goto _cmd_err;
    }
    resp[wlen++] = ret;  // 初始返回result

    if (len < sizeof(smartbox_common_info_set_cmd_t)) {
        ret = JL_PRO_STATUS_UNKOWN_CMD;
        goto _cmd_err;
    }
    smartbox_common_info_set_cmd_t common_info;
    memcpy(&common_info, &data[1], sizeof(smartbox_common_info_set_cmd_t));

    log_info("%s :", __func__);
    log_info("function 0x%04x\n", READ_BIG_U16(&common_info.function));
    log_info("version    %d\n", common_info.version);
    log_info("op         %d\n", common_info.op);
    put_buf((const u8 *)&common_info, sizeof(smartbox_common_info_set_cmd_t));

    switch (READ_BIG_U16(&common_info.function)) {
    case SMARTBOX_COMMON_INFO_SCREEN_BRIGHTENESS:
        ret = common_info_screen_brighteness(&resp[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen, &common_info, data, len);
        break;
    case SMARTBOX_COMMON_INFO_FUNCTION_RESOURCES:
        ret = common_info_function_resources(&resp[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen, version, &common_info, data, len);
        break;
#if (defined(TCFG_CAT1_MODULE_UPDATE_ENABLE) && TCFG_CAT1_MODULE_UPDATE_ENABLE)
    case SMARTBOX_COMMON_INFO_CAT1_MODULE:
        free(resp);
        return smartbox_cat1_info_set_cmd_deal(priv, OpCode, OpCode_SN, data, len);
#endif
    case SMARTBOX_COMMON_INFO_WATCH_DIAL_EXT:
        ret = common_info_watch_dial_ext(&resp[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen, version, &common_info);
        log_debug_hexdump(resp, wlen);
        break;
    case SMARTBOX_COMMON_INFO_DEVICE_SDK_INFO:
        ret = common_info_device_sdk_info(&resp[wlen], &wlen, TARGET_FEATURE_RESP_BUF_SIZE - wlen, version, &common_info);
        break;
    default:
        ret = JL_PRO_STATUS_UNKOWN_CMD;
        break;
    }

_cmd_err:
    if (resp) {
        resp[0] = ret;
    }
    log_debug("response len %d, ret %d\n", wlen, ret);
    log_info_hexdump(resp, wlen);
    JL_CMD_response_send(OpCode, ret, OpCode_SN, resp, wlen, ble_con_handle, spp_remote_addr);
    if (resp) {
        free(resp);
    }
    return ret;
}

#endif//SMART_BOX_EN


