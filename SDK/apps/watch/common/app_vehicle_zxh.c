/**
 * @file app_vehicle_zxh.c
 * @brief 将车辆真实数据和仪表本机控制接入 ZXH APP。
 * @note 本机控制仅限当前 BLE 会话，不授予整车控制或远程清零能力。
 *       查询期间不等待车辆锁，不读取硬件，不调用 UI 或 Flash 接口。
 */
#include "app_vehicle.h"
#include "app_meter_control.h"
#include "zxh_vehicle_protocol.h"
#include "zxh_ble.h"
#include <limits.h>
#include <string.h>

#if !defined(ZXH_PROTOCOL_TEST) || defined(ZXH_METER_BACKEND_TEST)
/** @brief 仅声明已启动的仪表本机功能，控制对象通过 CAPS 单独告知 APP。 */
uint32_t zxh_vehicle_supported_actions(void) { return app_meter_control_actions(); }
const char *zxh_vehicle_control_scope(void) { return "INSTRUMENT"; }

/** @brief 允许当前已订阅的 BLE 会话控制仪表，不开放 CTP 或整车执行接口。 */
int zxh_vehicle_authorize(void *client, const struct zxh_request *request)
{
    if (!client || !request || !zxh_ble_is_client(client) || !zxh_ble_client_active(client)) return ZXH_AUTH_FAILED;
    return (unsigned)request->action < ZXH_ACTION_COUNT &&
        (app_meter_control_actions() & (1u << request->action)) ? ZXH_OK : ZXH_ACTION_ERROR;
}

/** @brief 协议锁内只登记请求，实际操作及完成回包在后续 app_core/UI 周期执行。 */
int zxh_vehicle_submit(void *client, uint32_t token, const struct zxh_request *request, int *confirmed)
{
    if (confirmed) *confirmed = 0;
    return app_meter_control_submit(client, token, request);
}
#endif

/**
 * @brief 将快照读取结果转换为 APP 协议结果。
 * @param[out] state 成功时接收车辆快照，不能为空。
 * @return ZXH_OK、ZXH_BUSY 或 ZXH_EXECUTION_FAILED。
 */
static int read_snapshot(struct vehicle_state *state)
{
    int result = app_vehicle_try_snapshot(state);
    return result == 0 ? ZXH_OK : result == APP_VEHICLE_BUSY ? ZXH_BUSY : ZXH_EXECUTION_FAILED;
}

/**
 * @brief 将 64 位累计量转换为 APP 使用的有符号整数字段。
 * @param[in] value 以业务基础单位累计的数值。
 * @param[in] divisor 基础单位到目标单位的除数，必须非 0。
 * @return 换算后的非负整数；超出协议字段范围时返回未知值 -1。
 * @note 不截断高位，不把溢出后的数值作为真实里程或时长返回。
 */
static int count_value(uint64_t value, unsigned divisor)
{
    value /= divisor;
    return value > INT_MAX ? -1 : (int)value;
}

/**
 * @brief 允许当前 BLE 请求读取车辆状态或电池数据。
 * @param[in] client 协议入口传入的 BLE 会话令牌，不得解引用。
 * @param[in] topic 查询主题。
 * @return 合法 BLE 只读主题返回 ZXH_OK，否则返回 ZXH_AUTH_FAILED。
 * @note receive() 已在进入协议锁前检查 BLE 会话活跃且已订阅；回复发送时再次核对会话。
 *       此处只限定只读范围，不授予车主身份、车辆控制或其他传输通道的读取权限。
 */
int zxh_vehicle_authorize_read(void *client, const char *topic)
{
    if (!zxh_ble_is_client(client) || !topic) return ZXH_AUTH_FAILED;
    return !strcmp(topic, "ZXH_STATE") || !strcmp(topic, "ZXH_BMS") ? ZXH_OK : ZXH_AUTH_FAILED;
}

/**
 * @brief 为 APP 提供真实车辆状态及累计数据。
 * @param[out] state 输出状态，不能为空。
 * @return APP 协议结果；未接入的字段为 -1，暂忙时返回 ZXH_BUSY。
 * @note 驾驶模式与档位是不同字段；驻车不能当成锁车，掉线不能当成零车速。
 */
int zxh_vehicle_read_state(struct zxh_state *state)
{
    struct vehicle_state vehicle_state;
    const struct vehicle_values *v = &vehicle_state.values;
    int result;
    struct app_meter_state local;
    int local_result;
    if (!state) return ZXH_PARAMETER_ERROR;
    memset(state, 0xff, sizeof(*state));
    local_result = app_meter_control_snapshot(&local);
    if (local_result == ZXH_BUSY) return ZXH_BUSY;
    if (local_result == ZXH_OK) {
        state->power_on = local.power_on; state->light_on = local.light_on;
        state->mode = local.mode; state->finding = local.finding;
        state->power_state = local.power_state;
    }
    result = read_snapshot(&vehicle_state);
    if (result != ZXH_OK) return local_result == ZXH_OK ? ZXH_OK : result;
    state->link = vehicle_state.link;
    state->ride_time_s = count_value(vehicle_state.ride_time_ms, 1000);
    if (v->valid & VEHICLE_DISTANCE_VALID) {
        state->odometer_m = count_value(v->odometer_mm, 1000);
        state->trip_m = count_value(v->trip_mm, 1000);
    }
    if (local_result == ZXH_OK && !local.power_on) return ZXH_OK;
    int ride_mode = vehicle_values_ride_mode(v);
    if (ride_mode >= 0) state->mode = ride_mode;
    if (v->valid & VEHICLE_FAULTS_VALID) state->faults = (int)v->faults;
    if (vehicle_state.link == VEHICLE_LINK_ONLINE) {
        if (v->valid & VEHICLE_SPEED_VALID) state->speed_dkph = v->speed_dkph;
        if (v->valid & VEHICLE_GEAR_VALID) state->gear = v->gear;
        if (v->valid & VEHICLE_STATUS_VALID) {
            state->status = (int)v->status;
            state->known_status = (int)v->known_status;
        }
    }
    if (vehicle_state.auxiliary_gear && (v->valid & VEHICLE_GEAR_VALID)) state->gear = v->gear;
    if (vehicle_state.link != VEHICLE_LINK_ONLINE && vehicle_state.auxiliary_status && (v->valid & VEHICLE_STATUS_VALID)) {
        state->status = (int)(v->status & vehicle_state.auxiliary_status);
        state->known_status = (int)(v->known_status & vehicle_state.auxiliary_status);
    }
    return ZXH_OK;
}

/**
 * @brief 为 APP 提供一线通实际报告的电池数值。
 * @param[out] bms 输出电池数据，不能为空。
 * @return APP 协议结果；未知电流为 INT32_MIN，未知温度为 -32768，其余未知字段为 -1。
 * @note 没有电压、温度、充电状态等反馈时保留未知；回充标志不能替代充电状态。
 */
int zxh_vehicle_read_bms(struct zxh_bms *bms)
{
    struct vehicle_state state;
    const struct vehicle_values *v = &state.values;
    int result;
    if (!bms) return ZXH_PARAMETER_ERROR;
    memset(bms, 0xff, sizeof(*bms));
    bms->current_ma = INT32_MIN;
    bms->temperature_dc = -32768;
    {
        struct app_meter_state local;
        result = app_meter_control_snapshot(&local);
        if (result == ZXH_BUSY) return result;
        /* 关机和过渡态只返回未知，不把关机前的 SOC 当作实时电量。 */
        if (result == ZXH_OK && !local.power_on) return ZXH_OK;
    }
    result = read_snapshot(&state);
    if (result != ZXH_OK) return result;
    if ((v->valid & VEHICLE_STATUS_VALID) && (v->known_status & VEHICLE_CHARGING) &&
        (state.link == VEHICLE_LINK_ONLINE || (state.auxiliary_status & VEHICLE_CHARGING)))
        bms->charging = !!(v->status & VEHICLE_CHARGING);
    if (state.link == VEHICLE_LINK_ONLINE) {
        if (v->valid & VEHICLE_SOC_VALID) bms->soc = v->soc;
        if (v->valid & VEHICLE_CURRENT_VALID) bms->current_ma = v->current_ma;
        if (v->valid & VEHICLE_VOLTAGE_VALID) bms->voltage_mv = v->voltage_mv;
    }
    return ZXH_OK;
}
