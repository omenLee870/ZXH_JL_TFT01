#include "youyibao_sif.h"
#include "vehicle_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "vehicle_storage.h"

/* 用两份模拟 Flash 和待落盘缓存，区分“写入 RAM”与“真正断电后可恢复”。 */
static uint32_t now;
static unsigned char flash_data[2][24], pending_data[2][24];
static unsigned flash_size[2], pending_size[2];
static unsigned write_count, flush_count, deleted_timers, paused_captures;
static int fail_write, fail_flush, irq_depth, spin_depth;
static int storage_read(unsigned slot, void *buffer, unsigned length)
{
    assert(!irq_depth && !spin_depth && slot < 2);
    if (!flash_size[slot] || flash_size[slot] > length) return -1;
    memcpy(buffer, flash_data[slot], flash_size[slot]);
    return (int)flash_size[slot];
}
static int storage_write(unsigned slot, const void *buffer, unsigned length)
{
    assert(!irq_depth && !spin_depth && slot < 2 && length <= 24);
    ++write_count;
    if (fail_write) return -1;
    memcpy(pending_data[slot], buffer, length);
    pending_size[slot] = length;
    return (int)length;
}
static int storage_flush(void)
{
    unsigned slot;
    assert(!irq_depth && !spin_depth);
    ++flush_count;
    if (fail_flush == 1) return -1;
    for (slot = 0; slot < 2; ++slot) if (pending_size[slot]) {
        memcpy(flash_data[slot], pending_data[slot],
            fail_flush == 2 ? pending_size[slot] / 2 : pending_size[slot]);
        flash_size[slot] = pending_size[slot];
        pending_size[slot] = 0;
    }
    return fail_flush ? -1 : 0;
}
static const struct vehicle_storage_io test_io = { storage_read, storage_write, storage_flush };
static void storage_erase(void)
{
    memset(flash_data, 0, sizeof(flash_data));
    memset(flash_size, 0, sizeof(flash_size));
    memset(pending_size, 0, sizeof(pending_size));
    write_count = flush_count = 0;
    fail_write = fail_flush = 0;
}
static void storage_test(void)
{
    struct vehicle_storage s, restored;
    uint32_t legacy[4] = {0x594f4401u, 12345000, 0, 0};
    storage_erase();
    assert(!vehicle_storage_load(&s, &test_io, 0));
    assert(!vehicle_storage_poll(&s, &test_io, 0, 0, 60000) && !write_count);
    assert(!vehicle_storage_poll(&s, &test_io, 600000, 1, 59999));
    assert(vehicle_storage_poll(&s, &test_io, 600000, 1, 60000) == 1);
    assert(!vehicle_storage_poll(&s, &test_io, 610000, 0, 61000));
    assert(!vehicle_storage_poll(&s, &test_io, 610000, 0, 89999));
    assert(vehicle_storage_poll(&s, &test_io, 610000, 0, 90000) == 1);
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 610000);
    assert(!vehicle_storage_poll(&s, &test_io, 1610000, 1, 119999));
    assert(vehicle_storage_poll(&s, &test_io, 1610000, 1, 120000) == 1);
    fail_write = 1;
    assert(vehicle_storage_poll(&s, &test_io, 1710000, 1, 180000) == -1);
    assert(s.odometer_mm == 1610000 && write_count == 4);
    fail_write = 0;
    assert(!vehicle_storage_poll(&s, &test_io, 1710000, 1, 184999) && write_count == 4);
    fail_flush = 1;
    assert(vehicle_storage_poll(&s, &test_io, 1710000, 1, 185000) == -1);
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 1610000);
    memset(pending_size, 0, sizeof(pending_size)); /* 突然断电丢弃 RAM 缓存。 */
    fail_flush = 0;
    assert(vehicle_storage_save(&s, &test_io, 1710000, 185001) == 1); /* 关机不等重试间隔。 */
    assert(!vehicle_storage_save(&s, &test_io, 1710000, 185002));
    assert(vehicle_storage_save(&s, &test_io, 1, 185003) == -1);
    fail_flush = 2;
    assert(vehicle_storage_save(&s, &test_io, 1810000, 185004) == -1);
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 1710000);
    flash_data[0][0] ^= 1; flash_data[1][0] ^= 1;
    assert(!vehicle_storage_load(&restored, &test_io, 0) && !restored.odometer_mm);

    storage_erase();
    legacy[3] = legacy[0] ^ legacy[1] ^ legacy[2];
    memcpy(flash_data[0], legacy, sizeof(legacy)); flash_size[0] = sizeof(legacy);
    assert(vehicle_storage_load(&s, &test_io, 0) && s.odometer_mm == 12345000);
    assert(vehicle_storage_save(&s, &test_io, 12345001, 1) == 1 && s.active_slot == 1);
    assert(!memcmp(flash_data[0], legacy, sizeof(legacy)));
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 12345001);
    s.sequence = 0xfffffffeu;
    assert(vehicle_storage_save(&s, &test_io, 12345002, 2) == 1);
    assert(vehicle_storage_save(&s, &test_io, 12345003, 3) == 1 && !s.sequence);
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 12345003);
    s.saved_at_ms = 0xfffffff0u; s.attempted = 0;
    assert(vehicle_storage_poll(&s, &test_io, 12345004, 1, 0xfffffff0u + 60000u) == 1);
    puts("Vehicle storage: checkpoint, stop deferral, retry, durable flush, torn record, legacy, wrap PASS");
}

/* 编译实际接收模块，检查最终结算、关机保存和开机恢复的连接。 */
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
#define YOUYIBAO_RUNTIME_TEST 1
#define TCFG_YYB_SIF_ENABLE 1
#define TCFG_YYB_SIF_IO 5
#define TCFG_YYB_WHEEL_CIRCUMFERENCE_MM 0
#define TCFG_YYB_HALL_EDGES_PER_WHEEL_REV 0
#define TCFG_YYB_TIMEOUT_MS 1000
#define TCFG_YYB_CURRENT_TWOS_COMPLEMENT 0
#define TCFG_YYB_SPEED_DIVISOR 17
#define CFG_YYB_ODOMETER 58
#define CFG_YYB_ODOMETER_BACKUP 57
#define CFG_YYB_CALIBRATION 59
#define VM_ERR_NONE 0
#define VM_RAM_FLUSH_NONE (-242)
#define DEFINE_SPINLOCK(name) int name
#define IO_PORT_SPILT(io) (io)
#define BIT(n) (1u << (n))
#define late_initcall(fn)
enum { TIMERx, GPTIMER_MODE_CAPTURE_EDGE_ANYEDGE, PORT_INPUT_PULLDOWN_100K, PORT_HIGHZ };
#define GPTIMER_ERR_INIT_FAIL ((u32)-2)
struct gptimer_config {
    struct { unsigned filter, max_period, port, pin; } capture;
    void (*irq_cb)(u32, void *);
    unsigned irq_priority, mode;
};
static void local_irq_disable(void) { assert(!irq_depth); irq_depth = 1; }
static void local_irq_enable(void) { assert(irq_depth && !spin_depth); irq_depth = 0; }
static void spin_lock(int *p) { (void)p; assert(irq_depth && !spin_depth); spin_depth = 1; }
static void spin_unlock(int *p) { (void)p; assert(spin_depth); spin_depth = 0; }
static u32 sys_timer_get_ms(void) { return now; }
static u16 sys_timer_add(void *p, void (*cb)(void *), unsigned period)
{ (void)p; assert(cb && period == 50); return 1; }
static void sys_timer_del(u16 id) { assert(id == 1); ++deleted_timers; }
static u32 gptimer_init(int type, const struct gptimer_config *cfg)
{ assert(type == TIMERx && cfg->irq_cb); return 42; }
static void gptimer_start(u32 id) { assert(id == 42); }
static void gptimer_pause(u32 id) { assert(id == 42); ++paused_captures; }
static void gptimer_deinit(u32 id) { assert(id == 42); }
static u32 gptimer_get_capture_cnt2us(u32 id) { (void)id; return 500; }
static unsigned gpio_read_port(unsigned io) { (void)io; return 0; }
static void gpio_set_mode(unsigned io, unsigned mode) { (void)io; (void)mode; }
static int syscfg_read(unsigned id, void *buf, unsigned size)
{ return id == 59 ? -1 : storage_read(id == 57, buf, size); }
static int syscfg_write(unsigned id, const void *buf, unsigned size)
{ assert(id == 57 || id == 58); return storage_write(id == 57, buf, size); }
static int get_vm_ram_storage_enable(void) { return 1; }
static int get_vm_ram_storage_in_irq_enable(void) { return 0; }
static int vm_flush2flash(unsigned off) { assert(!off); return storage_flush(); }
#include "../../SDK/apps/watch/common/youyibao_sif.c"

/**
 * @brief 验证车辆业务的超时、未知值、恢复、计时和时基回绕行为。
 * @retval 无。
 * @note 使用固定输入和断言检查业务结果，不依赖控制器硬件或 RTOS。
 */
static void vehicle_business_test(void)
{
    struct vehicle_core c;
    struct vehicle_input in = {0};
    vehicle_core_init(&c, 0);
    assert(c.state.link == VEHICLE_LINK_WAITING && c.state.motion == VEHICLE_MOTION_UNKNOWN);
    assert(c.state.values.speed_dkph == -1 && c.state.values.current_ma == INT32_MIN);
    in.online = 1; in.source = VEHICLE_SOURCE_YYB;
    in.received_at_ms = 100; in.timeout_ms = 1000;
    in.values.valid = VEHICLE_SPEED_VALID | VEHICLE_SOC_VALID | VEHICLE_DISTANCE_VALID |
        VEHICLE_STATUS_VALID | VEHICLE_FAULTS_VALID | VEHICLE_CURRENT_VALID;
    in.values.speed_dkph = 360; in.values.soc = 83; in.values.current_ma = -12000;
    in.values.odometer_mm = 12345000; in.values.trip_mm = 45000;
    in.values.known_status = VEHICLE_BRAKE;
    in.values.status = VEHICLE_BRAKE | VEHICLE_REVERSE;
    vehicle_core_update(&c, &in, 100);
    assert(c.state.link == VEHICLE_LINK_ONLINE && c.state.motion == VEHICLE_MOVING);
    assert(c.state.values.status == VEHICLE_BRAKE && c.state.values.current_ma == -12000);
    assert(c.state.values.voltage_mv == -1 && c.state.values.gear == -1);
    vehicle_core_poll(&c, 600);
    vehicle_core_poll(&c, 600);
    assert(c.state.ride_time_ms == 500);
    /* 重复读取同一帧不能延长它的有效期。 */
    vehicle_core_update(&c, &in, 900);
    vehicle_core_poll(&c, 20000);
    assert(c.state.ride_time_ms == 1000 && c.state.link == VEHICLE_LINK_LOST);
    assert(c.state.motion == VEHICLE_MOTION_UNKNOWN && c.state.values.speed_dkph == -1);
    assert(c.state.values.odometer_mm == 12345000 && c.state.values.trip_mm == 45000);
    assert(c.state.values.faults == VEHICLE_COMMUNICATION_FAULT && !c.state.values.status);
    vehicle_core_update(&c, &in, 20000);
    assert(c.state.link == VEHICLE_LINK_LOST); /* 排队延迟的过期输入不能恢复在线。 */
    in.received_at_ms = 20000; in.values.speed_dkph = 0; in.values.current_ma = 0;
    vehicle_core_update(&c, &in, 20000);
    vehicle_core_poll(&c, 20500);
    assert(c.state.motion == VEHICLE_STOPPED && c.state.ride_time_ms == 1000);
    assert(c.state.values.speed_dkph == 0 && c.state.values.current_ma == 0);
    in.received_at_ms = 20500;
    in.values.valid = VEHICLE_SOC_VALID | VEHICLE_LOAD_VALID | VEHICLE_GEAR_VALID | VEHICLE_VOLTAGE_VALID;
    in.values.soc = 101; in.values.load_percent = -1; in.values.gear = -1; in.values.voltage_mv = -1;
    vehicle_core_update(&c, &in, 20500);
    assert(c.state.link == VEHICLE_LINK_ONLINE && c.state.motion == VEHICLE_MOTION_UNKNOWN);
    assert(c.state.values.soc == -1 && c.state.values.load_percent == -1);
    assert(!(c.state.values.valid & (VEHICLE_SOC_VALID | VEHICLE_LOAD_VALID | VEHICLE_GEAR_VALID | VEHICLE_VOLTAGE_VALID)));
    assert(c.state.values.odometer_mm == 12345000); /* 缺少里程字段的输入不能清空历史里程。 */
    in.online = 0;
    vehicle_core_update(&c, &in, 20500);
    assert(c.state.link == VEHICLE_LINK_LOST);
    vehicle_core_init(&c, 0xfffffff0u);
    memset(&in, 0, sizeof(in));
    in.online = 1; in.received_at_ms = 0xfffffff0u; in.timeout_ms = 1000;
    in.values.valid = VEHICLE_SPEED_VALID; in.values.speed_dkph = 360;
    vehicle_core_update(&c, &in, 0xfffffff0u);
    vehicle_core_poll(&c, 0xfffffff0u + 200u);
    assert(c.state.ride_time_ms == 200 && c.state.link == VEHICLE_LINK_ONLINE);
    vehicle_core_poll(&c, 0xfffffff0u + 1000u);
    assert(c.state.ride_time_ms == 1000 && c.state.link == VEHICLE_LINK_LOST);
    puts("Vehicle business: freshness, unknown/zero, recovery, signed current, ride time, mileage ownership, wrap PASS");
}

static void pulse(struct yyb_decoder *d, unsigned level, uint32_t us)
{
    now += (us + 999) / 1000;
    yyb_decoder_pulse(d, level, us, now);
}
static void frame(struct yyb_decoder *d, const uint8_t f[12], int lsb, unsigned t)
{
    unsigned i, b;
    pulse(d, 0, t * 40 > 20000 ? t * 40 : 21000);
    pulse(d, 1, t);
    for (i = 0; i < 12; ++i) {
        for (b = 0; b < 8; ++b) {
            unsigned bit = (f[i] >> (lsb ? b : 7 - b)) & 1;
            pulse(d, 0, bit ? t : t * 2);
            pulse(d, 1, bit ? t * 2 : t);
        }
    }
}
static void finish(uint8_t f[12])
{
    unsigned i;
    /* Independent check-byte construction for test fixtures. */
    f[11] = 0;
    for (i = 0; i < 11; ++i) f[11] ^= f[i];
}

static void runtime_shutdown_test(void)
{
    struct vehicle_storage restored;
    struct yyb_view view;
    uint8_t f[12] = {0x59,0x42,0,0,0,0,0,2,100,80,0,0};
    storage_erase(); now = 0;
    assert(!vehicle_storage_load(&restored, &test_io, 0));
    assert(vehicle_storage_save(&restored, &test_io, 12345000, 0) == 1);
    assert(!sif_boot());
    youyibao_sif_snapshot(&view);
    assert(view.odometer_mm == 12345000 && !view.trip_mm && !view.online);
    finish(f); frame(&decoder, f, 0, 500);
    assert(decoder.view.speed_dkph == 360);
    now += 100; link_check(NULL);
    youyibao_sif_snapshot(&view);
    assert(view.odometer_mm == 12346000 && view.trip_mm == 1000 && write_count == 1);
    now += 50;
    fail_flush = 1;
    assert(youyibao_sif_shutdown() == -1);
    assert(odometer_storage.odometer_mm == 12345000);
    fail_flush = 0;
    assert(!youyibao_sif_shutdown());
    assert(deleted_timers == 1 && paused_captures == 1);
    youyibao_sif_snapshot(&view);
    assert(!view.online && view.speed_dkph == -1 && view.odometer_mm == 12346500);
    assert(view.trip_mm == 1500);
    assert(vehicle_storage_load(&restored, &test_io, 0) && restored.odometer_mm == 12346500);
    /* 模拟 CPU 重启：持久存储保留，驱动 BSS 和小计清零。 */
    memset(&decoder, 0, sizeof(decoder));
    memset(&odometer_storage, 0, sizeof(odometer_storage));
    memset(pending_size, 0, sizeof(pending_size)); storage_ready = 0; now = 0;
    assert(!sif_boot());
    youyibao_sif_snapshot(&view);
    assert(view.odometer_mm == 12346500 && !view.trip_mm && !view.online);
    unsigned writes = write_count;
    assert(!youyibao_sif_shutdown() && write_count == writes);
    puts("Youyibao runtime: final 50ms, shutdown retry, unlocked Flash, reboot ODO/trip recovery PASS");
}

static void unequal_frame(struct yyb_decoder *d, const uint8_t f[12],
                          int lsb, unsigned short_us, unsigned long_us)
{
    unsigned i;
    /* Deliberately independent of the sync HIGH: compare each bit locally. */
    pulse(d, 0, 55304); pulse(d, 1, 591);
    for (i = 0; i < 96; ++i) {
        unsigned bit = (f[i / 8] >> (lsb ? i % 8 : 7 - i % 8)) & 1;
        pulse(d, 0, bit ? short_us : long_us);
        pulse(d, 1, bit ? long_us : short_us);
    }
}

static void test_relative_timing(const struct yyb_config *config, const uint8_t f[12])
{
    struct yyb_decoder d;
    struct yyb_config c = *config;
    /* Captured at PC05, 2026-10-15 11:34:22: sync plus first 9 data bits. */
    static const unsigned measured[] = {
        55304,591,891,601,394,1097,874,583,398,1097,
        399,1103,895,601,899,596,399,1199,713,601
    };
    unsigned i, lsb;
    c.bit_order = YYB_AUTO;
    yyb_decoder_init(&d,&c);
    for (i=0;i<sizeof(measured)/sizeof(measured[0]);++i)
        pulse(&d,i&1,measured[i]);
    assert(d.active && d.bits == 9 && d.msb[0] == 0x59);
    assert(!d.view.timing_errors && !d.view.good_frames);

    for (lsb=0;lsb<2;++lsb) {
        yyb_decoder_init(&d,&c);
        /* Full frame with the original TestBox's 100us phase/byte delays. */
        pulse(&d,0,55304); pulse(&d,1,591);
        for (i=0;i<96;++i) {
            unsigned bit=(f[i/8]>>(lsb?i%8:7-i%8))&1;
            unsigned low=(bit?400:900)-((i && i%8==0)?100:0);
            unsigned high=(bit?1100:600)+((i%8==7)?100:0);
            pulse(&d,0,low); pulse(&d,1,high);
        }
        assert(d.view.good_frames == 1 && !d.view.timing_errors);
        assert(!memcmp(d.view.raw,f,12));
        assert(d.view.bit_order == (lsb?YYB_LSB_FIRST:YYB_MSB_FIRST));

        /* No hidden 1:2 ratio requirement or sync-relative timing window. */
        unequal_frame(&d,f,lsb,500,501);
        unequal_frame(&d,f,lsb,300,1700);
        assert(d.view.good_frames == 3 && !d.view.timing_errors);
        assert(!memcmp(d.view.raw,f,12));
    }
    yyb_decoder_init(&d,&c);
    pulse(&d,0,25000); pulse(&d,1,500);
    pulse(&d,0,500); pulse(&d,1,500);
    assert(!d.active && d.view.timing_errors == 1);
    pulse(&d,0,25000); pulse(&d,1,500);
    pulse(&d,0,0); pulse(&d,1,500);
    assert(!d.active && d.view.timing_errors == 2);
    pulse(&d,0,25000); pulse(&d,1,500);
    pulse(&d,0,500); pulse(&d,1,0);
    assert(!d.active && d.view.timing_errors == 3);
    frame(&d,f,0,500);
    assert(d.view.good_frames == 1);
    /* Comparison decoding still requires the frame header and XOR. */
    {
        uint8_t bad[12];
        memcpy(bad,f,12); bad[0]=0x58; finish(bad);
        unequal_frame(&d,bad,0,400,1100);
        assert(d.view.good_frames == 1 && d.view.header_errors == 1);
        memcpy(bad,f,12); bad[11]^=1;
        unequal_frame(&d,bad,0,400,1100);
        assert(d.view.good_frames == 1 && d.view.checksum_errors == 1);
    }
}
int main(void)
{
    storage_test();
    runtime_shutdown_test();
    vehicle_business_test();
    struct yyb_config c = {2000, 60, 1000, YYB_AUTO, 0, 0};
    struct yyb_decoder d;
    struct yyb_view v = {0};
    uint8_t f[12] = {0x59,0x42,0xfc,0x7f,0xfc,0xff,0x85,0,60,90,70,0};
    unsigned t, i;
    finish(f);
    assert(yyb_parse_frame(f, 12, &c, &v) == 0);
    assert(v.flags == ((1u << 22) - 1));
    assert(v.faults == YYB_FAULT_MASK);
    assert(v.gear == 4 && v.current_a == -5);
    assert(v.hall_count_500ms == 60 && v.speed_dkph == 144);
    assert(v.soc == 90 && v.current_percent == 70 && v.soc_valid);
    {
        static const struct { unsigned byte, bit; uint32_t flag; } signals[] = {
            {2,3,YYB_PARK}, {2,2,YYB_SIDE_STAND}, {3,6,YYB_HALL_FAULT},
            {3,5,YYB_THROTTLE_FAULT}, {3,4,YYB_CONTROLLER_FAULT}, {3,3,YYB_UNDERVOLTAGE},
            {3,2,YYB_CRUISE}, {3,1,YYB_ASSIST}, {3,0,YYB_PHASE_FAULT},
            {4,6,YYB_RUNNING}, {4,5,YYB_BRAKE}, {4,4,YYB_PROTECTION},
            {4,3,YYB_REGEN}, {4,2,YYB_ANTI_RUNAWAY}, {5,7,YYB_CURRENT_70},
            {5,6,YYB_ONE_KEY}, {5,5,YYB_EKK}, {5,4,YYB_OVERCURRENT},
            {5,3,YYB_STALL}, {5,2,YYB_REVERSE}, {5,1,YYB_EBRAKE}, {5,0,YYB_SPEED_LIMIT}
        };
        uint8_t single[12] = {0x59,0x42};
        for (i=0;i<sizeof(signals)/sizeof(signals[0]);++i) {
            memset(single+2,0,10);
            single[signals[i].byte] = 1u << signals[i].bit;
            finish(single);
            assert(yyb_parse_frame(single,12,&c,&v) == 0);
            assert(v.flags == signals[i].flag);
            assert(v.faults == (signals[i].flag & YYB_FAULT_MASK));
        }
    }
    for (i = 0; i < 8; ++i) {
        f[4] = (i & 3) | ((i & 4) ? 0x80 : 0);
        finish(f);
        assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.gear == i);
    }
    c.current_twos_complement = 1;
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.current_a == -123);
    c.current_twos_complement = 0;
    f[7] = 0x12; f[8] = 0x34; f[9] = 101; f[10] = 255; finish(f);
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.hall_count_500ms == 0x1234);
    assert(!v.soc_valid && !v.current_percent_valid);
    c.wheel_circumference_mm = c.hall_edges_per_wheel_rev = 0;
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.speed_dkph == -1);
    c.speed_divisor = 17;
    f[7] = 1; f[8] = 0x10; finish(f); /* 272 / 17 = 16 km/h. */
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.speed_dkph == 160);
    f[8] = 0x20; finish(f); /* 288 / 17 = 16 (reference integer truncation). */
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.speed_dkph == 160);
    c.speed_divisor = 8;
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.speed_dkph == 360);
    c.speed_divisor = 26;
    assert(yyb_parse_frame(f,12,&c,&v) == 0 && v.speed_dkph == 110);
    c.speed_divisor = 17;
    assert(yyb_parse_frame(f,11,&c,&v) == -1);
    f[0] = 0x58; finish(f);
    assert(yyb_parse_frame(f,12,&c,&v) == -2);
    f[0] = 0x59; finish(f);
    f[11] ^= 1;
    assert(yyb_parse_frame(f,12,&c,&v) == -3);
    finish(f);
    test_relative_timing(&c,f);
    /* Both undocumented bit orders; adaptive timing including slow Tosc range. */
    for (t = 500; t <= 10000; t += 500) {
        yyb_decoder_init(&d,&c);
        frame(&d,f,0,t);
        assert(d.view.online && d.view.good_frames == 1);
        assert(d.view.bit_order == YYB_MSB_FIRST && d.view.short_pulse_us == t);
        assert(!memcmp(d.view.raw,f,12));
        frame(&d,f,1,t);
        assert(d.view.bit_order == YYB_LSB_FIRST && d.view.good_frames == 2);
    }
    c.bit_order = YYB_MSB_FIRST;
    yyb_decoder_init(&d,&c);
    frame(&d,f,1,500);
    assert(!d.view.online && d.view.header_errors == 1);
    frame(&d,f,0,500);
    assert(d.view.online);
    f[11] ^= 0x80;
    frame(&d,f,0,500);
    assert(d.view.checksum_errors == 1 && d.view.good_frames == 1);
    finish(f);
    /* Missing or invalid edges abort the partial frame, then recover at sync. */
    pulse(&d,0,25000); pulse(&d,1,500);
    pulse(&d,0,500); pulse(&d,1,500);
    assert(!d.active && d.view.timing_errors);
    frame(&d,f,0,500);
    assert(d.view.good_frames == 2);
    pulse(&d,0,25000); pulse(&d,1,500);
    pulse(&d,0,500); pulse(&d,0,500); pulse(&d,1,1000);
    assert(!d.active);
    frame(&d,f,0,500);
    pulse(&d,0,25000); pulse(&d,1,500);
    yyb_decoder_poll(&d,now+50);
    assert(!d.active);
    frame(&d,f,0,500);
    yyb_decoder_poll(&d,d.view.last_frame_ms+999);
    assert(d.view.online);
    yyb_decoder_poll(&d,d.view.last_frame_ms+1000);
    assert(!d.view.online && !d.view.flags && !d.view.faults);
    assert(d.view.speed_dkph == -1 && !d.view.soc_valid);
    /* Sync requires both >20ms idle and >31T; no acceptance without a sync. */
    yyb_decoder_init(&d,&c);
    pulse(&d,0,20000); pulse(&d,1,500); assert(!d.active);
    pulse(&d,0,30000); pulse(&d,1,1000); assert(!d.active);
    pulse(&d,0,32000); pulse(&d,1,1000); assert(d.active);
    /* Legal ±20% pulse jitter. */
    yyb_decoder_init(&d,&c);
    pulse(&d,0,25000); pulse(&d,1,500);
    for (i=0;i<96;++i) {
        unsigned bit=(f[i/8]>>(7-i%8))&1;
        pulse(&d,0,bit?600:800); pulse(&d,1,bit?1200:400);
    }
    assert(d.view.online && d.view.good_frames == 1);
    /* Timer wrap-around and slow frames don't create false link loss. */
    now = 0xfffffff0;
    yyb_decoder_init(&d,&c); frame(&d,f,0,10000);
    assert(d.view.timeout_ms == 4500);
    yyb_decoder_poll(&d,d.view.last_frame_ms+1000); assert(d.view.online);
    yyb_decoder_poll(&d,d.view.last_frame_ms+4500); assert(!d.view.online);
    /* Exact distance at 36km/h; never integrate through a long disconnected gap. */
    c.speed_divisor = 17;
    c.timeout_ms = 2000;
    f[7]=2; f[8]=100; finish(f); /* 612 / 17 = 36 km/h. */
    yyb_decoder_init(&d,&c); frame(&d,f,0,500);
    yyb_decoder_poll(&d,d.view.last_frame_ms+1000);
    assert(d.view.odometer_mm == 10000 && d.view.trip_mm == 10000);
    yyb_decoder_poll(&d,d.view.last_frame_ms+100000);
    assert(!d.view.online && d.view.odometer_mm == 20000);
    frame(&d,f,0,500);
    yyb_decoder_poll(&d,d.view.last_frame_ms+1000);
    assert(d.view.odometer_mm == 30000);
    puts("Youyibao SIF: relative HIGH/LOW decoding, measured pulses, TestBox timing, fields, errors, recovery, timeout PASS");
    return 0;
}
