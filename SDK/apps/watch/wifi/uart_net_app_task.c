/**
 * @file uart_net_app_task.c
 * @brief UART NETIF LwIP 初始化 + 网络应用层任务
 *
 * 编译开关: TCFG_UART_NET_ENABLE (app_config.h / sdk_config.h)
 *
 * 当 TCFG_EXT_WIFI_ENABLE 也开启时:
 *   - WiFi 负责 CTP / net_event / 应用层服务
 *   - UART NETIF 仅初始化 LwIP 网卡 + DHCP，不重复注册
 */

#include "app_config.h"
#include "event.h"

#if TCFG_UART_NET_ENABLE

#include "system/includes.h"
#include "lwip.h"
#include "os/os_compat.h"
#include "driver/cpu/periph/uart.h"

#if !TCFG_EXT_WIFI_ENABLE
#include "server/ctp_server.h"
#include "http/http_server.h"
#include "dev_desc.h"
#endif

/* HTTP GET baidu.com 验证 UART→4G 链路 */
#define UART_NET_HTTP_TEST

#define LOG_TAG             "[UART_NET]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

struct uart_eth_config {
    u32 baudrate;
    u16 tx_pin;
    u16 rx_pin;
    u8  parity;
    u32 circular_buf_len;
};
extern void uart_ethernetif_set_platform_data(const struct uart_eth_config *cfg);
extern void lwip_uart_set_mac_addr(u8 *mac_addr);

#define UART_NET_APP_TASK_NAME "uart_net_app_task"

static const u8 uart_net_mac_addr[6] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x58
};

/*==============================================================================
 * UART NETIF LwIP 初始化 (参照 WL82 lte_app_task.c uart_net_init)
 *============================================================================*/

static int uart_net_init(void)
{
    struct uart_eth_config cfg = {
        .baudrate         = TCFG_UART_NETIF_BAUD,
        .tx_pin           = TCFG_UART_NETIF_TX_PIN,
        .rx_pin           = TCFG_UART_NETIF_RX_PIN,    /* 接线后使能 RX */
        .parity           = UART_PARITY_DISABLE,
        .circular_buf_len = 64 * 1024,
    };

    uart_ethernetif_set_platform_data(&cfg);
    lwip_uart_set_mac_addr((u8 *)uart_net_mac_addr);

    Init_LwIP(UART_NETIF);
    lwip_netif_set_up(UART_NETIF);
    lwip_renew(UART_NETIF, 1);                  /* DHCP 客户端 (STA 模式) */

    log_info("init done, waiting DHCP...");
    return 0;
}

/*==============================================================================
 * DHCP 事件回调 — IP 就绪后启动应用层服务
 *============================================================================*/

static volatile u8 g_uart_net_ready = 0;

int uart_lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    switch (event) {
    case LWIP_UART_DHCP_BOUND_SUCC: {
        char ip[16];
        Get_IPAddress(UART_NETIF, ip);
        log_info("DHCP bound: %s", ip);

        lwip_set_default_netif(UART_NETIF);
        g_uart_net_ready = 1;
        break;
    }
    case LWIP_UART_DHCP_BOUND_TIMEOUT:
        log_info("DHCP timeout, retrying...");
        break;
    default:
        break;
    }
    return 0;
}

#if !TCFG_EXT_WIFI_ENABLE
/*==============================================================================
 * CTP 事件处理 (仅 WiFi 未启用时，避免与 wifi_app_task.c 重复定义)
 *============================================================================*/

int ctp_cmd_analysis(const char *topic, char *content, void *priv);

static int net_event_handler(void *e)
{
    struct net_event *event = (struct net_event *)e;
    struct ctp_arg *event_arg = (struct ctp_arg *)event->arg;

    switch (event->event) {
    case NET_EVENT_CMD:
        log_info("IN NET_EVENT_CMD");
        ctp_cmd_analysis(event_arg->topic, event_arg->content, event_arg->cli);

        if (event_arg->content) {
            free(event_arg->content);
        }
        event_arg->content = NULL;
        if (event_arg) {
            free(event_arg);
        }
        return true;

    case NET_EVENT_DATA:
        break;
    }
    return false;
}

void wifi_app_event_handler(struct sys_event *event)
{
    int type = event->type;
    switch (type) {
    case SYS_NET_EVENT:
        net_event_handler((void *)event->payload);
        break;
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}
#endif /* !TCFG_EXT_WIFI_ENABLE */

#ifdef UART_NET_HTTP_TEST
/*==============================================================================
 * HTTP GET 验证 (UART → WL82 → 4G → baidu.com)
 *============================================================================*/
#include "http/http_cli.h"
#include <stdlib.h>

static void uart_net_http_test(void)
{
    log_info("starting HTTP GET http://www.baidu.com...");

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        log_error("calloc ctx failed");
        return;
    }

    http_body_obj http_body = {
        .buf_len = 4 * 1024, .buf_count = 1,
        .p = (char *)malloc(4 * 1024),
    };
    if (!http_body.p) {
        log_error("malloc recv buf failed");
        free(ctx);
        return;
    }

    ctx->url             = "http://www.baidu.com";
    ctx->timeout_millsec = 30000;
    ctx->priv            = &http_body;
    ctx->connection      = "close";

    int ret = httpcli_get(ctx);
    if (ret != HERROR_OK) {
        log_error("httpcli_get failed, err=%d", ret);
    } else {
        log_info("success! received %u bytes", http_body.recv_len);
        int show = http_body.recv_len > 256 ? 256 : http_body.recv_len;
        log_info("\n==== response (first %d bytes) ====\n%.*s\n==== end ====\n",
                 show, show, http_body.p);
    }

    httpcli_close(ctx);
    if (http_body.p) {
        free(http_body.p);
    }
    free(ctx);
    log_info("test done");
}
#endif /* UART_NET_HTTP_TEST */

/*==============================================================================
 * 应用层服务任务 — 等 UART NETIF 就绪后启动
 *
 * 当 WiFi 也启用时: 只做 HTTP 验证测试 (调试用), 不重复启动 iperf/ctp/http_server
 * 当 WiFi 未启用时: 启动全部应用层服务
 *============================================================================*/

static void uart_net_app_task(void *priv)
{
    while (!g_uart_net_ready) {
        os_time_dly(50);
    }
    os_time_dly(200);

#if !TCFG_EXT_WIFI_ENABLE
    log_info("starting app services...");

    void iperf_test(void);
    iperf_test();

    int ctp_init(u8 use_alive);
    ctp_init(1);

    extern int http_virfile_reg(const char *path, const char *contents, unsigned long len);
    http_virfile_reg(DEV_DESC_PATH, DEV_DESC_CONTENT, strlen(DEV_DESC_CONTENT));

    extern int http_get_server_init(unsigned short port);
    http_get_server_init(8080);

#ifdef CONFIG_ENABLE_VLIST
    int preview_init(u16 port, int callback(void *priv, u8 * data, size_t len));
    int playback_init(u16 port, int callback(void *priv, u8 * data, size_t len));
    preview_init(VIDEO_PREVIEW_PORT, NULL);
    playback_init(VIDEO_PLAYBACK_PORT, NULL);
#endif

    log_info("services started");
#else
    log_info("UART NETIF ready (services handled by WiFi)");
#endif

#ifdef UART_NET_HTTP_TEST
    uart_net_http_test();
#endif

    int msg[32];
    while (1) {
        int res = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (res != OS_TASKQ) {
            continue;
        }
    }
}

/*==============================================================================
 * 初始化入口 — late_initcall 自动执行
 *============================================================================*/

static int uart_net_app_init(void)
{
    log_info("init UART NETIF");

    uart_net_init();
    return os_task_create(uart_net_app_task, NULL, 10, 1024, 64, UART_NET_APP_TASK_NAME);
}
late_initcall(uart_net_app_init);

#endif /* TCFG_UART_NET_ENABLE */
