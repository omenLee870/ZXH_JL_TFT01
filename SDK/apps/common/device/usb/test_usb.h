#ifndef _TEST_USB_H_
#define _TEST_USB_H_

#include "system/includes.h"
#include "app_config.h"

/* ========== Total enable ========== */
#ifndef TCFG_USB_TEST_ENABLE
#define TCFG_USB_TEST_ENABLE    0
#endif

#if TCFG_USB_TEST_ENABLE

/* ========== Transfer types (USB spec bmAttributes values) ========== */
#define TEST_XFER_BULK      2
#define TEST_XFER_ISOC      1
#define TEST_XFER_INT       3

/* ========== Frame format: [seq:4][data_len:4][data:data_len] ========== */
#define FRAME_HDR_SIZE      8

/* ========== Max channels ========== */
#define TEST_CHANNEL_MAX    7

/* ========== Per-channel config ==========
 * Each channel = one bidirectional EP, all run simultaneously
 * Override TEST_CHANNEL_CONFIG in board config for different chips
 *
 * Default: EP1=BULK, EP2=INTR, EP3=ISO, EP4=ISO, EP5/6=off
 */
struct test_ch_cfg {
    u8 ep;
    u8 type;
    u16 maxp;
    u8 enable;
};

#ifndef TEST_CHANNEL_CONFIG
#define TEST_CHANNEL_CONFIG                        \
    { 1, TEST_XFER_INT, 64,   1 },               \
    { 2, TEST_XFER_BULK,  64,   1 },               \
    { 3, TEST_XFER_ISOC, 192, 1 },               \
    { 4, TEST_XFER_ISOC, 96, 1 },               \
    { 5, TEST_XFER_INT,  16,   1 },               \
    { 6, TEST_XFER_BULK,  64,   1 }
#endif

/* ========== CTRL via EP0 ========== */
#ifndef TEST_EP_CTRL_ENABLE
#define TEST_EP_CTRL_ENABLE  1
#endif
#ifndef TEST_CTRL_MAXP
#define TEST_CTRL_MAXP       256
#endif

/* ========== Interface protocol marker ========== */
#define TEST_INTERFACE_PROTOCOL 0x5E

/* ========== Vendor request commands ========== */
#define TEST_VENDOR_REQ_SEND    0x01   /* Host -> Device */
#define TEST_VENDOR_REQ_RECV    0x02   /* Device -> Host */
#define TEST_VENDOR_REQ_GET_STAT 0x03  /* Device -> Host */
#define TEST_VENDOR_REQ_RESET   0x04   /* Host -> Device */
#define TEST_VENDOR_REQ_START   0x05   /* Host -> Device: start transfer */

/* ========== Statistics per endpoint ========== */
struct test_ep_stats {
    u32 tx_bytes;
    u32 tx_packets;
    u32 rx_bytes;
    u32 rx_packets;
    u32 rx_errors;
    u32 rx_lost;
    u32 last_seq;
};

#endif /* TCFG_USB_TEST_ENABLE */
#endif /* _TEST_USB_H_ */
