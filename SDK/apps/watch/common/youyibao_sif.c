#include "youyibao_sif.h"
#include "vehicle_storage.h"
#ifndef YOUYIBAO_RUNTIME_TEST
#include "app_config.h"
#include "system/includes.h"
#include "system/spinlock.h"
#include "gptimer.h"
#include "gpio.h"
#include "utils/syscfg_id.h"
#include "user_cfg_id.h"
#include "device/vm.h"
#endif
#include <string.h>

struct saved_settings { u32 magic; struct yyb_config config; u32 checksum; };

static DEFINE_SPINLOCK(sif_lock);
static struct yyb_decoder decoder;
static struct yyb_config settings = {
    .wheel_circumference_mm = TCFG_YYB_WHEEL_CIRCUMFERENCE_MM,
    .hall_edges_per_wheel_rev = TCFG_YYB_HALL_EDGES_PER_WHEEL_REV,
    .timeout_ms = TCFG_YYB_TIMEOUT_MS,
    .bit_order = YYB_AUTO,
    .current_twos_complement = TCFG_YYB_CURRENT_TWOS_COMPLEMENT,
    .speed_divisor = TCFG_YYB_SPEED_DIVISOR,
};
static u32 capture_id;
static u16 check_timer;
static volatile u8 running;
static struct vehicle_storage odometer_storage; /**< app_core 维护的持久化状态。 */
static unsigned storage_ready; /**< VM 恢复完成后才允许保存。 */

static void lock(void) { local_irq_disable(); spin_lock(&sif_lock); }
static void unlock(void) { spin_unlock(&sif_lock); local_irq_enable(); }

/**
 * @brief 读取总里程的指定记录。
 * @param[in] slot 0 对应原 VM 58，1 对应备用 VM 57。
 * @param[out] buffer 输出缓冲。
 * @param[in] length 最大读取长度。
 * @return 实际读取字节数或 SDK 错误码。
 */
static int odometer_read(unsigned slot, void *buffer, unsigned length)
{
    return syscfg_read(slot ? CFG_YYB_ODOMETER_BACKUP : CFG_YYB_ODOMETER, buffer, length);
}

/**
 * @brief 向非当前槽位写入一份总里程记录。
 * @param[in] slot 目标槽位。
 * @param[in] buffer 记录内容。
 * @param[in] length 记录长度。
 * @return 实际写入字节数或 SDK 错误码。
 * @note 仅从普通任务调用，不得持有捕获自旋锁。
 */
static int odometer_write(unsigned slot, const void *buffer, unsigned length)
{
    return syscfg_write(slot ? CFG_YYB_ODOMETER_BACKUP : CFG_YYB_ODOMETER, buffer, length);
}

/**
 * @brief 将可能启用的 VM RAM 缓存刷新到 Flash。
 * @return 0 表示已落盘或无需刷新，-1 表示落盘失败。
 * @note 参数 0 保留 VM 的后续写入能力，避免提前关闭其他模块的关机保存。
 */
static int odometer_flush(void)
{
    if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
        int result = vm_flush2flash(0);
        if (result != VM_ERR_NONE && result != VM_RAM_FLUSH_NONE) return -1;
    }
    return 0;
}

static const struct vehicle_storage_io odometer_io = {
    odometer_read, odometer_write, odometer_flush
}; /**< 存储读写均在捕获锁外执行。 */

#if TCFG_YYB_SIF_ENABLE
#define YYB_DIAG_PULSES 20
/* Written in the capture IRQ, copied with sif_lock, printed only by the task. */
static struct {
    u32 edges, rises, falls, repeated_level, last_edge_ms, last_us;
    u32 sample_us[YYB_DIAG_PULSES];
    u8 sample_level[YYB_DIAG_PULSES]; /* Level of the interval that just ended. */
    u8 sample_count, last_level;
} capture_diag;
static u32 diag_at_ms, diag_good_frames;

static void capture_irq(u32 tid, void *priv)
{
    u32 us = gptimer_get_capture_cnt2us(tid);
    unsigned new_level = !!gpio_read_port(IO_PORT_SPILT(TCFG_YYB_SIF_IO));
    (void)priv;
    lock();
    if (running) {
        u32 now = sys_timer_get_ms();
        if (capture_diag.edges && capture_diag.last_level == new_level)
            capture_diag.repeated_level++;
        capture_diag.edges++;
        if (new_level) capture_diag.rises++;
        else capture_diag.falls++;
        capture_diag.last_level = new_level;
        capture_diag.last_edge_ms = now;
        capture_diag.last_us = us;
        /* Keep a sync interval and its following bits, including inverted input. */
        if (us > 20000) capture_diag.sample_count = 0;
        if (capture_diag.sample_count < YYB_DIAG_PULSES) {
            unsigned n = capture_diag.sample_count++;
            capture_diag.sample_us[n] = us;
            capture_diag.sample_level[n] = !new_level;
        }
        /* The capture counter measures the interval ending at this edge.
         * Only fixed-size state, no heap, printing, UI or blocking calls. */
        yyb_decoder_pulse(&decoder, !new_level, us, now);
    }
    unlock();
}

static void diagnostic_log(u32 now)
{
    struct yyb_view v;
    typeof(capture_diag) c;
    u32 unit;
    unsigned bits, active, level, i, used;
    char wave[YYB_DIAG_PULSES * 14 + 1];
    if ((u32)(now - diag_at_ms) < 2000) return;
    diag_at_ms = now;
    lock();
    c = capture_diag;
    v = decoder.view;
    unit = decoder.unit_us;
    bits = decoder.bits;
    active = decoder.active;
    unlock();
    level = !!gpio_read_port(IO_PORT_SPILT(TCFG_YYB_SIF_IO));
    printf("[YYB_DIAG] edge=%u rise=%u fall=%u repeat=%u pin=%u age_ms=%u last_us=%u online=%u good=%u xor=%u head=%u timing=%u T=%u bits=%u active=%u\n",
        c.edges, c.rises, c.falls, c.repeated_level, level,
        c.edges ? now - c.last_edge_ms : now, c.last_us, v.online,
        v.good_frames, v.checksum_errors, v.header_errors, v.timing_errors,
        unit, bits, active);
    if (v.good_frames != diag_good_frames) {
        printf("[YYB_RAW] %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X soc=%u speed_dkph=%d flags=%x\n",
            v.raw[0], v.raw[1], v.raw[2], v.raw[3], v.raw[4], v.raw[5],
            v.raw[6], v.raw[7], v.raw[8], v.raw[9], v.raw[10], v.raw[11],
            v.soc, v.speed_dkph, v.flags);
    } else if (c.sample_count) {
        used = 0;
        for (i = 0; i < c.sample_count; ++i)
            used += snprintf(wave + used, sizeof(wave) - used, "%c%u ",
                c.sample_level[i] ? 'H' : 'L', c.sample_us[i]);
        printf("[YYB_WAVE] interval_us: %s\n", wave);
    }
    diag_good_frames = v.good_frames;
}

static void link_check(void *priv)
{
    struct yyb_view v;
    u32 now = sys_timer_get_ms();
    (void)priv;
    lock();
    if (!running) { unlock(); return; }
    yyb_decoder_poll(&decoder, now);
    v = decoder.view;
    unlock();
    if (storage_ready && vehicle_storage_poll(&odometer_storage, &odometer_io, v.odometer_mm,
        v.online && v.speed_dkph > 0, now) < 0)
        printf("[YYB] odometer save failed\n");
    diagnostic_log(now);
}
#endif

int youyibao_sif_start(void)
{
#if TCFG_YYB_SIF_ENABLE
    const struct gptimer_config cfg = {
        .capture.filter = 100000, /* Reject only very short spikes (<~10us). */
        .capture.max_period = 1000000,
        .capture.port = TCFG_YYB_SIF_IO / 16,
        .capture.pin = BIT(TCFG_YYB_SIF_IO % 16),
        .irq_cb = capture_irq,
        .irq_priority = 3,
        .mode = GPTIMER_MODE_CAPTURE_EDGE_ANYEDGE,
    };
    if (running) return 0;
    lock();
    uint64_t odo = decoder.view.odometer_mm, trip = decoder.view.trip_mm;
    yyb_decoder_init(&decoder, &settings);
    decoder.view.odometer_mm = odo > odometer_storage.odometer_mm ? odo : odometer_storage.odometer_mm;
    decoder.view.trip_mm = trip;
    memset(&capture_diag, 0, sizeof(capture_diag));
    diag_at_ms = sys_timer_get_ms();
    diag_good_frames = 0;
    unlock();
    capture_id = gptimer_init(TIMERx, &cfg);
    if (capture_id == (u32)-1 || capture_id == GPTIMER_ERR_INIT_FAIL) {
        printf("[YYB] no free capture timer\n");
        return -1;
    }
    /* Unlike ports D/E/H, port C has no PMC GPIO owner to release. */
    gpio_set_mode(IO_PORT_SPILT(TCFG_YYB_SIF_IO), PORT_INPUT_PULLDOWN_100K);
    running = 1;
    check_timer = sys_timer_add(NULL, link_check, 50);
    if (!check_timer) {
        running = 0;
        gptimer_deinit(capture_id);
        gpio_set_mode(IO_PORT_SPILT(TCFG_YYB_SIF_IO), PORT_HIGHZ);
        return -2;
    }
    gptimer_start(capture_id);
    printf("[YYB] RX PC05 capture=%u, wheel=%u mm hall/rev=%u divisor=%u order=%u diag=3 bit=H>L\n",
        capture_id, settings.wheel_circumference_mm, settings.hall_edges_per_wheel_rev,
        settings.speed_divisor, settings.bit_order);
    return 0;
#else
    return -1;
#endif
}

void youyibao_sif_stop(void)
{
#if TCFG_YYB_SIF_ENABLE
    if (!running) return;
    lock();
    /* 在关闭捕获前结算最后一段有效时间，不能漏掉最后一个周期的里程。 */
    yyb_decoder_poll(&decoder, sys_timer_get_ms());
    running = 0;
    unlock();
    sys_timer_del(check_timer);
    check_timer = 0;
    printf("[YYB_POWER] capture=%u pause begin\n", capture_id);
    gptimer_pause(capture_id);
    printf("[YYB_POWER] capture=%u deinit begin\n", capture_id);
    gptimer_deinit(capture_id);
    printf("[YYB_POWER] capture released\n");
    gpio_set_mode(IO_PORT_SPILT(TCFG_YYB_SIF_IO), PORT_HIGHZ);
#endif
    lock();
    uint64_t odo = decoder.view.odometer_mm, trip = decoder.view.trip_mm;
    yyb_decoder_init(&decoder, &settings);
    decoder.view.odometer_mm = odo;
    decoder.view.trip_mm = trip;
    unlock();
}

/**
 * @brief 停止接收后保存冻结的最终总里程。
 * @return 0 成功或无需保存，-1 保存失败。
 * @note 调用方必须是 app_core；停止和保存允许重复调用，没有变化时不重复写入。
 */
int youyibao_sif_shutdown(void)
{
    struct yyb_view view;
    youyibao_sif_stop();
    if (!storage_ready) return 0;
    youyibao_sif_snapshot(&view);
    return vehicle_storage_save(&odometer_storage, &odometer_io, view.odometer_mm,
        sys_timer_get_ms()) < 0 ? -1 : 0;
}

int youyibao_sif_configure(const struct yyb_config *cfg)
{
    if (!cfg || cfg->bit_order > YYB_LSB_FIRST || cfg->timeout_ms < 200 ||
        cfg->timeout_ms > 60000 || cfg->current_twos_complement > 1 ||
        cfg->wheel_circumference_mm > 20000 || cfg->hall_edges_per_wheel_rev > 1000000 ||
        cfg->speed_divisor > 1000)
        return -1;
    lock();
    uint64_t odo = decoder.view.odometer_mm, trip = decoder.view.trip_mm;
    settings = *cfg;
    yyb_decoder_init(&decoder, &settings);
    decoder.view.odometer_mm = odo;
    decoder.view.trip_mm = trip;
    unlock();
    return 0;
}

void youyibao_sif_get_config(struct yyb_config *cfg)
{
    if (!cfg) return;
    lock();
    *cfg = settings;
    unlock();
}

static u32 settings_checksum(const struct yyb_config *c)
{
    return 0x59424259u ^ c->wheel_circumference_mm ^ c->hall_edges_per_wheel_rev ^
        c->timeout_ms ^ c->bit_order ^ ((u32)c->current_twos_complement << 8) ^
        ((u32)c->speed_divisor << 16);
}

int youyibao_sif_save_config(const struct yyb_config *cfg)
{
    struct saved_settings saved = {0};
    struct yyb_config before;
    int ret;
    youyibao_sif_get_config(&before);
    ret = youyibao_sif_configure(cfg);
    if (ret) return ret;
    saved.magic = 0x59594202;
    saved.config = *cfg;
    saved.checksum = settings_checksum(cfg);
    if (syscfg_write(CFG_YYB_CALIBRATION, &saved, sizeof(saved)) != (int)sizeof(saved)) {
        youyibao_sif_configure(&before);
        return -2;
    }
    return 0;
}

int youyibao_sif_reset_trip(void)
{
    lock();
    decoder.view.trip_mm = 0;
    decoder.distance_remainder = 0;
    unlock();
    return 0;
}

void youyibao_sif_snapshot(struct yyb_view *v)
{
    if (!v) return;
    lock();
    *v = decoder.view;
    if (!running) { v->online = 0; v->speed_dkph = -1; }
    unlock();
}

static int sif_boot(void)
{
#if TCFG_YYB_SIF_ENABLE
    struct saved_settings saved;
    if (syscfg_read(CFG_YYB_CALIBRATION, &saved, sizeof(saved)) == (int)sizeof(saved) &&
        saved.magic == 0x59594202 && saved.checksum == settings_checksum(&saved.config))
        youyibao_sif_configure(&saved.config);
    vehicle_storage_load(&odometer_storage, &odometer_io, sys_timer_get_ms());
    storage_ready = 1;
    return youyibao_sif_start();
#else
    return 0;
#endif
}
late_initcall(sif_boot);
