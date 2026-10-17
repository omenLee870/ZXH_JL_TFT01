/**
 * @file app_meter_control.h
 * @brief 仪表电源生命周期，以及灯光、显示模式和寻车控制。
 * @note 逻辑关机关闭仪表业务，保留 RTOS、BLE 控制和 UI 唤醒通道；完全断电沿用 SDK 流程。
 */
#ifndef APP_METER_CONTROL_H
#define APP_METER_CONTROL_H
#include "zxh_vehicle_protocol.h"

enum app_meter_power_state {
    APP_METER_RUNNING = 0, /**< 采集、业务、显示均已完成开机。 */
    APP_METER_STOPPING,    /**< 正在退出显示、停止播放、保存及释放采集资源。 */
    APP_METER_OFF,         /**< 业务已关闭，仅保留启动和按需寻车所需的基础服务。 */
    APP_METER_STARTING,    /**< 正在恢复采集与显示，不提供旧的实时数据。 */
    APP_METER_FAULT        /**< 收尾或恢复失败，保持业务关闭并允许重试。 */
};
struct app_meter_state {
    int power_on; /**< 仅 RUNNING 为 1；过渡、关闭和失败状态均为 0。 */
    int light_on; /**< 仪表大灯图标开关，不表示整车灯线的反馈。 */
    int mode;     /**< 0 ECO、1 CITY、2 SPORT，仅改变仪表显示。 */
    int finding;  /**< 寻车提示音正在播放或在指定时段内重复。 */
    int power_state; /**< app_meter_power_state，所有入口共享同一电源状态。 */
};

/**
 * @brief 在 app_core 初始化本机控制服务，重复调用不会重置已有状态。
 * @return 0 已启动，-1 线程错误、资源创建失败或已经进入真正关机流程。
 */
int app_meter_control_start(void);
/** @brief SDK 最终断电前关闭控制桥；APP 逻辑关机保留控制桥，不调用此入口。 */
void app_meter_control_stop(void);
/** @brief 返回已接通的动作位图；逻辑关机仍提供开机与寻车，最终断电后返回 0。 */
uint32_t app_meter_control_actions(void);
/**
 * @brief 非阻塞读取仪表本机状态，协议和 UI 均通过副本访问。
 * @param[out] state 输出缓冲，不能为空；失败时不修改。
 * @return ZXH_OK、ZXH_BUSY 或 ZXH_EXECUTION_FAILED。
 */
int app_meter_control_snapshot(struct app_meter_state *state);
/**
 * @brief 按值保存协议请求，交给后续 app_core/UI 周期执行。
 * @param[in] client 已通过入口会话检查的 BLE 令牌，不得解引用。
 * @param[in] token 用于匹配完成回包的非零操作编号。
 * @param[in] request 操作及参数；复制后不保留调用方指针。
 * @return ZXH_PENDING 表示已接收，其他结果表示未提交；成功须等待完成回包。
 */
int app_meter_control_submit(void *client, uint32_t token, const struct zxh_request *request);
/** @brief 取消该 BLE 会话尚未执行的请求；已经完成的显示状态继续保留。 */
void app_meter_control_disconnect(void *client);
/** @brief 在 LVGL 线程每 100ms 调用，执行屏幕操作并反馈实际结果。 */
void app_meter_control_ui_poll(void);
/** @brief 仅临时点亮升级专用页面，保留升级前的业务电源状态。 */
void app_meter_control_ui_wake_for_update(void);
/** @brief 无锁读取电源策略，供播放器与异步页面入口拦截关机后的重新启动。 */
int app_meter_control_media_allowed(void);
/** @brief 关闭状态仅允许本机寻车音；最终断电提示音仍由 SDK 管理。 */
int app_meter_control_tone_allowed(void *priv, void *callback);
#endif
