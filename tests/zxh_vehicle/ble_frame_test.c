#include <assert.h>
#include <stdio.h>
#include "../../SDK/apps/watch/ble/zxh_ble_frame.h"
static unsigned int checks;
static void check(int value) { checks++; assert(value); }
int main(void)
{
    struct zxh_ble_decoder d = {0}; char body[1025]; uint8_t packet[20];
    unsigned int length, offset, index, n; int ret;
    memset(body, 'A', sizeof(body));
    for (length = 1; length <= 1024; ++length) {
        memset(&d, 0, sizeof(d)); ret = 0;
        for (offset = index = 0; offset < length; offset += 14, ++index) {
            n = zxh_ble_frame_make(body, length, offset, 255, index, packet);
            check(n <= 20);
            ret = zxh_ble_frame_feed(&d, packet, n, 100 + index);
            check(ret == (offset + 14 >= length ? 1 : 0));
        }
        check(ret == 1 && d.total == length && !memcmp(d.data, body, length));
    }
    n = zxh_ble_frame_make("ZXH-v1", 6, 0, 0xab, 0, packet);
    { const uint8_t golden[] = {0x5a,0x58,0xab,0,6,0,'Z','X','H','-','v','1'};
      check(n == sizeof(golden) && !memcmp(packet, golden, n)); }
    check(zxh_ble_frame_feed(&d, packet, 6, 0) == -1);
    packet[0] = 0; check(zxh_ble_frame_feed(&d, packet, n, 0) == -1); packet[0] = 0x5a;
    packet[4] = packet[5] = 0; check(zxh_ble_frame_feed(&d, packet, n, 0) == -1);
    packet[4] = 1; packet[5] = 4; check(zxh_ble_frame_feed(&d, packet, n, 0) == -1);
    n = zxh_ble_frame_make(body, 30, 0, 1, 0, packet); check(zxh_ble_frame_feed(&d, packet, n, 0) == 0);
    n = zxh_ble_frame_make(body, 30, 28, 1, 2, packet); check(zxh_ble_frame_feed(&d, packet, n, 1) == -1);
    n = zxh_ble_frame_make(body, 20, 0, 1, 0, packet); check(zxh_ble_frame_feed(&d, packet, n, 0) == 0);
    n = zxh_ble_frame_make(body, 20, 14, 2, 1, packet); check(zxh_ble_frame_feed(&d, packet, n, 1) == -1);
    n = zxh_ble_frame_make(body, 20, 0, 1, 0, packet); check(zxh_ble_frame_feed(&d, packet, n, 0xfffffff0u) == 0);
    n = zxh_ble_frame_make(body, 20, 14, 1, 1, packet); check(zxh_ble_frame_feed(&d, packet, n, 0xfffffff0u + 5001u) == -1);
    printf("PASS: %u C BLE frame assertions\n", checks); return 0;
}
