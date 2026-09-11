#include "app_config.h"

#if TCFG_NETWORK_ENABLE
/* Includes ------------------------------------------------------------------*/
#include "typedef.h"
#include "lwip/err.h"
#include "etharp.h"
#include "ethip6.h"
#include "lwip/dhcp6.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip.h"
#include "lwip/sys.h"
#include "os/os_api.h"
#include "jldtp/jldtp.h"
#include "jldtp/jldtp_manager.h"
#include "jldtp/jldtp_multi_channel.h"
#include "jldtp/jldtp_channel_ids.h"

static void spi_netif_remove_callback(struct netif *netif);

/* ─── lwip netif 标识 ─── */
#define IFNAME0 's'
#define IFNAME1 'p'

/* ─── JLDTP 通道配置 ─── */
#define SPI_NET_CHANNEL_PRIORITY       2
#define SPI_NET_TX_BUF_SIZE            (16 * 2048)
#define SPI_NET_RX_TIMEOUT_MS          5000
#define SPI_NET_REMOTE_CHIP_ID         MULTI_CHIP_ID_1

/* ─── 调试: 每秒吞吐率 ───
 * 定义后打印 TX/RX kB/s (每秒), 不定义则无开销
 */
#define SPI_NETIF_THROUGHPUT_DEBUG

/* ─── 全局状态 ─── */
static jldtp_channel_t g_spi_net_channel = NULL;
static jldtp_mc_handle_t g_spi_mc_handle = NULL;
static u8 spi_net_mac_addr[6];
static volatile u8 spi_net_link_up;

#ifdef SPI_NETIF_THROUGHPUT_DEBUG
static u32 g_tx_bytes, g_tx_pkts, g_tx_fails;
static u32 g_rx_bytes, g_rx_pkts, g_rx_drops;
static u32 g_last_report_ms;

static void spi_netif_report(void)
{
    u32 now = jiffies_msec();
    u32 elapsed = jiffies_msec2offset(g_last_report_ms, now);

    if (elapsed < 1000) {
        return;
    }
    g_last_report_ms = now;

    u32 tx_kbps = (g_tx_bytes * 1000) / (elapsed * 1024);
    u32 rx_kbps = (g_rx_bytes * 1000) / (elapsed * 1024);

    printf("[SPI_NET] %us: TX=%u/%u pkts %u kB/s | RX=%u/%u pkts %u kB/s %s\n",
           elapsed / 1000,
           g_tx_pkts - g_tx_fails, g_tx_pkts, tx_kbps,
           g_rx_pkts, g_rx_pkts, rx_kbps,
           g_tx_fails ? "(fail)" : "");

    if (g_rx_drops) {
        printf("[SPI_NET] rx_drops=%u\n", g_rx_drops);
    }

    g_tx_bytes = g_tx_pkts = g_tx_fails = 0;
    g_rx_bytes = g_rx_pkts = g_rx_drops = 0;
}
#else
#define spi_netif_report() do{}while(0)
#endif

/* ─── MAC 地址 ─── */
void lwip_spi_set_mac_addr(u8 *mac_addr)
{
    memcpy(spi_net_mac_addr, mac_addr, sizeof(spi_net_mac_addr));
}

void lwip_spi_get_mac_addr(u8 *mac_addr)
{
    memcpy(mac_addr, spi_net_mac_addr, sizeof(spi_net_mac_addr));
}

/* ─── RX 回调: JLDTP → lwip ─── */
static jldtp_result_t spi_net_rx_callback(void *user_data, jldtp_frame_t *frame,
        u8 *data, u16 data_length)
{
    struct netif *netif = (struct netif *)user_data;
    struct pbuf *q, *p;
    u16 offset = 0;

    if (!netif || !data || data_length == 0) {
        return JLDTP_ERROR_INVALID_PARAM;
    }
    if (!spi_net_link_up) {
        return JLDTP_SUCCESS;
    }

#if ETH_PAD_SIZE
    data_length += ETH_PAD_SIZE;
#endif

    p = pbuf_alloc(PBUF_RAW, data_length, PBUF_POOL);
    if (p == NULL) {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
#ifdef SPI_NETIF_THROUGHPUT_DEBUG
        g_rx_drops++;
#endif
        return JLDTP_ERROR_MEMORY;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    for (q = p; q != NULL; q = q->next) {
        memcpy((u8_t *)q->payload, data + offset, q->len);
        offset += q->len;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    LINK_STATS_INC(link.recv);

#ifdef SPI_NETIF_THROUGHPUT_DEBUG
    g_rx_bytes += data_length;
    g_rx_pkts++;
#endif

    if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
    }

    return JLDTP_SUCCESS;
}

/* ─── SPI NET RX 处理任务 ───
 * 把 pbuf 分配/拷贝/投递从 SPI RX 中断移到独立 task,
 * 缩短中断占用, 避免高频 RX 阻塞 SPI 收发。
 */
#define SPI_NET_RX_TASK_NAME   "spi_net_rx"
#define SPI_NET_RX_TASK_PRIO   15
#define SPI_NET_RX_TASK_STK    1024
#define SPI_NET_RX_TASK_Q      1024

static void spi_net_rx_task(void *priv)
{
    /* Q_CALLBACK 消息 = [type] + [func, argc, ch, frame] 共 5 个 int,
     * 数组需足够大 (对齐 app_camera_event_task 用 32) */
    int msg[16];
    while (1) {
        int ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        /* Q_CALLBACK 消息由 os_taskq_pend 内部自动调用 msg[0] 指向的回调 */
    }
}

/* ─── JLDTP 通道初始化 ─── */
static int lwip_spi_dev_init(void)
{
    jldtp_channel_config_t chan_cfg;

    g_spi_mc_handle = jldtp_manager_get_mc_handle(SPI_NET_REMOTE_CHIP_ID);
    if (!g_spi_mc_handle) {
        printf("spi_netif: jldtp_manager_get_mc_handle failed\n");
        return ERR_IF;
    }

    memset(&chan_cfg, 0, sizeof(chan_cfg));
    chan_cfg.channel_id    = JLDTP_ID_SPI_NET;
    chan_cfg.priority      = SPI_NET_CHANNEL_PRIORITY;
    chan_cfg.timeout_ms    = SPI_NET_RX_TIMEOUT_MS;
    chan_cfg.max_tx_buf_size = SPI_NET_TX_BUF_SIZE;
    /* 创建独立 RX 任务, 失败则退回中断上下文处理 */
    if (os_task_create(spi_net_rx_task, NULL, SPI_NET_RX_TASK_PRIO,
                       SPI_NET_RX_TASK_STK, SPI_NET_RX_TASK_Q,
                       SPI_NET_RX_TASK_NAME)) {
        printf("spi_netif: create rx task failed\n");
        chan_cfg.task_name = NULL;
    } else {
        chan_cfg.task_name = SPI_NET_RX_TASK_NAME;
    }

    g_spi_net_channel = jldtp_manager_open_channel(SPI_NET_REMOTE_CHIP_ID, &chan_cfg);
    if (!g_spi_net_channel) {
        printf("spi_netif: jldtp_manager_open_channel failed\n");
        return ERR_IF;
    }

    spi_net_link_up = 1;
    printf("spi_netif: ch 0x%02X opened, mtu=%d, tx_buf=%d\n",
           JLDTP_ID_SPI_NET,
           jldtp_get_mtu_size(jldtp_manager_get_dtp_handle(SPI_NET_REMOTE_CHIP_ID)),
           SPI_NET_TX_BUF_SIZE);

    return 0;
}

/* ─── lwip 低层初始化 ─── */
static void low_level_init(struct netif *netif)
{
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, spi_net_mac_addr, sizeof(spi_net_mac_addr));
    netif->mtu = NETIF_MTU;

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                   NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

#if LWIP_IPV6
    netif->flags |= NETIF_FLAG_MLD6;
    netif->output_ip6 = ethip6_output;
#if LWIP_IPV6_DHCP6
    static struct dhcp6 dhcp6;
    dhcp6_set_struct(netif, &dhcp6);
#endif
#endif
}

/* ─── TX: lwip pbuf → JLDTP MC ─── */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    jldtp_result_t ret;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);
#endif

    if (!g_spi_net_channel || !spi_net_link_up) {
#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE);
#endif
        return ERR_IF;
    }
    if (p->tot_len > NETIF_MTU + SIZEOF_ETH_HDR) {
#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE);
#endif
        return ERR_ARG;
    }

    {
        static u8 buf[NETIF_MTU + SIZEOF_ETH_HDR];
        u8 *pos = buf;
        struct pbuf *q;
        int retry;
        for (q = p; q != NULL; q = q->next) {
            memcpy(pos, q->payload, q->len);
            pos += q->len;
        }
        for (retry = 0; retry < 100; retry++) {
            ret = jldtp_mc_send_data(g_spi_net_channel, buf, p->tot_len);
            if (ret == JLDTP_SUCCESS) {
                break;
            }
            if (ret != JLDTP_ERROR_MEMORY) {
                break;
            }
            os_time_dly(1);
        }
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    if (ret != JLDTP_SUCCESS) {
#ifdef SPI_NETIF_THROUGHPUT_DEBUG
        g_tx_fails++;
#endif
        return ERR_MEM;
    }

#ifdef SPI_NETIF_THROUGHPUT_DEBUG
    g_tx_bytes += p->tot_len;
    g_tx_pkts++;
    spi_netif_report();
#endif

    LINK_STATS_INC(link.xmit);
    return ERR_OK;
}

/* ─── lwip netif 入口 ─── */
err_t spi_ethernetif_init(struct netif *netif)
{
    int ret;
    u32 now = jiffies_msec();

#if LWIP_NETIF_HOSTNAME
    netif->hostname = LWIP_LOCAL_HOSTNAME;
#endif

#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS (100 * 1000000)
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    ret = lwip_spi_dev_init();
    if (ret) {
        printf("spi_ethernetif_init: dev_init err %d\n", ret);
        return ret;
    }
    low_level_init(netif);
    jldtp_mc_register_data_callback(g_spi_net_channel, netif, spi_net_rx_callback);
    netif_set_remove_callback(netif, spi_netif_remove_callback);


#ifdef SPI_NETIF_THROUGHPUT_DEBUG
    g_last_report_ms = now;
#endif

    printf("spi_ethernetif_init: ready\n");
    return ERR_OK;
}

/* ─── netif 移除 ─── */
static void spi_netif_remove_callback(struct netif *netif)
{
    spi_net_link_up = 0;
    if (g_spi_net_channel) {
        jldtp_mc_close_channel(g_spi_net_channel);
        g_spi_net_channel = NULL;
    }
    printf("spi_netif: removed\n");
}

#endif
