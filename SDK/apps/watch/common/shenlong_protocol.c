#include "shenlong_protocol.h"
#include <string.h>

static uint16_t be16(const uint8_t *p) { return ((uint16_t)p[0] << 8) | p[1]; }
static void put16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v; }
uint8_t sl_xor(const uint8_t *p, size_t n)
{
    uint8_t value = 0;
    while (n--) value ^= *p++;
    return value;
}

size_t sl_encode_command(const struct sl_command *c, uint8_t *o, size_t cap)
{
    size_t n;
    if (!c || !o || c->drive > 2 || c->assist > 15 || c->throttle_pwm > 1000) return 0;
    n = c->extended ? 21 : 20;
    if (cap < n) return 0;
    o[0] = 1; o[1] = n; o[2] = 1;
    o[3] = c->drive; o[4] = c->assist; o[5] = c->control; o[6] = c->magnets;
    put16(o + 7, c->diameter_tenth_inch);
    o[9] = c->sensitivity; o[10] = c->start_strength; o[11] = c->inner_magnets;
    o[12] = c->speed_limit; o[13] = c->current_limit;
    put16(o + 14, c->undervoltage_tenth_v); put16(o + 16, c->throttle_pwm);
    o[18] = c->control2;
    if (c->extended) o[19] = c->assist_pwm;
    o[n - 1] = sl_xor(o, n - 1);
    return n;
}

void sl_decoder_init(struct sl_decoder *d, uint16_t diameter)
{
    memset(d, 0, sizeof(*d));
    d->diameter_tenth_inch = diameter;
    d->input.source = VEHICLE_SOURCE_SHENLONG;
    d->input.timeout_ms = SL_TIMEOUT_MS;
    d->input.values.valid = VEHICLE_DISTANCE_VALID;
}

void sl_poll(struct sl_decoder *d, uint32_t now)
{
    struct vehicle_input *in = &d->input;
    if (d->distance_clock && in->online && in->values.speed_dkph >= 0) {
        uint32_t age = d->distance_at_ms - in->received_at_ms;
        uint32_t dt = now - d->distance_at_ms;
        uint64_t units;
        if (age >= SL_TIMEOUT_MS) dt = 0;
        else if (dt > SL_TIMEOUT_MS - age) dt = SL_TIMEOUT_MS - age;
        units = (uint64_t)in->values.speed_dkph * dt + d->distance_remainder;
        in->values.odometer_mm += units / 36;
        in->values.trip_mm += units / 36;
        d->distance_remainder = units % 36;
    }
    d->distance_at_ms = now; d->distance_clock = 1;
    if ((uint32_t)(now - in->received_at_ms) >= SL_TIMEOUT_MS) in->online = 0;
}

int sl_parse_feedback(struct sl_decoder *d, const uint8_t *f, size_t n, uint32_t now)
{
    struct vehicle_values *v;
    uint16_t current, period;
    if (!d || !f || n < 14 || n > SL_MAX_FRAME || f[0] != 2 || f[1] != n || f[2] != 1 || sl_xor(f, n)) return -1;
    current = be16(f + 5);
    /* Sign bit is reserved, not a documented negative-current representation. */
    if (current & 0x8000) return -2;
    sl_poll(d, now);
    memcpy(d->raw, f, n); d->raw_length = n;
    v = &d->input.values;
    v->valid = VEHICLE_DISTANCE_VALID | VEHICLE_CURRENT_VALID | VEHICLE_STATUS_VALID | VEHICLE_FAULTS_VALID;
    v->status = 0;
    v->known_status = VEHICLE_BRAKE | VEHICLE_CRUISE | VEHICLE_CHARGING;
    v->faults = 0;
    if (f[3] & 0x40) v->faults |= VEHICLE_HALL_FAULT;
    if (f[3] & 0x20) v->faults |= VEHICLE_THROTTLE_FAULT;
    if (f[3] & 0x10) v->faults |= VEHICLE_CONTROLLER_FAULT;
    if (f[3] & 8) v->faults |= VEHICLE_UNDERVOLTAGE;
    if (f[3] & 1) v->faults |= VEHICLE_PHASE_FAULT;
    if (f[3] & 0x84) v->status |= VEHICLE_CRUISE;
    if (f[4] & 0x20) v->status |= VEHICLE_BRAKE;
    if (f[4] & 8) v->status |= VEHICLE_CHARGING;
    if (f[4] & 0x10) v->faults |= VEHICLE_COMMUNICATION_FAULT;
    /* Optional project extension, appended after the original range field.
     * [13..16] = 'Z', 'X', version 1, lamps. Never reinterpret reserved bits.
     * Unknown tags/versions keep normal telemetry, without claiming lamp data. */
    if (n >= 18 && f[13] == 'Z' && f[14] == 'X' && f[15] == 1 && !(f[16] & 0xf0)) {
        static const uint32_t lamps[] = {VEHICLE_HEADLIGHT, VEHICLE_POSITION_LIGHT,
            VEHICLE_LEFT_TURN, VEHICLE_RIGHT_TURN};
        unsigned i;
        for (i = 0; i < 4; i++) {
            v->known_status |= lamps[i];
            if (f[16] & (1u << i)) v->status |= lamps[i];
        }
    }
    /* PAS status bit6=1 also means unsupported: retain in raw diagnostics only. */
    v->current_ma = (int32_t)(current & 0x3fff) * ((current & 0x4000) ? 100 : 1000);
    v->load_percent = f[7];
    if (f[7] <= 100) v->valid |= VEHICLE_LOAD_VALID;
    v->soc = f[10];
    /* 0 also means no BMS according to PDF: conservatively show unknown. */
    if (f[10] > 0 && f[10] <= 100) v->valid |= VEHICLE_SOC_VALID;
    period = be16(f + 8); d->wheel_period_ms = period;
    v->speed_dkph = -1;
    if (period && d->diameter_tenth_inch) {
        /* diameter*0.1in -> mm, circumference*pi; rounded 0.1km/h. */
        uint64_t numerator = (uint64_t)d->diameter_tenth_inch * 254 * 3141593 * 36;
        uint64_t denominator = (uint64_t)period * 100000000;
        v->speed_dkph = period >= 4000 ? 0 : (int32_t)((numerator + denominator/2) / denominator);
        v->valid |= VEHICLE_SPEED_VALID;
    }
    d->remaining_km = be16(f + 11);
    d->input.received_at_ms = now; d->input.online = 1; d->good++;
    return 0;
}

void sl_feed(struct sl_decoder *d, const uint8_t *data, size_t n, uint32_t now)
{
    size_t i;
    if (d->used && (uint32_t)(now - d->last_byte_ms) > 30) {
        d->used = 0; d->errors++;
    }
    d->last_byte_ms = now;
    for (i = 0; i < n; ++i) {
        /* used is always < SL_MAX_FRAME at this point. */
        d->buffer[d->used++] = data[i];
        while (d->used) {
            uint8_t len;
            if (d->buffer[0] != 2) goto discard;
            if (d->used < 3) break;
            len = d->buffer[1];
            if (len < 14 || len > SL_MAX_FRAME || d->buffer[2] != 1) goto discard;
            if (d->used < len) break;
            if (sl_parse_feedback(d, d->buffer, len, now)) goto discard;
            memmove(d->buffer, d->buffer + len, d->used - len);
            d->used -= len;
            continue;
discard:
            memmove(d->buffer, d->buffer + 1, --d->used); d->errors++;
        }
    }
}
