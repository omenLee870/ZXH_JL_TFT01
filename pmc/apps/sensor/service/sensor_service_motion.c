
#include "health_manager.h"
#include "sensor_hub.h"
#include "service/sensor_algorithm_jl_motion.h"
/* L0/L1/L2 → RAM；L3 → Flash（FIFO 类同向，函数级 AT 覆盖） */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#endif
#pragma const_seg(".sensor_const")
#include "sensor_seg.h"
#define LOG_TAG_CONST      	SENSOR_SERVICE
#define LOG_TAG     		"[SENSOR_SRV_MOTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"


#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&& TCFG_SENSOR_HUB	&& TCFG_SENSOR_SERVICE_MOTION_ENABLE)


extern int sport_personal_info_get(personal_information *info);
extern void rtc_read_time(struct sys_time *time);
extern u32 timestamp_mytime_2_utc_sec(struct sys_time *mytime);


enum {
    G_SENSOR_DIR_TOP,
    G_SENSOR_DIR_BOTTOM,
    G_SENSOR_DIR_LEFT,
    G_SENSOR_DIR_RIGHT,
};


algo_type             open_algo;
static accel_config   accel = {.lsb_g = 1024, .sps = 25};
static user_info      user = {.ages = 28, .gender = 1, .height = 170, .weight = 60, .step_factor = 45};

static u16           buf_rri[5];
static u32           timestamp_steps = 0;

static u8 			 sensor_dir  = G_SENSOR_DIR_TOP;
/* 床头钟 dir 算法独立开关（默认关闭 = 0）。
 * dir 计算的唯一消费者是床头钟（bedside watch）；未进床头钟时每秒算一次没有意义
 * 且白耗电，故默认 0，由床头钟进入页面时经 SHM_CMD_DIR_ENABLE 打开、退出时关闭。
 * 注意：本开关只闸「方向计算」，与计步/睡眠算法（jl_motion_run）完全解耦，
 *       计步总闸仍是 module/shm_algo_gsensor.c 的 gsensor_algo_enable。 */
static u8 			 gsensor_dir_enable = 0;
#define abs(x)  ((x)>0?(x):-(x) )
AT_GSENSOR_COLD static u8 __time_2_age(int year, int month, int day)
{
    struct sys_time ntime;

    int xx, yy;
    if ((ntime.day - day) < 0) {
        xx = 1;
    } else {
        xx = 0;
    }
    if ((ntime.month - month - xx) < 0) {
        yy = 1;
    } else {
        yy = 0;
    }
    return ntime.year - year - yy;
}

AT_GSENSOR_COLD int sensor_service_motion_init(int init)
{
    sensor_hub_enable(SENSOR_DRV_ACCELER, 1);
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 0, 1);
    personal_information info = {0};
    info.height = 175;
    info.weight = 60;
    info.birth_y = 1995;
    info.birth_m = 1;
    info.birth_d = 1;
    int  info_valid = sport_personal_info_get(&info);
    u8 age = __time_2_age(info.birth_y, info.birth_m, info.birth_d);
    age = 25;
    log_info("gender=%d ages=%d height=%d weight=%d", info.gender, age, info.height, info.weight);
    /* printf("%s %d",__func__,__LINE__); */
    accel.sps   = 25;
    accel.lsb_g = 1024;

    open_algo.all = 0;
    open_algo.step_counter   = 1;
    open_algo.step_frequency = 1;
    open_algo.distance       = 1;
    open_algo.calories       = 1;
    open_algo.calories_amr   = 1;
    open_algo.sleep          = 1;
    open_algo.wrist_raise    = 1;/* 抬腕检测（jl_motion_run 内嵌） */

    if (info_valid) {
        user.gender      = info.gender;
        user.ages        = age;
        user.height      = info.height;
        user.weight      = info.weight;
        user.step_factor = 45;  //用于估算步行距离
    }

    /* algo_motion_input_debug(1); */
    jl_motion_init(open_algo, accel, user);
    return 0;
}
AT_GSENSOR_COLD void sensor_service_motion_cfg_update(void)
{
    personal_information info = {0};
    int  info_valid = sport_personal_info_get(&info);
    u8 age = __time_2_age(info.birth_y, info.birth_m, info.birth_d);
    log_info("gender=%d ages=%d height=%d weight=%d", info.gender, age, info.height, info.weight);

    accel.sps   = 25;
    accel.lsb_g = 1024;

    open_algo.all = 0;
    open_algo.step_counter   = 1;
    open_algo.step_frequency = 1;
    open_algo.distance       = 1;
    open_algo.calories       = 1;
    open_algo.calories_amr   = 1;
    open_algo.sleep          = 1;
    open_algo.wrist_raise    = 1;/* 抬腕检测（jl_motion_run 内嵌） */

    if (info_valid) {
        user.gender      = info.gender;
        user.ages        = age;
        user.height      = info.height;
        user.weight      = info.weight;
        user.step_factor = 45;  //用于估算步行距离
    }

    /* algo_motion_input_debug(1); */
    /* algo_motion_init(open_algo, accel, user); */
}

#define G_SENSOR_CHECK_INTERVAL  500
#define G_SENSOR_CHECK_CNT_VAL_1		2
#define G_SENSOR_CHECK_CNT_VAL_2		3
AT_GSENSOR_ALGO static void gsensor_dir_deal(accel_data *accel, short point)
{
    int x_cnt = 0;
    int y_cnt = 0;
    int z_cnt = 0;
    int x_dir = 0;
    int y_dir = 0;
    int z_dir = 0;

    for (int i = 0; i < point; i++) {
        if ((abs(accel[i].z) - abs(accel[i].x) > G_SENSOR_CHECK_INTERVAL) && (abs(accel[i].z) - abs(accel[i].y) > G_SENSOR_CHECK_INTERVAL)) {
            z_cnt ++;
            z_dir += ((accel[i].z > 0) ? 1 : -1);
        } else if ((abs(accel[i].x) - abs(accel[i].z) > G_SENSOR_CHECK_INTERVAL) && (abs(accel[i].x) - abs(accel[i].y) > G_SENSOR_CHECK_INTERVAL)) {
            x_cnt ++;
            x_dir += ((accel[i].x > 0) ? 1 : -1);
        } else if ((abs(accel[i].y) - abs(accel[i].z) > G_SENSOR_CHECK_INTERVAL) && (abs(accel[i].y) - abs(accel[i].x) > G_SENSOR_CHECK_INTERVAL)) {
            y_cnt ++;
            y_dir += ((accel[i].y > 0) ? 1 : -1);
        }
    }
    if (y_cnt > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
        if (y_dir > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
            sensor_dir = G_SENSOR_DIR_TOP;
        } else if (y_dir * -1 > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
            sensor_dir = G_SENSOR_DIR_BOTTOM;
        }
    } else if (x_cnt > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
        if (x_dir > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
            sensor_dir = G_SENSOR_DIR_LEFT;
        } else if (x_dir * -1 > point * G_SENSOR_CHECK_CNT_VAL_1 / G_SENSOR_CHECK_CNT_VAL_2) {
            sensor_dir = G_SENSOR_DIR_RIGHT;
        }
    }
    static u8 last_dir = 0xff;
    if (sensor_dir != last_dir) {
        log_info("dir:%d", sensor_dir);
        last_dir = sensor_dir;
    }
    /* log_debug("%s x_cnt%d x_dir:%d y_cnt:%d y_dir:%d z_cnt:%d z_dir:%d sensor:%d", */
    /* __func__, x_cnt, x_dir, y_cnt, y_dir, z_cnt, z_dir, sensor_dir); */
    /* printf("%s x_cnt%d x_dir:%d y_cnt:%d y_dir:%d z_cnt:%d z_dir:%d sensor:%d", */
    /* __func__, x_cnt, x_dir, y_cnt, y_dir, z_cnt, z_dir, sensor_dir); */
}
AT_SENSOR_ALGO u8 gsensor_dir_get()
{
    return sensor_dir;
}
AT_SENSOR_COLD void print_accel(accel_data *accel, short point)
{
    log_debug("%s point:%d", __func__, point);
    for (short i = 0; i < point; i++) {
        log_debug("xyz:%d,%d,%d", accel[i].x, accel[i].y, accel[i].z);
    }
}
AT_GSENSOR_COLD void print_alog_out(algo_out *out)
{
    log_debug("%s update:%x steps:%d sed:%d cal:%d amt:%d act:%d dist:%d stepf:%d sleep:%d shake:%d wrist_event:%d ", __func__, (u16)out->update.all, out->steps, out->sedentary, out->calories, out->calories_amr, out->activity, out->distance, out->step_frequency, out->sleep, out->shake, out->wrist_event);
}
AT_GSENSOR_ALGO static void __sensor_service_motion_get(struct algo_value *buf)
{
    axis_data_t gsensor[50];
    u16 point = sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, gsensor,  sizeof(gsensor));
    if (!point) {
        log_debug("%s point:%d", __func__, point);
    }

    /* print_accel((accel_data *)gsensor, point); */
    struct sys_time time;
    rtc_read_time(&time);

    u32 timestamp = timestamp_mytime_2_utc_sec(&time);

#if 0
    algo_out out;
    memset(&out, 0, sizeof(algo_out));
#else
    log_debug("%s get:%d", __func__, point);
    algo_out out =  jl_motion_run(timestamp, (accel_data *)gsensor, point);
    /* print_alog_out(&out); */
    /* printf("%s %d",__func__,__LINE__); */
#endif

    if (gsensor_dir_enable) {
        gsensor_dir_deal((accel_data *)gsensor, point);/* dir 检测复用算法同一份 accel 数据，避开 cbuf 竞争 */
    }
    if (out.update.step_counter) {
        log_debug("steps:%d", out.steps);
    }
    memcpy(buf, &out, sizeof(struct algo_value));
}
/* 床头钟 dir 算法开关：由 module/shm_algo_gsensor.c 在收到
 * SHM_CMD_DIR_ENABLE / SHM_CMD_DIR_DISABLE 时调用（跨核链路见 apps/main.c）。
 * 只控方向计算，不动 jl_motion_run / 计步累加（省电，且不影响计步）。 */
AT_GSENSOR_ALGO void sensor_service_motion_dir_enable(void)
{
    gsensor_dir_enable = 1;
    log_info("%s dir_enable:%d", __func__, gsensor_dir_enable);
}
AT_GSENSOR_ALGO void sensor_service_motion_dir_disable(void)
{
    gsensor_dir_enable = 0;
    log_info("%s dir_enable:%d", __func__, gsensor_dir_enable);
}
/* 原 sensor_service_motion_dir_update() 已删除（pmc_gsensor）。
 * 它每秒独立调一次 sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, ...)，与
 * __sensor_service_motion_get() 争抢同一个 cbuf 通道（index 0）：hal 层
 * sensor_raw_data_get() 是消费型读，读完即 cbuf_mult_read_alloc_len_updata()
 * 推进该 index 的读指针，谁先读谁把数据取走，另一方只能拿到 0。
 * 而它原本又在 sport_health_manager_task() 里紧跟 update_sec_all() 之后调用，
 * 后者已把 index 0 读空，故该路径实际恒拿不到数据，属结构性无效。
 * 现在 dir 只在 __sensor_service_motion_get() 里复用计步算法同一份 accel 计算，
 * 每 tick 只读一次 index 0，零额外取数、零 FIFO 争用；更新周期随
 * CONFIG_SPORT_HEALTH_UPDATE_PERIOD（1s），与床头钟 1s 轮询同频。 */
AT_GSENSOR_ALGO int sensor_service_motion_get(int type, void *priv)
{
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        __sensor_service_motion_get((struct algo_value *)priv);
        break;
    default:
        break;
    }
    return 0;
}
AT_SENSOR_COLD int sensor_service_motion_set(int type, void *priv)
{
    switch (type) {
    case SHM_CMD_INFO_SET:
        sensor_service_motion_cfg_update();
        break;
    default:
        break;
    }
    return 0;
}
REGISTER_SENSOR_SERVICE(gsensor_service)
{
    .mod = 	SHM_MOD_GSENSOR_ALGO,
    .init =	 sensor_service_motion_init,
    .enable = NULL,
    .get_result = sensor_service_motion_get,
    .set_config =  sensor_service_motion_set,
};
#endif //TCFG_SPORT_HEALTH_ENABLE & (TCFG_ACCELER_MASTER_ENABLE | TCFG_ACCELER_P11_ENABLE)
