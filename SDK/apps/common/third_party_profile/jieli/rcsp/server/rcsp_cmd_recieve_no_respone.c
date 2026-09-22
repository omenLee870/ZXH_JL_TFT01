#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_cmd_recieve_no_respone.data.bss")
#pragma data_seg(".rcsp_cmd_recieve_no_respone.data")
#pragma const_seg(".rcsp_cmd_recieve_no_respone.text.const")
#pragma code_seg(".rcsp_cmd_recieve_no_respone.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "rcsp_extra_flash_cmd.h"
#include "sensor_log_notify.h"
#include "rcsp_trace.h"
#include "translator/rcsp_translator.h"

#define LOG_TAG_CONST       RCSP
#define LOG_TAG     		"[rcsp rx no resp]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

///>>>>>>>>>>>>>收到APP发来不需要响应的命令处理

#if (RCSP_MODE)

/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	log_char(x)
#define rcsp_printf                    	log_info
#define rcsp_put_buf(x,len)				log_info_hexdump(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

void rcsp_cmd_recieve_no_respone(void *priv, u8 OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_CMD_TRACE_EN
    /* 抓包: 手机下发但无需设备回复的命令 (此回调无 SN 参数) */
    rcsp_trace_frame("<--", "RX CMD_NO_RESP", OpCode, 0xff, 0, data, len);
#endif
    rcsp_printf("cmd_recieve_no_respone, %x\n", OpCode);
    switch (OpCode) {
#if RCSP_TRANSLATOR_ENABLE
    case JL_OPCODE_TRANSLATOR:
        rcsp_translator_cmd_deal(0, data, len, 0,
                                 ble_con_handle, spp_remote_addr);
        break;
#endif
    default:
#if JL_RCSP_EXTRA_FLASH_OPT
        if (0 == JL_rcsp_extra_flash_cmd_no_resp(priv, OpCode, data, len)) {
            break;
        }
#endif
        if (0 == JL_rcsp_sensors_log_notify(priv, OpCode, data, len)) {
            break;
        }
    }
}
#endif//RCSP_MODE

