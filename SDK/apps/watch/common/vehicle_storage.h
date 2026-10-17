/**
 * @file vehicle_storage.h
 * @brief 总里程双记录恢复、保存节流及失败重试接口。
 * @note 只保存总里程；小计和骑行时长不写入存储。调用方在同一个普通任务中串行调用。
 */
#ifndef VEHICLE_STORAGE_H
#define VEHICLE_STORAGE_H
#include <stdint.h>

/** @brief 存储接口，slot 取 0 或 1，读写返回实际字节数，flush 成功返回 0。 */
struct vehicle_storage_io {
    int (*read)(unsigned slot, void *buffer, unsigned length); /**< 读取指定记录。 */
    int (*write)(unsigned slot, const void *buffer, unsigned length); /**< 写入非当前记录。 */
    int (*flush)(void); /**< 确认写入已落到非易失存储，不能只停留在 RAM。 */
};

/** @brief 总里程保存上下文，不包含协议、RTOS 或界面对象。 */
struct vehicle_storage {
    uint64_t odometer_mm; /**< 最近一次确认保存成功的总里程，单位 mm。 */
    uint32_t sequence; /**< 当前记录序号，允许无符号回绕。 */
    uint32_t saved_at_ms; /**< 本次开机最近一次保存成功的时刻。 */
    uint32_t attempted_at_ms; /**< 最近一次尝试保存的时刻。 */
    unsigned active_slot; /**< 当前有效记录所在槽位；下一次写另一个槽位。 */
    unsigned attempted, was_moving, stop_pending; /**< 重试节流和停车待保存状态。 */
};

/**
 * @brief 从双记录恢复最新总里程，并兼容原有单记录格式。
 * @param[out] storage 待初始化的保存上下文。
 * @param[in] io 存储接口，所有回调均不能为空。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 1 已恢复有效记录。
 * @retval 0 没有有效记录，从零开始。
 */
int vehicle_storage_load(struct vehicle_storage *storage, const struct vehicle_storage_io *io, uint32_t now_ms);

/**
 * @brief 按保存周期、里程变化及停车状态处理保存。
 * @param[in,out] storage 保存上下文。
 * @param[in] io 存储接口。
 * @param[in] odometer_mm 当前总里程，不能低于已保存值。
 * @param[in] moving 非 0 表示有效车速大于零。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 1 已写入并确认保存。
 * @retval 0 本次无需保存。
 * @retval -1 写入、落盘失败或里程倒退。
 * @note 每 60 秒或新增 1km 保存；正常写入间隔至少 30 秒，停车请求保留到间隔满足。
 *       失败后每 5 秒允许重试；没有里程变化时不写入。
 */
int vehicle_storage_poll(struct vehicle_storage *storage, const struct vehicle_storage_io *io,
    uint64_t odometer_mm, unsigned moving, uint32_t now_ms);

/**
 * @brief 正常关机前立即保存已变化的总里程。
 * @param[in,out] storage 保存上下文。
 * @param[in] io 存储接口。
 * @param[in] odometer_mm 最终总里程，单位 mm。
 * @param[in] now_ms 当前毫秒时基。
 * @return 1 已保存，0 没有变化，-1 保存失败。
 * @note 不等待节流间隔，不循环重试；失败时保留上一份有效记录及待保存状态。
 */
int vehicle_storage_save(struct vehicle_storage *storage, const struct vehicle_storage_io *io,
    uint64_t odometer_mm, uint32_t now_ms);
#endif
