#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".udisk_block_test.data.bss")
#pragma data_seg(".udisk_block_test.data")
#pragma const_seg(".udisk_block_test.text.const")
#pragma code_seg(".udisk_block_test.text")
#endif
#if 0
#include "cpu/includes.h"
#include "system/timer.h"
#include "system/event.h"
#include "device/ioctl_cmds.h"
#include "device_drive.h"
#include "app_config.h"
#include "app_msg.h"

#include "usb_config.h"
#include "usb/host/usb_host.h"
//#include "usb_ctrl_transfer.h"
//#include "usb_bulk_transfer.h"
#include "usb_storage.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define WR_TEST_ROUNDS  2
#define WR_TEST_STEP    2
#define WR_LOG_MB       16
#define WR_SEED_BASE    0x5A5A0000u
#define WR_SEED_STEP    0x13572467u

static u32 wr_dbg_prints;

static void wr_fill_buffer(u32 *buf, u32 lba, u32 num_sectors, u32 seed)
{
    u32 i, k;
    for (i = 0; i < num_sectors; i++) {
        u32 base = lba + i + seed;
        for (k = 0; k < 128; k++) {
            buf[i * 128 + k] = base + k;
        }
    }
}


static u32 verify_pattern(const void *buf, u32 lba, u32 num_sectors, u32 seed)
{
    u32 i, k;
    u32 err_sectors = 0;

    for (i = 0; i < num_sectors; i++) {
        const u32 *sec = (const u32 *)((const u8 *)buf + i * 512);
        u32 base = lba + i + seed;
        u32 bad = 0, first_bad = 0;
        int delta0 = 0, constant = 1;

        for (k = 0; k < 128; k++) {
            if (sec[k] != base + k) {
                if (bad == 0) {
                    first_bad = k;
                    delta0 = (int)sec[k] - (int)(base + k);
                } else if ((int)sec[k] - (int)(base + k) != delta0) {
                    constant = 0;
                }
                bad++;
            }
        }
        if (bad == 0) {
            continue;
        }
        err_sectors++;

        if (wr_dbg_prints >= 16) {
            continue;
        }
        wr_dbg_prints++;

        u32 uniform = 1;
        for (k = 1; k < 128; k++) {
            if (sec[k] != sec[0]) {
                uniform = 0;
                break;
            }
        }


        r_f_printf("MISMATCH lba %d(+%d): random corruption, bad %d/128, word[%d] exp %08x got %08x\n",
                   (int)lba, (int)i, (int)bad, (int)first_bad,
                   base + first_bad, sec[first_bad]);
        r_f_printf("  head:");
        for (k = 0; k < 8; k++) {
            r_f_printf(" %08x", sec[k]);
        }
        r_f_printf("\n");

    }
    return err_sectors;
}

static void handle_udisk_wr_test(int *msg)
{
    struct device *device = dev_open("udisk0", NULL);
    if (!device) {
        r_f_printf("udisk0 open failed\n");
        return;
    }

    const u32 max_lba = 247808;


    u32 buf_sectors = WR_TEST_STEP;
    if (buf_sectors > max_lba) {
        buf_sectors = max_lba;
    }
    u32 *p = malloc(buf_sectors * 512);
    if (!p) {
        r_f_printf("wr test malloc failed\n");
        dev_close(device);
        return;
    }

    u32 total_err = 0;
    u32 r;

    for (r = 0; r < WR_TEST_ROUNDS; r++) {
        u32 seed = WR_SEED_BASE + r * WR_SEED_STEP;
        u32 log_gran = WR_LOG_MB * 2048;     // ??????????
        u32 err = 0;
        u32 lba = 0;

        wr_dbg_prints = 0;

        while (lba < max_lba) {
            u32 n = (max_lba - lba < buf_sectors) ? (max_lba - lba) : buf_sectors;


            wr_fill_buffer(p, lba, n, seed);
            if (dev_bulk_write(device, p, lba, n) != n) {
                r_f_printf("write error at lba %d, size %d sectors\n", lba, n);
                total_err++;
                goto out;
            }

            memset(p, 0, n * 512);

            if (dev_bulk_read(device, p, lba, n) != n) {
                r_f_printf("read error at lba %d, size %d sectors\n", lba, n);
                total_err++;
                break;
            }


            err += verify_pattern(p, lba, n, seed);

            lba += n;
            if ((lba % log_gran) < n) {
                log_info("wr test round %d: %d/%d MB (w+r), err %d\n",
                         r + 1, lba / 2048, max_lba / 2048, err);
            }
        }

        total_err += err;
        if (err == 0) {
            log_info("wr test round %d: PASS (%d MB, seed %08x)\n",
                     r + 1, max_lba / 2048, seed);
        } else {
            r_f_printf("wr test round %d: FAIL, %d bad sectors (seed %08x)\n",
                       r + 1, err, seed);
        }
    }

    if (total_err == 0) {
        log_info("wr test ALL PASS: %d rounds, %d MB per round\n",
                 WR_TEST_ROUNDS, max_lba / 2048);
    } else {
        r_f_printf("wr test FAILED: total %d bad sectors\n", total_err);
    }

out:
    free(p);
    dev_close(device);
}

#endif
