/* app_core-only transport. Hardware pins and protocol selection live in board cfg. */
#ifndef SHENLONG_RUNTIME_TEST
#include "app_config.h"
#endif
#include "shenlong_protocol.h"
#include "vehicle_test_control.h"
#if TCFG_SHENLONG_UART_ENABLE
#ifndef SHENLONG_RUNTIME_TEST
#include "system/includes.h"
#include "uart.h"
#include "app_meter_control.h"
#include "app_vehicle.h"
#endif
#include "vehicle_storage.h"
#ifndef SHENLONG_RUNTIME_TEST
#include "user_cfg_id.h"
#include "device/vm.h"
#endif
#include <string.h>

static struct sl_decoder decoder;
static struct vehicle_test_control test_control;
static struct vehicle_storage storage;
static int uart_id = -1;
static u16 poll_timer;
static u8 loaded;
static u32 tx_at, log_at, tx_count, tx_errors;
static int tx_state, snapshot_result;
static u32 snapshot_skips;
static u8 *rx_dma, *tx_dma;

static int odo_read(unsigned slot, void *buffer, unsigned n)
{ return syscfg_read(slot ? CFG_YYB_ODOMETER_BACKUP : CFG_YYB_ODOMETER, buffer, n); }
static int odo_write(unsigned slot, const void *buffer, unsigned n)
{ return syscfg_write(slot ? CFG_YYB_ODOMETER_BACKUP : CFG_YYB_ODOMETER, buffer, n); }
static int odo_flush(void)
{
    if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
        int result = vm_flush2flash(0);
        if (result != VM_ERR_NONE && result != VM_RAM_FLUSH_NONE) return -1;
    }
    return 0;
}
static const struct vehicle_storage_io storage_io = {odo_read, odo_write, odo_flush};

static void transport_poll(void *priv)
{
    u8 data[64];
    int n;
    u32 now = sys_timer_get_ms();
    struct app_meter_state meter;
    (void)priv;
    if (uart_id < 0) return;
    sl_poll(&decoder, now);
    n = uart_recv_bytes(uart_id, data, sizeof(data));
    if (n > 0) vehicle_test_feed(&test_control, &decoder, data, n, now);
    if ((u32)(now - tx_at) >= 120 &&
        (tx_state = uart_is_send_complete(uart_id)) > 0) {
        /* Linked BR33 uart_v3 returns 2 before the first transfer, 0 while
         * busy, and 1 after TX completes. The public header only documents
         * 0/1; accepting only 1 deadlocks the very first command forever.
         * Positive states are idle; never treat a negative driver error as idle. */
        struct sl_command command = {
            .drive = 2, .assist = 10, .control = 0x80,
            .magnets = 46, .diameter_tenth_inch = TCFG_SHENLONG_DIAMETER_TENTH_INCH,
            .sensitivity = 12, .start_strength = 1, .speed_limit = 60,
            .current_limit = 20, .undervoltage_tenth_v = 420, .control2 = 8
        };
        snapshot_result = app_meter_control_snapshot(&meter);
        if (snapshot_result == ZXH_OK) {
            struct vehicle_state vehicle;
            if (!app_vehicle_try_snapshot(&vehicle)) {
                int mode = vehicle_values_ride_mode(&vehicle.values);
                if (mode >= 0) meter.mode = mode;
            }
            command.control = (meter.power_on ? 0x80 : 0) | (meter.light_on ? 0x20 : 0);
            command.assist = meter.mode == 0 ? 5 : meter.mode == 2 ? 15 : 10;
            if (!decoder.input.online) command.control |= 0x10;
            /* Only send a known current instrument state. */
            n = sl_encode_command(&command, tx_dma, 24);
            if (uart_send_bytes(uart_id, tx_dma, n) == n) tx_count++;
            else tx_errors++;
        } else snapshot_skips++;
        tx_at = now;
    } else if ((tx_state = uart_is_send_complete(uart_id)) > 0) {
        const char *reply = vehicle_test_reply(&test_control);
        if (reply) {
            n = strlen(reply); memcpy(tx_dma, reply, n);
            if (uart_send_bytes(uart_id, tx_dma, n) == n) vehicle_test_reply_sent(&test_control);
            else tx_errors++;
        }
    }
    if (vehicle_storage_poll(&storage, &storage_io, decoder.input.values.odometer_mm,
            decoder.input.online && decoder.input.values.speed_dkph > 0, now) < 0)
        printf("[SL] odometer save failed\n");
    if ((u32)(now - log_at) >= 2000) {
        printf("[SL] uart=%d online=%u rx=%u err=%u tx=%u txerr=%u period=%u speed=%d current_ma=%d soc=%d txstate=%d snapshot=%d skip=%u atset=%u atclear=%u atage=%u\n",
            uart_id, decoder.input.online, decoder.good, decoder.errors, tx_count, tx_errors,
            decoder.wheel_period_ms, decoder.input.values.speed_dkph,
            decoder.input.values.current_ma, decoder.input.values.soc,
            tx_state, snapshot_result, snapshot_skips, test_control.sets, test_control.clears,
            test_control.sets ? (u32)(now - test_control.last_set_ms) : 0);
        log_at = now;
    }
}

int shenlong_uart_start(void)
{
    const struct uart_config config = {
        .baud_rate = 9600, .tx_pin = TCFG_SHENLONG_TX_IO, .rx_pin = TCFG_SHENLONG_RX_IO,
        /* BR33 UART: MARK parity appends a constant high bit after 8 data bits.
         * 8M1 is wire-equivalent to 8N2. Do not use this on UART v1 (parity ignored).
         * No undocumented register writes. Verify waveform before vehicle use. */
        .parity = UART_PARITY_all_1, .tx_wait_mutex = 1
    };
    struct uart_dma_config dma = {
        .rx_timeout_thresh = 15000, .frame_size = 256,
        .rx_cbuffer_size = 256,
        .irq_priority = 3, .event_mask = UART_EVENT_RX_TIMEOUT | UART_EVENT_RX_DATA,
    };
    uint64_t odo, trip;
    if (uart_id >= 0) return 0;
    if (!loaded) {
        sl_decoder_init(&decoder, TCFG_SHENLONG_DIAMETER_TENTH_INCH);
        vehicle_storage_load(&storage, &storage_io, sys_timer_get_ms());
        decoder.input.values.odometer_mm = storage.odometer_mm;
        loaded = 1;
    }
    odo = decoder.input.values.odometer_mm; trip = decoder.input.values.trip_mm;
    sl_decoder_init(&decoder, TCFG_SHENLONG_DIAMETER_TENTH_INCH);
    decoder.input.values.odometer_mm = odo; decoder.input.values.trip_mm = trip;
    vehicle_test_init(&test_control);
    rx_dma = dma_malloc(256); tx_dma = dma_malloc(VEHICLE_TEST_LINE);
    if (!rx_dma || !tx_dma) goto fail;
    dma.rx_cbuffer = rx_dma;
    uart_id = uart_init(TCFG_SHENLONG_UART_ID, &config);
    if (uart_id < 0) { printf("[SL] UART unavailable\n"); goto fail; }
    if (uart_dma_init(uart_id, &dma) < 0) goto fail;
    tx_at = log_at = sys_timer_get_ms();
    tx_state = 2; snapshot_result = -1; snapshot_skips = 0;
    poll_timer = sys_timer_add(NULL, transport_poll, 10);
    if (!poll_timer) goto fail;
    printf("[SL] 9600 8N2 (8M1 wire) uart=%d TX=PC04 RX=PC05 diag=6 test=AT_ZXH3 mode\n", uart_id);
    return 0;
fail:
    if (uart_id >= 0) uart_deinit(uart_id);
    uart_id = -1;
    if (rx_dma) { dma_free(rx_dma); rx_dma = NULL; }
    if (tx_dma) { dma_free(tx_dma); tx_dma = NULL; }
    return -1;
}

int shenlong_uart_shutdown(void)
{
    if (poll_timer) { sys_timer_del(poll_timer); poll_timer = 0; }
    sl_poll(&decoder, sys_timer_get_ms());
    if (uart_id >= 0) { uart_deinit(uart_id); uart_id = -1; }
    if (rx_dma) { dma_free(rx_dma); rx_dma = NULL; }
    if (tx_dma) { dma_free(tx_dma); tx_dma = NULL; }
    decoder.input.online = 0; decoder.used = 0;
    app_vehicle_aux_publish(VEHICLE_AUX_TEST, 0, 0, -1, 0);
    return loaded && vehicle_storage_save(&storage, &storage_io,
        decoder.input.values.odometer_mm, sys_timer_get_ms()) < 0 ? -1 : 0;
}
int shenlong_uart_reset_trip(void)
{
    sl_poll(&decoder, sys_timer_get_ms());
    decoder.input.values.trip_mm = 0; decoder.distance_remainder = 0;
    return 0;
}
void shenlong_uart_read(struct vehicle_input *out)
{
    sl_poll(&decoder, sys_timer_get_ms());
    *out = decoder.input;
    if (uart_id < 0) out->online = 0;
}
#ifndef SHENLONG_RUNTIME_TEST
static int shenlong_boot(void) { return shenlong_uart_start(); }
late_initcall(shenlong_boot);
#endif
#endif
