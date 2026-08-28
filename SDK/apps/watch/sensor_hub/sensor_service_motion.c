
#include "app_config.h"
#include "includes.h"
#include "sensor_algorithm_jl_motion.h"
#include "shm_info_storage.h"
#include "system/timer.h"
#include "timestamp/timestamp.h"
#include "sensor_hub.h"
#include "health_manager.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_SRV_MOTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sensor_hub.data.bss")
#pragma data_seg(".sensor_hub.data")
#pragma const_seg(".sensor_hub.text.const")
#pragma code_seg(".sensor_hub.text")
#endif


#if TCFG_SPORT_HEALTH_ENABLE &&TCFG_SENSOR_HUB	&& (TCFG_ACCELER_MASTER_ENABLE || TCFG_ACCELER_P11_ENABLE)

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
#define abs(x)  ((x)>0?(x):-(x) )
static u8 __time_2_age(int year, int month, int day)
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

void sensor_service_motion_init(void)
{
    sensor_hub_enable(SENSOR_DRV_ACCELER, 1);
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 0, 1);
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

    if (info_valid) {
        user.gender      = info.gender;
        user.ages        = age;
        user.height      = info.height;
        user.weight      = info.weight;
        user.step_factor = 45;  //用于估算步行距离
    }

    /* algo_motion_input_debug(1); */
    algo_motion_init(open_algo, accel, user);
}
void sensor_service_motion_cfg_update(void)
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

    if (info_valid) {
        user.gender      = info.gender;
        user.ages        = age;
        user.height      = info.height;
        user.weight      = info.weight;
        user.step_factor = 45;  //用于估算步行距离
    }

    /* algo_motion_input_debug(1); */
    algo_motion_init(open_algo, accel, user);
}

#define G_SENSOR_CHECK_INTERVAL  500
#define G_SENSOR_CHECK_CNT_VAL_1		2
#define G_SENSOR_CHECK_CNT_VAL_2		3
static void gsensor_dir_deal(accel_data *accel, short point)
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
    log_debug("%s x_cnt%d x_dir:%d y_cnt:%d y_dir:%d z_cnt:%d z_dir:%d sensor:%d",
              __func__, x_cnt, x_dir, y_cnt, y_dir, z_cnt, z_dir, sensor_dir);
}
u8 gsensor_dir_get()
{
    return sensor_dir;
}

void print_accel(accel_data *accel, short point)
{
    printf("%s point:%d", __func__, point);
    for (short i = 0; i < point; i++) {
        printf("xyz:%d,%d,%d", accel[i].x, accel[i].y, accel[i].z);
    }
}

void sensor_service_motion_get(struct algo_value *buf)
{
    axis_data_t gsensor[50];
    u16 point = sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, gsensor,  sizeof(gsensor));
    if (!point) {
        log_warn("%s point:%d", __func__, point);
    }
    /* print_accel((accel_data *)gsensor, point); */
    struct sys_time time;
    rtc_read_time(&time);

    u32 timestamp = timestamp_mytime_2_utc_sec(&time);
    algo_out out =  algo_motion_run(timestamp, (accel_data *)gsensor, point, 0, 0);

    gsensor_dir_deal((accel_data *)gsensor, point);
    // LOG("update:%d", out.update.all);
    if (out.update.step_counter) {
        log_info("steps:%d", out.steps);
    }
    memcpy(buf, &out.steps, sizeof(struct algo_value));
}
#endif //TCFG_SPORT_HEALTH_ENABLE & (TCFG_ACCELER_MASTER_ENABLE | TCFG_ACCELER_P11_ENABLE)
