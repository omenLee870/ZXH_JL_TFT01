/**
 * @file app_vehicle.c
 * @brief 适配车辆数据、串行处理车辆操作，并向其他 RTOS 任务提供只读快照。
 * @note app_core 是内部状态的更新线程。协议读取在车辆状态锁外执行，锁内只更新或
 *       复制业务上下文，不调用 LVGL、Flash 保存或车辆控制接口。
 */
#ifndef APP_VEHICLE_TEST
#include "app_config.h"
#include "system/includes.h"
#endif
#include "app_vehicle.h"
#include "app_meter_control.h"
#include "vehicle_controller.h"
#include <string.h>

static struct vehicle_core vehicle;     /**< 由 app_core 维护的车辆业务上下文。 */
/* 状态访问只包含更新或复制，不需要递归进入。
 * 使用初值为 1 的信号量统一保护状态；本 SDK 的 os_sem_pend(-1) 能正确区分成功和忙。
 * 禁止混用 OSMutexAccept/os_mutex_post：SDK 的 Accept 不建立递归计数，Post 会下溢，
 * 导致首次 APP 查询后状态锁不再释放，随后 UI 停刷且查询持续返回 1004。 */
static OS_SEM vehicle_gate;             /**< 二值信号量，持有期间不调用外部业务接口。 */
static volatile unsigned initialized;   /**< 初始化完成标志；车辆数据仍需通过状态锁访问。 */
static volatile unsigned short vehicle_timer; /**< app_core 的 50ms 定时器；成功创建后允许提交操作。 */
static volatile unsigned shutdown_state; /**< 0 运行、1 正在收尾、2 已收尾。 */
static int shutdown_result; /**< 最近一次关机收尾结果，失败时允许 app_core 再次尝试保存。 */
static unsigned final_shutdown; /**< SDK 最终断电后禁止再次申请业务资源。 */
static unsigned vehicle_generation; /**< 关机递增，丢弃上一电源周期排队的操作。 */

/**
 * @brief 周期读取车辆数据并更新业务状态。
 * @param[in] priv 定时器私有参数，本模块未使用。
 * @retval 无。
 * @note 定时器由 app_core 注册，SDK 将回调投递回 app_core；此函数不在中断中执行。
 */
static void vehicle_poll(void *priv)
{
    struct vehicle_input input;
    (void)priv;
    /* 周期更新和消息处理只允许在同一个任务内执行。 */
    if (!initialized || shutdown_state || strcmp(os_current_task(), "app_core")) return;
    vehicle_controller_read(&input);
    if (os_sem_pend(&vehicle_gate, 0)) return;
    vehicle_core_update(&vehicle, &input, sys_timer_get_ms());
    os_sem_post(&vehicle_gate);
}

/**
 * @brief 在 app_core 中执行已排队的车辆操作。
 * @param[in] command 队列复制的操作编号，不引用发送方的临时缓冲。
 * @param[in] generation 提交时的电源周期编号，过期消息不执行。
 * @retval 无。
 * @note 协议操作在车辆状态锁外执行，随后立即发布更新后的统一状态。
 *       当前只接通小计里程清零；不写 Flash，不调用 LVGL，也不发送控制器命令。
 */
static void vehicle_command_callback(int command, unsigned generation)
{
    if (!initialized || shutdown_state || generation != vehicle_generation || strcmp(os_current_task(), "app_core")) return;
    switch (command) {
    case APP_VEHICLE_CMD_RESET_TRIP:
        app_vehicle_reset_trip_now();
        break;
    default:
        break;
    }
}

/** @brief app_core 同步清零 TRIP 并发布快照；不修改 ODO、不在状态锁内调用驱动。 */
int app_vehicle_reset_trip_now(void)
{
    if (!initialized || shutdown_state || strcmp(os_current_task(), "app_core")) return -1;
    if (vehicle_controller_reset_trip() != VEHICLE_CONTROLLER_OK) return -1;
    vehicle_poll(NULL);
    return 0;
}

/**
 * @brief 通过 SDK 的 app_core 消息队列提交操作。
 * @param[in] command 操作编号。
 * @return 已入队、未就绪、不支持或投递失败，见 app_vehicle_post_result。
 * @note 使用 SDK 已有的有界队列和按值参数；队列满时立即返回，不循环等待。
 */
int app_vehicle_post(enum app_vehicle_command command)
{
    if (command != APP_VEHICLE_CMD_RESET_TRIP) return APP_VEHICLE_UNSUPPORTED;
    if (!(vehicle_controller_capabilities() & VEHICLE_CONTROLLER_RESET_TRIP))
        return APP_VEHICLE_UNSUPPORTED;
    if (!initialized || !vehicle_timer || shutdown_state) return APP_VEHICLE_NOT_READY;
    int argv[4] = {(int)vehicle_command_callback, 2, (int)command, (int)vehicle_generation};
    return os_taskq_post_type("app_core", Q_CALLBACK, 4, argv) ?
        APP_VEHICLE_BUSY : APP_VEHICLE_QUEUED;
}

/**
 * @brief 在应用线程初始化并启动车辆业务服务。
 * @retval 0 初始化成功或已经启动。
 * @retval -1 不在 app_core 调用，或状态信号量、定时器创建失败。
 * @note 先读取一次数据，再启动周期更新；重试只补建尚未成功创建的资源。
 */
int app_vehicle_start(void)
{
    if (shutdown_state || strcmp(os_current_task(), "app_core")) return -1;
    if (!initialized) {
        if (os_sem_create(&vehicle_gate, 1)) return -1;
        vehicle_core_init(&vehicle, sys_timer_get_ms());
        initialized = 1;
    }
    if (!vehicle_timer) {
        vehicle_poll(NULL);
        vehicle_timer = sys_timer_add(NULL, vehicle_poll, 50);
        if (!vehicle_timer) return -1;
    }
    return 0;
}

/**
 * @brief 冻结数据源、保存里程并发布最终掉线状态。
 * @return 0 已完成，-1 线程不匹配、状态锁异常或保存失败。
 * @note 删除周期定时器后，已排队的更新与控制回调也由 shutdown_state 拦截。
 *       存储失败不循环等待；后续收尾入口可在 app_core 再尝试一次。
 */
int app_vehicle_suspend(void)
{
    struct vehicle_input input;
    uint32_t stopped_at_ms;
    int result;
    if (shutdown_state == 2 && shutdown_result == 0) return 0;
    if (strcmp(os_current_task(), "app_core")) return -1;
    if (!shutdown_state) ++vehicle_generation;
    shutdown_state = 1;
    if (vehicle_timer) {
        sys_timer_del(vehicle_timer);
        vehicle_timer = 0;
    }
    stopped_at_ms = sys_timer_get_ms();
    result = vehicle_controller_shutdown();
    /* 保存可能耗时，骑行计时截止到停止开始，不能把 Flash 写入时间算作骑行。 */
    if (initialized) {
        vehicle_controller_read(&input);
        if (os_sem_pend(&vehicle_gate, 0)) {
            shutdown_result = -1;
            shutdown_state = 2;
            return -1;
        }
        vehicle_core_update(&vehicle, &input, stopped_at_ms);
        os_sem_post(&vehicle_gate);
    }
    shutdown_result = result == VEHICLE_CONTROLLER_OK ? 0 : -1;
    shutdown_state = 2;
    return shutdown_result;
}

int app_vehicle_prepare_shutdown(void)
{
    if (final_shutdown && shutdown_state == 2 && shutdown_result == 0) return 0;
    if (strcmp(os_current_task(), "app_core")) return -1;
    final_shutdown = 1;
    app_meter_control_stop();
    return app_vehicle_suspend();
}

/** @brief 恢复接收与周期任务，保持里程上下文，失败时回收已经申请的接收资源。 */
int app_vehicle_resume(void)
{
    if (final_shutdown || strcmp(os_current_task(), "app_core")) return -1;
    if (!shutdown_state && vehicle_timer) return 0;
    if (vehicle_controller_start() != VEHICLE_CONTROLLER_OK) return -1;
    shutdown_state = 0;
    if (app_vehicle_start()) {
        /* 定时器申请失败时也必须释放捕获资源，不能留下半开机状态。 */
        app_vehicle_suspend();
        return -1;
    }
    return 0;
}

/**
 * @brief 获取供界面等任务使用的车辆状态副本。
 * @param[out] state 输出缓冲；为空时直接返回。
 * @retval 无。
 * @note 只能在任务上下文调用。锁内只复制，超时和计时处理在副本上执行，
 *       避免读取方修改业务状态，也避免 UI 刷新次数影响内部骑行计时。
 */
void app_vehicle_snapshot(struct vehicle_state *state)
{
    struct vehicle_core copy;
    if (!state) return;
    if (!initialized) vehicle_core_init(&copy, sys_timer_get_ms());
    else {
        if (os_sem_pend(&vehicle_gate, 0)) vehicle_core_init(&copy, sys_timer_get_ms());
        else {
            copy = vehicle;
            os_sem_post(&vehicle_gate);
        }
        /* 即使业务线程暂时繁忙，读取方也不能把过期数据当作静止或在线。 */
        vehicle_core_poll(&copy, sys_timer_get_ms());
    }
    *state = copy.state;
}

/**
 * @brief 不等待状态锁地读取车辆快照。
 * @param[out] state 输出缓冲，不能为空。
 * @return 0 表示成功，其他结果为未就绪或暂忙。
 * @note APP 等查询接口使用本接口，状态锁暂忙时立即返回，不阻塞通信任务。
 */
int app_vehicle_try_snapshot(struct vehicle_state *state)
{
    struct vehicle_core copy;
    if (!state || !initialized || final_shutdown) return APP_VEHICLE_NOT_READY;
    /* -1 使用 SDK 已导出的非阻塞信号量分支：成功为 0，无可用计数时为非 0。 */
    if (os_sem_pend(&vehicle_gate, -1)) return APP_VEHICLE_BUSY;
    copy = vehicle;
    os_sem_post(&vehicle_gate);
    vehicle_core_poll(&copy, sys_timer_get_ms());
    *state = copy.state;
    return 0;
}
