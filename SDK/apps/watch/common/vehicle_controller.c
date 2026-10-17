/**
 * @file vehicle_controller.c
 * @brief 将当前控制器协议接到统一车辆业务接口。
 * @note 当前只启用优宜宝接收；不会将一线通状态位当作可下发的控制命令。
 *       更换控制器协议时在本模块转换字段，业务服务及界面继续使用统一状态。
 */
#ifndef VEHICLE_CONTROLLER_TEST
#include "app_config.h"
#endif
#include "vehicle_controller.h"
#include "youyibao_sif.h"
#include <string.h>

/**
 * @brief 返回当前协议已经接通的能力。
 * @return 接收数据和本地小计清零的能力位；协议关闭时返回 0。
 */
uint32_t vehicle_controller_capabilities(void)
{
#if TCFG_YYB_SIF_ENABLE
    return VEHICLE_CONTROLLER_TELEMETRY | VEHICLE_CONTROLLER_RESET_TRIP;
#else
    return 0;
#endif
}

#if TCFG_YYB_SIF_ENABLE
/**
 * @brief 把优宜宝协议字段转换为统一的车辆业务字段。
 * @param[out] input 已清空的输出缓冲，不能为空。
 * @retval 无。
 * @note 电流由 A 换算成 mA；保留实际接收时间和慢速帧有效期。
 *       状态位、故障位分别映射；不支持的电压字段不设置有效位。
 */
static void read_youyibao(struct vehicle_input *input)
{
    static const struct { uint32_t wire, business; } statuses[] = {
        {YYB_PARK, VEHICLE_PARK}, {YYB_SIDE_STAND, VEHICLE_SIDE_STAND},
        {YYB_CRUISE, VEHICLE_CRUISE}, {YYB_ASSIST, VEHICLE_ASSIST},
        {YYB_RUNNING, VEHICLE_RUNNING}, {YYB_BRAKE, VEHICLE_BRAKE},
        {YYB_REGEN, VEHICLE_REGEN}, {YYB_ONE_KEY, VEHICLE_ONE_KEY},
        {YYB_CURRENT_70, VEHICLE_CURRENT_70}, {YYB_EKK, VEHICLE_EKK},
        {YYB_REVERSE, VEHICLE_REVERSE}, {YYB_EBRAKE, VEHICLE_EBRAKE},
        {YYB_SPEED_LIMIT, VEHICLE_SPEED_LIMIT}
    };
    static const struct { uint32_t wire, business; } faults[] = {
        {YYB_HALL_FAULT, VEHICLE_HALL_FAULT}, {YYB_THROTTLE_FAULT, VEHICLE_THROTTLE_FAULT},
        {YYB_CONTROLLER_FAULT, VEHICLE_CONTROLLER_FAULT}, {YYB_UNDERVOLTAGE, VEHICLE_UNDERVOLTAGE},
        {YYB_PHASE_FAULT, VEHICLE_PHASE_FAULT}, {YYB_PROTECTION, VEHICLE_PROTECTION},
        {YYB_ANTI_RUNAWAY, VEHICLE_ANTI_RUNAWAY}, {YYB_OVERCURRENT, VEHICLE_OVERCURRENT},
        {YYB_STALL, VEHICLE_STALL}
    };
    struct yyb_view wire;
    unsigned i;
    youyibao_sif_snapshot(&wire);
    input->source = VEHICLE_SOURCE_YYB;
    input->online = wire.online;
    input->received_at_ms = wire.last_frame_ms;
    input->timeout_ms = wire.timeout_ms;
    input->values.valid = VEHICLE_SPEED_VALID | VEHICLE_GEAR_VALID | VEHICLE_CURRENT_VALID |
        VEHICLE_DISTANCE_VALID | VEHICLE_STATUS_VALID | VEHICLE_FAULTS_VALID;
    if (wire.soc_valid) input->values.valid |= VEHICLE_SOC_VALID;
    if (wire.current_percent_valid) input->values.valid |= VEHICLE_LOAD_VALID;
    input->values.speed_dkph = wire.speed_dkph;
    input->values.current_ma = (int32_t)wire.current_a * 1000;
    input->values.soc = wire.soc;
    input->values.gear = wire.gear;
    input->values.load_percent = wire.current_percent;
    input->values.odometer_mm = wire.odometer_mm;
    input->values.trip_mm = wire.trip_mm;
    for (i = 0; i < sizeof(statuses) / sizeof(statuses[0]); ++i) {
        input->values.known_status |= statuses[i].business;
        if (wire.flags & statuses[i].wire) input->values.status |= statuses[i].business;
    }
    for (i = 0; i < sizeof(faults) / sizeof(faults[0]); ++i)
        if (wire.faults & faults[i].wire) input->values.faults |= faults[i].business;
}
#endif

/**
 * @brief 读取当前接入的数据源。
 * @param[out] input 输出缓冲，允许为空。
 * @retval 无。
 * @note 每次完整初始化输出，防止未接入或缺失的字段沿用上一次数据。
 */
void vehicle_controller_read(struct vehicle_input *input)
{
    if (!input) return;
    memset(input, 0, sizeof(*input));
#if TCFG_YYB_SIF_ENABLE
    read_youyibao(input);
#endif
}

/**
 * @brief 执行当前协议模块提供的本地小计清零。
 * @return 成功、不支持或执行失败。
 */
int vehicle_controller_reset_trip(void)
{
#if TCFG_YYB_SIF_ENABLE
    return youyibao_sif_reset_trip() == 0 ? VEHICLE_CONTROLLER_OK : VEHICLE_CONTROLLER_FAILED;
#else
    return VEHICLE_CONTROLLER_UNSUPPORTED;
#endif
}

/**
 * @brief 完成当前接入方式的关机收尾。
 * @return 已完成或保存失败。
 */
int vehicle_controller_shutdown(void)
{
#if TCFG_YYB_SIF_ENABLE
    return youyibao_sif_shutdown() == 0 ? VEHICLE_CONTROLLER_OK : VEHICLE_CONTROLLER_FAILED;
#else
    return VEHICLE_CONTROLLER_OK;
#endif
}

int vehicle_controller_start(void)
{
#if TCFG_YYB_SIF_ENABLE
    return youyibao_sif_start() == 0 ? VEHICLE_CONTROLLER_OK : VEHICLE_CONTROLLER_FAILED;
#else
    return VEHICLE_CONTROLLER_OK;
#endif
}
