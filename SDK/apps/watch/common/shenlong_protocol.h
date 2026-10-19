#ifndef SHENLONG_PROTOCOL_H
#define SHENLONG_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include "vehicle_core.h"

/* 20180915 protocol: big endian, full frame XOR=0, 9600 baud 8N2. */
#define SL_MAX_FRAME 25
#define SL_TIMEOUT_MS 1000
struct sl_command {
    uint8_t drive, assist, control, magnets;
    uint16_t diameter_tenth_inch;
    uint8_t sensitivity, start_strength, inner_magnets, speed_limit, current_limit;
    uint16_t undervoltage_tenth_v, throttle_pwm;
    uint8_t control2, assist_pwm, extended;
};
struct sl_decoder {
    struct vehicle_input input;
    uint8_t buffer[SL_MAX_FRAME], used;
    uint8_t raw[SL_MAX_FRAME], raw_length; /* Preserve original level and reserved fields. */
    uint16_t wheel_period_ms, remaining_km;
    uint32_t good, errors, last_byte_ms, distance_at_ms;
    uint16_t diameter_tenth_inch;
    uint8_t distance_clock, distance_remainder;
};
uint8_t sl_xor(const uint8_t *data, size_t len);
size_t sl_encode_command(const struct sl_command *command, uint8_t *out, size_t capacity);
void sl_decoder_init(struct sl_decoder *decoder, uint16_t diameter_tenth_inch);
int sl_parse_feedback(struct sl_decoder *decoder, const uint8_t *frame, size_t length, uint32_t now_ms);
void sl_feed(struct sl_decoder *decoder, const uint8_t *data, size_t length, uint32_t now_ms);
void sl_poll(struct sl_decoder *decoder, uint32_t now_ms);

/* Firmware entry points are serialized by app_core; no UI calls in driver. */
int shenlong_uart_start(void);
int shenlong_uart_shutdown(void);
int shenlong_uart_reset_trip(void);
void shenlong_uart_read(struct vehicle_input *input);
#endif
