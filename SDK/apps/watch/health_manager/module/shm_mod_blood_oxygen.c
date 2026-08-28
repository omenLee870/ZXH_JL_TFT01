
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM_BO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif

#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_BLOOD_OXYGEN)

#define HEALTH_RATE_DATA_TEST 			1
#define HEALTH_FILE_TYPE				F_TYPE_BLOOD_OXYGEN
#define HEALTH_FILE_INVERVAL			(5)
#define HEALTH_FILE_TEST				0

struct blood_oxygen_data {
    u8 status;
    u8 bo_cur;
    u8 bo_max;
    u8 bo_min;
    u8 bo_avg;
    u16 avg_cnt;
#if HEALTH_FILE_INVERVAL
    u16 rec_timer_id;
#endif

};

#define BO_MAX_DEFAULT  (0x0)
#define BO_MIN_DEFAULT  (0xff)
#define BO_CUR_DEFAULT  (0x0)

static struct blood_oxygen_data __info = {0};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mod_value_clr 重置统计数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mod_value_clr()
{
    /* if(__info.status == SHM_MOD_STA_ENABLE){ */
    /* return -SHM_ERR_MOD_STA_ERR; */
    /* } */
    __info.bo_cur = BO_CUR_DEFAULT;
    __info.bo_min = BO_MIN_DEFAULT;
    __info.bo_max = BO_MAX_DEFAULT;
    __info.bo_avg = BO_CUR_DEFAULT;
    __info.avg_cnt = 0;
    return  SHM_ERR_OK;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_file_read  读取文件记录
 *
 * @param p
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_file_read(struct health_file_data_info *p)
{
    struct sys_time time;
#if 0//只读当天
    rtc_read_time(&time);
#else//由句柄指定日期
    memcpy(&time, &p->file_time, sizeof(struct sys_time));
#endif
    void *fp  = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);
    if (!fp) {
        return -SHM_ERR_FILE_NOT_FIND;
    }
    int file_len = sport_health_file_get_len(fp);
    int data_offset = sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);

    struct health_file_total_head file_total_head;
    sport_health_file_read(fp, (u8 *)&file_total_head, 0, sizeof(struct health_file_total_head));
    p->interval = file_total_head.interval;
    p->rlen = file_len - data_offset;
    p->rbuf = zalloc(p->rlen);
    sport_health_file_read(fp, p->rbuf, data_offset, p->rlen);
    return SHM_ERR_OK;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_file_save  保存数据到文件
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_file_save()
{
    int file_len = 0;
    struct sys_time time;
    rtc_read_time(&time);
    int data_offset = 0;
    int time_offset = 0;

    time_offset  = (time.hour * 60 + time.min)  / HEALTH_FILE_INVERVAL;
    log_info("%s %d %d %d", __func__, time.hour, time.min, time_offset);
    struct health_file_total_head file_head = {
        .type = HEALTH_FILE_TYPE,
        .year = time.year,
        .month = time.month,
        .day  = time.day,
        .crc  = 0xffff,
        .version = 0,
        .reserve = 0,
        .interval = HEALTH_FILE_INVERVAL,
    };

    struct health_file_data_head data_head = {
        .hour = 0,
        .min = 0,
        .len = time_offset + 1,
    };

    void *fp  = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);
    if (!fp) {
        fp = sport_health_file_open(HEALTH_FILE_TYPE, 0);
        sport_health_common_swapX((u8 *) &file_head.year, (u8 *) &file_head.year, 2);
        file_len = sport_health_file_write(fp, (u8 *) &file_head, file_len, sizeof(struct health_file_total_head));

        sport_health_common_swapX((u8 *) &data_head.len, (u8 *) &data_head.len, 2);
        file_len = sport_health_file_write(fp, (u8 *) &data_head, file_len, sizeof(struct health_file_data_head));

        //写入数据
        data_offset = time_offset +  sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);
        file_len = sport_health_file_write(fp, (u8 *) &__info.bo_cur, data_offset, 1);

        //更新校验值//只需要crc不同，即可，不做实际校验。
        file_head.crc =  time_offset;
        sport_health_file_update(fp, (u8 *) &file_head, 0, sizeof(struct health_file_total_head));
    } else {
        //续写
        file_len = sport_health_file_get_len(fp);
        data_offset = time_offset +  sizeof(struct health_file_total_head) + sizeof(struct health_file_data_head);
        if (data_offset > file_len) {
            //写入数据
            file_len = sport_health_file_write(fp, (u8 *) &__info.bo_cur, data_offset, 1);
            if (file_len <= 0) {
                sport_health_file_close(fp);
                return -SHM_ERR_FILE_WRITE_FAIL;
            }
            //更新校验值//只需要crc不同，即可，不做实际校验。
            file_head.crc =  time_offset;
            sport_health_common_swapX((u8 *) &file_head.year, (u8 *) &file_head.year, 2);
            sport_health_file_update(fp, (u8 *) &file_head, 0, sizeof(struct health_file_total_head));
            sport_health_common_swapX((u8 *) &data_head.len, (u8 *) &data_head.len, 2);
            file_len = sport_health_file_write(fp, (u8 *) &data_head, file_len, sizeof(struct health_file_data_head));
            if (file_len <= 0) {
                sport_health_file_close(fp);
                return -SHM_ERR_FILE_WRITE_FAIL;
            }
        }
    }
#if 0
    file_len = sport_health_file_get_len(fp);
    u8 *rw_buf = zalloc(file_len);
    sport_health_file_read(fp, rw_buf, 0, file_len);
    put_buf(rw_buf, file_len);
    free(rw_buf);
#endif
    if (fp) {
        sport_health_file_close(fp);
    }
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_mode_update_sec_deal 测量时更新数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int bo_mode_update_sec_deal()
{
    u8 blood_oxygen = 0;
    int ret = sport_health_manager_value_get(SHM_MOD_SPO2_ALGO, SHM_GET_TYPE_REAL_VALUE, &blood_oxygen);
#if HEALTH_RATE_DATA_TEST
    blood_oxygen = 80 + rand32() % 30;
#endif//HEALTH_RATE_DATA_TEST
    if (blood_oxygen) {
        __info.bo_cur = blood_oxygen;
        __info.bo_max = (__info.bo_cur > __info.bo_max) ? __info.bo_cur : __info.bo_max;
        __info.bo_min = (__info.bo_cur < __info.bo_min) ? __info.bo_cur : __info.bo_min;
        if (__info.avg_cnt) {
            u32 bo_sum  = __info.bo_avg * __info.avg_cnt + __info.bo_cur;
            __info.avg_cnt ++;
            __info.bo_avg = bo_sum / __info.avg_cnt;
        } else {
            __info.avg_cnt ++;
            __info.bo_avg = __info.bo_cur;
        }

        __info.status = SHM_MOD_STA_DATA_UPDATE_SUCC;
    }
    log_debug("%s bo:%d bo_cur:%d min:%d max:%d avg:%d", __func__, blood_oxygen, __info.bo_cur, __info.bo_min, __info.bo_max, __info.bo_avg);
    return  ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief bo_save_continue_cb  存储时更新数据回调
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
static void bo_save_continue_cb(void *p)
{
    sport_health_manager_msg_post(SHM_MOD_HEART_RATE, SHM_CMD_SAVE_CONTINUE, NULL, 0);

}
static int bo_mod_io_ctrl(int cmd, void *priv)
{
    log_info("%s cmd=%d status:%d", __func__, cmd, __info.status);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        bo_mod_value_clr();
#if 1
        sport_health_manager_msg_post_self(SHM_MOD_HEART_RATE, SHM_CMD_REC_ENABLE, NULL);
#endif
        break;
    case SHM_CMD_ENABLE:
        //todo
        sport_health_manager_msg_post_self(SHM_MOD_SPO2_ALGO, SHM_CMD_ENABLE, NULL);
        __info.status = SHM_MOD_STA_ENABLE;
        break;
    case SHM_CMD_DISBALE:
        __info.status = SHM_MOD_STA_DISABLE;
        //todo
        sport_health_manager_msg_post_self(SHM_MOD_SPO2_ALGO, SHM_CMD_DISBALE, NULL);
        break;
    case SHM_CMD_REC_ENABLE:
        if (!__info.rec_timer_id) {
            __info.rec_timer_id = usr_timer_add(NULL, bo_save_continue_cb, HEALTH_FILE_INVERVAL * 60 * 1000, 0);
        }
        break;
    case SHM_CMD_REC_DISABLE:
        if (__info.rec_timer_id) {
            usr_timer_del(__info.rec_timer_id);
            __info.rec_timer_id = 0;
        }
        break;
    case SHM_CMD_UPDATE:
    case SHM_CMD_UPDATE_SEC:
        if (__info.status == SHM_MOD_STA_ENABLE || (__info.status == SHM_MOD_STA_DATA_UPDATE_SUCC) || (__info.status == SHM_MOD_STA_DATA_UPDATE_NULL)) {
            //TODO
            bo_mode_update_sec_deal();
        }
        break;
    case SHM_CMD_SAVE_CONTINUE:
#if HEALTH_FILE_INVERVAL
    {
        int sensor_close  = (__info.status == SHM_MOD_STA_DISABLE) ? 1 : 0;
        if (sensor_close) {
            sport_health_manager_msg_post_self(SHM_MOD_SPO2_ALGO, SHM_CMD_ENABLE, NULL);
            __info.status = SHM_MOD_STA_ENABLE;
        }
        bo_mode_update_sec_deal();
        bo_mode_file_save();
        if (sensor_close) {
            __info.status = SHM_MOD_STA_DISABLE;
            //todo
            sport_health_manager_msg_post_self(SHM_MOD_SPO2_ALGO, SHM_CMD_DISBALE, NULL);
        }
    }
#endif
    break;
    case SHM_CMD_CLEAR_VALUE:
        __info.status = SHM_MOD_STA_DATA_UPDATE_NULL;
        //todo
        bo_mod_value_clr();
        break;
    default:
        break;
    }
    return ret;
}

static int bo_mod_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_cur, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_MAX_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_max, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_MIN_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_min, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_AVG_VALUE:
        if (__info.status ==  SHM_MOD_STA_DATA_UPDATE_SUCC) {
            memcpy(priv, &__info.bo_avg, 1);
        } else {
            ret = SHM_ERR_DATA_ERR;
        }
        break;
    case SHM_GET_TYPE_REC_VALUE: {
        ret = bo_mode_file_read((struct health_file_data_info *)priv);
    }
    break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(blood_oxygen)
{
    .module = SHM_MOD_BLOOD_OXYGEN,
     .io_ctrl = bo_mod_io_ctrl,
      .get_value = bo_mod_get_value,
};
#endif
