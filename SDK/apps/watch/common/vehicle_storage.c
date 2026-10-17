/**
 * @file vehicle_storage.c
 * @brief 通过轮换记录和 CRC 校验保存、恢复总里程。
 * @note 写入和落盘均成功后才更新内存中的已保存值；损坏的新记录不覆盖旧记录的恢复能力。
 */
#include "vehicle_storage.h"
#include <string.h>

#define STORE_MAGIC 0x594f4402u
#define LEGACY_MAGIC 0x594f4401u
#define SAVE_INTERVAL_MS 60000u
#define MIN_INTERVAL_MS 30000u
#define RETRY_INTERVAL_MS 5000u
#define SAVE_DISTANCE_MM 1000000u

/** @brief 固定 20 字节记录，沿用原工程的小端 32 位字段布局。 */
struct store_record {
    uint32_t magic, sequence, low, high, crc;
};

/**
 * @brief 计算记录前四个字段的 CRC32。
 * @param[in] record 待校验记录。
 * @return CRC32 校验值。
 * @note 按低字节优先处理字段，校验不依赖主机结构体填充或字节序。
 */
static uint32_t record_crc(const struct store_record *record)
{
    uint32_t words[4] = {record->magic, record->sequence, record->low, record->high};
    uint32_t crc = 0xffffffffu;
    unsigned i, byte, bit;
    for (i = 0; i < 4; ++i) for (byte = 0; byte < 4; ++byte) {
        crc ^= (words[i] >> (byte * 8)) & 0xffu;
        for (bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0);
    }
    return ~crc;
}

/**
 * @brief 恢复最新有效记录；两份均不可用时尝试原有 16 字节格式。
 * @param[out] storage 保存上下文。
 * @param[in] io 存储接口。
 * @param[in] now_ms 本次开机时基。
 * @return 1 已恢复，0 没有有效记录。
 */
int vehicle_storage_load(struct vehicle_storage *storage, const struct vehicle_storage_io *io, uint32_t now_ms)
{
    struct store_record record[2];
    unsigned valid[2], slot, selected;
    memset(storage, 0, sizeof(*storage));
    storage->active_slot = 1;
    storage->saved_at_ms = now_ms;
    for (slot = 0; slot < 2; ++slot) {
        valid[slot] = io->read(slot, &record[slot], sizeof(record[slot])) == (int)sizeof(record[slot]) &&
            record[slot].magic == STORE_MAGIC && record[slot].crc == record_crc(&record[slot]);
    }
    if (valid[0] || valid[1]) {
        selected = valid[0] ? 0 : 1;
        if (valid[0] && valid[1] && record[1].sequence != record[0].sequence &&
            (uint32_t)(record[1].sequence - record[0].sequence) < 0x80000000u) selected = 1;
        storage->active_slot = selected;
        storage->sequence = record[selected].sequence;
        storage->odometer_mm = ((uint64_t)record[selected].high << 32) | record[selected].low;
        return 1;
    }
    /* 旧记录仍位于槽 0；缓冲区留足新格式大小，以兼容不同 VM 读取返回长度。 */
    {
        uint32_t legacy[5] = {0};
        if (io->read(0, legacy, 16) == 16 && legacy[0] == LEGACY_MAGIC &&
            legacy[3] == (legacy[0] ^ legacy[1] ^ legacy[2])) {
            storage->active_slot = 0;
            storage->odometer_mm = ((uint64_t)legacy[2] << 32) | legacy[1];
            return 1;
        }
    }
    return 0;
}

/**
 * @brief 写入另一槽位，并在确认落盘后提交保存状态。
 * @param[in,out] storage 保存上下文。
 * @param[in] io 存储接口。
 * @param[in] odometer_mm 当前累计总里程。
 * @param[in] now_ms 当前毫秒时基。
 * @return 1 已保存，0 无变化，-1 失败或里程倒退。
 */
int vehicle_storage_save(struct vehicle_storage *storage, const struct vehicle_storage_io *io,
    uint64_t odometer_mm, uint32_t now_ms)
{
    struct store_record record;
    unsigned slot = storage->active_slot ^ 1u;
    if (odometer_mm < storage->odometer_mm) return -1;
    if (odometer_mm == storage->odometer_mm) return 0;
    record.magic = STORE_MAGIC;
    record.sequence = storage->sequence + 1;
    record.low = (uint32_t)odometer_mm;
    record.high = (uint32_t)(odometer_mm >> 32);
    record.crc = record_crc(&record);
    storage->attempted = 1;
    storage->attempted_at_ms = now_ms;
    if (io->write(slot, &record, sizeof(record)) != (int)sizeof(record) || io->flush() != 0) return -1;
    storage->active_slot = slot;
    storage->sequence = record.sequence;
    storage->odometer_mm = odometer_mm;
    storage->saved_at_ms = now_ms;
    storage->stop_pending = 0;
    return 1;
}

/**
 * @brief 保留停车保存请求，并按时间、里程变化和失败间隔决定是否保存。
 * @param[in,out] storage 保存上下文。
 * @param[in] io 存储接口。
 * @param[in] odometer_mm 当前总里程。
 * @param[in] moving 当前运动状态。
 * @param[in] now_ms 当前毫秒时基。
 * @return 保存结果，见 vehicle_storage_save。
 */
int vehicle_storage_poll(struct vehicle_storage *storage, const struct vehicle_storage_io *io,
    uint64_t odometer_mm, unsigned moving, uint32_t now_ms)
{
    uint32_t elapsed = now_ms - storage->saved_at_ms;
    if (storage->was_moving && !moving) storage->stop_pending = 1;
    if (moving) storage->stop_pending = 0;
    storage->was_moving = !!moving;
    if (odometer_mm < storage->odometer_mm) return -1;
    if (odometer_mm == storage->odometer_mm || elapsed < MIN_INTERVAL_MS) return 0;
    if (storage->attempted && (uint32_t)(now_ms - storage->attempted_at_ms) < RETRY_INTERVAL_MS) return 0;
    if (elapsed < SAVE_INTERVAL_MS && odometer_mm - storage->odometer_mm < SAVE_DISTANCE_MM && !storage->stop_pending) return 0;
    return vehicle_storage_save(storage, io, odometer_mm, now_ms);
}
