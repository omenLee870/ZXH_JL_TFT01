#include "vehicle_test_control.h"
#include "app_vehicle.h"
#include <stdio.h>
#include <string.h>

static uint32_t status_from_flags(unsigned flags)
{
    uint32_t status = (flags & 15u) << 13;
    if (flags & 16u) status |= VEHICLE_REVERSE;
    if (flags & 64u) status |= VEHICLE_CHARGING;
    return status;
}
static unsigned flags_from_status(uint32_t status)
{ return ((status >> 13) & 15u) | ((status & VEHICLE_REVERSE) ? 16u : 0u) |
    ((status & VEHICLE_CHARGING) ? 64u : 0u); }

/* Strict decimal syntax: no signs, trailing text, overflow or unbounded sscanf. */
static int number(const char **text, unsigned *value, char delimiter)
{
    const char *p = *text; unsigned n = 0;
    if (*p < '0' || *p > '9') return -1;
    do { if (n > 6553 || (n == 6553 && *p > '5')) return -1;
        n = n * 10 + *p++ - '0'; } while (*p >= '0' && *p <= '9');
    if (*p != delimiter) return -1;
    *text = delimiter ? p + 1 : p; *value = n; return 0;
}
static void execute(struct vehicle_test_control *t)
{
    const char *p = t->line;
    char *reply;
    unsigned id = 0, flags = 0, gear = 255, mode = 255, error = 0;
    enum { GET, SET, CLEAR } action;
    struct vehicle_state state;
    if (t->count == 4) return; /* No side effect without room for its acknowledgment. */
    reply = t->replies[(t->head + t->count++) % 4];
    if (!strncmp(p, "AT+ZXHSET=", 10)) { action = SET; p += 10;
        if (number(&p, &id, ',') || number(&p, &flags, ',')) error = 1;
        else {
            const char *comma = strchr(p, ',');
            if (number(&p, &gear, comma ? ',' : 0) || (comma && number(&p, &mode, 0))) error = 1;
            else if ((flags & ~95u) || (gear > 15 && gear != 255) || (mode > 2 && mode != 255)) error = 2;
        }
    } else if (!strncmp(p, "AT+ZXHGET=", 10)) { action = GET; p += 10; error = number(&p, &id, 0) ? 1 : 0;
    } else if (!strncmp(p, "AT+ZXHCLEAR=", 12)) { action = CLEAR; p += 12; error = number(&p, &id, 0) ? 1 : 0;
    } else { action = GET; error = 1; }
    if (!id) error = 1;
    if (!error && id == t->previous_id) {
        if (!strcmp(t->previous, t->line)) { strcpy(reply, t->previous_reply); return; }
        error = 4; /* Request ID reused for different content. */
    }
    if (!error && action != GET) {
        if (app_vehicle_aux_publish(VEHICLE_AUX_TEST,
            action == SET ? (VEHICLE_AUX_STATUS_MASK & (mode == 255 ? ~(uint32_t)VEHICLE_RIDE_MODE_MASK : ~0u)) : 0u,
            action == SET ? status_from_flags(flags) | (mode < 3 ? VEHICLE_MODE_ECO << mode : 0) : 0,
            action == SET && gear != 255 ? (int)gear : -1,
            action == SET ? VEHICLE_TEST_LEASE_MS : 0)) error = 3;
        else if (action == SET) { ++t->sets; t->last_set_ms = t->last_byte_ms; }
        else ++t->clears;
    }
    if (!error && app_vehicle_try_snapshot(&state)) error = 3;
    if (error) snprintf(reply, VEHICLE_TEST_LINE, "+ZXH:%u,ERROR,%u\r\n", id, error);
    else {
        unsigned known = flags_from_status(state.values.known_status);
        unsigned physical = flags_from_status(state.physical_status);
        if (state.values.valid & VEHICLE_GEAR_VALID) known |= 32;
        if (state.physical_gear) physical |= 32;
        int current_mode = vehicle_values_ride_mode(&state.values);
        if (current_mode >= 0) known |= 128;
        if (current_mode >= 0 && (state.physical_status & VEHICLE_RIDE_MODE_MASK) == VEHICLE_RIDE_MODE_MASK) physical |= 128;
        snprintf(reply, VEHICLE_TEST_LINE, "+ZXH:%u,OK,3,%u,%u,%u,%u,%u\r\n", id,
            flags_from_status(state.values.status),
            (state.values.valid & VEHICLE_GEAR_VALID) ? (unsigned)state.values.gear : 255u, known, physical,
            current_mode >= 0 ? (unsigned)current_mode : 255u);
    }
    /* Failed/not-ready requests may be retried. Successful duplicates don't renew a lease. */
    if (!error) { t->previous_id = id; strcpy(t->previous, t->line); strcpy(t->previous_reply, reply); }
}
void vehicle_test_init(struct vehicle_test_control *t) { memset(t, 0, sizeof(*t)); }
const char *vehicle_test_reply(const struct vehicle_test_control *t)
{ return t->count ? t->replies[t->head] : NULL; }
void vehicle_test_reply_sent(struct vehicle_test_control *t)
{ if (t->count) { t->head = (t->head + 1) % 4; --t->count; } }

void vehicle_test_feed(struct vehicle_test_control *t, struct sl_decoder *d,
    const uint8_t *data, unsigned n, uint32_t now)
{
    unsigned i;
    if (t->ascii && (uint32_t)(now - t->last_byte_ms) > 250u) t->ascii = t->used = t->discard = 0;
    sl_feed(d, NULL, 0, now); /* Expire an abandoned binary prefix before recognizing ASCII. */
    t->last_byte_ms = now;
    for (i = 0; i < n; ++i) {
        uint8_t b = data[i];
        /* A binary frame owns all of its bytes, even a payload containing AT/CR/LF. */
        if (d->used) { sl_feed(d, &b, 1, now); continue; }
        if (b == 2) { t->ascii = t->used = t->discard = 0; sl_feed(d, &b, 1, now); continue; }
        if (!t->ascii) {
            if (b == 'A') { t->ascii = 1; t->used = 0; }
            else { if (b != '\r' && b != '\n') sl_feed(d, &b, 1, now); continue; }
        }
        if (b == '\n') {
            if (!t->discard) { t->line[t->used] = 0; execute(t); }
            t->ascii = t->used = t->discard = 0;
        } else if (b != '\r') {
            if (b < 32 || b > 126 || t->used >= VEHICLE_TEST_LINE - 1) t->discard = 1;
            else if (!t->discard) t->line[t->used++] = b;
        }
    }
}
