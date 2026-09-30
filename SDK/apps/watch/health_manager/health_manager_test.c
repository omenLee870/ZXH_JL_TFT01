/* ---------------------------------------------------------------------------
 * 运动健康模块自动化测试（编译期断言 + 运行期自检）。
 *
 * 约束：
 *   1. 本文件不注册启动钩子、不引入可变全局状态，**只有被显式调用才会执行**；
 *   2. 协议值 / 结构体布局全部用编译期断言固定，漂移会在编译阶段直接失败；
 *   3. 断言基准为 pi32 目标（q32s / q32f，32 位指针）实测布局，非宿主 x86；
 *   4. 被测函数若为 static，本文件不改其可见性，改用公开接口间接验证，
 *      无法覆盖的用例在文件末尾以注释登记，交 PM 裁决。
 * ------------------------------------------------------------------------- */
#include "app_config.h"

#if TCFG_SPORT_HEALTH_ENABLE

#include "health_manager/health_manager.h"
#include "health_manager_test.h"
#include "sensor_hub/sensor_service.h"
#include "power/p11_app_msg.h"
#include "sensor_algorithm_jl_motion.h"
#include "rtc/rtc_dev.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG             "[SHM-TEST]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif

/* 编译期断言：条件为假时数组长度为负 → 编译直接失败（SDK 无 STATIC_ASSERT 宏） */
#define SHM_TEST_ASSERT(_name, _cond) \
    typedef char shm_test_assert_##_name[(_cond) ? 1 : -1]

/* ---------------------------------------------------------------- 布局断言 */
/* 跨核共享结构体尺寸 / 成员偏移（pi32 目标实测值，宿主 x86 指针宽度不同，勿套用） */
SHM_TEST_ASSERT(algo_type_size,             sizeof(algo_type) == 2);
SHM_TEST_ASSERT(algo_value_size,            sizeof(struct algo_value) == 38);
SHM_TEST_ASSERT(algo_value_steps_off,       __builtin_offsetof(struct algo_value, steps) == 2);
SHM_TEST_ASSERT(algo_value_sleep_off,       __builtin_offsetof(struct algo_value, sleep) == 30);
SHM_TEST_ASSERT(algo_value_shake_off,       __builtin_offsetof(struct algo_value, shake) == 34);
SHM_TEST_ASSERT(heart_data_size,            sizeof(struct heart_data) == 4);
SHM_TEST_ASSERT(spo2_data_size,             sizeof(struct spo2_data) == 4);
/* 小核采样包：2 字节头 + 16 字节数据区。必须小于 algo_value(38B)，
 * 否则会撑大 union 进而改变跨核报文尺寸 */
SHM_TEST_ASSERT(shm_sample_pack_size,       sizeof(struct shm_sample_pack) == 18);
SHM_TEST_ASSERT(shm_sample_pack_fits_u,     sizeof(struct shm_sample_pack) <= sizeof(health_data_u));
SHM_TEST_ASSERT(health_data_u_size,         sizeof(health_data_u) == 38);
/* btsdk 侧为 56 B；pmc 侧因 algo_value 尾字段为 60 B。只断言本侧，理由见文末登记项 2b */
SHM_TEST_ASSERT(health_data_msg_size,       sizeof(health_data_msg_t) == 56);
SHM_TEST_ASSERT(health_data_msg_data_off,   __builtin_offsetof(health_data_msg_t, data) == 16);
SHM_TEST_ASSERT(daily_active_size,          sizeof(struct daily_active) == 12);
SHM_TEST_ASSERT(file_total_head_size,       sizeof(struct health_file_total_head) == 11);
SHM_TEST_ASSERT(file_data_head_size,        sizeof(struct health_file_data_head) == 4);
SHM_TEST_ASSERT(sport_health_module_size,   sizeof(struct sport_health_module) == 12);
SHM_TEST_ASSERT(sensor_service_size,        sizeof(sensor_service_t) == 20);
/* 函数指针表元素必须能装下 mod + 4 个函数指针（32 位目标） */
SHM_TEST_ASSERT(sensor_service_fits_mod,    sizeof(sensor_service_t) >= sizeof(s8) + sizeof(void *) * 4);

/* ------------------------------------------------------------ 枚举值断言 */
/* SHM_MOD：跨核 module 字段，Append only */
SHM_TEST_ASSERT(shm_mod_begin,              SHM_MOD_BEGIN == 0);
SHM_TEST_ASSERT(shm_mod_gsensor_algo,       SHM_MOD_GSENSOR_ALGO == 1);
SHM_TEST_ASSERT(shm_mod_heart_rate_algo,    SHM_MOD_HEART_RATE_ALGO == 2);
SHM_TEST_ASSERT(shm_mod_spo2_algo,          SHM_MOD_SPO2_ALGO == 3);
SHM_TEST_ASSERT(shm_mod_daily_active,       SHM_MOD_DAILY_ACTIVE == 4);
SHM_TEST_ASSERT(shm_mod_heart_rate,         SHM_MOD_HEART_RATE == 6);
SHM_TEST_ASSERT(shm_mod_sleep,              SHM_MOD_SLEEP == 15);
SHM_TEST_ASSERT(shm_mod_det_mense,          SHM_MOD_DET_MENSE == 22);
SHM_TEST_ASSERT(shm_mod_end,                SHM_MOD_END == 23);
/* SHM_CMD：跨核控制字，新增命令一律追加在 MAX 之前（插到中间会让值位移，
 * 两核固件版本不同步时按相邻命令静默错配） */
SHM_TEST_ASSERT(shm_cmd_enable,             SHM_CMD_ENABLE == 2);
SHM_TEST_ASSERT(shm_cmd_disable,            SHM_CMD_DISABLE == 3);
SHM_TEST_ASSERT(shm_cmd_disable_alias,      SHM_CMD_DISBALE == SHM_CMD_DISABLE);
SHM_TEST_ASSERT(shm_cmd_update,             SHM_CMD_UPDATE == 6);
SHM_TEST_ASSERT(shm_cmd_update_sec,         SHM_CMD_UPDATE_SEC == 7);
SHM_TEST_ASSERT(shm_cmd_update_all,         SHM_CMD_UPDATE_ALL == 8);
SHM_TEST_ASSERT(shm_cmd_save_continue,      SHM_CMD_SAVE_CONTINUE == 15);
SHM_TEST_ASSERT(shm_cmd_info_set,           SHM_CMD_INFO_SET == 17);
SHM_TEST_ASSERT(shm_cmd_target_set,         SHM_CMD_TARGET_SET == 18);
SHM_TEST_ASSERT(shm_cmd_priv,               SHM_CMD_PRIV == 19);
SHM_TEST_ASSERT(shm_cmd_update_vaile_p11,   SHM_CMD_UPDATE_VAILE_P11 == 20);
SHM_TEST_ASSERT(shm_cmd_update_sample_p11,  SHM_CMD_UPDATE_SAMPLE_P11 == 21);
SHM_TEST_ASSERT(shm_cmd_dir_enable,         SHM_CMD_DIR_ENABLE == 22);
SHM_TEST_ASSERT(shm_cmd_dir_disable,        SHM_CMD_DIR_DISABLE == 23);
SHM_TEST_ASSERT(shm_cmd_max,                SHM_CMD_MAX == 24);
/* SHM_TYPE：跨核 type 字段 */
SHM_TEST_ASSERT(shm_type_real_value,        SHM_GET_TYPE_REAL_VALUE == 0);
SHM_TEST_ASSERT(shm_type_max_value,         SHM_GET_TYPE_MAX_VALUE == 1);
SHM_TEST_ASSERT(shm_type_avg_value,         SHM_GET_TYPE_AVG_VALUE == 3);
SHM_TEST_ASSERT(shm_type_info,              SHM_GET_TYPE_INFO == 4);
SHM_TEST_ASSERT(shm_type_target,            SHM_GET_TYPE_TARGET == 8);
SHM_TEST_ASSERT(shm_type_data_analysis,     SHM_GET_TYPE_DATA_ANALYSIS == 11);
SHM_TEST_ASSERT(shm_type_sample_buf,        SHM_GET_TYPE_SAMPLE_BUF == 12);
/* SAMPLE_BUF 必须排在 MAX 之前：模块分发的 type 上界校验按 MAX 判，插到 MAX 之后会被拦掉 */
SHM_TEST_ASSERT(shm_type_max,               SHM_GET_TYPE_MAX == 13);
/* SHM_ERR */
SHM_TEST_ASSERT(shm_err_ok,                 SHM_ERR_OK == 0);
SHM_TEST_ASSERT(shm_err_mod_not_find,       SHM_ERR_MOD_NOT_FIND == 2);
SHM_TEST_ASSERT(shm_err_mod_invalid_param,  SHM_ERR_MOD_INVALID_PARAM == 13);
/* MSG_P11_*：双核 IPC 消息号，必须与 pmc 侧逐字一致 */
SHM_TEST_ASSERT(msg_p11_sensor_timer,       MSG_P11_SENSOR_TIMER == 8);
SHM_TEST_ASSERT(msg_p11_algorithm_event,    MSG_P11_ALGORITHM_EVENT == 9);
SHM_TEST_ASSERT(msg_p11_power_recover,      MSG_P11_POWER_RECOVER == 13);
SHM_TEST_ASSERT(msg_p11_hm_loop,            MSG_P11_HEALTH_MANAGER_LOOP == 14);
SHM_TEST_ASSERT(msg_p11_hm_data_m2p,        MSG_P11_HEALTH_MANAGER_DATA_M2P == 15);
SHM_TEST_ASSERT(msg_p11_hm_data_p2m,        MSG_P11_HEALTH_MANAGER_DATA_P2M == 16);
SHM_TEST_ASSERT(msg_p11_hm_data_p2p,        MSG_P11_HEALTH_MANAGER_DATA_P2P == 17);
SHM_TEST_ASSERT(msg_p11_sys_wake,           MSG_P11_SYS_WAKE == 0xff);

/* 更新周期宏必须可配且落在合理区间（默认 1000ms） */
SHM_TEST_ASSERT(update_period_nonzero,      TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS > 0);
SHM_TEST_ASSERT(update_period_upper,        TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS <= 5000);

/* ------------------------------------------------- 分支判定（纯函数复现） */
/* 复现 shm_mod_daily_active.c: shm_daily_active_hour_file() 的落盘准入条件。
 * 原函数为 static 且依赖 rtc_read_time / 文件系统，本文件不改其可见性，
 * 仅以等价布尔式做真值表校验（见函数末尾注释「未覆盖用例登记」）。 */
static int shm_test_hour_flush_admit(u8 min, u8 hour, u8 day, u8 last_hour, u8 last_day)
{
    if (min != 0) {
        return 0;                       /* 非整点不落盘 */
    }
    if ((hour == last_hour) && (day == last_day)) {
        return 0;                       /* 该整点已落盘（UPDATE_SEC 每秒触发，需去重） */
    }
    return 1;
}

/* 复现 shm_mod_daily_active.c: shm_daily_active_update() 的跨天清零条件 */
static int shm_test_day_rollover_clear(u8 storage_day, u8 cur_day)
{
    return ((storage_day != 0) && (cur_day != storage_day)) ? 1 : 0;
}

struct shm_test_flush_case {
    u8 min;
    u8 hour;
    u8 day;
    u8 last_hour;
    u8 last_day;
    u8 expect;
};

struct shm_test_clear_case {
    u8 storage_day;
    u8 cur_day;
    u8 expect;
};

void health_manager_test_case_shm_layout(void)
{
    log_info("layout: algo_type=%d algo_value=%d union=%d msg=%d data_off=%d",
             (int)sizeof(algo_type), (int)sizeof(struct algo_value), (int)sizeof(health_data_u),
             (int)sizeof(health_data_msg_t), (int)__builtin_offsetof(health_data_msg_t, data));
    log_info("layout: daily_active=%d total_head=%d data_head=%d shm_mod=%d svc=%d",
             (int)sizeof(struct daily_active), (int)sizeof(struct health_file_total_head),
             (int)sizeof(struct health_file_data_head), (int)sizeof(struct sport_health_module),
             (int)sizeof(sensor_service_t));
    log_info("layout: all compile-time asserts passed");
}

void health_manager_test_case_shm_enum(void)
{
    log_info("enum: MOD_END=%d CMD_MAX=%d UPDATE_VAILE_P11=%d TYPE_MAX=%d",
             (int)SHM_MOD_END, (int)SHM_CMD_MAX, (int)SHM_CMD_UPDATE_VAILE_P11, (int)SHM_GET_TYPE_MAX);
    log_info("enum: MSG LOOP=%d M2P=%d P2M=%d P2P=%d WAKE=0x%x",
             (int)MSG_P11_HEALTH_MANAGER_LOOP, (int)MSG_P11_HEALTH_MANAGER_DATA_M2P,
             (int)MSG_P11_HEALTH_MANAGER_DATA_P2M, (int)MSG_P11_HEALTH_MANAGER_DATA_P2P,
             (int)MSG_P11_SYS_WAKE);
    log_info("enum: all compile-time asserts passed");
}

void health_manager_test_case_hour_flush(void)
{
    const struct shm_test_flush_case flush_tbl[] = {
        { 0, 10, 5, 0xff, 0xff, 1 },    /* 整点、首次 → 落盘 */
        { 30, 10, 5, 0xff, 0xff, 0 },   /* 非整点 → 不落盘 */
        { 0, 10, 5, 10, 5, 0 },         /* 同小时同日 → 去重不落盘 */
        { 0, 11, 5, 10, 5, 1 },         /* 进入下一小时 → 落盘 */
        { 0, 10, 6, 10, 5, 1 },         /* 跨天、小时号相同 → 仍落盘 */
    };
    const struct shm_test_clear_case clear_tbl[] = {
        { 5, 6, 1 },                    /* storage 有效且跨天 → 清零 */
        { 5, 5, 0 },                    /* 同一天 → 不清零 */
        { 0, 6, 0 },                    /* storage 未初始化（day==0）→ 不清零 */
    };
    int fail = 0;
    unsigned int i;

    for (i = 0; i < sizeof(flush_tbl) / sizeof(flush_tbl[0]); i++) {
        const struct shm_test_flush_case *c = &flush_tbl[i];
        int got = shm_test_hour_flush_admit(c->min, c->hour, c->day, c->last_hour, c->last_day);
        if (got != c->expect) {
            fail++;
            log_error("hour_flush[%d] expect=%d got=%d", (int)i, c->expect, got);
        }
    }
    for (i = 0; i < sizeof(clear_tbl) / sizeof(clear_tbl[0]); i++) {
        const struct shm_test_clear_case *c = &clear_tbl[i];
        int got = shm_test_day_rollover_clear(c->storage_day, c->cur_day);
        if (got != c->expect) {
            fail++;
            log_error("day_clear[%d] expect=%d got=%d", (int)i, c->expect, got);
        }
    }
    log_info("hour_flush: %d flush + %d clear cases, fail=%d",
             (int)(sizeof(flush_tbl) / sizeof(flush_tbl[0])),
             (int)(sizeof(clear_tbl) / sizeof(clear_tbl[0])), fail);
}

void health_manager_test_case_sensor_service(void)
{
    sensor_service_t *svc;
    sensor_service_t *peer;
    int count = 0;
    int dup = 0;

    /* 注意：当前 REGISTER_SENSOR_SERVICE 使用点可能为 0，段为空属合法状态，
     * 因此这里只统计与查重，**不断言非空**（函数指针表迁移完成后计数会自然上升）。
     * 注意：不可写 `begin >= end` 形式的段首尾比较——两侧是不同数组对象，
     * clang 会报 -Wtautological-compare 且构建开了 -Werror；空段由 for 条件自然跳过。 */
    list_for_each_sensor_service(svc) {
        count++;
        for (peer = svc + 1; peer < sensor_service_end; peer++) {
            if (peer->mod == svc->mod) {
                dup++;
                log_error("sensor_service dup mod:%d", (int)svc->mod);
            }
        }
    }
    log_info("sensor_service: %d entries, dup=%d", count, dup);
}

void health_manager_test_case_period_cfg(void)
{
    /* 静态检查结论（本文件不改被测源码，仅记录）：
     * 跨核高频路径 sport_health_manager_msg_post(..., need_pend) 的 need_pend 由
     * health_manager.c 按 cpu_in_irq() / os_current_task() 自行判定，health_manager
     * 侧模块回调不主动阻塞等待；数据偏差由 1s 节拍（TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS）
     * 决定，秒级容忍。若后续新增 need_pend=1 的高频调用，须在此登记并复核。 */
    log_info("period: TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS=%d ms (no-blocking-wait note logged)",
             (int)TCFG_SPORT_HEALTH_UPDATE_PERIOD_MS);
}

/* ------------------------------------------------------------- 测试环境准备 */
/* 把 RTC 写为固定时刻，供跨天/整点用例观察。
 * 不设此步时 RTC 停在编译默认值（实测 2025-10-01 00:00:00）且不自增，
 * 整点落盘 / 跨天收尾用例需先设定 RTC 才等得到，故测试阶段先设时间。
 * 仅在 TCFG_SPORT_HEALTH_TEST_ENABLE=1 时写 RTC；=0 时本函数为空，
 * 正式固件的开机时间仍由 RCSP/APP 同步或恢复出厂默认决定。 */
void health_manager_test_rtc_setup(void)
{
#if TCFG_SPORT_HEALTH_TEST_ENABLE
    struct sys_time t = {
        .year  = SHM_TEST_RTC_YEAR,
        .month = SHM_TEST_RTC_MONTH,
        .day   = SHM_TEST_RTC_DAY,
        .hour  = SHM_TEST_RTC_HOUR,
        .min   = SHM_TEST_RTC_MIN,
        .sec   = SHM_TEST_RTC_SEC,
    };
    rtc_write_time(&t);

    struct sys_time back = {0};
    rtc_read_time(&back);
    log_info("rtc_setup: set %04d-%02d-%02d %02d:%02d:%02d, read back %04d-%02d-%02d %02d:%02d:%02d",
             (int)t.year, (int)t.month, (int)t.day, (int)t.hour, (int)t.min, (int)t.sec,
             (int)back.year, (int)back.month, (int)back.day,
             (int)back.hour, (int)back.min, (int)back.sec);
#endif
}

void health_manager_test_run(void)
{
    log_info("===== health_manager self-test begin =====");
    health_manager_test_rtc_setup();
    health_manager_test_case_shm_layout();
    health_manager_test_case_shm_enum();
    health_manager_test_case_hour_flush();
    health_manager_test_case_sensor_service();
    health_manager_test_case_period_cfg();
    log_info("===== health_manager self-test end =====");
}

/* ---------------------------------------------------------------------------
 * 未覆盖 / 需 PM 裁决的用例登记（本文件不改被测函数可见性）：
 *   1. shm_daily_active_hour_file() / shm_daily_active_update() 为 static 且依赖
 *      rtc_read_time() 与文件系统，无法在不改可见性的前提下直接调用；
 *      已用等价布尔式做真值表覆盖（health_manager_test_case_hour_flush）。
 *   2. 跨核 sizeof：btsdk struct algo_value = 38 B（无
 *      wrist_event），pmc 侧同名结构多尾部 int wrist_event（42 B），
 *      两侧不等 → 本文件**不断言跨核等长**，仅断言 btsdk 侧自身布局。
 *   2b. 上述差异沿 union 向上传染，health_data_msg_t 在 pmc 侧为 60 B、
 *      btsdk 侧为 56 B（差 4 B，来自 algo_value 尾字段）。PM 离线 gcc -m32 实测确认
 *      **被跨核消费的字段偏移两侧完全一致**（data@16 / steps@18 / sleep@46 /
 *      hr.real@16 / spo2.real@16），差异全部落在尾部无人读取区域；接收侧
 *      sensor_driver_p11.c 用 int msg[32]（128 B ≥ 60 B）+ 运行时 len 校验兜底，
 *      不越界、不错位。故本文件仍只断言 btsdk 侧自身布局（56 B），不断言两侧等长。
 *   3. `.sensor_service` 段：④ 阶段实测（sdk.map / nm sdk.elf）
 *      段位于 0x08086858，大小 0x14 = 20 B，恰为 1 条 wrist_tilt_service
 *      （交付 7 处注册，其余 6 处被守卫排除）。段非空但条目少，
 *      故 test 只统计 + 查重、不断言数量（见 case_sensor_service）。
 *   4. shm_daily_active_close_prev_day()（跨天收尾）与
 *      shm_daily_active_flush_now()（关机落盘）同为 static，且依赖 rtc_read_time()
 *      与文件系统，本文件同样无法直接调用；其准入条件与已覆盖的
 *      shm_test_day_rollover_clear() / 写盘格式共用，故不另建真值表。
 *   5. 关机落盘链路 sport_health_manager_save_all_on_poweroff() 依赖
 *      health_manager 任务已创建 + os_sem 超时，属集成场景（需真机/仿真），
 *      单测无法覆盖；其预算算术（SHM_POWEROFF_SAVE_BUDGET_MS 与单模块上限的
 *      截断关系）已在本文件 case_hour_flush 同类布尔复现手法之外，留 ⑤ 阶段
 *      由集成测试验证（关机落盘用例）。
 * ------------------------------------------------------------------------- */

#endif/* TCFG_SPORT_HEALTH_ENABLE */
