/**
 * @file app_vehicle.h
 * @brief RTOS 环境中的车辆业务服务入口和只读快照接口。
 * @note app_core 更新内部状态，其他任务通过快照获取独立副本；接口不得在中断中调用。
 */
#ifndef APP_VEHICLE_H
#define APP_VEHICLE_H
#include "vehicle_core.h"

/** @brief 交给 app_core 执行的车辆操作；只列出已经接通的操作。 */
enum app_vehicle_command {
    APP_VEHICLE_CMD_RESET_TRIP = 1 /**< 清除小计里程，保留总里程和骑行时间。 */
};

/** @brief 操作提交结果；入队成功与实际执行完成是两个阶段。 */
enum app_vehicle_post_result {
    APP_VEHICLE_QUEUED = 0,     /**< 消息已复制到 app_core 队列，等待执行。 */
    APP_VEHICLE_NOT_READY = -1, /**< 服务尚未成功启动，没有提交消息。 */
    APP_VEHICLE_UNSUPPORTED = -2, /**< 操作或当前数据源不支持，没有提交消息。 */
    APP_VEHICLE_BUSY = -3       /**< 状态锁暂忙或 SDK 队列投递失败，没有自动重试。 */
};

/**
 * @brief 启动车辆业务服务。
 * @retval 0 初始化成功，或服务已经启动。
 * @retval -1 调用线程错误、状态信号量创建失败或定时器创建失败。
 * @note 必须由 app_core 调用，并在 UI 初始化前启动；每 50ms 更新一次。
 *       重复启动不会重置已有业务状态；定时器创建失败后允许再次调用重试。
 */
int app_vehicle_start(void);

/**
 * @brief 由 app_core 关闭车辆采集并保存里程，保留供下次开机使用的业务上下文。
 * @return 0 已关闭并保存，-1 线程、锁或保存失败；失败后允许再次收尾。
 * @note 不关闭蓝牙控制服务；与最终断电共用车辆收尾，禁止 UI 线程直接调用。
 */
int app_vehicle_suspend(void);
/**
 * @brief 由 app_core 重新申请采集资源并恢复车辆周期任务。
 * @return 0 已恢复，-1 恢复失败；失败时撤销已申请资源，保持关闭。
 * @note 保留里程与骑行累计值，重新等待新报文，不复用关机前的实时电量和车速。
 */
int app_vehicle_resume(void);

/**
 * @brief 将车辆操作按值投递给 app_core，调用方不直接修改车辆状态。
 * @param[in] command 要执行的操作，取值见 app_vehicle_command。
 * @return app_vehicle_post_result；0 仅表示已入队，不表示执行完成。
 * @note 供普通任务调用，不得在中断中调用；即使调用方就是 app_core 也统一排队。
 *       只投递一次，不等待处理、不申请堆内存、不持有车辆状态锁，也不保留调用方指针。
 *       APP 接入时仍需在入口处理鉴权；本接口不提供车辆控制权限或控制器反馈。
 */
int app_vehicle_post(enum app_vehicle_command command);
/** @brief 仅 app_core 可调用，清零后立即更新快照；0 成功。 */
int app_vehicle_reset_trip_now(void);

/**
 * @brief 获取当前车辆业务状态的独立副本。
 * @param[out] state 调用方提供的输出缓冲；为空时直接返回。
 * @retval 无。
 * @note 可在普通任务中调用，不得在中断中调用；启动前返回首次等待及未知数值。
 *       复制内部状态时使用状态锁，随后在副本上检查超时，不修改内部状态或延长在线。
 */
void app_vehicle_snapshot(struct vehicle_state *state);

/**
 * @brief 非阻塞获取车辆状态副本，供持有其他模块锁的查询接口使用。
 * @param[out] state 输出缓冲，不能为空；失败时不修改缓冲区。
 * @retval 0 已复制快照并检查数据时效。
 * @retval APP_VEHICLE_NOT_READY 服务未就绪或输出缓冲为空。
 * @retval APP_VEHICLE_BUSY 车辆状态锁暂不可用，本次没有等待。
 * @note 仅供普通任务调用；锁内只复制，数据时效在独立副本上检查。
 */
int app_vehicle_try_snapshot(struct vehicle_state *state);

/**
 * @brief 正常关机或软件重启前，停止车辆业务并保存最终总里程。
 * @retval 0 收尾成功，或里程没有变化、无需写入。
 * @retval -1 调用线程错误、状态锁异常或保存失败。
 * @note 首次收尾必须在 app_core 执行；已成功收尾后可重复调用。
 *       即使业务服务初始化失败，也会尝试保存已启动的数据源。
 *       不新增关机按键或界面，写 Flash 时不持有车辆状态锁。
 */
int app_vehicle_prepare_shutdown(void);
#endif
