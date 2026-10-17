#include "youyibao_sif.h"
#include <string.h>

uint8_t yyb_checksum(const uint8_t *f)
{
    uint8_t sum = 0;
    unsigned i;
    for (i = 0; i < YYB_FRAME_SIZE - 1; ++i) sum ^= f[i];
    return sum;
}

int yyb_parse_frame(const uint8_t *f, size_t len, const struct yyb_config *c,
                    struct yyb_view *v)
{
    static const uint32_t map[4][8] = {
        {0, 0, YYB_SIDE_STAND, YYB_PARK, 0, 0, 0, 0},
        {YYB_PHASE_FAULT, YYB_ASSIST, YYB_CRUISE, YYB_UNDERVOLTAGE,
         YYB_CONTROLLER_FAULT, YYB_THROTTLE_FAULT, YYB_HALL_FAULT, 0},
        {0, 0, YYB_ANTI_RUNAWAY, YYB_REGEN, YYB_PROTECTION, YYB_BRAKE, YYB_RUNNING, 0},
        {YYB_SPEED_LIMIT, YYB_EBRAKE, YYB_REVERSE, YYB_STALL,
         YYB_OVERCURRENT, YYB_EKK, YYB_ONE_KEY, YYB_CURRENT_70}
    };
    unsigned byte, bit;
    if (!f || !c || !v || len != YYB_FRAME_SIZE) return -1;
    if (f[0] != 0x59 || f[1] != 0x42) return -2;
    if (yyb_checksum(f) != f[11]) return -3;
    memcpy(v->raw, f, YYB_FRAME_SIZE);
    v->flags = 0;
    for (byte = 0; byte < 4; ++byte)
        for (bit = 0; bit < 8; ++bit)
            if (f[byte + 2] & (1u << bit)) v->flags |= map[byte][bit];
    v->faults = v->flags & YYB_FAULT_MASK;
    v->gear = (f[4] & 3) + ((f[4] & 0x80) ? 4 : 0);
    v->current_a = c->current_twos_complement ?
        ((f[6] & 0x80) ? (int)f[6] - 256 : f[6]) :
        ((f[6] & 0x80) ? -(int)(f[6] & 0x7f) : f[6]);
    v->hall_count_500ms = ((uint16_t)f[7] << 8) | f[8];
    v->soc = f[9];
    v->soc_valid = f[9] <= 100;
    v->current_percent = f[10];
    v->current_percent_valid = f[10] <= 100;
    v->speed_dkph = -1;
    if (c->wheel_circumference_mm && c->hall_edges_per_wheel_rev) {
        uint64_t denom = (uint64_t)c->hall_edges_per_wheel_rev * 1000;
        uint64_t speed = ((uint64_t)v->hall_count_500ms *
            c->wheel_circumference_mm * 72 + denom / 2) / denom;
        v->speed_dkph = speed > 2147483647u ? 2147483647 : (int32_t)speed;
    } else if (c->speed_divisor) {
        /* Preserve the reference program's integer km/h truncation. */
        v->speed_dkph = (v->hall_count_500ms / c->speed_divisor) * 10;
    }
    return 0;
}

void yyb_decoder_reset(struct yyb_decoder *d)
{
    d->active = d->have_low = d->bits = 0;
}

void yyb_decoder_init(struct yyb_decoder *d, const struct yyb_config *c)
{
    memset(d, 0, sizeof(*d));
    if (c) d->config = *c;
    if (!d->config.timeout_ms) d->config.timeout_ms = 1000;
    /* 初始化时向车辆业务层发布默认有效期，首次收帧后再按实际帧速更新。 */
    d->view.timeout_ms = d->config.timeout_ms;
    d->view.speed_dkph = -1;
}

static uint32_t link_timeout(const struct yyb_decoder *d)
{
    uint32_t timeout = d->config.timeout_ms;
    uint32_t slow_timeout = d->view.short_pulse_us * 400 / 1000 + 500;
    return slow_timeout > timeout ? slow_timeout : timeout;
}

static void distance_update(struct yyb_decoder *d, uint32_t now)
{
    if (d->distance_clock_valid && d->view.online && d->view.speed_dkph >= 0) {
        uint32_t elapsed = now - d->distance_at_ms;
        uint32_t age = d->distance_at_ms - d->view.last_frame_ms;
        uint32_t timeout = link_timeout(d);
        uint64_t units, mm;
        if (age >= timeout) elapsed = 0;
        else if (elapsed > timeout - age) elapsed = timeout - age;
        /* 0.1 km/h * ms / 36 = mm. Carry the fractional millimetre. */
        units = (uint64_t)d->view.speed_dkph * elapsed + d->distance_remainder;
        mm = units / 36;
        d->distance_remainder = units % 36;
        d->view.odometer_mm += mm;
        d->view.trip_mm += mm;
    }
    d->distance_at_ms = now;
    d->distance_clock_valid = 1;
}

int yyb_decoder_pulse(struct yyb_decoder *d, unsigned level, uint32_t us, uint32_t now)
{
    uint32_t low;
    unsigned bit, idx, offset;
    int rm = -2, rl = -2;
    d->edge_ms = now;
    if (!level) {
        if (d->have_low) { d->view.timing_errors++; yyb_decoder_reset(d); }
        d->low_us = us;
        d->have_low = 1;
        return 0;
    }
    if (!d->have_low) {
        if (d->active) d->view.timing_errors++;
        yyb_decoder_reset(d);
        return 0;
    }
    low = d->low_us;
    d->have_low = 0;
    /* PDF's Tosc and recommended T disagree. Accept both: 0.35..11ms.
     * Long idle must meet BOTH >20ms and >31 short pulses. */
    if (us >= 350 && us <= 11000 && low > 20000 && low / us >= 31 &&
        (uint64_t)low > (uint64_t)us * 31) {
        d->unit_us = us;
        d->bits = 0;
        d->active = 1;
        memset(d->msb, 0, sizeof(d->msb));
        memset(d->lsb, 0, sizeof(d->lsb));
        return 0;
    }
    if (!d->active) return 0;
    /* Compare the actual LOW/HIGH durations of this bit. The sync HIGH
     * establishes frame boundaries, not an exact timing reference for data. */
    if (!low || !us || low == us) {
        d->view.timing_errors++;
        yyb_decoder_reset(d);
        return 0;
    }
    bit = us > low;
    idx = d->bits / 8;
    offset = d->bits % 8;
    d->msb[idx] |= bit << (7 - offset);
    d->lsb[idx] |= bit << offset;
    if (++d->bits != 96) return 0;
    d->active = 0;
    distance_update(d, now);
    if (d->config.bit_order != YYB_LSB_FIRST)
        rm = yyb_parse_frame(d->msb, YYB_FRAME_SIZE, &d->config, &d->view);
    if (rm != 0 && d->config.bit_order != YYB_MSB_FIRST)
        rl = yyb_parse_frame(d->lsb, YYB_FRAME_SIZE, &d->config, &d->view);
    if (rm != 0 && rl != 0) {
        if (rm == -3 || rl == -3) d->view.checksum_errors++;
        else d->view.header_errors++;
        return 0;
    }
    d->view.bit_order = rm == 0 ? YYB_MSB_FIRST : YYB_LSB_FIRST;
    d->view.short_pulse_us = d->unit_us;
    /* 同步发布本帧有效期，避免业务层将正常慢速报文提前判为掉线。 */
    d->view.timeout_ms = link_timeout(d);
    d->view.last_frame_ms = now;
    d->view.good_frames++;
    d->view.online = 1;
    return 1;
}

void yyb_decoder_poll(struct yyb_decoder *d, uint32_t now)
{
    uint32_t timeout = link_timeout(d);
    distance_update(d, now);
    if (d->active && (uint32_t)(now - d->edge_ms) > d->unit_us * 4 / 1000 + 20) {
        d->view.timing_errors++;
        yyb_decoder_reset(d);
    }
    if (d->view.online && (uint32_t)(now - d->view.last_frame_ms) >= timeout) {
        d->view.online = 0;
        d->view.flags = d->view.faults = 0;
        d->view.speed_dkph = -1;
        d->view.soc_valid = d->view.current_percent_valid = 0;
        d->view.current_a = 0;
        d->view.gear = 0;
        d->view.hall_count_500ms = 0;
    }
}
