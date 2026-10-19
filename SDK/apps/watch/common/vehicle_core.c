/**
 * @file vehicle_core.c
 * @brief 车辆数据有效性、通信状态、运动判定及骑行时间处理。
 * @note 本模块只修改调用方传入的上下文，不访问硬件、不刷新界面、不保存 Flash。
 *       里程由现有数据源累计，本模块只接收里程结果，避免重复计数。
 */
#include "vehicle_core.h"
#include <limits.h>
#include <string.h>

int vehicle_core_aux_publish(struct vehicle_core *core, enum vehicle_aux_source source,
    uint32_t known, uint32_t status, int gear, uint32_t now, uint32_t timeout)
{
    if (!core || (unsigned)source >= VEHICLE_AUX_COUNT || (known & ~VEHICLE_AUX_STATUS_MASK) ||
        (status & ~known) || gear < -1 || gear > 15 || timeout > 60000u) return -1;
    uint32_t mode_known = known & VEHICLE_RIDE_MODE_MASK, mode_status = status & VEHICLE_RIDE_MODE_MASK;
    if (mode_known && (mode_known != VEHICLE_RIDE_MODE_MASK ||
        !mode_status || (mode_status & (mode_status - 1u)))) return -1;
    struct vehicle_aux_input *p = &core->auxiliary[source];
    p->known = known; p->status = status; p->gear = gear;
    p->received_at_ms = now; p->timeout_ms = timeout;
    return 0;
}

void vehicle_core_aux_apply(const struct vehicle_core *core, struct vehicle_state *out, uint32_t now)
{
    unsigned i;
    out->auxiliary_status = out->physical_status = 0;
    out->auxiliary_gear = out->physical_gear = 0;
    for (i = 0; i < VEHICLE_AUX_COUNT; ++i) {
        const struct vehicle_aux_input *p = &core->auxiliary[i];
        if (!p->timeout_ms || (uint32_t)(now - p->received_at_ms) >= p->timeout_ms) continue;
        if (p->known) {
            out->values.valid |= VEHICLE_STATUS_VALID;
            out->values.status = (out->values.status & ~p->known) | p->status;
            out->values.known_status |= p->known;
            out->auxiliary_status |= p->known;
            if (i == VEHICLE_AUX_PHYSICAL) out->physical_status |= p->known;
        }
        if (p->gear >= 0) {
            out->values.valid |= VEHICLE_GEAR_VALID; out->values.gear = p->gear;
            out->auxiliary_gear = 1;
            if (i == VEHICLE_AUX_PHYSICAL) out->physical_gear = 1;
        }
    }
}

/**
 * @brief 将过期实时数据标为未知，保留已有里程。
 * @param[in,out] core 核心上下文，不能为空。
 * @retval 无。
 * @note 从未收到在线输入时保持首次等待；曾经在线时转为掉线并生成通信故障。
 *       未知车速必须对应未知运动状态，不能因掉线把车辆判断为静止。
 */
static void invalidate_live(struct vehicle_core *core)
{
    struct vehicle_values *v = &core->state.values;
    v->valid &= VEHICLE_DISTANCE_VALID;
    v->status = v->known_status = 0;
    v->speed_dkph = v->voltage_mv = -1;
    v->current_ma = INT32_MIN;
    v->soc = v->gear = v->load_percent = -1;
    v->faults = core->received_once ? VEHICLE_COMMUNICATION_FAULT : 0;
    if (core->received_once) v->valid |= VEHICLE_FAULTS_VALID;
    core->state.link = core->received_once ? VEHICLE_LINK_LOST : VEHICLE_LINK_WAITING;
    core->state.motion = VEHICLE_MOTION_UNKNOWN;
}

/**
 * @brief 初始化业务状态与计时基准。
 * @param[out] core 核心上下文，不能为空。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 无。
 */
void vehicle_core_init(struct vehicle_core *core, uint32_t now_ms)
{
    memset(core, 0, sizeof(*core));
    core->clock_ms = now_ms;
    invalidate_live(core);
}

/**
 * @brief 结算本次骑行时间并处理通信超时。
 * @param[in,out] core 核心上下文，不能为空，调用方负责串行访问。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 无。
 * @note 计时上限为最近报文的有效期，避免线程延迟或掉线后继续累计。
 */
void vehicle_core_poll(struct vehicle_core *core, uint32_t now_ms)
{
    if (core->state.link == VEHICLE_LINK_ONLINE) {
        uint32_t age = core->clock_ms - core->state.received_at_ms;
        uint32_t elapsed = now_ms - core->clock_ms;
        /* 将本次时间差裁到报文有效区间；重复处理相同时间不会再次累计。 */
        if (age >= core->timeout_ms) elapsed = 0;
        else if (elapsed > core->timeout_ms - age) elapsed = core->timeout_ms - age;
        if ((core->state.values.valid & VEHICLE_SPEED_VALID) && core->state.values.speed_dkph > 0)
            core->state.ride_time_ms += elapsed;
        if ((uint32_t)(now_ms - core->state.received_at_ms) >= core->timeout_ms)
            invalidate_live(core);
    }
    core->clock_ms = now_ms;
}

/**
 * @brief 接受一次车辆数据输入，生成完整业务快照。
 * @param[in,out] core 核心上下文，不能为空，调用方负责串行访问。
 * @param[in] input 协议适配后的输入，允许为空。
 * @param[in] now_ms 当前毫秒时基，必须与 input 的接收时间使用同一时钟。
 * @retval 无。
 * @note 无效或过期输入不能恢复在线；字段按有效位和取值范围检查，不伪造缺失数据。
 */
void vehicle_core_update(struct vehicle_core *core, const struct vehicle_input *input, uint32_t now_ms)
{
    struct vehicle_values *v = &core->state.values;
    if (!input) return;
    vehicle_core_poll(core, now_ms);
    core->state.source = input->source;
    /* 通信断开后仍可保留本机已累计的里程，不能随实时字段一并清空。 */
    if (input->values.valid & VEHICLE_DISTANCE_VALID) {
        v->odometer_mm = input->values.odometer_mm;
        v->trip_mm = input->values.trip_mm;
        v->valid |= VEHICLE_DISTANCE_VALID;
    }
    if (input->online) core->received_once = 1;
    /* 使用报文接收时间判断新鲜度，读取数据的当前时间不能替代它。 */
    if (!input->online || !input->timeout_ms ||
        (uint32_t)(now_ms - input->received_at_ms) >= input->timeout_ms) {
        invalidate_live(core);
        return;
    }
    /* 每次替换完整快照，防止不同报文的字段混在一起。里程只接收已有累计值，不再积分一遍。 */
    {
        uint64_t odo = v->odometer_mm, trip = v->trip_mm;
        uint32_t distance_valid = v->valid & VEHICLE_DISTANCE_VALID;
        *v = input->values;
        if (!(v->valid & VEHICLE_DISTANCE_VALID)) {
            v->odometer_mm = odo;
            v->trip_mm = trip;
            v->valid |= distance_valid;
        }
    }
    /* 未提供或越界的字段清有效位，并写入各自约定的未知值。 */
    if (!(v->valid & VEHICLE_SPEED_VALID) || v->speed_dkph < 0) {
        v->valid &= ~VEHICLE_SPEED_VALID;
        v->speed_dkph = -1;
    }
    if (!(v->valid & VEHICLE_SOC_VALID) || v->soc < 0 || v->soc > 100) {
        v->valid &= ~VEHICLE_SOC_VALID;
        v->soc = -1;
    }
    if (!(v->valid & VEHICLE_LOAD_VALID) || v->load_percent < 0 || v->load_percent > 100) {
        v->valid &= ~VEHICLE_LOAD_VALID;
        v->load_percent = -1;
    }
    if (!(v->valid & VEHICLE_GEAR_VALID) || v->gear < 0 || v->gear > 255) {
        v->valid &= ~VEHICLE_GEAR_VALID;
        v->gear = -1;
    }
    if (!(v->valid & VEHICLE_VOLTAGE_VALID) || v->voltage_mv < 0) {
        v->valid &= ~VEHICLE_VOLTAGE_VALID;
        v->voltage_mv = -1;
    }
    if (!(v->valid & VEHICLE_CURRENT_VALID)) v->current_ma = INT32_MIN;
    if (!(v->valid & VEHICLE_STATUS_VALID)) v->known_status = 0;
    v->status &= v->known_status;
    if (!(v->valid & VEHICLE_FAULTS_VALID)) v->faults = 0;
    core->state.received_at_ms = input->received_at_ms;
    core->timeout_ms = input->timeout_ms;
    core->state.link = VEHICLE_LINK_ONLINE;
    /* 沿用参考工程 1.0 km/h 的业务阈值；无有效车速时保持未知。 */
    core->state.motion = !(v->valid & VEHICLE_SPEED_VALID) ? VEHICLE_MOTION_UNKNOWN :
        v->speed_dkph >= 10 ? VEHICLE_MOVING : VEHICLE_STOPPED;
}
