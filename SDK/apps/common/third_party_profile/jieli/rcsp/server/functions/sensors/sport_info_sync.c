#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sport_info_sync.data.bss")
#pragma data_seg(".sport_info_sync.data")
#pragma const_seg(".sport_info_sync.text.const")
#pragma code_seg(".sport_info_sync.text")
#endif
#include "rcsp_config.h"
#include "sport_info_sync.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_api.h"

#if (RCSP_MODE && JL_RCSP_SENSORS_DATA_OPT)
#define LOG_TAG_CONST      	RCSP
#define LOG_TAG     		"[RCSP_SPORT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

enum {
    SPORT_INFO_SYNC_READ_INFO,
    SPORT_INFO_SYNC_STRTT_EXERCISE,
    SPORT_INFO_SYNC_END_EXERCISE_BY_APP,
    SPORT_INFO_SYNC_END_EXERCISE_BY_FW,
    SPORT_INFO_SYNC_PAUSE_EXERCISE,
    SPORT_INFO_SYNC_KEEP_EXERCISE,
    SPORT_INFO_SYNC_READ_REAL_TIME_INFO,
    SPORT_INFO_SYNC_REAL_TIME_INTERVAL_SET,
};

static struct rcsp_watch_execise *__execise_hd = NULL;

// 2字节数据的小端转大端
static u16 swap_endian_2bytes(u16 value)
{
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

// 4字节数据的小端转大端
static u32 swap_endian_4bytes(u32 value)
{
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0xFF000000) >> 24);
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_read_info 同步运动信息
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_read_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = 0;

    u8 rcsp_data[1 + 10] = {0};
    rcsp_data[0] = SPORT_INFO_SYNC_READ_INFO;

    u8 ctrl_status = __execise_hd->execise_ctrl_status_get();
    if (ctrl_status ==  RCSP_SPORT_STATUS_NULL) { // rcsp_data[1 + 0] = 运动模式，失败ret = 非0，直接goto __sport_info_sync_read_info_end
        rcsp_data[1] = 0x00;
    } else {
        rcsp_data[1] =  __execise_hd->execise_mode_get();

    }
    u32 e_start_time = __execise_hd->get_sport_start_time(NULL);

    rcsp_data[1 + 1] = ((ctrl_status == RCSP_SPORT_STATUS_START) || (ctrl_status == RCSP_SPORT_STATUS_CONTINNUE)) ? 0x1 : 0x0;

    rcsp_data[1 + 2] = (e_start_time >> 24 & 0xff); // rcsp_data[1 + (2:5)] = 运动id，这里需要转化为大端，失败同上
    rcsp_data[1 + 3] = (e_start_time >> 16 & 0xff);
    rcsp_data[1 + 4] = (e_start_time >> 8  & 0xff);
    rcsp_data[1 + 5] = (e_start_time >> 0  & 0xff);

    rcsp_data[1 + 6] = 0x00; // rcsp_data[1 + 6] = 是否需要app记录gps，失败同上
    rcsp_data[1 + 7] = 0x00; // rcsp_data[1 + 7] = 最大心率，失败同上
    rcsp_data[1 + 8] = 0x03; // rcsp_data[1 + (8:9)] = app读取定时读取数据的间隔时间，这里需要转化为大端，失败同上
    rcsp_data[1 + 9] = 0xE8;
    log_debug("%s run_status:%d ", __func__, ctrl_status);
__sport_info_sync_read_info_end:
    if (ret) {
        // 失败
        log_error("%s error", __func__);
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        // 成功
        log_info("%s succ!", __func__);
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, rcsp_data, sizeof(rcsp_data), 0, NULL);
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_start_exercise_rcsp 开始运动
 *
 * @param data			null时为手表端发起
 * @param data_len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_info_sync_start_exercise_rcsp(u8 *data, u16 *data_len)
{
    int ret = 0;
    u8 rcsp_data[1 + 1] = {0};
    rcsp_data[0] = SPORT_INFO_SYNC_STRTT_EXERCISE;
    // rcsp_data[1] = 运动模式，如果失败ret = 非0，直接goto __sport_info_sync_sync_start_exercise_nodify_end

    u8 ctrl_status = __execise_hd->execise_ctrl_status_get();
    if (ctrl_status == 0) {
        rcsp_data[1] = 0x00;
    } else {
        rcsp_data[1] = __execise_hd->execise_mode_get();
    }
    log_debug("%s run_status:%d mode:%d", __func__, ctrl_status, rcsp_data[1]);
__sport_info_sync_sync_start_exercise_nodify_end:
    if (data) {
        memcpy(data, rcsp_data, sizeof(rcsp_data));
        *data_len = sizeof(rcsp_data);
    } else if (0 == ret) {
        // 固件 -> app
        JL_CMD_send(JL_OPCODE_SPORTS_DATA_SYNC, rcsp_data, sizeof(rcsp_data), JL_NOT_NEED_RESPOND, 0, NULL);
    }

    return ret;

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_start_exercise app端发起运动
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_start_exercise(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    // app触发开始运动
    u8 sport_status = __execise_hd->execise_ctrl_status_get();
    if (sport_status == 0) {
        __execise_hd->execise_ctrl_status_set(data[0], RCSP_SPORT_STATUS_START);
    }

    int ret = sport_info_sync_start_exercise_rcsp(data, &len);
    log_debug("%s mode:%d", __func__, data[0]);
    if (ret) {
        // 失败
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        // 成功
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, len, 0, NULL);
    }

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_end_exercise_by_app app端结束运动
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_end_exercise_by_app(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    int ret = 0;
    u8 rcsp_data[1 + 32] = {0};
    u8 sport_status = __execise_hd->execise_ctrl_status_get();
    log_debug("sport_status=%d", sport_status);
    if (sport_status != 0) {
        __execise_hd->execise_ctrl_status_set(-1, RCSP_SPORT_STATUS_STOP);
        __execise_hd->execise_ctrl_status_clr();
    }
    rcsp_data[0] = SPORT_INFO_SYNC_END_EXERCISE_BY_APP;
    u32 e_end_time = __execise_hd->get_sport_end_time(NULL); // rcsp_data[1 + (0:3)] = 结束时间，这里需要转化成大端，失败ret = 非0，直接goto __sport_info_sync_end_exercise_by_app_end
    rcsp_data[1 + 0] = (e_end_time >> 24) & 0xff;
    rcsp_data[1 + 1] = (e_end_time >> 16) & 0xff;
    rcsp_data[1 + 2] = (e_end_time >> 8) & 0xff;
    rcsp_data[1 + 3] = (e_end_time) & 0xff;
    //	u32 e_recovery_time=get_recovery_time(NULL);// rcsp_data[1 + (4:7)] = 运动回复时间，这里需要转化为大端，失败同上
    rcsp_data[1 + 4] = 0;
    rcsp_data[1 + 5] = 1;
    rcsp_data[1 + 6] = 0;
    rcsp_data[1 + 7] = 10;
    u16 e_id = __execise_hd->get_sport_recode_id();
    u16 e_size = __execise_hd->get_sport_recode_size();
    rcsp_data[1 + 8] = (e_id >> 8) & 0xff;
    rcsp_data[1 + 9] = e_id & 0xff;
    rcsp_data[1 + 10] = (e_size >> 8) & 0xff;
    rcsp_data[1 + 11] = (e_size & 0xff);
    int e_intensity_time[5] = {0};
    __execise_hd->execise_info_get_intensity_time(e_intensity_time, sizeof(e_intensity_time));
    for (int i = 0; i < 5; i++) {
        rcsp_data[1 + 12 + 4 * i] = (e_intensity_time[i] >> 24) & 0xff;
        rcsp_data[1 + 13 + 4 * i] = (e_intensity_time[i] >> 16) & 0xff;
        rcsp_data[1 + 14 + 4 * i] = (e_intensity_time[i] >> 8) & 0xff;
        rcsp_data[1 + 15 + 4 * i] = (e_intensity_time[i]) & 0xff;
    }

    // rcsp_data[1 + (8:31)] = 运动强度，需要转化为大端,失败同上
    __execise_hd->execise_ctrl_status_clr();
__sport_info_sync_end_exercise_by_app_end:
    if (ret) {
        // 失败
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        // 成功
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, rcsp_data, sizeof(rcsp_data), 0, NULL);
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_end_exercise_by_fw 手表端结束运动
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_info_sync_end_exercise_by_fw(void)
{
    int ret = 0;
    u8 rcsp_data[1 + 32] = {0};
    rcsp_data[0] = SPORT_INFO_SYNC_END_EXERCISE_BY_FW;
    u32 e_end_time = __execise_hd->get_sport_end_time(NULL);
    // rcsp_data[1 + (0:3)] = 结束时间，这里需要转化成大端，失败ret = 非0，直接goto __sport_info_sync_end_exercise_by_fw_end
    rcsp_data[1 + 0] = (e_end_time >> 24) & 0xff;
    rcsp_data[1 + 1] = (e_end_time >> 16) & 0xff;
    rcsp_data[1 + 2] = (e_end_time >> 8) & 0xff;
    rcsp_data[1 + 3] = (e_end_time) & 0xff;

    // rcsp_data[1 + (4:7)] = 运动回复时间，这里需要转化为大端，失败同上
    rcsp_data[1 + 4] = 0;
    rcsp_data[1 + 5] = 1;
    rcsp_data[1 + 6] = 0;
    rcsp_data[1 + 7] = 10;
    u16 e_id = __execise_hd->get_sport_recode_id();
    u16 e_size = __execise_hd->get_sport_recode_size();
    rcsp_data[1 + 8] = (e_id >> 8) & 0xff;
    rcsp_data[1 + 9] = e_id & 0xff;
    rcsp_data[1 + 10] = (e_size >> 8) & 0xff;
    rcsp_data[1 + 11] = (e_size & 0xff);
    // rcsp_data[1 + (8:31)] = 运动强度，需要转化为大端？失败同上
    int e_intensity_time[5] = {0};
    __execise_hd->execise_info_get_intensity_time(e_intensity_time, sizeof(e_intensity_time));
    for (int i = 0; i < 5; i++) {
        rcsp_data[1 + 12 + 4 * i] = (e_intensity_time[i] >> 24) & 0xff;
        rcsp_data[1 + 13 + 4 * i] = (e_intensity_time[i] >> 16) & 0xff;
        rcsp_data[1 + 14 + 4 * i] = (e_intensity_time[i] >> 8) & 0xff;
        rcsp_data[1 + 15 + 4 * i] = (e_intensity_time[i]) & 0xff;
    }
__sport_info_sync_end_exercise_by_fw_end:
    if (0 == ret) {
        JL_CMD_send(JL_OPCODE_SPORTS_DATA_SYNC, rcsp_data, sizeof(rcsp_data), JL_NOT_NEED_RESPOND, 0, NULL);
    }
    return ret;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_pause_exercise_rcsp 手表端暂停运动
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_info_sync_pause_exercise_rcsp(void)
{
    u8 rcsp_data = SPORT_INFO_SYNC_PAUSE_EXERCISE;
    JL_CMD_send(JL_OPCODE_SPORTS_DATA_SYNC, &rcsp_data, sizeof(rcsp_data), JL_NOT_NEED_RESPOND, 0, NULL);
    return 0;

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_pause_exrcise APP端暂停运动
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_pause_exrcise(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 sport_status = __execise_hd->execise_ctrl_status_get();
    printf("sport_status=%d", sport_status);
    if ((sport_status == 1) | (sport_status == 3)) {
        __execise_hd->execise_ctrl_status_set(-1, RCSP_SPORT_STATUS_PAUSE);
    }
    u8 rcsp_data = SPORT_INFO_SYNC_PAUSE_EXERCISE;
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &rcsp_data, 1, 0, NULL);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_keep_exercise_rcsp 手表端继续运动
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_info_sync_keep_exercise_rcsp(void)
{
    u8 rcsp_data = SPORT_INFO_SYNC_KEEP_EXERCISE;
    JL_CMD_send(JL_OPCODE_SPORTS_DATA_SYNC, &rcsp_data, sizeof(rcsp_data), JL_NOT_NEED_RESPOND, 0, NULL);
    return 0;

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_keep_exercise app端继续运动
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_keep_exercise(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    u8 sport_status = __execise_hd->execise_ctrl_status_get();
    log_debug("sport_status=%d", sport_status);
    if (sport_status == 2) {
        __execise_hd->execise_ctrl_status_set(-1, RCSP_SPORT_STATUS_CONTINNUE);
    }
    u8 rcsp_data = SPORT_INFO_SYNC_KEEP_EXERCISE;
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &rcsp_data, 1, 0, NULL);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_info_sync_read_real_time_info 运动中实时更新数据
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void sport_info_sync_read_real_time_info(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    printf("%s", __func__);

    int ret = 0;
    u8 rcsp_data[1 + 21] = {0};
    rcsp_data[0] = SPORT_INFO_SYNC_READ_REAL_TIME_INFO;
    rcsp_data[1 + 0] = 0x00; // rcsp_data[1 + 0] = 类型，失败ret = 非0，goto __sport_info_sync_read_real_time_info_end
    // rcsp_data[1 + (1:4)] = 运动步数，需要转化为大端，失败同上
    u32 e_step = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_STEPS);
    rcsp_data[1 + 1] = (e_step >> 24) & 0xff;
    rcsp_data[1 + 2] = (e_step >> 16) & 0xff;
    rcsp_data[1 + 3] = (e_step >> 8) & 0xff;
    rcsp_data[1 + 4] = (e_step) & 0xff;
    // rcsp_data[1 + (5:6)] = 运动距离，需要转化为大端，失败同上
    u16 e_distance = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_DISTANCE);
    rcsp_data[1 + 5] = (e_distance >> 8);
    rcsp_data[1 + 6] = e_distance & 0xff;
    // rcsp_data[1 + (7:10)] = 运动时长，需要转化为大端，失败同上
    u32 e_motion_time = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_MOTION_TIME); //跟UI同步时间
    rcsp_data[1 + 7] = (e_motion_time >> 24) & 0xff;
    rcsp_data[1 + 8] = (e_motion_time >> 16) & 0xff;
    rcsp_data[1 + 9] = (e_motion_time >> 8) & 0xff;
    rcsp_data[1 + 10] = (e_motion_time) & 0xff;
    // rcsp_data[1 + (11:12)] = 速度，需要转化为大端，失败同上
    u16 e_speed = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_SPEED);
    rcsp_data[1 + 11] = e_speed >> 8;
    rcsp_data[1 + 12] = e_speed & 0xff;
    // rcsp_data[1 + (13:14)] = 热量，需要转化为大端，失败同上
    u16 e_kcal = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_KCAL);
    rcsp_data[1 + 13] = e_kcal >> 8;
    rcsp_data[1 + 14] = e_kcal & 0xff;
    // rcsp_data[1 + (15:16)] = 步频，需要转化为大端，失败同上
    u16 e_step_freq = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_STEP_FREQ);
    rcsp_data[1 + 15] = e_step_freq >> 8;
    rcsp_data[1 + 16] = e_step_freq & 0xff;
    // rcsp_data[1 + (17:18)] = 步幅，需要转化为大端，失败同上
    u16 e_step_stride = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_STEP_STRIDE);
    rcsp_data[1 + 17] = e_step_stride >> 8;
    rcsp_data[1 + 18] = e_step_stride & 0xff;
    // rcsp_data[1 + 19] = 运动强度区间，失败同上
    rcsp_data[1 + 19] = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_EXERCISE_INTENSITY);
    // rcsp_data[1 + 20] = 运动实时心率，失败同上
    rcsp_data[1 + 20] = __execise_hd->execise_info_get_data(RCSP_SPORT_DATA_HR);
    log_debug("%x-0- %04x %02x %04x %04x %02x %02x %02x", rcsp_data[0], e_step, e_distance, e_motion_time, e_speed, e_kcal, e_step_freq, e_step_stride);
__sport_info_sync_read_real_time_info_end:
    if (ret) {
        // 失败
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
    } else {
        // 成功
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, rcsp_data, sizeof(rcsp_data), 0, NULL);
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief JL_rcsp_sports_info_sync_real_time_interval_set 测试用接口，调整周期
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
static void JL_rcsp_sports_info_sync_real_time_interval_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    // 测试使用的命令，固件实时数据采集间隔时间
    u16 real_time_interval = data[0] << 8 | data[1];

    u8 rcsp_data = SPORT_INFO_SYNC_REAL_TIME_INTERVAL_SET;
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &rcsp_data, 1, 0, NULL);
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief JL_rcsp_sports_info_sync_funciton 功能入口
 *
 * @param priv
 * @param OpCode
 * @param OpCode_SN
 * @param data
 * @param len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int JL_rcsp_sports_info_sync_funciton(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)

{
    int ret = -1;
    if (!__execise_hd) {
        return ret;
    }
    if (JL_OPCODE_SPORTS_DATA_SYNC == OpCode) {
        u8 op = data[0];
        switch (op) {
        case SPORT_INFO_SYNC_READ_INFO:
            sport_info_sync_read_info(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_STRTT_EXERCISE:
            sport_info_sync_start_exercise(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_END_EXERCISE_BY_APP:
            sport_info_sync_end_exercise_by_app(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_PAUSE_EXERCISE:
            sport_info_sync_pause_exrcise(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_KEEP_EXERCISE:
            sport_info_sync_keep_exercise(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_READ_REAL_TIME_INFO:
            sport_info_sync_read_real_time_info(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        case SPORT_INFO_SYNC_REAL_TIME_INTERVAL_SET:
            JL_rcsp_sports_info_sync_real_time_interval_set(priv, OpCode, OpCode_SN, data + 1, len - 1);
            break;
        }
        ret = 0;
    }
    return ret;
}

int rcsp_register_sport_info_sync_interface(struct rcsp_watch_execise *sport_info_sync_interface)
{
    __execise_hd = sport_info_sync_interface;
    return 0;
}

#else
int sport_info_sync_end_exercise_by_fw(void)
{
    return 0;
}
int sport_info_sync_start_exercise_rcsp(u8 *data, u16 *data_len)
{
    return 0;
}
int sport_info_sync_keep_exercise_rcsp(void)
{
    return 0;
}
int sport_info_sync_pause_exercise_rcsp(void)
{
    return 0;
}
#endif

