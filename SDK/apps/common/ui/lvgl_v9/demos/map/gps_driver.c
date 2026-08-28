#include "gps_driver.h"
#include "app_config.h"
#include "system/includes.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define GPS_UART_BAUD       9600
#define GPS_TX_PIN          IO_PORTC_02
#define GPS_RX_PIN          IO_PORTC_01
#define GPS_DMA_BUF_SIZE    1024

static int gps_uart = -1;
static gps_data_t gps_info;
static char gps_line_buf[256];
static u16 gps_line_len;
static bool gps_line_overflow;

/* DMA receive — both buffers must be dma_malloc */
static u8 *gps_dma_buf;
static u8 *gps_rx_buf;
static volatile bool gps_data_ready;
static volatile u16 gps_rx_len;

/* debug */
static volatile int gps_poll_cnt;
static volatile int gps_byte_cnt;

/* --- NMEA parsing --- */

static s32 gps_convert_coord(double value)
{
    int d = (int)(value / 100);
    double m = value - d * 100;
    return (s32)((d + m / 60.0) * 10000000);
}

static void gps_parse_rmc(char *line)
{
    char *p = line;
    double lat, lon;
    char ns, ew;
    int comma = 0;

    while (*p) {
        if (*p == ',') {
            comma++;
        }
        if (comma == 2) {
            break;
        }
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    if (*p != 'A') {
        gps_info.valid = 0;
        return;
    }

    while (*p && *p != ',') {
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    lat = strtod(p, NULL);
    gps_info.latitude_e7 = gps_convert_coord(lat);

    while (*p && *p != ',') {
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    ns = *p;
    if (ns == 'S') {
        gps_info.latitude_e7 = -gps_info.latitude_e7;
    } else if (ns != 'N') {
        gps_info.valid = 0;
        return;
    }

    while (*p && *p != ',') {
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    lon = strtod(p, NULL);
    gps_info.longitude_e7 = gps_convert_coord(lon);

    while (*p && *p != ',') {
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    ew = *p;
    if (ew == 'W') {
        gps_info.longitude_e7 = -gps_info.longitude_e7;
    } else if (ew != 'E') {
        gps_info.valid = 0;
        return;
    }

    gps_info.valid = 1;
}

static void gps_parse_gga(char *line)
{
    char *p = line;
    int comma = 0;

    while (*p) {
        if (*p == ',') {
            comma++;
        }
        if (comma == 6) {
            break;
        }
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    gps_info.fix_mode = (*p >= '1' && *p <= '6') ? *p - '0' : 0;

    while (*p && *p != ',') {
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    gps_info.satellites_used = atoi(p);
}

static void gps_parse_txt(char *line)
{
    /* GPTXT format: $GPTXT,total,num,severity,text*cs  → skip 4 commas */
    char *p = line;
    int comma = 0;
    while (*p) {
        if (*p == ',') {
            comma++;
        }
        if (comma == 4) {
            break;
        }
        p++;
    }
    if (!*p) {
        return;
    }
    p++;
    if (strncmp(p, "ANTENNA OPEN", 12) == 0) {
        gps_info.antenna_status = GPS_ANT_OPEN;
    } else if (strncmp(p, "ANTENNA SHORT", 13) == 0) {
        gps_info.antenna_status = GPS_ANT_SHORT;
    }
}

static void gps_parse_line(char *line)
{
    if (!strncmp(line, "$GNRMC", 6) || !strncmp(line, "$GPRMC", 6)) {
        gps_parse_rmc(line);
    } else if (!strncmp(line, "$GNGGA", 6) || !strncmp(line, "$GPGGA", 6)) {
        gps_parse_gga(line);
    } else if (!strncmp(line, "$GPTXT", 6)) {
        gps_parse_txt(line);
    }
}

/* --- UART DMA ISR --- */

static void gps_uart_isr_cb(uart_dev uart_num, enum uart_event event)
{
    if (event == UART_EVENT_RX_TIMEOUT) {
        s32 len = uart_recv_bytes(uart_num, gps_rx_buf, GPS_DMA_BUF_SIZE - 1);
        if (len > 0) {
            gps_rx_buf[len] = 0;
            gps_rx_len = len;
            gps_data_ready = true;
        }
    }
}

/* --- Public API --- */

void gps_init(void)
{
    memset(&gps_info, 0, sizeof(gps_info));
    gps_info.antenna_status = GPS_ANT_UNKNOWN;
    gps_line_len = 0;
    gps_line_overflow = false;

    gps_dma_buf = dma_malloc(GPS_DMA_BUF_SIZE);
    gps_rx_buf = dma_malloc(GPS_DMA_BUF_SIZE);
    if (!gps_dma_buf || !gps_rx_buf) {
        printf("[GPS] dma_malloc fail\n");
        goto gps_init_err;
    }

    struct uart_config config = {
        .baud_rate = GPS_UART_BAUD,
        .tx_pin = GPS_TX_PIN,
        .rx_pin = GPS_RX_PIN,
        .parity = UART_PARITY_DISABLE,
        .tx_wait_mutex = 0,
    };

    gps_uart = uart_init(-1, &config);
    if (gps_uart < 0) {
        printf("[GPS] uart init fail\n");
        goto gps_init_err;
    }

    /* DMA init: ISR fires on RX_TIMEOUT (gap in data), reads all available bytes */
    u32 uart_timeout = 25 * 1000000 / GPS_UART_BAUD;  // ~2604us, > 2 bytes @ 9600

    struct uart_dma_config dma = {0};
    dma.rx_timeout_thresh = uart_timeout;
    dma.frame_size = GPS_DMA_BUF_SIZE;
    dma.event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT;
    dma.irq_priority = 2;
    dma.irq_callback = gps_uart_isr_cb;
    dma.rx_cbuffer = gps_dma_buf;
    dma.rx_cbuffer_size = GPS_DMA_BUF_SIZE;

    int ret = uart_dma_init(gps_uart, &dma);
    if (ret < 0) {
        printf("[GPS] dma init fail\n");
        uart_deinit(gps_uart);
        gps_uart = -1;
        goto gps_init_err;
    }

    printf("[GPS] init ok, dev=%d\n", gps_uart);
    return;

gps_init_err:
    if (gps_rx_buf) {
        free(gps_rx_buf);
        gps_rx_buf = NULL;
    }
    if (gps_dma_buf) {
        free(gps_dma_buf);
        gps_dma_buf = NULL;
    }
}

void gps_deinit(void)
{
    if (gps_uart >= 0) {
        uart_deinit(gps_uart);
        gps_uart = -1;
    }
    if (gps_rx_buf) {
        free(gps_rx_buf);
        gps_rx_buf = NULL;
    }
    if (gps_dma_buf) {
        free(gps_dma_buf);
        gps_dma_buf = NULL;
    }
}

void gps_poll(void)
{
    if (gps_uart < 0) {
        return;
    }
    if (!gps_data_ready) {
        return;
    }

    gps_data_ready = false;
    gps_byte_cnt += gps_rx_len;
    gps_poll_cnt++;

    for (u16 i = 0; i < gps_rx_len; i++) {
        char c = gps_rx_buf[i];

        if (c == '\n') {
            if (!gps_line_overflow && gps_line_len > 6 && gps_line_buf[0] == '$') {
                gps_line_buf[gps_line_len] = 0;
                gps_parse_line(gps_line_buf);
            }
            gps_line_len = 0;
            gps_line_overflow = false;
        } else if (c != '\r') {
            if (!gps_line_overflow) {
                if (gps_line_len < sizeof(gps_line_buf) - 1) {
                    gps_line_buf[gps_line_len++] = c;
                } else {
                    gps_line_overflow = true;
                }
            }
        }
    }
}

void gps_get_info(gps_data_t *out)
{
    if (out) {
        memcpy(out, &gps_info, sizeof(gps_data_t));
    }
}

const char *gps_antenna_str(int status)
{
    switch (status) {
    case GPS_ANT_OK:
        return "OK";
    case GPS_ANT_OPEN:
        return "OPEN";
    case GPS_ANT_SHORT:
        return "SHORT";
    default:
        return "?";
    }
}

void gps_get_debug_counters(int *poll_cnt, int *byte_cnt)
{
    if (poll_cnt) {
        *poll_cnt = gps_poll_cnt;
    }
    if (byte_cnt) {
        *byte_cnt = gps_byte_cnt;
    }
}
