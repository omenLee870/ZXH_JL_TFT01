#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"
#include "timestamp/timestamp.h"
#include "health_manager/health_manager.h"


#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM_SLEEP]"
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




#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_SLEEP)
//睡眠记录配置
#define HEALTH_FILE_TYPE						F_TYPE_SLEEP
#define HEALTH_FILE_VERSION						(0)		//协议约定
#define HEALTH_FILE_INTERVAL					(0Xff)  //协议约定
#define TCFG_SPORT_HEALTH_AWAKE_MAX_LEN			30	//30min的清醒数据将被过滤

//测试数据配置
#define TCFG_SPORT_HEALTH_SLEEP_TEST			0
#define TCFG_SPORT_HEALTH_SLEEP_TEST_YEAR		2024
#define TCFG_SPORT_HEALTH_SLEEP_TEST_MONTH		9
#define TCFG_SPORT_HEALTH_SLEEP_TEST_DAY		7
#define TCFG_SPORT_HEALTH_SLEEP_TARGET_MIN		(8*60*60)//7h

#pragma pack(1)//
struct sleep_storage_data {
    u8 stage;
    u8 sleep_min;//min
};
#pragma pack()//

static void motion_sleep_data_test_get(sleep_data *out);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief motion_sleep_data_get_switch 睡眠数据选择接口
 *
 * @param out	数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int motion_sleep_data_get_switch(sleep_data *out)
{
    //使用模拟测试数据
#if TCFG_SPORT_HEALTH_SLEEP_TEST
    motion_sleep_data_test_get(out);
#else
    //使用JL算法数据
    //若当前处于睡眠，则拒绝数据获取
    if (!algo_motion_sleep_is_terminated()) {
        return false;
    }
    //获取数据
    algo_motion_sleep_get(out);
#endif

    return true;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_sleep_data_stroage 睡眠数据存储
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int sport_health_sleep_data_stroage()
{
    sleep_data out = {0};
    //获取睡眠数据
    if (!motion_sleep_data_get_switch(&out)) {
        //需要忽略本次数据
        return SHM_ERR_OK;
    }
    //没有数据，或者只有清醒数据，忽略
    if ((out.blocks == 0) || ((out.blocks == 1) && (out.chart[0].stage == SLEEP_STAGE_AWAKE))) {
        return SHM_ERR_OK;
    }
    //获取系统时间
    struct sys_time time;
    rtc_read_time(&time);
#if TCFG_SPORT_HEALTH_SLEEP_TEST	//测试使用
    struct sys_time _time = {
        .year = TCFG_SPORT_HEALTH_SLEEP_TEST_YEAR,
        .month = TCFG_SPORT_HEALTH_SLEEP_TEST_MONTH,
        .day = TCFG_SPORT_HEALTH_SLEEP_TEST_DAY + 1,
        .hour = 22,
        .min = 30,
    };
    memcpy(&time, &_time, sizeof(struct sys_time));
#endif
    //获取文件句柄
    int file_len = 0;
    int swap_days = 0;
    void *fp  = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);

#if TCFG_SPORT_HEALTH_SLEEP_TEST//测试使用
    if (fp) {
        sport_health_file_delete(fp);
        sport_health_file_close(fp);
        fp = NULL;
    }
#endif
    if (!fp) {
        //若句柄不存在，则新建文件
        struct health_file_total_head file_head = {
            .type = HEALTH_FILE_TYPE,
            .year = time.year,
            .month = time.month,
            .day  = time.day,
            .crc  = 0,
            .interval = HEALTH_FILE_INTERVAL,
            .version = HEALTH_FILE_VERSION,
            .reserve = 0,
        };
        sport_health_common_swapX((u8 *)&file_head.year, (u8 *)&file_head.year, 2);
        fp = sport_health_file_open(HEALTH_FILE_TYPE, 0);
        file_len = sport_health_file_write(fp, (u8 *)&file_head, file_len, sizeof(struct health_file_total_head));
    } else {
        //否则续写
        file_len = sport_health_file_get_len(fp);
    }

    u8 valid_point_num = 0;
    u8 data_head_flag = 0;
    struct health_file_data_head data_head = {0};
    for (int i = 0; i < out.blocks - 1; i++) {
        //数据开头的清醒数据不需要记录
        if ((i == 0) && (out.chart[i].stage == SLEEP_STAGE_AWAKE)) {
            continue;
        }
        //写入数据包头
        if (!data_head_flag) {
            struct sys_time start_time;
            timestamp_utc_sec_2_mytime(out.chart[i].timestamp, &start_time);
            data_head .hour = start_time.hour;
            data_head .min = start_time.min;
            data_head .len = 0xffff;
            file_len = sport_health_file_write(fp, (u8 *)&data_head, file_len, sizeof(struct health_file_data_head));
            data_head_flag = 1;
        }
        //按格式写入数据到文件
        int block_len = out.chart[i + 1].timestamp - out.chart[i].timestamp;
        struct sys_time block_time_begin;
        timestamp_utc_sec_2_mytime(out.chart[i].timestamp, &block_time_begin);
        struct sys_time block_time_end;
        timestamp_utc_sec_2_mytime(out.chart[i + 1].timestamp, &block_time_end);
        struct sleep_storage_data sleep_block = {
            .stage = (out.chart[i].stage == SLEEP_STAGE_AWAKE) ? 0xff : out.chart[i].stage,
        };

        log_debug("[%d:%d]~[%d %d]", block_time_begin.hour, block_time_begin.min, block_time_end.hour, block_time_end.min);
        if (block_time_end.hour * 60 + block_time_end.min < block_time_begin.hour * 60 + block_time_begin.min) {
            //存在跨天数据，则在0点分段写入，方便绘图
            swap_days = 1;
            int sleep_min = 24 * 60 - (block_time_begin.hour * 60 + block_time_begin.min);
            do {
                //睡眠时间超过255分钟时，需要分段写入
                if (sleep_min > 255) {
                    sleep_block.sleep_min = 255;
                    log_debug("[1]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                    sleep_min -= 255;
                } else {
                    sleep_block.sleep_min = sleep_min;
                    log_debug("[2]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                }
            } while (sleep_min > 255);

            sleep_min = block_time_end.hour * 60 + block_time_end.min;
            do {
                //睡眠时间超过255分钟时，需要分段写入
                if (sleep_min > 255) {
                    sleep_block.sleep_min = 255;
                    log_debug("[3]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                    sleep_min -= 255;
                } else {
                    sleep_block.sleep_min = sleep_min;
                    log_debug("[4]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                }
            } while (sleep_min > 255);
        } else { //正常写入
            int sleep_min = (out.chart[i + 1].timestamp - out.chart[i].timestamp + 30) / 60;
            do {
                //睡眠时间超过255分钟时，需要分段写入
                if (sleep_min > 255) {
                    sleep_block.sleep_min = 255;
                    log_debug("[5]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                    sleep_min -= 255;
                } else {
                    sleep_block.sleep_min = sleep_min;
                    log_debug("[6]index:%d stage:%d len:%d", valid_point_num, sleep_block.stage, sleep_block.sleep_min);
                    file_len = sport_health_file_write(fp, (u8 *)&sleep_block, file_len, sizeof(struct sleep_storage_data));
                    valid_point_num++;
                }
            } while (sleep_min > 255);
        }
    }
    //记录数据长度
    data_head.len = valid_point_num * 2;
    sport_health_common_swapX((u8 *) & (data_head.len), (u8 *) & (data_head.len), 2);
    sport_health_file_update(fp, (u8 *)&data_head, sizeof(struct health_file_total_head), sizeof(struct health_file_data_head));
    //更新文件头的crc和跨天信息
    struct health_file_total_head re_file_head;
    sport_health_file_read(fp, (u8 *)& re_file_head, 0, sizeof(struct health_file_total_head));
    for (int i = 0; i < 16; i++) {
        //用reserve区记录分段信息
        re_file_head.reserve |= (swap_days << re_file_head.crc);
        //CRC做强制校验，有差异即可
        re_file_head.crc++;
    }
    sport_health_file_update(fp, (u8 *)&re_file_head, 0, sizeof(struct health_file_total_head));
    //关闭文件，结束记录
    sport_health_file_close(fp);
    return SHM_ERR_OK;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_sleep_rec_data_get  获取记录数据
 *
 * @param value		句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int sport_health_sleep_rec_data_get(struct sleep_data_analysis *value)
{
    sleep_data *out = (sleep_data *)value->data;
    //获取日期
    struct sys_time time;
#if 0//只读当天
    rtc_read_time(&time);
#else//由句柄指定日期
    memcpy(&time, &value->file_time, sizeof(struct sys_time));
#endif
    //打开文件
    void *fp = sport_health_file_open_by_time(HEALTH_FILE_TYPE, time.year, time.month, time.day);
    if (!fp) {
        out->blocks = 0;
        //文件不存在
        return -SHM_ERR_FILE_NOT_FIND;
    }

    /* int block_idx = 0; */
    int file_total = sport_health_file_get_len(fp);
    int file_offset  = 0;
    //读取文件头
    struct health_file_total_head total_head;
    sport_health_file_read(fp, (u8 *)&total_head, 0, sizeof(struct health_file_total_head));
    sport_health_common_swapX((u8 *)&total_head.year, (u8 *)&total_head.year, 2);

    file_offset += sizeof(struct health_file_total_head);
    struct sys_time start_time;

    u32 timestamp_start;
    u32 timestamp_block;
    //分段读睡眠数据
    int sleep_data_cnt = 0;
    do {
        //数据包头
        struct health_file_data_head data_head;
        sport_health_file_read(fp, (u8 *)&data_head, file_offset, sizeof(struct health_file_data_head));
        file_offset += sizeof(struct health_file_data_head);

        start_time.year = total_head.year;
        start_time.month = total_head.month;
        start_time.day  = total_head.day;
        start_time.hour = data_head.hour;
        start_time.min = data_head.min;
        start_time.sec = 0;
        timestamp_start = timestamp_mytime_2_utc_sec(&start_time);
        //数据跨天
        if (total_head.reserve & BIT(sleep_data_cnt)) {
            timestamp_start -= 24 * 60 * 60;
        }
        timestamp_block = timestamp_start;

        u16 blocks = data_head.len;
        //异常数据退出
        if (blocks == 0xffff) {
            break;
        }

        sport_health_common_swapX((u8 *)&blocks, (u8 *)&blocks, 2);
        u8 *rbuf = zalloc(blocks);
        sport_health_file_read(fp, rbuf, file_offset, blocks);
        file_offset += blocks;

        struct sleep_storage_data *p = (struct sleep_storage_data *)rbuf;
        //读取该段数据
        for (int i = 0; i < blocks / 2; i++) {
            //记录睡眠的状态和起始时间戳
            p->stage = (p->stage == 0xff) ? SLEEP_STAGE_AWAKE : p->stage;
            out->chart[out->blocks].stage = p->stage;
            out->chart[out->blocks].timestamp = timestamp_block;
#if 0		//debug使用
            struct sys_time block_t;
            timestamp_utc_sec_2_mytime(out->chart[out->blocks].timestamp, &block_t);
            printf("[SLEEP_TEST_READ]%d STA:%d [%4d-%2d-%2d-%2d:%2d:%2d]", out->blocks, out->chart[out->blocks].stage,
                   block_t.year, block_t.month, block_t.day, block_t.hour, block_t.min, block_t.sec);
#endif//TCFG_SPORT_HEALTH_SLEEP_TEST
            //sw下一段睡眠
            timestamp_block += p->sleep_min * 60;
            out->blocks++;
            p++;
        }
        //清醒作为包尾,用特殊值标记，跟清醒数据区分
        out->chart[out->blocks].stage = SLEEP_STAGE_BLOCK_END;
        out->chart[out->blocks].timestamp = timestamp_block;
        out->blocks++;

        free(rbuf);
    } while (file_offset < file_total);

    sport_health_file_close(fp);
    return SHM_ERR_OK;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_health_sleep_data_analysis 解析睡眠数据
 *
 * @param value		句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int sport_health_sleep_data_analysis(struct sleep_data_analysis *value)
{
    sleep_data *data = (sleep_data *)value->data;

    for (int i = 0; i < data->blocks - 1; i++) {
        int time_len = data->chart[i + 1].timestamp -	data->chart[i].timestamp;
        //统计各类型睡眠的时长
        if (data->chart[i].stage == SLEEP_STAGE_DEEP) {
            value->deep_min += time_len;
        } else if (data->chart[i].stage == SLEEP_STAGE_LIGHT) {
            value->light_min += time_len;
        } else if (data->chart[i].stage == SLEEP_STAGE_REM) {
            value->rem_min += time_len;
        } else if (data->chart[i].stage == SLEEP_STAGE_AWAKE) {
            //可能包含有较长的清醒数据,根据项目需求进行过滤
#if TCFG_SPORT_HEALTH_AWAKE_MAX_LEN
            time_len =  time_len > TCFG_SPORT_HEALTH_AWAKE_MAX_LEN ? 0 : time_len;
#endif
            value->awake_min += time_len;
        } else {
            time_len = 0;
        }
        value->total_min += time_len;
    }
    return SHM_ERR_OK;
}

static int shm_sleep_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_SAVE_SINGLE:
        ret = sport_health_sleep_data_stroage();
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }
    return ret;
}

static int shm_sleep_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
        break;
    case SHM_GET_TYPE_INFO_STORAGE:
        //get date from file
        ret = sport_health_sleep_rec_data_get((struct sleep_data_analysis *)priv);
        break;
    case SHM_GET_TYPE_DATA_ANALYSIS:
        ret = sport_health_sleep_data_analysis((struct sleep_data_analysis *)priv);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(sleep)
{
    .module =  SHM_MOD_SLEEP,
     .io_ctrl = shm_sleep_io_crtl,
      .get_value = shm_sleep_get_value,
};

/*****************************************************************************************************/
#if TCFG_SPORT_HEALTH_SLEEP_TEST
/* ------------------------------------------------------------------------------------*/
/**
 * @brief motion_sleep_data_test_get 模拟数据
 *
 * @param out
 */
/* ------------------------------------------------------------------------------------*/
static void motion_sleep_data_test_get(sleep_data *out)
{
    struct sys_time time = {
        .year = TCFG_SPORT_HEALTH_SLEEP_TEST_YEAR,
        .month = TCFG_SPORT_HEALTH_SLEEP_TEST_MONTH,
        .day = TCFG_SPORT_HEALTH_SLEEP_TEST_DAY,
        .hour = 22,
        .min = 30,
    };
    struct sys_time block_t;
    int timestamp_tmp = timestamp_mytime_2_utc_sec(&time);
    int sleep_time = 0;
    out->blocks = 0;
    do {
        out->chart[out->blocks].stage = rand32() % 3;
        out->chart[out->blocks].timestamp = timestamp_tmp;
        timestamp_utc_sec_2_mytime(out->chart[out->blocks].timestamp, &block_t);
        int len  = rand32() % 3000 + 10 * 60;
        printf("[SLEEP_TEST_GET]%d STA:%d [%4d-%2d-%2d-%2d:%2d:%2d]len:%d \n", out->blocks, out->chart[out->blocks].stage,
               block_t.year, block_t.month, block_t.day, block_t.hour, block_t.min, block_t.sec, len);
        timestamp_tmp += len;
        sleep_time += len;
        out->blocks++;
    } while (sleep_time < TCFG_SPORT_HEALTH_SLEEP_TARGET_MIN);

}

static sleep_data_analysis value = {
    .file_time.year = TCFG_SPORT_HEALTH_SLEEP_TEST_YEAR,
    .file_time.month = TCFG_SPORT_HEALTH_SLEEP_TEST_MONTH,
    .file_time.day = TCFG_SPORT_HEALTH_SLEEP_TEST_DAY + 1,
    .file_time.hour = 22,
    .file_time.min = 30,
    .file_time.blocks = 0,
};
int sleep_data_test()
{
    sport_health_sleep_data_stroage();

    sport_health_sleep_rec_data_get(&value);
    return  0;
}
#endif//TCFG_SPORT_HEALTH_SLEEP_TEST



#endif// SPORT_HEALTH_MANAGE_SLEEP_ENABLE

