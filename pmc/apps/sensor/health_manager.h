#include "includes.h"
#include "sensor_hub.h"
#include "sensor_driver.h"
#include "sensor_service.h"
#ifndef  __HEALTH_MANAGER__
#define  __HEALTH_MANAGER__

typedef enum {
    SHM_MOD_BEGIN,					//开始
    //algo_mod
    SHM_MOD_GSENSOR_ALGO,			//GSENSOR
    SHM_MOD_HEART_RATE_ALGO,		//心率算法
    SHM_MOD_SPO2_ALGO,				//血氧算法
    //app_mod
    SHM_MOD_DAILY_ACTIVE,			//活动量
    SHM_MOD_SPORT,					//运动
    SHM_MOD_HEART_RATE,				//心率
    SHM_MOD_BLOOD_OXYGEN,			//血氧
    SHM_MOD_WEAR_DETECTION,			//佩戴检测
    SHM_MOD_BLOOD_PRESSURE,			//血压
    SHM_MOD_HEAT,					//压力
    SHM_MOD_AIR_PRESSURE,			//气压
    SHM_MOD_ALTITUDE,				//海拔
    SHM_MOD_COMPASS,				//指南针
    SHM_MOD_TEMPERATURE,			//体温
    SHM_MOD_SLEEP,					//睡眠
    //DET_MOD
    SHM_MOD_DET_SEDENTARY,			//久坐
    SHM_MOD_DET_FALL,				//跌倒
    SHM_MOD_DET_WRIST,				//翻腕
    SHM_MOD_DET_HEART_RATE_HIGH,	//心率过高
    SHM_MOD_DET_BLOOD_OXYGEN_LOW,	//血氧过低
    SHM_MOD_DET_DRINK,				//喝水提醒
    SHM_MOD_DET_MENSE,				//女性健康

    SHM_MOD_END,					//结束
} SHM_MOD;/*update sec 时 按BEGIN -> END顺序执行*/

typedef enum {
    SHM_CMD_INIT,
    SHM_CMD_RELEASE,
    SHM_CMD_ENABLE,
    SHM_CMD_DISABLE,
    SHM_CMD_REC_ENABLE,
    SHM_CMD_REC_DISABLE,

    // SHM_CMD_UPDATE_PREB,
    SHM_CMD_UPDATE,
    SHM_CMD_UPDATE_SEC,
    SHM_CMD_UPDATE_ALL,
    // SHM_CMD_UPDATE_AFTER,

    SHM_CMD_CLEAR_VALUE,
    SHM_CMD_CLEAR_VALUE_DAY,

    SHM_CMD_START,
    SHM_CMD_CONTINUE,
    SHM_CMD_PAUSE,
    SHM_CMD_STOP,

    SHM_CMD_SAVE_CONTINUE,
    SHM_CMD_SAVE_SINGLE,

    SHM_CMD_INFO_SET,
    SHM_CMD_TARGET_SET,

    SHM_CMD_PRIV,

    SHM_CMD_MAX,
} SHM_CMD;

typedef enum {
    SHM_GET_TYPE_REAL_VALUE,		//当前/实时数据
    SHM_GET_TYPE_MAX_VALUE,			//最大
    SHM_GET_TYPE_MIN_VALUE,			//最小
    SHM_GET_TYPE_AVG_VALUE,			//平均

    SHM_GET_TYPE_INFO,				//信息
    SHM_GET_TYPE_INFO_STORAGE,		//存储文件信息
    SHM_GET_TYPE_STORAGE_TOTAL,		//存储文件数量
    SHM_GET_TYPE_REC_VALUE,			//记录数据
    SHM_GET_TYPE_TARGET,			//目标
    SHM_GET_TYPE_STORAGE_ID,		//存储文件id
    SHM_GET_TYPE_WEEK_DATA,			//周数据
    SHM_GET_TYPE_DATA_ANALYSIS,		//数据分析
    SHM_GET_TYPE_MAX,
} SHM_TYPE;

typedef enum {
    SHM_ERR_OK,

    SHM_ERR_OS_ERR,					//1

    SHM_ERR_MOD_NOT_FIND,			//2
    SHM_ERR_MOD_CB_NOT_DEFINE,		//3
    SHM_ERR_TYPE_NOT_DEFINE,		//4
    SHM_ERR_MOD_NO_THIS_CMD,		//5
    SHM_ERR_MOD_NO_THIS_TYPE,		//6

    SHM_ERR_MOD_NOR_INIT,			//7
    SHM_ERR_MOD_NOT_ENABLE,			//8
    SHM_ERR_MOD_ALLOC,				//9

    SHM_ERR_FILE_NOT_FIND,			//10
    SHM_ERR_MOD_STA_ERR,			//11
    SHM_ERR_DATA_ERR,				//12
    SHM_ERR_MOD_INVALID_PARAM,			//13
} SHM_ERR;

enum {
    SHM_SPORT_STATUS_NONE,
    SHM_SPORT_STATUS_RUN,
    SHM_SPORT_STATUS_PAUSE,
    SHM_SPORT_STATUS_CONTINUE,
    SHM_SPORT_STATUS_STOP,
    SHM_SPORT_STATUS_START = SHM_SPORT_STATUS_RUN,
};

enum {
    SHM_MOD_STA_NOT_INIT,
    SHM_MOD_STA_INIT,
    SHM_MOD_STA_DISABLE,
    SHM_MOD_STA_DATA_UPDATE_NULL,
    SHM_MOD_STA_ENABLE,

    SHM_MOD_STA_DATA_UPDATE_SUCC,
};
#if  1
typedef struct sport_info_personal_info {
    u16 height;
    u16 weight;

    u16 birth_y;
    u8  birth_m;
    u8  birth_d;

    u8  gender;
} personal_information;
#pragma pack()
struct sys_time {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
};
#endif
#if 0 	//不使用杰理算法库时，定义结构体传参

struct algo_value {
    int       steps;           //步数
    int       sedentary;       //久坐时间，单位 秒
    int       calories;        //卡路里 = 基础代谢率（BMR） + 活动代谢率（AMR） 单位 卡
    int       calories_amr;    //活动代谢率（AMR） 单位 卡
    int       activity;        //活动类型，参考 activity_type
    int       distance;        //距离, 单位cm
    int       step_frequency;  //步频，单位spm (步/分钟)
    int       sleep;           //睡眠, 参考sleep_type
    int       shake;           //摇晃次数
};
typedef  struct algo_value algo_out;
#else
#include "sensor_algorithm_jl_motion.h"
// typedef algo_out struct algo_value;

#endif
#define DISTANCE_MAP(dist) ((dist))

struct daily_active {
    u32 steps: 17;		//步数预留13万步
    u32 calories: 12;	//热量 4096kc
    u32 distance: 17;	//距离130km
    u32 stand_times: 5;	//有效站立次数
    u32 day: 5;			//日期
    u32 init: 1;			//
    u32 reserve: 7;		//保留位可拓展
};

struct sport_value {
    u8 run_status;		//
    u8 status;			//
    u8 type;			//类型

    u32 steps_c;		//步数
    u32 calories_c;		//热量
    u32 distance_c;		//距离 cm
    u32 step_freq_c;	//
    u16 speed_c;		//速度cm/s
    u16 step_stride_c;	//步幅cm/步
    u32 run_sec;			//sec

    u8 heart_real;			//实时心率
    u8 heart_val;			//平均心率
    u8 heart_max;			//最大心率
    u8 heart_min;			//最小心率

    u32 steps_s;		//步数
    u32 calories_s;		//热量
    u32 distance_s;		//距离
    u32 pause_sec;			//sec
    u32 tmp_sec;			//sec

    u16 last_data_time;
    u16 file_pack_num;
    void *file_hd;
    u8 file_index;
    u32 file_len;

    struct sys_time start_time;		//sec
};
#define SPORT_MODE_OUTDOOR			0
#define SPORT_MODE_INDOOR			1
enum {
    SHM_SPORT_TYPE_NONE,				//非运动
    SHM_SPORT_TYPE_OUTDOOR_RUNNING,		//户外跑
    SHM_SPORT_TYPE_INDOOR_RUNNING,		//室内跑步

    //expand 需要跟app协商序号
    SHM_SPORT_TYPE_OUTDOOR_WALIING,		//健走
    SHM_SPORT_TYPE_OUTDOOR_RIDE,		//户外骑行
    SHM_SPORT_TYPE_MOUNTAINEERING,		//爬山
    SHM_SPORT_TYPE_BADMINTON,			//羽毛球
    SHM_SPORT_TYPE_VOLLEYBALL,			//排球
    SHM_SPORT_TYPE_SKIING,				//滑雪
    SHM_SPORT_TYPE_SPEED_SKATING,		//速滑
    SHM_SPORT_TYPE_INDOOR_RIDE,			//室内骑行
    SHM_SPORT_TYPE_STRENGTH_TRAINING,	//力量训练
    SHM_SPORT_TYPE_ABDOMINAL_CURL,		//仰卧起坐
    SHM_SPORT_TYPE_PUSH_UP,				//俯卧撑
    SHM_SPORT_TYPE_AEROBIC_EXERCISE,	//有氧运动
    SHM_SPORT_TYPE_CALLISTHENICS,		//健身操
    SHM_SPORT_TYPE_YOGA,				//瑜伽
    SHM_SPORT_TYPE_DANCE,				//跳舞
    SHM_SPORT_TYPE_ROPE_SKIPPING,		//跳绳
    SHM_SPORT_TYPE_POOL_SWIMMING,		//泳池游泳
    SHM_SPORT_TYPE_FREE_TIME,			//自由活动
};

enum {
    SP_DATA_STEPS,
    SP_DATA_DISTANCE,
    SP_DATA_CALORIES,
    SP_DATA_TIME,
    SP_DATA_HR,
};

#pragma pack(1)	//非对齐编译

struct health_file_total_head {
    u8 type;
    u16 year;
    u8 month;
    u8 day;
    u16 crc;
    u8 version;
    u8 interval;
    u16 reserve;
};
struct health_file_data_head {
    u8 hour;
    u8 min;
    u16 len;
};
#pragma pack(0)	//对齐编译

struct health_file_data_info {
    struct sys_time file_time;
    int rlen;
    int interval;
    u8 *rbuf;
};
struct sleep_data_analysis {
    struct sys_time file_time;
    int total_min;
    int awake_min;
    int light_min;
    int deep_min;
    int rem_min;
    void *data;
};

struct sport_health_module {
    u8 module;
    int(*io_ctrl)(int cmd, void *priv);
    int (*get_value)(int type, void *pirv);
};
#define REGISTER_SPORT_HEALTH_MODULE(target) \
        const struct sport_health_module sport_health_##target sec(.sport_health_module) =
extern const struct sport_health_module sport_health_module_begin[];
extern const struct sport_health_module sport_health_module_end[];
#define list_for_each_sport_health_module(p) \
    for (p =(struct sport_health_module*)sport_health_module_begin; p < (struct sport_health_module*)sport_health_module_end; p++)

struct heart_data {
    u8 real;
    u8 max;
    u8 min;
    u8 avg;
};
struct spo2_data {
    u8 real;
    u8 max;
    u8 min;
    u8 avg;
};
struct test_data {
    int steps;
};
typedef union {
#if 0
    // struct test_data acceler;
    struct daily_active acceler;
#else
    struct algo_value acceler;
    struct heart_data hr_data;
    struct spo2_data spo2_data;
#endif
} health_data_u;

typedef struct {
    int msg;
    int module;
    int type;
    int time;
    health_data_u data;
} health_data_msg_t;


int sport_health_manager_msg_post_self(int module, int cmd, void *priv);
int sport_health_manager_msg_post(int module, int cmd, int priv_len, void *priv);
#endif// __HEALTH_MANAGER__

