/** Test command channel alongside OEM telemetry; ASCII commands never enter the OEM decoder. */
#ifndef VEHICLE_TEST_CONTROL_H
#define VEHICLE_TEST_CONTROL_H
#include "shenlong_protocol.h"
#define VEHICLE_TEST_LINE 96
#define VEHICLE_TEST_LEASE_MS 1500u
struct vehicle_test_control {
    char line[VEHICLE_TEST_LINE], replies[4][VEHICLE_TEST_LINE];
    char previous[VEHICLE_TEST_LINE], previous_reply[VEHICLE_TEST_LINE];
    unsigned used, ascii, discard, head, count, previous_id;
    uint32_t last_byte_ms;
    uint32_t sets, clears, last_set_ms;
};
void vehicle_test_init(struct vehicle_test_control *test);
void vehicle_test_feed(struct vehicle_test_control *test, struct sl_decoder *decoder,
    const uint8_t *data, unsigned n, uint32_t now_ms);
/** Peek until DMA accepts the reply, then consume. No TX buffer mutation while busy. */
const char *vehicle_test_reply(const struct vehicle_test_control *test);
void vehicle_test_reply_sent(struct vehicle_test_control *test);
#endif
