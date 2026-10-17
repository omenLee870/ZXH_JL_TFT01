#ifndef YOUYIBAO_SIF_H
#define YOUYIBAO_SIF_H
#include <stdint.h>
#include <stddef.h>

#define YYB_FRAME_SIZE 12
enum yyb_bit_order { YYB_AUTO, YYB_MSB_FIRST, YYB_LSB_FIRST };
enum yyb_flag {
    YYB_PARK = 1u << 0, YYB_SIDE_STAND = 1u << 1,
    YYB_HALL_FAULT = 1u << 2, YYB_THROTTLE_FAULT = 1u << 3,
    YYB_CONTROLLER_FAULT = 1u << 4, YYB_UNDERVOLTAGE = 1u << 5,
    YYB_CRUISE = 1u << 6, YYB_ASSIST = 1u << 7, YYB_PHASE_FAULT = 1u << 8,
    YYB_RUNNING = 1u << 9, YYB_BRAKE = 1u << 10, YYB_PROTECTION = 1u << 11,
    YYB_REGEN = 1u << 12, YYB_ANTI_RUNAWAY = 1u << 13,
    YYB_CURRENT_70 = 1u << 14, YYB_ONE_KEY = 1u << 15,
    YYB_EKK = 1u << 16, YYB_OVERCURRENT = 1u << 17, YYB_STALL = 1u << 18,
    YYB_REVERSE = 1u << 19, YYB_EBRAKE = 1u << 20, YYB_SPEED_LIMIT = 1u << 21
};
#define YYB_FAULT_MASK (YYB_HALL_FAULT | YYB_THROTTLE_FAULT | YYB_CONTROLLER_FAULT | \
    YYB_UNDERVOLTAGE | YYB_PHASE_FAULT | YYB_PROTECTION | YYB_ANTI_RUNAWAY | \
    YYB_OVERCURRENT | YYB_STALL)

struct yyb_config {
    uint32_t wheel_circumference_mm;
    uint32_t hall_edges_per_wheel_rev; /* Includes motor gearing, all 3 Halls. */
    uint32_t timeout_ms;
    uint8_t bit_order;
    uint8_t current_twos_complement; /* Default: sign bit + 7-bit magnitude. */
    uint16_t speed_divisor; /* Reference YXT: count / (17 + adjustment - 9). */
};
struct yyb_view {
    uint8_t raw[YYB_FRAME_SIZE];
    uint32_t flags, faults;
    uint32_t last_frame_ms, short_pulse_us;
    uint32_t timeout_ms; /**< 当前报文有效期，单位 ms，包含慢速帧所需的扩展时间。 */
    uint32_t good_frames, checksum_errors, header_errors, timing_errors;
    int32_t speed_dkph; /* -1 if no calibration/divisor, or the link is lost. */
    int16_t current_a;
    uint16_t hall_count_500ms;
    uint8_t gear, soc, current_percent, soc_valid, current_percent_valid;
    uint8_t online, bit_order;
    uint64_t odometer_mm, trip_mm; /* Locally integrated, not wire fields. */
};
/* Pure decoder: feed the duration of the level which just ended, in us.
 * Each bit is LOW then HIGH: HIGH>LOW is 1, LOW>HIGH is 0.
 * Nominal timing is 0=2T,T; 1=T,2T. Sync=long LOW,T HIGH.
 * Caller serializes access; firmware adapter never touches LVGL in its ISR. */
struct yyb_decoder {
    struct yyb_config config;
    struct yyb_view view;
    uint32_t low_us, unit_us, edge_ms;
    uint8_t msb[YYB_FRAME_SIZE], lsb[YYB_FRAME_SIZE];
    uint8_t bits, active, have_low;
    uint8_t distance_clock_valid, distance_remainder;
    uint32_t distance_at_ms;
};
void yyb_decoder_init(struct yyb_decoder *d, const struct yyb_config *config);
void yyb_decoder_reset(struct yyb_decoder *d);
int yyb_decoder_pulse(struct yyb_decoder *d, unsigned ended_level,
                      uint32_t duration_us, uint32_t now_ms);
void yyb_decoder_poll(struct yyb_decoder *d, uint32_t now_ms);
int yyb_parse_frame(const uint8_t *frame, size_t len, const struct yyb_config *config,
                    struct yyb_view *view);
uint8_t yyb_checksum(const uint8_t *frame);

/* Firmware: start/stop/configure from task context; snapshot is thread safe.
 * Configuration changes reset synchronization and invalidate old telemetry. */
int youyibao_sif_start(void);
void youyibao_sif_stop(void);
/**
 * @brief 在 app_core 中冻结最终里程、停止接收并保存总里程。
 * @return 0 成功或没有变化，-1 保存失败。
 * @note 写存储在捕获锁外执行；不保存小计，不修改协议设置。
 */
int youyibao_sif_shutdown(void);
int youyibao_sif_configure(const struct yyb_config *config);
int youyibao_sif_save_config(const struct yyb_config *config);
void youyibao_sif_get_config(struct yyb_config *config);
int youyibao_sif_reset_trip(void);
void youyibao_sif_snapshot(struct yyb_view *view);
#endif
