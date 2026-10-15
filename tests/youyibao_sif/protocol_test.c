#include "youyibao_sif.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t now;
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
