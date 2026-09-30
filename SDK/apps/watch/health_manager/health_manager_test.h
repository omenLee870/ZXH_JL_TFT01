#ifndef __HEALTH_MANAGER_TEST__
#define __HEALTH_MANAGER_TEST__

#include "app_config.h"

#if TCFG_SPORT_HEALTH_ENABLE

/* ---------------------------------------------------------------------------
 * 运动健康模块自动化测试入口。
 *
 * 设计约束：
 *   1. 本文件不注册任何启动钩子、不创建全局状态，**只有被显式调用才会执行**；
 *   2. 所有用例都在编译期断言 + 运行期打印两级自检，编译期断言保证协议值漂移
 *      会在 `make -j` 阶段直接失败；
 *   3. 断言对象为 pi32 目标（q32s/q32f，32 位指针）实测布局。
 * ------------------------------------------------------------------------- */

/* 测试总开关：0=完全不参与编译期之外的运行（交付态）；
 * 1=开机时在 health_manager 任务里跑一次自检 + 设定测试用 RTC。
 * ⚠️ ⑤ 测试阶段置 1，测完必须复位为 0（§5.8 上板前清理项，交付前 grep 复核）。 */
#ifndef TCFG_SPORT_HEALTH_TEST_ENABLE
#define TCFG_SPORT_HEALTH_TEST_ENABLE   0
#endif

/* 测试用 RTC 起始时间（仅 TCFG_SPORT_HEALTH_TEST_ENABLE=1 时生效）。
 * 默认 23:58:00，使开机后约 2 分钟即跨 0 点，供 TC-09 跨天收尾用例观察。 */
#define SHM_TEST_RTC_YEAR    2025
#define SHM_TEST_RTC_MONTH   10
#define SHM_TEST_RTC_DAY     1
#define SHM_TEST_RTC_HOUR    23
#define SHM_TEST_RTC_MIN     58
#define SHM_TEST_RTC_SEC     0

/* 总入口：一次跑完全部用例（结构体布局 / 枚举值 / 整点落盘 / 函数指针表 / 更新周期） */
void health_manager_test_run(void);

/* 测试用 RTC 设定：把 RTC 写为 SHM_TEST_RTC_* 指定的时刻。
 * 仅测试阶段调用；正式固件不调用（开机时间由 RCSP/APP 同步或恢复出厂默认）。 */
void health_manager_test_rtc_setup(void);

/* 分项用例：可单独调用，便于上板时定位失败项 */
void health_manager_test_case_shm_layout(void);      /* 跨核共享结构体尺寸 / 成员偏移 */
void health_manager_test_case_shm_enum(void);        /* SHM_MOD / SHM_CMD / SHM_TYPE / MSG_P11_* 取值 */
void health_manager_test_case_hour_flush(void);      /* 整点落盘 / 跨天清零分支判定真值表 */
void health_manager_test_case_sensor_service(void);  /* sensor_service 表遍历 + mod 唯一性 */
void health_manager_test_case_period_cfg(void);      /* 更新周期宏 + 无阻塞等待静态结论登记 */

#endif/* TCFG_SPORT_HEALTH_ENABLE */

#endif/* __HEALTH_MANAGER_TEST__ */
