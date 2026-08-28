
#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-SPORT]"
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
struct sport_type_map {
    u32 mode: 3;			//outdoor indoor other
    u32 index: 7;		//列表索引
    u32 sport_type: 8;	//运动类型
    u32 data_cfg: 18;	//数据配置
};
struct sport_type_map sport_type_map_tab[] = {
    {SPORT_MODE_OUTDOOR	,		0,		SHM_SPORT_TYPE_OUTDOOR_WALIING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		1,		SHM_SPORT_TYPE_OUTDOOR_RUNNING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		2,		SHM_SPORT_TYPE_OUTDOOR_RIDE,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		3,	    SHM_SPORT_TYPE_MOUNTAINEERING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		4,		SHM_SPORT_TYPE_VOLLEYBALL,				BIT(SP_DATA_STEPS) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		5,		SHM_SPORT_TYPE_BADMINTON,				BIT(SP_DATA_STEPS) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		6,		SHM_SPORT_TYPE_SKIING,					BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_OUTDOOR	,		7,		SHM_SPORT_TYPE_SPEED_SKATING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},


    {SPORT_MODE_INDOOR	,		0,		SHM_SPORT_TYPE_INDOOR_RUNNING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		1,		SHM_SPORT_TYPE_INDOOR_RIDE,				BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		2,		SHM_SPORT_TYPE_STRENGTH_TRAINING,		BIT(SP_DATA_STEPS) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		3,		SHM_SPORT_TYPE_ABDOMINAL_CURL,			BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		4,		SHM_SPORT_TYPE_PUSH_UP,					BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		5,		SHM_SPORT_TYPE_AEROBIC_EXERCISE,		BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		6,		SHM_SPORT_TYPE_CALLISTHENICS,			BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		7,		SHM_SPORT_TYPE_YOGA,					BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		8,		SHM_SPORT_TYPE_DANCE,					BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		9,		SHM_SPORT_TYPE_ROPE_SKIPPING,			BIT(SP_DATA_STEPS) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		10,		SHM_SPORT_TYPE_POOL_SWIMMING,			BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {SPORT_MODE_INDOOR	,		11,		SHM_SPORT_TYPE_FREE_TIME,				BIT(SP_DATA_STEPS) | BIT(SP_DATA_DISTANCE) | BIT(SP_DATA_CALORIES) | BIT(SP_DATA_TIME) | BIT(SP_DATA_HR)},
    {0, 0, 0, 0},
};
/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_type_map_get
 *
 * @param sport_mode
 * @param list_index
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int sport_type_map_get_type(u8 sport_mode, u8 list_index)
{
    for (int i = 0; i < ARRAY_SIZE(sport_type_map_tab); i++) {
        log_debug("%s i:%d spmode:%d listindex:%d tabmode:%d tab_index:%d",
                  __func__, i, sport_mode, list_index, sport_type_map_tab[i].mode, sport_type_map_tab[i].index);
        if ((sport_mode == sport_type_map_tab[i].mode) && (list_index == sport_type_map_tab[i].index)) {
            log_debug("%s type:%d", __func__, sport_type_map_tab[i].sport_type);
            return sport_type_map_tab[i].sport_type;
        }
    }
    log_error("%s sp_mode:%d list_1index:%d", __func__, sport_mode, list_index);
    return -1;
}


#if (TCFG_SPORT_HEALTH_ENABLE&&TCFG_SPORT_HEALTH_SPORT)

#pragma pack(1)//平台非对齐编译
struct sport_file_head {
    u8 type;			//
    u8 version;			//0
    u8 interval;		//0~180s 基础包的时间间隔
    u8 mask;			//0xee
    u32 block_num: 15;	//数据块
    u32 file_size: 17;	//文件大小
    u8  reserved[5];//5
};
struct sport_file_data {
    u8 flag;		//1
    u8 len;
    u8 hr;			//he
    u16 step_freq;	//steps/min
    u16 speed;		//0.01km/h
};	//非必须
struct sport_file_start {
    u8 flag;		//0
    u8 len;			//
    u32 time;		//time
};
struct sport_file_pause {
    u8 flag;		//2
    u8 len;			//
    u32 time;		//time
};
struct sport_file_stop {
    u8 flag;		//0xff
    u8 len;			//
    u32 time;		//time
};
struct sport_file_end {
    u16 run_time;		//运动时长
    u32 reserved;		//保留位
    u16	distance;		//运动距离
    u16 calories;		//热量
    u32 steps;			//步数
    u8 recover_hour;	//运动恢复时间 小时
    u8 recover_min;		//运动恢复时间 分钟
};
#pragma pack()//平台对齐编译

#define SPORT_FILE_FLAG_START  (0x0)
#define SPORT_FILE_FLAG_DATA   (0x1)
#define SPORT_FILE_FLAG_PAUSE  (0x2)
#define SPORT_FILE_FLAG_END    (0xff)

#define SPORT_FILE_SIZEOF_START   (4)
#define SPORT_FILE_SIZEOF_DATA    (5)
#define SPORT_FILE_SIZEOF_PAUSE   (4)
#define SPORT_FILE_SIZEOF_END     (4)

#define SPORT_FILE_VERSION 		(0)
#define SPORT_FILE_INTERVAL		(0)
#define SPORT_FILE_MASK_CREATE	(0XFF)
#define SPORT_FILE_MASK_END	(0XEE)


struct sport_value *__sport_value = NULL;
#define __value 	(__sport_value)


/* ------------------------------------------------------------------------------------*/
/**
 * @brief sport_type_map_get_data_cfg
 *
 * @param sport_type
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u32 sport_type_map_get_data_cfg(u8 sport_type)
{
    for (int i = 0; i < ARRAY_SIZE(sport_type_map_tab); i++) {
        if (sport_type == sport_type_map_tab[i].sport_type) {
            return sport_type_map_tab[i].data_cfg;
        }
    }
    log_error("%s sp_type:%d ", __func__, sport_type);
    return -1;
}


static int __time2int(struct sys_time *time)
{
    int t;
    /* printf("time : %d-%d-%d,%d:%d:%d\n", time->year, time->month, time->day, time->hour, time->min, time->sec); */
    ASSERT(time->year >= 2010 && time->year <= 2010 + 0x3f, "input year need >= 2010 &&  <= 2073 \n");
    t = (time->sec & 0x3f) | ((time->min & 0x3f) << 6) | ((time->hour & 0x1f) << 12) | ((time->day & 0x1f) << 17) | ((time->month & 0xf) << 22) | (((time->year - 2010) & 0x3f) << 26);
    /* __int2time(t, time); */
    return t;
}




static void sport_value_dump(const char *func, struct sport_value *value)
{
    log_debug("fun:<%s> sta:%d type:%d c(steps:%d dist:%d cal:%d sfreq:%d) s(step:%d dist:%d cal:%d) pt:%d TIME(%04d-%02d-%02d %02d:%02d:%02d) file_len:%d hr(%d ~%d~ %d)\n", \
              func, value->run_status, value->type, value->steps_c, value->distance_c, value->calories_c, value->step_freq_c, value->steps_s, value->distance_s, value->calories_s, value->pause_sec, value->start_time.year, value->start_time.month, value->start_time.day, value->start_time.hour, value->start_time.min, value->start_time.sec, value->file_len, value->heart_val, value->heart_min, value->heart_max);


}
static int shm_sport_type_set(u32 type)
{
    if (__value) {
        sport_health_free(__value);
    }
    __value = sport_health_alloc(sizeof(struct sport_value));
    if (!__value) {
        return -SHM_ERR_MOD_ALLOC;
    }

    __value->type = type;
    return SHM_ERR_OK;
}
static int shm_sport_start()
{

    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }

    int type = __value->type;
    memset(__value, 0, sizeof(struct sport_value));
    __value->type = type;

    rtc_read_time(&__value->start_time);

    struct algo_value algo_out_value;
    int ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo_out_value);
    ASSERT(!ret);
    __value->steps_s = 	algo_out_value.steps;
    __value->calories_s = algo_out_value.calories;
    __value->distance_s =  DISTANCE_MAP(algo_out_value.distance);
    __value->heart_min = 0xff;
    __value->heart_max = 0;
    __value->run_status = SHM_SPORT_STATUS_RUN;
    __value->status = SHM_SPORT_STATUS_START;
    sport_value_dump(__func__, __value);
    return SHM_ERR_OK;
}

static int shm_sport_continue()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    struct algo_value algo_out_value;
    int ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, (void *)&algo_out_value);
    ASSERT(!ret);
    __value->steps_s = 	algo_out_value.steps;
    __value->calories_s = algo_out_value.calories;
    __value->distance_s =  DISTANCE_MAP(algo_out_value.distance);
    __value->pause_sec += (jiffies_msec() / 1000 - __value->tmp_sec);
    __value->run_status = SHM_SPORT_STATUS_RUN;
    __value->status = SHM_SPORT_STATUS_CONTINUE;
    sport_value_dump(__func__, __value);

    return SHM_ERR_OK;
}
static int shm_sport_pause()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    __value->tmp_sec = jiffies_msec() / 1000;
    __value->run_status = SHM_SPORT_STATUS_PAUSE;
    __value->status = SHM_SPORT_STATUS_PAUSE;
    sport_value_dump(__func__, __value);
    return SHM_ERR_OK;
}
static int shm_sport_stop()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    __value->run_status = SHM_SPORT_STATUS_PAUSE;
    __value->status = SHM_SPORT_STATUS_STOP;
    sport_value_dump(__func__, __value);

    return SHM_ERR_OK;
}
static int shm_sport_free()
{
    if (__value) {
        sport_health_free(__value);
        __value = NULL;
    }
    return SHM_ERR_OK;
}
static int shm_sport_update()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    if (__value->run_status == SHM_SPORT_STATUS_RUN) {
        struct algo_value algo_out_value;
        int ret = sport_health_manager_value_get(SHM_MOD_GSENSOR_ALGO, SHM_GET_TYPE_INFO, &algo_out_value);
        ASSERT(!ret);
        if (__value->steps_s > algo_out_value.steps) { /*跨天，可能存在1s的计数误差，忽略*/
            __value->steps_s = 0;
            __value->calories_s = 0;
            __value->distance_s = 0;
        }
        //两次统计作差，记录增量
        int distance_dt = (DISTANCE_MAP(algo_out_value.distance) - __value->distance_s);
        int step_dt  = (algo_out_value.steps - __value->steps_s);
        int calories_dt = (algo_out_value.calories - __value->calories_s);
        __value->steps_c += step_dt;
        __value->calories_c += calories_dt ;
        __value->distance_c += distance_dt;
        __value->step_freq_c = algo_out_value.step_frequency;
        __value->speed_c = distance_dt;
        __value->step_stride_c = (step_dt) ? distance_dt / step_dt : 0;
        //记录当前值用于下次作差使用
        __value->steps_s += step_dt;
        __value->calories_s += calories_dt;
        __value->distance_s += distance_dt;
        //心率计算
        u8 hr_curr = 90 + rand32() % 20;
        u32 hr_tmp = __value->heart_val * __value->run_sec + hr_curr;
        __value->run_sec ++;	//可以用rtc和pause_sec校准
        __value->heart_val = hr_tmp / __value->run_sec;
        __value->heart_max = (__value->heart_max > hr_curr) ? __value->heart_max : hr_curr;
        __value->heart_min = (__value->heart_min < hr_curr) ? __value->heart_min : hr_curr;

        sport_value_dump(__func__, __value);
    } else {
        __value->step_freq_c = 0;
    }
    return  SHM_ERR_OK;
}
static int shm_sport_file_start()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    __value->file_hd = sport_health_file_open(F_TYPE_SPORTRECORD, 0);
    //文件头
    struct sport_file_head file_head = {
        .type =  __value->type,
        .version = SPORT_FILE_VERSION,
        .interval = SPORT_FILE_INTERVAL,
        .mask = SPORT_FILE_MASK_CREATE,
        .block_num = -1,
        .file_size = -1,
    };
    sport_health_file_write(__value->file_hd, (u8 *)&file_head, __value->file_len, sizeof(struct sport_file_head));
    __value->file_len += sizeof(struct sport_file_head);
    //开始包
    struct sys_time time;
    rtc_read_time(&time);
    struct sport_file_start file_start = {
        .flag = SPORT_FILE_FLAG_START,
        .len = SPORT_FILE_SIZEOF_START,
        .time =   __time2int(&time),
    };
    sport_health_file_write(__value->file_hd, (u8 *)&file_start, __value->file_len, sizeof(struct sport_file_start));
    __value->file_len  += sizeof(struct sport_file_start);
    __value->file_pack_num ++;
    return SHM_ERR_OK;
}
static int shm_sport_file_pause()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    //暂停包
    struct sys_time time;
    rtc_read_time(&time);
    struct sport_file_pause file_pause = {
        .flag = SPORT_FILE_FLAG_PAUSE,
        .len = SPORT_FILE_SIZEOF_PAUSE,
        .time =   __time2int(&time),
    };
    sport_health_file_write(__value->file_hd, (u8 *)&file_pause, __value->file_len, sizeof(struct sport_file_pause));
    __value->file_len  += sizeof(struct sport_file_pause);

    __value->file_pack_num ++;
    return SHM_ERR_OK;
}
static int shm_sport_file_continue()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    //开始包
    struct sys_time time;
    rtc_read_time(&time);
    struct sport_file_start file_start = {
        .flag = SPORT_FILE_FLAG_START,
        .len = SPORT_FILE_SIZEOF_START,
        .time =   __time2int(&time),
    };
    sport_health_file_write(__value->file_hd, (u8 *)&file_start, __value->file_len, sizeof(struct sport_file_start));
    __value->file_len  += sizeof(struct sport_file_start);

    __value->file_pack_num ++;
    return SHM_ERR_OK;
}
static int shm_sport_file_stop()
{
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    //结束
    struct sys_time time;
    rtc_read_time(&time);
    struct sport_file_stop file_stop = {
        .flag = SPORT_FILE_FLAG_END,
        .len = SPORT_FILE_SIZEOF_END,
        .time =   __time2int(&time),
    };
    sport_health_file_write(__value->file_hd, (u8 *)&file_stop, __value->file_len, sizeof(struct sport_file_stop));
    __value->file_len  += sizeof(struct sport_file_stop);
    __value->file_pack_num ++;
    //文件尾
    struct sport_file_end file_end  = {
        .run_time = __value->run_sec,
        .distance = __value->distance_c,
        .calories = __value->calories_c,
        .steps = __value->steps_c,
        .recover_hour = 0xff,
        .recover_min = 0,
        .reserved = 0xffffffff,
    };
    put_buf((u8 *)&file_end, sizeof(struct sport_file_end));
    sport_health_file_write(__value->file_hd, (u8 *)&file_end, __value->file_len, sizeof(struct sport_file_end));
    __value->file_len += sizeof(struct sport_file_end);
#if  0
    {
        void *fp	= sport_health_file_open(F_TYPE_SPORTRECORD, 0);
        int id = sport_health_file_get_id(fp, 0);
        int rlen  = sport_health_file_get_len(fp);
        u8 *rbuf  =  zalloc(rlen);
        sport_health_file_read(fp, rbuf, 0, rlen);
        printf("%s %d rlen:%d flen:%d", __func__, __LINE__, rlen, __value->file_len);
        put_buf(rbuf, rlen);
    }
#endif
    //文件头
    struct sport_file_head file_head = {
        .type =  __value->type,
        .version = SPORT_FILE_VERSION,
        .interval = SPORT_FILE_INTERVAL,
        .mask = SPORT_FILE_MASK_END,
        .block_num = __value->file_pack_num,
        .file_size = __value->file_len,
    };
    sport_health_file_update(__value->file_hd, (u8 *)&file_head, 0, sizeof(struct sport_file_head));
    sport_health_file_close(__value->file_hd);

    sport_value_dump(__func__, __value);
#if 0
    {
        void *fp	= sport_health_file_open(F_TYPE_SPORTRECORD, 0);
        int id = sport_health_file_get_id(fp, 0);
        int rlen  = sport_health_file_get_len(fp);
        u8 *rbuf  =  zalloc(rlen);
        sport_health_file_read(fp, rbuf, 0, rlen);
        printf("%s %d len:%d", __func__, __LINE__, rlen);
        put_buf(rbuf, rlen);
    }
#endif
    return SHM_ERR_OK;
}
static int shm_sport_file_data()
{
#if SPORT_FILE_INTERVAL
    if (!__value) {
        return -SHM_ERR_MOD_NOR_INIT;
    }
    u32 curr_sec  = jiffies_msec() / 1000;
    if ((!__value->last_data_time) || (curr_sec - __value->last_data_time >= SPORT_FILE_INTERVAL)) {
        struct sport_file_data  file_data = {
            .flag = SPORT_FILE_FLAG_DATA,
            .len  = SPORT_FILE_SIZEOF_DATA,
        };
        //只需要接入数据，再删除
#error need writ data to file_data

        sport_health_file_write(__value->file_hd, (u8 *)&file_data, __value->file_len, sizeof(struct sport_file_data));
        __value->file_len  += sizeof(struct sport_file_data);
        __value->file_pack_num ++;

    }
#endif
    return SHM_ERR_OK;
}
static int shm_sport_file_get_rec_by_index(struct sport_value *value, u8 index)
{
    int ret = SHM_ERR_OK;
    struct sport_file_head file_head;
    void *fp	= sport_health_file_open(F_TYPE_SPORTRECORD, 0);
    int id = sport_health_file_get_id(fp, index);
    if (!id) {
        ret = SHM_ERR_FILE_NOT_FIND;
        sport_health_file_close(fp); //
        return ret;
    }
    ret = sport_health_file_read(fp, (u8 *)&file_head, 0, sizeof(struct sport_file_head));
    ASSERT(ret);
    put_buf((u8 *)&file_head, sizeof(struct sport_file_head));

    struct sport_file_end file_end;
    int file_len = sport_health_file_get_len(fp);
    ret = sport_health_file_read(fp, (u8 *)&file_end, file_len - sizeof(struct sport_file_end), sizeof(struct sport_file_end));
    ASSERT(ret);
    log_info("%s %d ", __func__, (u32)sizeof(struct sport_file_end));
    log_info_hexdump((u8 *)&file_end, sizeof(struct sport_file_end));
    value->type = file_head.type;
    value->steps_c = file_end.steps;
    value->calories_c = file_end.calories;
    value->distance_c = file_end.distance;
    value->run_sec = file_end.run_time;
    sport_health_file_close(fp);
    sport_value_dump(__func__, value);
    return ret;
}

static int shm_sport_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;

    switch (cmd) {
    case SHM_CMD_INFO_SET:
        ret = shm_sport_type_set((int)priv);
        break;
    case SHM_CMD_START:
        ret = shm_sport_start();
        shm_sport_update();
        if (!ret) {
            ret = shm_sport_file_start();
        }
        break;
    case SHM_CMD_CONTINUE:
        ret = shm_sport_continue();
        if (!ret) {
            ret = shm_sport_file_continue();
        }
        break;
    case SHM_CMD_PAUSE:
        ret = shm_sport_pause();
        if (!ret) {
            ret = shm_sport_file_pause();
        }
        break;
    case SHM_CMD_STOP:
        ret = shm_sport_stop();
        if (!ret) {
            ret = shm_sport_file_stop();
        }
        shm_sport_free();
        break;
    case SHM_CMD_UPDATE_SEC:
        ret = shm_sport_update();
        if (!ret) {
            ret = shm_sport_file_data();
        }
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_CMD;
        break;
    }
    return ret;
}

static int shm_sport_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_INFO:
        if (!__value) {
            return -SHM_ERR_MOD_NOR_INIT;
        }
        memcpy(priv, __value, sizeof(struct sport_value));
        sport_value_dump(__func__, __value);
        break;
    case SHM_GET_TYPE_INFO_STORAGE:
        //get date from file
        struct sport_value *sv = (struct sport_value *)priv;
        ret =  shm_sport_file_get_rec_by_index(sv, sv->file_index);
        break;
    case SHM_GET_TYPE_STORAGE_TOTAL: {
        u8 total =  sport_health_file_get_total(F_TYPE_SPORTRECORD);
        memcpy(priv, &total, 1);
    }
    break;
    case SHM_GET_TYPE_STORAGE_ID: {
        u8 total =  sport_health_file_get_total(F_TYPE_SPORTRECORD);
        if (total) {
            void *fp  = sport_health_file_open(F_TYPE_SPORTRECORD, 0);
            u16 id = sport_health_file_get_id(fp, total - 1);
            sport_health_file_close(fp);
            memcpy(priv, &id, 2);
        } else {
            memset(priv, 0, 2);
        }
    }
    break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(sport)
{
    .module =  SHM_MOD_SPORT,
     .io_ctrl = shm_sport_io_crtl,
      .get_value =   shm_sport_get_value,
};

#endif
