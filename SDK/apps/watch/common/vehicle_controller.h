/**
 * @file vehicle_controller.h
 * @brief 控制器接入接口及当前接入方式实际支持的能力。
 * @note 由 app_core 调用读取和操作接口；业务服务不直接解释控制器协议。
 *       当前接入优宜宝单向一线通，没有向控制器发送车辆控制命令的通道。
 */
#ifndef VEHICLE_CONTROLLER_H
#define VEHICLE_CONTROLLER_H

#include "vehicle_core.h"

/** @brief 当前接入模块已经实现的能力，不代表控制器在线或操作已经完成。 */
enum vehicle_controller_capability {
    VEHICLE_CONTROLLER_TELEMETRY = 1u << 0, /**< 接收并转换车辆数据。 */
    VEHICLE_CONTROLLER_RESET_TRIP = 1u << 1 /**< 清除仪表本地小计里程，不向控制器发命令。 */
};

/** @brief 控制器接入模块的同步操作结果。 */
enum vehicle_controller_result {
    VEHICLE_CONTROLLER_OK = 0,           /**< 操作已执行成功。 */
    VEHICLE_CONTROLLER_UNSUPPORTED = -1, /**< 当前接入方式不支持该操作。 */
    VEHICLE_CONTROLLER_FAILED = -2       /**< 底层接口执行失败。 */
};

/**
 * @brief 查询当前接入模块实际支持的能力。
 * @return vehicle_controller_capability 的组合；未接入数据源时为 0。
 * @note 只查询已实现能力，不访问硬件，也不等待控制器反馈。
 */
uint32_t vehicle_controller_capabilities(void);

/**
 * @brief 将当前控制器快照转换为统一业务输入。
 * @param[out] input 输出缓冲；为空时直接返回。
 * @retval 无。
 * @note 保留真实报文时间、有效期和字段有效位；没有接入协议时返回空数据源。
 *       不提供的电压等字段不能伪造；里程由已有模块累计，此处只复制。
 */
void vehicle_controller_read(struct vehicle_input *input);

/**
 * @brief 清除仪表本地小计里程。
 * @return vehicle_controller_result；成功时保留总里程。
 * @note 由 app_core 在业务互斥锁外调用；不写 Flash，不发送车辆控制命令。
 */
int vehicle_controller_reset_trip(void);

/**
 * @brief 停止当前控制器接入并保存最终总里程。
 * @return VEHICLE_CONTROLLER_OK 或 VEHICLE_CONTROLLER_FAILED。
 * @note 由 app_core 在业务锁外调用；未接入协议时直接成功返回。
 */
int vehicle_controller_shutdown(void);
/** @brief 由应用任务恢复接收资源；成功返回 VEHICLE_CONTROLLER_OK。 */
int vehicle_controller_start(void);

#endif
