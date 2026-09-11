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
#include "driver/cpu/periph/uart.h"
#include "os/os_api.h"

static int lwip_uart_dev_init(void);
static void uart_netif_remove_callback(struct netif *netif);

/* Define those to better describe your network interface. */
#define IFNAME0 'u'
#define IFNAME1 'l'

struct uart_eth_config {
    u32 baudrate;
    u16 tx_pin;
    u16 rx_pin;
    u8  parity;             /* maps to enum uart_parity from BR33's uart.h */
    u32 circular_buf_len;   /* size of the DMA circular buffer */
};

static s32 g_uart_num = -1;                   /* UART number, -1 means not initialized */
static u8 *uart_cbuf = NULL;                  /* DMA circular buffer */
static u32 uart_cbuf_len = 64 * 1024;          /* circular buffer size */
static u8 *uart_recv_buf = NULL;              /* receive buffer */
static u32 uart_recv_buf_len = 20 * 1024;      /* receive buffer size — smaller than cbuf, drain rate decides throughput */
static u8 *uart_send_buf = NULL;              /* send buffer */
static u32 uart_send_buf_len = 2 * 1024;      /* send buffer size */
static int lwip_uart_recv_pid = 0;            /* thread pid */
static volatile u8 lwip_uart_task_kill;       /* exit flag */
static u8 lwip_uart_mac_addr[6];              /* mac address */

static struct uart_eth_config g_uart_cfg = {
    .baudrate         = 4000000,
    .tx_pin           = IO_PORTB_05,
    .rx_pin           = IO_PORTB_04,
    .parity           = UART_PARITY_DISABLE,
    .circular_buf_len = 64 * 1024,
};

static char g_uart_dev_name[6] = "uart1";

void uart_ethernetif_set_dev_name(const char *name)
{
    if (!name) {
        return;
    }

    strncpy(g_uart_dev_name, name, sizeof(g_uart_dev_name) - 1);
    g_uart_dev_name[sizeof(g_uart_dev_name) - 1] = '\0';
}

void uart_ethernetif_set_platform_data(const struct uart_eth_config *cfg)
{
    if (!cfg) {
        return;
    }
    memcpy(&g_uart_cfg, cfg, sizeof(g_uart_cfg));
}

void lwip_uart_set_mac_addr(u8 *mac_addr)
{
    memcpy(lwip_uart_mac_addr, mac_addr, sizeof(lwip_uart_mac_addr));
}

void lwip_uart_get_mac_addr(u8 *mac_addr)
{
    memcpy(mac_addr, lwip_uart_mac_addr, sizeof(lwip_uart_mac_addr));
}

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    memcpy(netif->hwaddr, lwip_uart_mac_addr, sizeof(lwip_uart_mac_addr));

    /* maximum transfer unit */
    netif->mtu = NETIF_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

#if LWIP_IPV6
    netif->flags |= NETIF_FLAG_MLD6;
    netif->output_ip6 = ethip6_output;

#if LWIP_IPV6_DHCP6
    static struct dhcp6 dhcp6;
    dhcp6_set_struct(netif, &dhcp6);
#endif
#endif

    /* Do whatever else is needed to initialize interface. */
}

/**
 * This function should do the actual transmission of the packet.
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    if (g_uart_num < 0) {
        return ERR_IF_BUSY;
    }

    u8 *pos = uart_send_buf;
    for (struct pbuf *q = p; q != NULL; q = q->next) {
        memcpy(pos, q->payload, q->len);
        pos += q->len;
    }
    s32 ret = uart_send_blocking(g_uart_num, uart_send_buf, p->tot_len, 0);
    if (ret < 0) {
        return ERR_IF_BUSY;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 */
static struct pbuf *low_level_input(void *rx_pkt, int len)
{
    struct pbuf *q, *p;
    u16 i;

    if (len == 0) {
        return 0;
    }

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

        for (i = 0, q = p; q != NULL; q = q->next) {
            memcpy((u8_t *)q->payload, (u8_t *)rx_pkt + i, q->len);
            i += q->len;
        }

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);

    } else {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

    return p;
}

/**
 * Demux back-to-back ethernet frames from UART receive buffer.
 * Each uart_recv_blocking call may return multiple concatenated frames;
 * walk through the buffer, compute per-frame boundaries from EtherType,
 * validate MAC, and pass each valid frame to lwIP.
 */
static void ethernetif_input(void *param, void *data, int len)
{
    struct netif *netif = (struct netif *)param;
    u8 *buf = (u8 *)data;
    int offset = 0;

    while (offset + 14 <= len) {
        u8 *frame = buf + offset;
        u16 eth_type = (frame[12] << 8) | frame[13];
        int frame_len;

        /* Compute frame length from EtherType */
        if (eth_type == 0x0806) {
            frame_len = 42;                         /* ARP: 14 + 28 */
        } else if (eth_type == 0x0800 && offset + 18 <= len) {
            frame_len = 14 + ((frame[16] << 8) | frame[17]);  /* IPv4: header + IP total_len */
        } else if (eth_type <= 1500) {
            frame_len = 14 + eth_type;              /* 802.3 length */
        } else {
            offset++;                               /* unknown, skip 1 byte and retry */
            continue;
        }

        if (offset + frame_len > len) {
            break;  /* partial frame at end of buffer, wait for more data */
        }

        /* MAC validation: accept broadcast or unicast to our MAC */
        int is_bc = (frame[0] == 0xff && frame[1] == 0xff && frame[2] == 0xff &&
                     frame[3] == 0xff && frame[4] == 0xff && frame[5] == 0xff);
        int is_my = (memcmp(frame, lwip_uart_mac_addr, 6) == 0);

        if (is_bc || is_my) {
            struct pbuf *p = low_level_input(frame, frame_len);
            if (p) {
                if (netif->input(p, netif) != ERR_OK) {
                    pbuf_free(p);
                }
            }
        }

        offset += frame_len;
    }
}

static void lwip_uart_recv_task(void *priv)
{
    int ret = 0;
    struct netif *netif = (struct netif *)priv;

    while (!lwip_uart_task_kill) {
        ret = uart_recv_blocking(g_uart_num, uart_recv_buf, uart_recv_buf_len, 20);
        if (ret <= 0) {
            continue;
        }
        ethernetif_input(netif, uart_recv_buf, ret);
    }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t uart_ethernetif_init(struct netif *netif)
{
    int ret = 0;
#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = LWIP_LOCAL_HOSTNAME;
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS (100*1000000)
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    //netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    ret = lwip_uart_dev_init();
    if (ret) {
        printf("lwip_uart_dev_init err:%d\n", ret);
        return ret;
    }

    /* initialize the hardware */
    low_level_init(netif);

    if (g_uart_cfg.rx_pin != 0xffff) {
        thread_fork("lwip_uart_recv_task", 20, 8192, 0
                    , &lwip_uart_recv_pid, lwip_uart_recv_task, netif);
    }

    //ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    netif_set_remove_callback(netif, uart_netif_remove_callback);

    return ERR_OK;
}

static int lwip_uart_dev_init(void)
{
    int ret = 0;

    struct uart_config uart_cfg = {
        .baud_rate     = g_uart_cfg.baudrate,
        .tx_pin        = g_uart_cfg.tx_pin,
        .rx_pin        = g_uart_cfg.rx_pin,
        .parity        = (enum uart_parity)g_uart_cfg.parity,
        .tx_wait_mutex = 1,
    };
    g_uart_num = uart_init(-1, &uart_cfg);
    if (g_uart_num < 0) {
        printf("lwip uart_init err: %d\n", g_uart_num);
        return ERR_IF_BUSY;
    }

    /* TX buffer: dma_malloc required by BR33 uart_send_blocking */
    uart_send_buf = (u8 *)dma_malloc(uart_send_buf_len);
    if (!uart_send_buf) {
        ret = ERR_MEM;
        goto fail;
    }

    /* RX disabled (pin == 0xffff): skip DMA init, no receive thread needed */
    if (g_uart_cfg.rx_pin == 0xffff) {
        printf("lwip uart RX disabled (rx_pin == 0xffff), TX-only mode\n");
        return 0;
    }

    /* RX enabled: allocate buffers and init DMA */
    uart_cbuf = (u8 *)dma_malloc(uart_cbuf_len);
    if (!uart_cbuf) {
        ret = ERR_MEM;
        goto fail;
    }
    memset(uart_cbuf, 0, uart_cbuf_len);

    uart_recv_buf = (u8 *)dma_malloc(uart_recv_buf_len);
    if (!uart_recv_buf) {
        ret = ERR_MEM;
        goto fail;
    }

    u32 rx_timeout_thresh = (3 * 10000000 / g_uart_cfg.baudrate);  /* 3 byte-time */
    struct uart_dma_config dma_cfg = {
        .uart_rx             = {0},
        .cbuf                = {0},
        .rx_timeout_thresh   = rx_timeout_thresh,
        .frame_size          = uart_recv_buf_len,
        .rx_cbuffer_size     = g_uart_cfg.circular_buf_len,
        .rx_cbuffer          = uart_cbuf,
        .irq_callback        = NULL,
        .irq_priority        = 3,
        .event_mask          = UART_EVENT_RX_TIMEOUT,
    };
    ret = uart_dma_init(g_uart_num, &dma_cfg);
    if (ret < 0) {
        printf("lwip uart_dma_init err: %d\n", ret);
        ret = ERR_IF_BUSY;
        goto fail;
    }
    printf("lwip uart RX ready: num=%d baud=%d\n", g_uart_num, g_uart_cfg.baudrate);

    return 0;

fail:
    if (uart_cbuf)    {
        dma_free(uart_cbuf);
        uart_cbuf    = NULL;
    }
    if (uart_recv_buf) {
        dma_free(uart_recv_buf);
        uart_recv_buf = NULL;
    }
    if (uart_send_buf) {
        dma_free(uart_send_buf);
        uart_send_buf = NULL;
    }
    return ret;
}

static void uart_netif_remove_callback(struct netif *netif)
{
    /* 1. Set exit flag; kill receive thread if it was started */
    lwip_uart_task_kill = 1;

    if (lwip_uart_recv_pid) {
        thread_kill(&lwip_uart_recv_pid, KILL_WAIT);
    }

    /* 2. Deinit UART */
    if (g_uart_num >= 0) {
        uart_deinit(g_uart_num);
        g_uart_num = -1;
    }

    /* 3. Free buffers */
    if (uart_cbuf)    {
        dma_free(uart_cbuf);
        uart_cbuf    = NULL;
    }
    if (uart_send_buf) {
        dma_free(uart_send_buf);
        uart_send_buf = NULL;
    }
    if (uart_recv_buf) {
        dma_free(uart_recv_buf);
        uart_recv_buf = NULL;
    }
}

#endif
