#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".file_trans_back.data.bss")
#pragma data_seg(".file_trans_back.data")
#pragma const_seg(".file_trans_back.text.const")
#pragma code_seg(".file_trans_back.text")
#endif
#include "file_trans_back.h"
#include "rcsp.h"
#include "system/includes.h"
#include "file_operate/file_manager.h"
#include "rcsp_browser.h"
#include "fs/fs.h"
#include "rcsp_manage.h"
#include "rcsp_event.h"
#include "rcsp_config.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_packet.h"

#if (RCSP_MODE && TCFG_DEV_MANAGER_ENABLE)

#include "dev_status.h"

#define FTP_DOWNLOAD_FOLDER_NAME				"download" //下载目录

#define FILE_TRANS_BACK_TASK_NAME				"ftran_back"
#define FILE_TRANS_BACK_SPP_WAKEUP_WAIT_MAX      20

enum {
    FILE_TRANS_BACK_BY_NAME,
    FILE_TRANS_BACK_BY_CLUST,
    FILE_TRANS_BACK_BY_NAME_2,
    FILE_TRANS_BACK_FINISH = 0x80,
    FILE_TRANS_BACK_CANCEL = 0x81,
};

struct __file_trans_back {
    FILE *file;
    struct __dev *dev;
    struct vfscan *fsn;
    void (*end_callback)(void);
    u32 file_offset;
    u8 OpCode_SN;
    u8 op;
};

///播放参数，文件扫描时用，文件后缀等
static const char scan_parm[] = "-t"
#if (WATCH_FILE_TO_FLASH)
                                "ALL"
#else // WATCH_FILE_TO_FLASH
#if (TCFG_DEC_MP3_ENABLE)
                                "MP1MP2MP3"
#endif
#if (TCFG_DEC_WMA_ENABLE)
                                "WMA"
#endif
#if ( TCFG_DEC_WAV_ENABLE || TCFG_DEC_DTS_ENABLE)
                                "WAVDTS"
#endif
#if (TCFG_DEC_FLAC_ENABLE)
                                "FLA"
#endif
#if (TCFG_DEC_APE_ENABLE)
                                "APE"
#endif
#if (TCFG_DEC_M4A_ENABLE)
                                "M4AAAC"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
                                "MP4"
#endif
#if (TCFG_DEC_AMR_ENABLE)
                                "AMR"
#endif
#if (TCFG_DEC_DECRYPT_ENABLE)
                                "SMP"
#endif
#if (TCFG_DEC_MIDI_ENABLE)
                                "MID"
#endif
#if (TCFG_ENC_OPUS_ENABLE)
                                "OPS"
#endif
                                "TMP"
                                "JPG"
                                "AVI"
#endif // WATCH_FILE_TO_FLASH
                                " -sn -r"
                                ;

static struct __file_trans_back *trans_back = NULL;

static u8 g_trans_back_cancel_flag = -1;

static u32 g_dev_handle = RCSPDevMapSD1;

extern u8 check_le_pakcet_sent_finish_flag(void);
extern bool rcsp_send_list_is_empty(void);
extern void file_trans_idle_set(u8 file_trans_idle_flag);

static int get_file_prepare(u32 dev_handle)
{
    char *logo = rcsp_browser_dev_remap(dev_handle);
    if (NULL == logo) {
        printf("trans_back logo is null");
        return -1;
    }

    struct __dev *dev  = dev_manager_find_spec(logo, 0);
    if (NULL == dev) {
        printf("trans_back dev is null");
        return -1;
    }

    if (NULL == trans_back) {
        trans_back = zalloc(sizeof(struct __file_trans_back));
        if (NULL == trans_back) {
            printf("trans_back zalloc err\n");
            return -1;
        }
        trans_back->dev = dev;
    }
    return 0;
}

static int file_trans_back_offset_is_valid(FILE *file, u32 file_offset)
{
    struct vfs_attr attr = {0};

    if (!file || fget_attrs(file, &attr)) {
        return 0;
    }
    return file_offset <= attr.fsize;
}

static void creat_file_path(char *path, char *root_path, const char *folder, u8 *name, u16 name_len)
{
    strcat(path, root_path);
    if (folder) {
        strcat(path, folder);
        strcat(path, "/");
    }
    //strcat(path, name);
    /* memcpy(path + strlen(root_path) + strlen(folder) + 1, name, name_len); */
    memcpy(path + strlen(path), name, name_len);
    //printf("path = %s\n", path);
}

static u32 creat_path_len(char *root_path, const char *folder, u8 *name, u16 name_len)
{
    u32 root_path_len = 0, folder_len = 0;
    if (root_path) {
        root_path_len = strlen(root_path);
    }
    if (folder) {
        folder_len = strlen(folder);
    }
    u32 len = (root_path_len + folder_len + name_len + 1);
    if (folder) {
        len += strlen("/");
    }
    return len;
}

static u8 file_trans_back_response_send(u8 *data, u16 len, u8 dire, u8 OpCode_SN)
{
    u8 ret = 0;
    switch (dire) {
    case 0:
        ret = JL_CMD_response_send(JL_OPCODE_ONE_FILE_TRANS_BACK, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, len, 0, NULL);
        break;
    case 1:
        ret = JL_DATA_send(JL_OPCODE_DATA, JL_OPCODE_ONE_FILE_TRANS_BACK, data, len, JL_NOT_NEED_RESPOND, 0, NULL);
        break;
    case 2:
        ret = JL_CMD_send(JL_OPCODE_ONE_FILE_TRANS_BACK, data, len, JL_NEED_RESPOND, 0, NULL);
        break;
    case 0xFF:
        ret = JL_CMD_response_send(JL_OPCODE_ONE_FILE_TRANS_BACK, JL_PRO_STATUS_FAIL, OpCode_SN, data, len, 0, NULL);
        break;
    }
    return ret;
}

static int file_trans_back_is_spp(void)
{
#if defined(RCSP_USE_SPP)
    return RCSP_USE_SPP == get_defalut_bt_channel_sel();
#else
    return 0;
#endif
}

#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
static int file_trans_back_wait_spp_wakeup(u32 packet_count, u32 offset,
        u32 wakeup_base)
{
    u16 wait_count = 0;

    while (1) {
        if ((u8) - 1 != g_trans_back_cancel_flag) {
            return 1;
        }
        if (0 == get_rcsp_connect_status()) {
            return -1;
        }

        int ret = rcsp_spp_tx_wait();
        if (!ret) {
            if ((u8) - 1 != g_trans_back_cancel_flag) {
                return 1;
            }
            if (0 == get_rcsp_connect_status()) {
                return -1;
            }
            if ((packet_count <= 3) || (0 == (packet_count & 0x1f))) {
                printf("file trans spp wakeup: packet=%u, offset=%u, total=%u\n",
                       packet_count, offset,
                       rcsp_spp_tx_wakeup_count_get() - wakeup_base);
            }
            return 0;
        }
        if (ret != OS_TIMEOUT) {
            printf("file trans spp wakeup wait err: %d\n", ret);
            return -1;
        }

        wait_count++;
        if ((1 == wait_count) || (0 == (wait_count % 10))) {
            printf("file trans spp wakeup pending: packet=%u, offset=%u, waits=%u\n",
                   packet_count, offset, wait_count);
        }
        if (wait_count >= FILE_TRANS_BACK_SPP_WAKEUP_WAIT_MAX) {
            printf("file trans spp wakeup timeout: packet=%u, offset=%u\n",
                   packet_count, offset);
            return -1;
        }
    }
}
#endif

extern void notify_update_connect_parameter(u8 table_index);

static void file_trans_back_task(void *p)
{
    // 从文件句柄中获取当前文件的大小
    struct vfs_attr attr;
    fget_attrs(trans_back->file, &attr);
    // 获取文件数据
    u32 file_size = attr.fsize;

    notify_update_connect_parameter(3);

    // 预留 开始头(3byte) + opCode(2byte) + param_len(2byte) + state(1byte) + opCode_SN(1byte) + 结束(1byte)
    u16 resp_data_len = JL_packet_get_tx_max_mtu() - 10;
    u8 *resp_data = zalloc(resp_data_len);
    if (NULL == resp_data) {
        // 错误
        file_trans_back_response_send(&trans_back->op, sizeof(trans_back->op), (u8) - 1, trans_back->OpCode_SN);
        goto __file_trans_back_task_err;
    }

    // 把大小发送给app
    resp_data[0] = trans_back->op;
    resp_data[1] = ((u8 *)&file_size)[3];
    resp_data[2] = ((u8 *)&file_size)[2];
    resp_data[3] = ((u8 *)&file_size)[1];
    resp_data[4] = ((u8 *)&file_size)[0];
    file_trans_back_response_send(resp_data, sizeof(file_size) + 1, 0, trans_back->OpCode_SN);
    // 退出sniff
    file_trans_idle_set(0);

    u16 crc = 0;
    u32 spp_packet_count = 0;
    int is_spp = file_trans_back_is_spp();
#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
    int spp_wakeup_flow = is_spp && rcsp_spp_tx_flow_ctrl_ready();
    u32 spp_wakeup_base = rcsp_spp_tx_wakeup_count_get();
#else
    int spp_wakeup_flow = 0;
#endif
    resp_data_len -= 4;
    if (FILE_TRANS_BACK_BY_NAME != trans_back->op) {
        resp_data_len -= 2;
    }
    printf("file trans start: size=%u, offset=%u, payload=%u, spp=%d, wakeup=%d\n",
           file_size, trans_back->file_offset, resp_data_len,
           is_spp, spp_wakeup_flow);
    if (is_spp && !spp_wakeup_flow) {
        printf("file trans spp wakeup unavailable, use one-tick delay\n");
    }
    for (u32 offset = trans_back->file_offset, data_len = 0, ret = 0; offset < file_size;) {
        wdt_clear();
        // 假如当前spp或ble断开连接
        if (0 == get_rcsp_connect_status()) {
            goto __file_trans_back_task_err;
        }
        if ((u8) - 1 != g_trans_back_cancel_flag) {
            break;
        }

        data_len = (file_size - offset) > resp_data_len ? resp_data_len : file_size - offset;
        fseek(trans_back->file, offset, SEEK_SET);
        resp_data[0] = ((u8 *)&offset)[3];
        resp_data[1] = ((u8 *)&offset)[2];
        resp_data[2] = ((u8 *)&offset)[1];
        resp_data[3] = ((u8 *)&offset)[0];

        if (FILE_TRANS_BACK_BY_NAME == trans_back->op) {
            fread(resp_data + 4, data_len, 1, trans_back->file);
        } else {
            fread(resp_data + 6, data_len, 1, trans_back->file);
            // 填充crc
            if (JL_ERR_NONE == ret) {
                crc = CRC16_with_initval(resp_data + 6, data_len, crc);
            }
            data_len += 2;
            resp_data[4] = crc >> 8;
            resp_data[5] = crc & 0xFF;
        }

        // 发送文件数据
#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
        if (spp_wakeup_flow) {
            rcsp_spp_tx_wait_prepare();
        }
#endif
        ret = file_trans_back_response_send(resp_data, data_len + 4, 1, trans_back->OpCode_SN);
        if ((JL_ERR_SEND_DATA_OVER_LIMIT == ret) || (JL_ERR_SEND_BUSY == ret)) {
            os_time_dly(1);
            continue;
        }
        if (ret) {
            printf("file data trans err: %d\n", ret);
            break;
        }

        /* Only replace the original delay when SPP wakeup is available. */
        if (spp_wakeup_flow) {
            spp_packet_count++;
#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
            int wait_ret = file_trans_back_wait_spp_wakeup(
                               spp_packet_count, offset,
                               spp_wakeup_base);
            if (wait_ret < 0) {
                goto __file_trans_back_task_err;
            }
            if (wait_ret > 0) {
                break;
            }
#endif
        } else {
            /* Keep the original SDK pacing for BLE and SPP fallback. */
            os_time_dly(1);
            while (!(rcsp_send_list_is_empty() &&
                     check_le_pakcet_sent_finish_flag())) {
                os_time_dly(1);
                if (0 == get_rcsp_connect_status()) {
                    goto __file_trans_back_task_err;
                }
                if ((u8) - 1 != g_trans_back_cancel_flag) {
                    break;
                }
            }
        }
        offset += resp_data_len;
    }

    if (spp_wakeup_flow) {
        if (0 == get_rcsp_connect_status()) {
            goto __file_trans_back_task_err;
        }
        printf("file trans spp data done: packets=%u, wakeups=%u\n",
               spp_packet_count,
#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
               rcsp_spp_tx_wakeup_count_get() - spp_wakeup_base
#else
               0
#endif
              );
    }

    while (!(rcsp_send_list_is_empty() &&
             check_le_pakcet_sent_finish_flag())) {
        os_time_dly(10);
        if (0 == get_rcsp_connect_status()) {
            goto __file_trans_back_task_err;
        }
    }

    memset(resp_data, 0, resp_data_len + 1);
    if ((u8) - 1 == g_trans_back_cancel_flag) {
        // 如果传输完成发送结束命令
        resp_data[0] = FILE_TRANS_BACK_FINISH;
        file_trans_back_response_send(resp_data, 1 + 4, 2, trans_back->OpCode_SN);
    } else {
        // 取消
        resp_data[0] = FILE_TRANS_BACK_CANCEL;
        file_trans_back_response_send(resp_data, 1, 0, trans_back->OpCode_SN);
    }

__file_trans_back_task_err:
    notify_update_connect_parameter(-1);
    if (resp_data) {
        free(resp_data);
    }

    file_trans_idle_set(1);
    u16 close_post_retry = 0;
    while (!rcsp_msg_post(USER_MSG_RCSP_FILE_TRANS_BACK, 1, (int)p)) {
        close_post_retry++;
        if ((1 == close_post_retry) || (0 == (close_post_retry % 10))) {
            printf("file trans close post retry: %u\n", close_post_retry);
        }
        os_time_dly(10);
    }
    printf("file trans close post ok: retries=%u\n", close_post_retry);
    while (1) {
        os_time_dly(10);
    }
}

static int get_file_by_name(void *priv, u8 *data, u16 len)
{
    u32 file_offset;
    u8 offset = 4;
    u8 *file_name;
    char *root_path;
    char *folder = NULL;
    char *path;

    if (!data || len < 4) {
        goto __get_file_file_by_name_err;
    }

    file_offset = ((u32)data[0] << 24) | ((u32)data[1] << 16) |
                  ((u32)data[2] << 8) | data[3];

    trans_back->file_offset = file_offset;

    if (FILE_TRANS_BACK_BY_NAME != trans_back->op) {
        if (offset >= len) {
            goto __get_file_file_by_name_err;
        }
        offset++;
    }
    if (offset >= len) {
        goto __get_file_file_by_name_err;
    }
    file_name = data + offset;
    // 拼凑绝对路径
    root_path = dev_manager_get_root_path(trans_back->dev);
    if (!root_path) {
        goto __get_file_file_by_name_err;
    }
    if (RCSPDevMapFLASH == g_dev_handle ||
        RCSPDevMapFLASH_2 == g_dev_handle) {
        if ('/' == file_name[0]) {
            file_name++;
            offset++;
        }
    } else {
        folder = FTP_DOWNLOAD_FOLDER_NAME;
    }
    if (offset >= len) {
        goto __get_file_file_by_name_err;
    }
    path = zalloc(creat_path_len(root_path, folder, file_name, len - offset));
    if (!path) {
        goto __get_file_file_by_name_err;
    }
    creat_file_path(path, root_path, folder, file_name, len - offset);

    if (NULL == trans_back->file) {
        trans_back->file = fopen(path, "r");
    }

    if (path) {
        free(path);
    }

    if (NULL == trans_back->file) {
        // 文件不存在
        goto __get_file_file_by_name_err;
    }
    if (!file_trans_back_offset_is_valid(trans_back->file, file_offset)) {
        goto __get_file_file_by_name_err;
    }

    if (task_create(file_trans_back_task, priv, FILE_TRANS_BACK_TASK_NAME)) {
        goto __get_file_file_by_name_err;
    }
    return 0;

__get_file_file_by_name_err:
    rcsp_file_trans_back_close();
    return -1;
}

static int cancel_file_trans_back(u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = 0;
    if (trans_back && data && len >= 1) {
        trans_back->OpCode_SN = OpCode_SN;
        trans_back->op = FILE_TRANS_BACK_CANCEL;
        g_trans_back_cancel_flag = data[0];
#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
        rcsp_spp_tx_wait_wakeup();
#endif
    } else {
        ret = -1;
    }
    return ret;
}

// 关闭向手机发送文件
void rcsp_file_trans_back_close(void)
{
    if (!trans_back) {
        return;
    }

#if RCSP_FILE_TRANS_SPP_WAKEUP_FLOW_CTRL
    rcsp_spp_tx_wait_wakeup();
#endif
    task_kill(FILE_TRANS_BACK_TASK_NAME);
    g_trans_back_cancel_flag = -1;

    if (trans_back->fsn) {
        fscan_release(trans_back->fsn);
        trans_back->fsn = NULL;
    }

    // 关闭文件
    if (trans_back->file) {
        fclose(trans_back->file);
        trans_back->file = NULL;
    }

    if (trans_back) {
        free(trans_back);
        trans_back = NULL;
    }
}

static int get_file_by_clust(void *priv, u8 *data, u16 len)
{
    if (!data || len < 8) {
        goto __get_file_by_clust_err;
    }

    u32 file_offset = ((u32)data[0] << 24) | ((u32)data[1] << 16) |
                      ((u32)data[2] << 8) | data[3];
    u32 cluster = ((u32)data[4] << 24) | ((u32)data[5] << 16) |
                  ((u32)data[6] << 8) | data[7];
    trans_back->file_offset = file_offset;
    // find name by cluster
    if (NULL == trans_back->file) {
        if (NULL == trans_back->fsn) {
            trans_back->fsn = file_manager_scan_disk(trans_back->dev, NULL, scan_parm, 0, NULL);
        }
        if (NULL == trans_back->fsn) {
            goto __get_file_by_clust_err;
        }
        trans_back->file = file_manager_select(trans_back->dev, trans_back->fsn, FSEL_BY_SCLUST, cluster, NULL);
    }

    if (NULL == trans_back->fsn) {
        goto __get_file_by_clust_err;
    }

    if (NULL == trans_back->file) {
        goto __get_file_by_clust_err;
    }
    if (!file_trans_back_offset_is_valid(trans_back->file, file_offset)) {
        goto __get_file_by_clust_err;
    }

    if (task_create(file_trans_back_task, priv, FILE_TRANS_BACK_TASK_NAME)) {
        goto __get_file_by_clust_err;
    }
    return 0;

__get_file_by_clust_err:
    rcsp_file_trans_back_close();
    return -1;
}

// 向手机发送文件
void rcsp_file_trans_back_opt(void *priv, u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = 0;
    u8 offset = 0;
    u8 op;

    if (!data || !len) {
        return;
    }
    op = data[0];
    switch (op) {
    case FILE_TRANS_BACK_BY_NAME_2:
        if (len < 5) {
            ret = -1;
            break;
        }
        g_dev_handle = ((u32)data[1] << 24) | ((u32)data[2] << 16) |
                       ((u32)data[3] << 8) | data[4];
        offset = 5;
    /* fall through */
    case FILE_TRANS_BACK_BY_NAME:
        ret = get_file_prepare(g_dev_handle);
        if (ret) {
            break;
        }
        trans_back->OpCode_SN = OpCode_SN;
        trans_back->op = op;
        ret = get_file_by_name(priv, data + offset, len - offset);
        break;
    case FILE_TRANS_BACK_BY_CLUST:
        if (len < 5) {
            ret = -1;
            break;
        }
        g_dev_handle = ((u32)data[1] << 24) | ((u32)data[2] << 16) |
                       ((u32)data[3] << 8) | data[4];
        offset = 5;
        ret = get_file_prepare(g_dev_handle);
        if (ret) {
            break;
        }
        trans_back->OpCode_SN = OpCode_SN;
        trans_back->op = op;
        ret = get_file_by_clust(priv, data + offset, len - offset);
        break;
    case FILE_TRANS_BACK_CANCEL:
        ret = cancel_file_trans_back(OpCode_SN, data + 1, len - 1);
        break;
    }
    if (ret) {
        file_trans_back_response_send(&op, sizeof(op), (u8) - 1, OpCode_SN);
    }
}

#endif
