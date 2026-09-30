#ifndef __HEALTH_MANAGER__
#define __HEALTH_MANAGER__
#include "app_config.h"
#include "system/includes.h"
#include "data_storage/data_storage.h"
/* Cross-core module IDs. Must stay value-identical to
 * pmc/C51/br33_pmc_q32f/apps/sensor/health_manager.h, SHM_MOD_* is carried in
 * health_data_msg_t.module and matched on the other core. Append only. */
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
    SHM_MOD_DET_MENSE,					//女性健康

    SHM_MOD_END,					//结束
} SHM_MOD;/*update sec 时 按BEGIN -> END顺序执行*/
/* Cross-core commands. Must stay value-identical to
 * pmc/C51/br33_pmc_q32f/apps/sensor/health_manager.h, SHM_CMD_* is carried in the
 * M2P/P2M control payload and matched on the other core. 新增命令一律追加在
 * SHM_CMD_MAX 之前，不要插到已有项中间：值一旦位移，两核固件版本不同步时会把
 * 命令解析成相邻的另一条，且不会报错。 */
typedef enum {
    SHM_CMD_INIT,
    SHM_CMD_RELEASE,
    SHM_CMD_ENABLE,
    SHM_CMD_DISABLE,
    SHM_CMD_DISBALE = SHM_CMD_DISABLE,/* 历史拼写兼容（心率/血氧模块沿用） */
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

    /* 追加区：以下命令只能加在本区末尾，不要插到已有项之间（见文件头 SHM_CMD 值对齐约定） */
    // 大核内部专用命令（不跨核；与 sdk 大核 health_manager.h 末尾保持一致，两核 SHM_CMD 值对齐）
    SHM_CMD_UPDATE_VAILE_P11,   // 小核(pmc)算法数据回填（pmc_gsensor）
    SHM_CMD_UPDATE_SAMPLE_P11,  // 采样缓存整包投递（仅大核内部用：sensor_driver_p11 → 数据模块）
    // 床头钟 dir 算法独立开关（跨核：大核 UI → 小核算法；与计步总闸 gsensor_algo_enable 解耦）
    SHM_CMD_DIR_ENABLE,
    SHM_CMD_DIR_DISABLE,

    SHM_CMD_MAX,
} SHM_CMD;
/* Cross-core value types. Must stay value-identical to
 * pmc/C51/br33_pmc_q32f/apps/sensor/health_manager.h, SHM_GET_TYPE_* is carried in
 * health_data_msg_t.type and matched on the other core. Append only. */
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

    // 小核(pmc)采样缓存拉取：请求体与应答体都是 struct shm_sample_pack，
    // 必须排在 SHM_GET_TYPE_MAX 之前（模块分发会用它做上界校验）
    SHM_GET_TYPE_SAMPLE_BUF,		//采样缓存整包

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
#else
#include "sensor_algorithm_jl_motion.h"
// typedef algo_out struct algo_value;

#endif
#define DISTANCE_MAP(dist) ((dist))

/* 心率/血氧结构（对齐 pmc health_manager.h，P11 数据回填用） */
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

/* 小核(pmc)采样缓存交互。采样过程在 pmc 完成，大核整点拉取整包后落盘。
 * 以下宏与结构体必须与 pmc/C51/br33_pmc_q32f/apps/sensor/health_manager.h 逐字一致。 */
#define SHM_SAMPLE_INTERVAL_MIN     (5)								//采样间隔(分钟)：整点对齐，每小时 12 个刻度
#define SHM_SAMPLE_NUM              (60 / SHM_SAMPLE_INTERVAL_MIN)	//每小时采样点数(12)
#define SHM_SAMPLE_DURATION_SEC     (30)							//单次采样窗时长(秒)
#define SHM_SAMPLE_DATA_LEN         (16)							//数据区长度(>= SHM_SAMPLE_NUM 且为 2 的幂，兼作 pmc 环形缓存长度)

/* 交互数据封装：总长度(1B) + 采样间隔(1B) + 数据
 * 18B < sizeof(struct algo_value) 38B，放进 health_data_u 不撑大 union，
 * health_data_msg_t 仍是 56B，跨核 IPC 报文尺寸不变。 */
struct shm_sample_pack {
    u8 data_size;						//本次回传点数 = 环内最新 data_size 个刻度（0=环内尚空）
    u8 sample_int;						//采样间隔(分钟)
    u8 data[SHM_SAMPLE_DATA_LEN];		//由旧到新的采样点（pmc 环形取最近 SHM_SAMPLE_NUM 个）
};

/* 小核(pmc)跨核算法数据上报结构（对齐 pmc health_manager.h） */
typedef union {
    struct algo_value acceler;      /* gsensor 算法数据 */
    struct heart_data hr_data;
    struct spo2_data spo2_data;
    u8 dir;                         /* 小核 gsensor 方向（G_SENSOR_DIR_*，bedside 用） */
    struct shm_sample_pack sample;  /* 小核采样缓存整包（整点拉取） */
} health_data_u;

typedef struct {
    int msg;
    int module;
    int type;
    int time;
    health_data_u data;
} health_data_msg_t;

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


struct shm_file_ops {
    void *(*open)(u8 type, u16 index);
    int (*write)(void *hd, u8 *buf, int offset, int len);
    int (*read)(void *hd, u8 *buf, int offset, int len);
    int (*delete)(void *hd);
    int (*update)(void *hd, u8 *buf, int offset, int len);
    int (*close)(void *hd);
    void *(*open_by_time)(u8 type, u16 year, u8 month, u8 day);
    int (*len)(void *hd);
    int (*total)(u8 type);
    int (*get_id)(void *hd, u8 index);
};

int sport_health_manager_init();
int sport_health_manager_release();
/* 主动关机前统一落盘入口：逐个模块请求把当前数据写入文件，总等待时长有上限，
 * 超时即放弃剩余模块，保证关机不被拖死。由关机路径调用。 */
int sport_health_manager_save_all_on_poweroff(void);
int sport_health_manager_value_get(int module, int type, void *priv);
int sport_health_manager_msg_post(int module, int cmd, void *priv, int need_pend);
int sport_health_manager_msg_post_self(int module, int cmd, void *priv);

/* 床头时钟方向接口。P11 模式由 health_manager/algo_p11/shm_algo_gsensor.c
 * 提供，master 直连模式由 sensor_hub/sensor_service_motion.c 提供，两者互斥编译。 */
u8 gsensor_dir_get(void);
/* 小核方向回传入口（P11 模式），仅 algo_p11 与 sensor_driver_p11 使用 */
void shm_algo_gsensor_dir_update(u8 dir);

void *sport_health_alloc(u32 size);
void sport_health_free(void *p);

// void rtc_read_time(struct sys_time *time);

void *sport_health_file_open(u8 type, u16 id);
int sport_health_file_update(void *hd, u8 *buf, int offset, int len);
int sport_health_file_write(void *hd, u8 *buf, int offset, int len);
int sport_health_file_read(void *hd, u8 *buf, int offset, int len);
void *sport_health_file_open_by_time(u8 type, u16 year, u8 month, u8 day);
int sport_health_file_close(void *hd);
int sport_health_file_get_len(void *hd);
int sport_health_file_get_total(u8 type);
int sport_health_file_delete(void *hd);
int sport_health_file_get_id(void *hd, u8 index);




#include "health_manager/shm_common_api.h"


#endif//undef __HEALTH_MANAGER__

