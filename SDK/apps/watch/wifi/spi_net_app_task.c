/**
 * @file spi_net_app_task.c
 * @brief SPI NETIF LwIP 初始化 + 网络应用层任务
 *
 * 编译开关: TCFG_SPI_NET_ENABLE (sdk_config_dashboard.h)
 *
 * 依赖: JLDTP Manager 必须在 board_init() 中完成 SPI 传输层初始化
 *
 * 当 TCFG_EXT_WIFI_ENABLE 也开启时:
 *   - WiFi 负责 CTP / net_event / 应用层服务
 *   - SPI NETIF 仅初始化 LwIP 网卡 + DHCP，不重复注册
 *
 * BR33 作为 JLDTP SLAVE + DHCP Client:
 *   - WL82 端 SPI_NETIF 运行 DHCP Server (192.168.6.1/24)
 *   - BR33 端 SPI_NETIF 运行 DHCP Client, 从 WL82 获取 IP
 */

#include "app_config.h"
#include "event.h"

#if TCFG_SPI_NET_ENABLE

#include "system/includes.h"
#include "lwip.h"
#include "os/os_compat.h"
#include "jldtp/jldtp_manager.h"

#if !TCFG_EXT_WIFI_ENABLE
#include "server/ctp_server.h"
#include "http/http_server.h"
#include "dev_desc.h"
#endif

/* HTTP GET baidu.com 验证 SPI→WL82→4G 链路 */
#define SPI_NET_HTTP_TEST

#define LOG_TAG             "[SPI_NET]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

extern void lwip_spi_set_mac_addr(u8 *mac_addr);

#define SPI_NET_APP_TASK_NAME "spi_net_app_task"

static const u8 spi_net_mac_addr[6] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x68
};

/*==============================================================================
 * SPI NETIF LwIP 初始化 (参照 WL82 lte_app_task.c spi_net_init)
 *
 * BR33 作为 DHCP Client (STA 模式):
 *   - WL82 端运行 DHCP Server, IP 池 192.168.6.100+
 *   - BR33 端通过 DHCP 获取 IP, 网关指向 WL82 (192.168.6.1)
 *============================================================================*/

static int spi_net_init(void)
{
    lwip_spi_set_mac_addr((u8 *)spi_net_mac_addr);

    Init_LwIP(SPI_NETIF);
    lwip_netif_set_up(SPI_NETIF);
    lwip_renew(SPI_NETIF, 1);                  /* DHCP 客户端 (STA 模式) */

    log_info("init done, waiting DHCP...");
    return 0;
}

/*==============================================================================
 * DHCP 事件回调 — IP 就绪后启动应用层服务
 *============================================================================*/

static volatile u8 g_spi_net_ready = 0;

int spi_lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    switch (event) {
    case LWIP_UART_DHCP_BOUND_SUCC: {          /* SPI 复用 UART 事件 */
        char ip[16];
        Get_IPAddress(SPI_NETIF, ip);
        log_info("DHCP bound: %s", ip);

        lwip_set_default_netif(SPI_NETIF);
        g_spi_net_ready = 1;
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
    if (!e) {
        return false;
    }

    struct net_event *event = (struct net_event *)e;
    struct ctp_arg *event_arg = (struct ctp_arg *)event->arg;

    switch (event->event) {
    case NET_EVENT_CMD:
        if (!event_arg) {
            log_info("NET_EVENT_CMD with NULL arg");
            return true;
        }

        log_info("IN NET_EVENT_CMD");
        ctp_cmd_analysis(event_arg->topic, event_arg->content, event_arg->cli);

        if (event_arg->content) {
            free(event_arg->content);
        }
        event_arg->content = NULL;
        free(event_arg);
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

#ifdef SPI_NET_HTTP_TEST
/*==============================================================================
 * HTTP GET 验证 (SPI → WL82 → 4G → baidu.com)
 *============================================================================*/
#include "http/http_cli.h"
#include <stdlib.h>

static void spi_net_http_test(void)
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
#endif /* SPI_NET_HTTP_TEST */

/*==============================================================================
 * 应用层服务任务 — 等 SPI NETIF 就绪后启动
 *
 * 当 WiFi 也启用时: 只做 HTTP 验证测试 (调试用), 不重复启动 iperf/ctp/http_server
 * 当 WiFi 未启用时: 启动全部应用层服务
 *============================================================================*/

static void spi_net_app_task(void *priv)
{
    /* 等待 JLDTP manager 就绪 — board_init 中 jldtp_manager_init 在另一个 task 里 */
    log_info("waiting for JLDTP...");
    while (!jldtp_manager_get_mc_handle(MULTI_CHIP_ID_1)) {
        os_time_dly(50);
    }
    log_info("JLDTP ready, init SPI NETIF");

    spi_net_init();

    while (!g_spi_net_ready) {
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
    log_info("SPI NETIF ready (services handled by WiFi)");
#endif

#ifdef SPI_NET_HTTP_TEST
    spi_net_http_test();
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

static int spi_net_app_init(void)
{
    log_info("init SPI NETIF task");
    return os_task_create(spi_net_app_task, NULL, 10, 1024, 64, SPI_NET_APP_TASK_NAME);
}
late_initcall(spi_net_app_init);

#endif /* TCFG_SPI_NET_ENABLE */
