#include "app_config.h"

#if TCFG_LTE_PHY_ENABLE
#include "system/includes.h"
#include "lwip.h"
#include "lwip/dns.h"
#include "lte_module/lte_module.h"
#include "lwip/sockets.h"
#include "usbnet.h"


#define LOG_TAG             "[LTE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


static struct {
    void *dev;
    u16 check_timer;
    u8 wifi_on_flag;
    u8 hwaddr[6];
} lte_app_hdl;

#define __this	(&lte_app_hdl)

static char ip_addr[32];
static char gw_addr[32];

#if TCFG_EXT_WIFI_ENABLE && IP_NAPT_EXT

extern void ext_wifi_on(void);
extern void ext_wifi_off(void);

static void check_dhcps_client_ipaddr_timer(void *p)
{
    struct ip4_addr src_ipaddr, dest_ipaddr;
    /* char gw_addr[32]; */

    extern int dhcps_get_ipaddr(u8 hwaddr[6], struct ip4_addr * ipaddr);
    if (0 == dhcps_get_ipaddr(__this->hwaddr, &dest_ipaddr)) {
        sys_timer_del(__this->check_timer);
        __this->check_timer = 0;
        get_gateway(LTE_NETIF, gw_addr);
        inet_aton(gw_addr, &src_ipaddr);
        extern void ip_napt_ext_set_forward_addr(u32_t src_addr, u32_t dest_addr);
        ip_napt_ext_set_forward_addr(src_ipaddr.addr, dest_ipaddr.addr);
    }
}

void check_dhcps_client_ipaddr(u8 *hwaddr)
{
    memcpy(__this->hwaddr, hwaddr, 6);
    if (!__this->check_timer) {
        __this->check_timer = sys_timer_add_to_task("sys_timer", NULL, check_dhcps_client_ipaddr_timer, 500);
    }
}
#endif

int lte_lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    /* char ip_addr[32]; */
    /* char gw_addr[32]; */

    switch (event) {
    case LWIP_LTE_DHCP_BOUND_TIMEOUT:
        break;

    case LWIP_LTE_DHCP_BOUND_SUCC:
        Get_IPAddress(LTE_NETIF, ip_addr);
        get_gateway(LTE_NETIF, gw_addr);
        log_info("LTE DHCP SUCC, IP:[%s] , GW:[%s]", ip_addr, gw_addr);
        //此处4G网络已连通
        lwip_set_default_netif(LTE_NETIF);   //设置4G网卡为默认模块

        /* 优先使用 DHCP 下发的 DNS；仅当 DHCP 未提供 DNS 时，用网关/公共 DNS 兜底 */
        const ip_addr_t *dns0 = dns_getserver(0);
        if (!dns0 || ip_addr_isany_val(*dns0)) {
            ip_addr_t lte_dns;
            lte_dns.addr = inet_addr(gw_addr);
            dns_setserver(0, &lte_dns);
            IP4_ADDR(&lte_dns, 114, 114, 114, 114);
            dns_setserver(1, &lte_dns);
            printf("LTE DNS fallback: GW / 114.114.114.114\n");
        }

        void lte_network_test(void);
        lte_network_test();
        // int lws_full_test_start(void);
        // lws_full_test_start();
        /* void coremqtt_demo_start(void); */
        /* coremqtt_demo_start(); */
#if TCFG_EXT_WIFI_ENABLE && IP_NAPT_EXT
        if (!__this->wifi_on_flag) {
            ext_wifi_on();
            __this->wifi_on_flag = 1;
        }
#endif
#if TCFG_EXT_WIFI_ENABLE && IP_NAPT_EXT
        struct ip4_addr ipaddr;
        struct lan_setting *lan_setting_info = net_get_lan_info(EXT_WIFI_NETIF);
        IP4_ADDR(&ipaddr, lan_setting_info->WIRELESS_IP_ADDR0, lan_setting_info->WIRELESS_IP_ADDR1, lan_setting_info->WIRELESS_IP_ADDR2, lan_setting_info->WIRELESS_IP_ADDR3);
        extern void ip_napt_enable(u32_t addr, int enable);
        ip_napt_enable(ipaddr.addr, 1);
#endif
        break;

    default:
        break;
    }

    return 0;
}

char *get_lte_ip(void)
{
    return &ip_addr[0];
}

static void at_cmd_test(void *priv)
{
    /* 设置USB网卡模式为RNDIS */
    const char cmd[] = "AT+QCFG=\"usbnet\",3\r\n";
    /* 设置拨号上网(PDP数据承载) */
    const char cmd2[] = "AT+QNETDEVCTL=3,1,1\r\n";
    /* 重启模组使上述配置生效 */
    const char cmd3[] = "AT+CFUN=1,1\r\n";

    printf("%s, send: %s", __func__, cmd);
    usbnet_host_at_data_send(0, (u8 *)cmd, strlen(cmd));
    os_time_dly(20);

    printf("%s, send: %s", __func__, cmd2);
    usbnet_host_at_data_send(0, (u8 *)cmd2, strlen(cmd2));
    os_time_dly(20);

    printf("%s, send: %s", __func__, cmd3);
    usbnet_host_at_data_send(0, (u8 *)cmd3, strlen(cmd3));
}

static void at_port_rx_handler(u8 *buf, u32 len)
{
    if (len > 63) {
        len = 63;
    }
    buf[len] = '\0';
    printf("%s, %s\n", __func__, buf);
}


static int lte_state_cb(void *priv, int on)
{
    if (priv && !strncmp((const char *)priv, "wireless", strlen("wireless"))) {
        if (on) {
            log_info("lte on");
            dev_ioctl(__this->dev, LTE_NETWORK_START, 0);
        } else {
            log_info("lte off");
            dev_ioctl(__this->dev, LTE_NETWORK_STOP, 0);
#if 0//IP_NAPT_EXT	//TODO外挂wifi卸载会异常
            ext_wifi_off();
            __this->wifi_on_flag = 0;
#endif
        }
    }  else if (!strncmp((const char *)priv, "at_port", strlen("at_port"))) {
        if (on) {
            log_i("lte at_port on\r\n");

            usbnet_at_port_rx_handler_register(at_port_rx_handler);
            at_cmd_test(NULL);
        } else {
            log_i("lte at_port off\r\n");
        }
    }

    return 0;
}

static int lte_net_init(void)
{
    /* 模拟开关拉高，切换到 4G 网卡通路 */
    gpio_set_mode(IO_PORT_SPILT(IO_PORTC_06), PORT_OUTPUT_HIGH);

    __this->dev = dev_open("lte", NULL);
    return dev_ioctl(__this->dev, LTE_DEV_SET_CB, (u32)lte_state_cb);
}
late_initcall(lte_net_init);

#endif

