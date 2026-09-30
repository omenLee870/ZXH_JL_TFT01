#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE
#include "system/includes.h"
#include "device/device.h"
#include "wireless/wifi_dev.h"
#include "wireless/wifi_connect.h"
#include "wireless/wifi_def.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "os/os_compat.h"
#include "server/server_core.h"
#include "server/ctp_server.h"
#include "event.h"
#include "http/http_server.h"
#include "dev_desc.h"
#include "screen_mirror.h"
#include "wifi_adapter.h"
#if TCFG_AIC8800D40_ENABLE
#include "sdio_port.h"
#include "sdio_def.h"
#include "asm/psram_api.h"
#endif

#define LOG_TAG             "[EXT_WIFI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define AP_SSID TCFG_EXT_WIFI_AP_SSID
#define AP_PWD  TCFG_EXT_WIFI_AP_PWD

#define STA_SSID TCFG_EXT_WIFI_STA_SSID
#define STA_PWD  TCFG_EXT_WIFI_STA_PWD

#define P2P_SSID TCFG_EXT_WIFI_P2P_SSID

#define WIFI_APP_TASK_NAME "ext_wifi_app_task"

/*
 *	若启用wifi模块，最好先将IOVDD档位设置为3.3V，强驱等级设置为level 2，CONFIG_OS_AFFINITY_ENABLE置0
 *
 * */

enum WIFI_APP_MSG_CODE {
    WIFI_MSG_TICK_1_SEC,
    WIFI_MSG_SMP_CFG_COMPLETED,
    WIFI_MSG_STA_SCAN_COMPLETED,
    WIFI_MSG_STA_NETWORK_STACK_DHCP_SUCC,
};

#define AP_TEST_MODE       0
#define STA_TEST_MODE      1
#define P2P_TEST_MODE 	   2

#if (TCFG_EXT_WIFI_MODE & BIT(AP_TEST_MODE))
#define EXT_WIFI_TEST_MODE 		AP_TEST_MODE
#elif (TCFG_EXT_WIFI_MODE & BIT(STA_TEST_MODE))
#define EXT_WIFI_TEST_MODE 		STA_TEST_MODE
#elif (TCFG_EXT_WIFI_MODE & BIT(P2P_TEST_MODE))
#define EXT_WIFI_TEST_MODE 		P2P_TEST_MODE
#else
#error "TCFG_EXT_WIFI_MODE error "
#endif


static void *wifi_dev = NULL;
extern unsigned int time_lapse(unsigned int *handle, unsigned int time_out);
extern u32 ext_wifi_get_upload_rate(void);
extern u32 ext_wifi_get_download_rate(void);
extern int wifi_module_is_init(void);


static void wifi_app_timer_func(void *p)
{
    os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_TICK_1_SEC);
}

static int network_user_callback(void *network_ctx, enum WIFI_EVENT state, void *priv)
{
    struct cfg_info info = {0};

    switch (state) {

    case WIFI_EVENT_MODULE_INIT:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_INIT");
#if 0
//#if !TCFG_AIC8800D40_ENABLE
//wifi module port seting
        info.port_status = 0;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
        msleep(10);
        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_CS, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_WKUP, (u32)&info);

        msleep(100);

        info.mode = AP_MODE;
        info.ssid = AP_SSID;
        info.pwd  = AP_PWD;
        info.force_default_mode = 1;
        dev_ioctl(wifi_dev, DEV_SET_DEFAULT_MODE, (u32)&info);
#endif
        break;

    case WIFI_EVENT_MODULE_START:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_START");
#if !TCFG_AIC8800D40_ENABLE
        info.mode = AP_MODE;
        info.ssid = AP_SSID;
        info.pwd  = AP_PWD;
        info.force_default_mode = 1;
        dev_ioctl(wifi_dev, DEV_SAVE_DEFAULT_MODE, (u32)&info);

        wpa_supplicant_set_connect_timeout(20);
#endif
        break;

    case WIFI_EVENT_MODULE_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_STOP");
        break;

    case WIFI_EVENT_AP_START:
        log_info("ext_network_user_callback->WIFI_EVENT_AP_START");
        break;

    case WIFI_EVENT_AP_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_AP_STOP");
        break;

    case WIFI_EVENT_STA_START:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_START");
        break;

    case WIFI_EVENT_MODULE_START_ERR:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_START_ERR");
        break;

    case WIFI_EVENT_STA_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_STOP");
        break;

    case WIFI_EVENT_STA_DISCONNECT:
        log_info("ext_network_user_callback->WIFI_STA_DISCONNECT");
        break;

    case WIFI_EVENT_STA_SCAN_COMPLETED:
        log_info("ext_network_user_callback->WIFI_STA_SCAN_COMPLETED");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_STA_SCAN_COMPLETED);
        break;

    case WIFI_EVENT_STA_CONNECT_SUCC:
        dev_ioctl(wifi_dev, DEV_GET_WIFI_CHANNEL, (u32)&info);
        log_info("ext_network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d", info.sta_channel);
        break;

    case WIFI_EVENT_MP_TEST_START:
        log_info("ext_network_user_callback->WIFI_EVENT_MP_TEST_START");
        break;

    case WIFI_EVENT_MP_TEST_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_MP_TEST_STOP");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        log_info("ext_network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID");
        break;

    case WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT");
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_STA_NETWORK_STACK_DHCP_SUCC);
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT");
        break;

    case WIFI_EVENT_P2P_START:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_START");
        break;

    case WIFI_EVENT_P2P_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_STOP");
        break;

    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED");
#if TCFG_NET_SCR_ENABLE
        if (get_net_scr_status()) {
            int net_scr_stop(void);
            net_scr_stop();
        }
#endif
#if TCFG_AIC8800D40_ENABLE
        int wlan_enable_p2p(char fhost_vif_idx);
        wlan_enable_p2p(0);
#endif
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC");
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT");
        break;

    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT");
        break;

    case WIFI_EVENT_SMP_CFG_COMPLETED:
        log_info("ext_network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_SMP_CFG_COMPLETED);
        break;

    case WIFI_EVENT_PM_SUSPEND:
        log_info("ext_network_user_callback->WIFI_EVENT_PM_SUSPEND");
        break;

    case WIFI_EVENT_PM_RESUME:
        log_info("ext_network_user_callback->WIFI_EVENT_PM_RESUME");
        break;

    case WIFI_EVENT_AP_ON_ASSOC:
        struct eth_addr *hwaddr = (struct eth_addr *)network_ctx;
        log_info("ext_network_user_callback->WIFI_EVENT_AP_ON_ASSOC hwaddr = %02x:%02x:%02x:%02x:%02x:%02x",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
#if TCFG_LTE_PHY_ENABLE && IP_NAPT_EXT
        extern void check_dhcps_client_ipaddr(u8 * hwaddr);
        check_dhcps_client_ipaddr(hwaddr->addr);
#endif
        break;

    case WIFI_EVENT_AP_ON_DISCONNECTED:
        hwaddr = (struct eth_addr *)network_ctx;
        log_info("ext_network_user_callback->WIFI_EVENT_AP_ON_DISCONNECTED hwaddr = %02x:%02x:%02x:%02x:%02x:%02x",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
        break;

#if !TCFG_AIC8800D40_ENABLE
    case WIFI_EVENT_P2P_WSC_OPERATION:
        PP2P_GO_STA_INFO sta_info = (PP2P_GO_STA_INFO)network_ctx;
        printf("WIFI_EVENT_P2P_WSC_OPERATION device name[%d] = %.*s, mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
               sta_info->dev_name_len, sta_info->dev_name_len, sta_info->dev_name,
               sta_info->dev_addr[0], sta_info->dev_addr[1], sta_info->dev_addr[2], sta_info->dev_addr[3], sta_info->dev_addr[4], sta_info->dev_addr[5]);
        void ext_p2p_wsc_trigger(void);
        ext_p2p_wsc_trigger();
#endif
        break;

    case WIFI_EVENT_P2P_GO_STA_CONNECTED:
    case WIFI_EVENT_P2P_GO_STA_DISCONNECTED: {
        const char *peer_state = (state == WIFI_EVENT_P2P_GO_STA_CONNECTED) ?
                                 "connected" : "disconnected";
#if TCFG_AIC8800D40_ENABLE
        /* AIC passes a temporary peer MAC in priv, valid during this callback. */
        const struct eth_addr *peer_mac = (const struct eth_addr *)priv;

        if (peer_mac) {
            log_info("ext_network_user_callback->WIFI_EVENT_P2P_GO_STA %s, mac=%02x:%02x:%02x:%02x:%02x:%02x",
                     peer_state,
                     peer_mac->addr[0], peer_mac->addr[1], peer_mac->addr[2],
                     peer_mac->addr[3], peer_mac->addr[4], peer_mac->addr[5]);
        } else {
            log_info("ext_network_user_callback->WIFI_EVENT_P2P_GO_STA %s, mac=unavailable", peer_state);
        }
#else
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GO_STA %s", peer_state);
#endif
        break;
    }
    default:
        break;
    }

    return 0;
}

#if TCFG_NETWORK_ENABLE
int ctp_cmd_analysis(const char *topic, char *content, void *priv);
int net_event_handler(void *e)
{
    struct net_event *event = (struct net_event *)e;
    struct ctp_arg *event_arg = (struct ctp_arg *)event->arg;
    /* struct net_event *net = &event->u.net; */

    switch (event->event) {
    case NET_EVENT_CMD:
        log_info("IN NET_EVENT_CMD\n");
        ctp_cmd_analysis(event_arg->topic, event_arg->content, event_arg->cli);

        if (event_arg->content) {
            free(event_arg->content);
        }
        event_arg->content = NULL;
        if (event_arg) {
            free(event_arg);
        }
        event_arg = NULL;
        return true;
        break;
    case NET_EVENT_DATA:
        break;
    }
    return false;
}
#endif

void wifi_app_event_handler(struct sys_event *event)
{

    int type = event->type;
    switch (type) {
    case SYS_NET_EVENT:
#if TCFG_NETWORK_ENABLE
        net_event_handler((void *)event->payload);
#endif
        break;
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}

// 启动测试
int wifi_test(void)
{
    struct cfg_info info = {0};

    if (wifi_dev == NULL) {
        wifi_dev = dev_open("wifi", NULL);
        if (wifi_dev == NULL) {
            log_error("wifi dev open failed");
            return -1;
        }
        info.cb = network_user_callback;
        info.net_priv = NULL;
        dev_ioctl(wifi_dev, DEV_SET_CB_FUNC, (u32)&info);
    }

    info.port_status = PORT_OUTPUT_LOW;
    dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
    ext_wifi_on();

    info.p2p_role = 1;
    info.ssid = P2P_SSID;
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_P2P_MODE, (u32)&info);
    return 0;
}

static void ext_wifi_app_task(void *priv)
{
    int msg[32];
    int res;
    unsigned int timehdl = 0;
    struct cfg_info info = {0};

    os_time_dly(100);

#if !TCFG_DASHBOARD_HARDWARE_TEST_ENABLE
    wifi_dev = wifi_adapter_open();

    info.cb = network_user_callback;
    info.net_priv = NULL;
    wifi_adapter_set_event_callback(info.cb);

#if TCFG_AIC8800D40_ENABLE && WIFI_ADAPTER_ROLE_CYCLE_TEST && \
    !WIFI_APP_SDIO_PSRAM_DMA_TEST
    wifi_adapter_run_cycle_test();
    return;
#endif

#if defined(TCFG_EXT_WIFI_RST_PIN)&&(TCFG_EXT_WIFI_RST_PIN != NO_CONFIG_PORT)
    gpio_set_mode(IO_PORT_SPILT(TCFG_EXT_WIFI_RST_PIN), PORT_OUTPUT_LOW);
    os_time_dly(5);
    gpio_hw_set_spl(IO_PORT_SPILT(TCFG_EXT_WIFI_RST_PIN), 1);//根据模组规格书配复位时序
    gpio_set_mode(IO_PORT_SPILT(TCFG_EXT_WIFI_RST_PIN), PORT_OUTPUT_HIGH);
    os_time_dly(100);
#else

    wifi_adapter_module_power_on();

#endif

#if (EXT_WIFI_TEST_MODE == AP_TEST_MODE)
#if TCFG_AIC8800D40_ENABLE
    wifi_adapter_start_ap(info.ssid, info.pwd);
#else
    info.mode = AP_MODE;
    info.ssid = AP_SSID;
    info.pwd  = AP_PWD;
    info.force_default_mode = 0;
    dev_ioctl(wifi_dev, DEV_AP_MODE, (u32)&info);
#endif
#elif (EXT_WIFI_TEST_MODE == STA_TEST_MODE)
#if TCFG_AIC8800D40_ENABLE
    wifi_adapter_start_sta(info.ssid, info.pwd);
#else
    info.mode = STA_MODE;
    info.ssid = STA_SSID;
    info.pwd  = STA_PWD;
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_STA_MODE, (u32)&info);
#endif
#elif (EXT_WIFI_TEST_MODE == P2P_TEST_MODE)
#if TCFG_AIC8800D40_ENABLE
    wifi_adapter_start_ap_p2p(AP_SSID, AP_PWD, P2P_SSID);
#else
    info.p2p_role = 1;
    info.ssid = P2P_SSID;
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_P2P_MODE, (u32)&info);
#endif
#endif


    void iperf_test(void);
    iperf_test();


    int ctp_init(u8 use_alive);
    ctp_init(1);

    extern int http_virfile_reg(const char *path, const char *contents, unsigned long len);
    http_virfile_reg(DEV_DESC_PATH, DEV_DESC_CONTENT, strlen(DEV_DESC_CONTENT)); //注册虚拟文件描述文档,可在dev_desc.h修改
    http_get_server_init(8080); //8080
#endif /* !TCFG_DASHBOARD_HARDWARE_TEST_ENABLE */

#ifdef CONFIG_ENABLE_VLIST
    int preview_init(u16 port, int callback(void *priv, u8 * data, size_t len));
    int playback_init(u16 port, int callback(void *priv, u8 * data, size_t len));
    preview_init(VIDEO_PREVIEW_PORT, NULL); //2226
    playback_init(VIDEO_PLAYBACK_PORT, NULL);
#endif


    while (1) {
        //printf("==check sys alive==\n");
        // 添加消息队列接收
        res = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (res != OS_TASKQ) {
            continue;
        }
    }
}

static int ext_wireless_net_init(void)//主要是create wifi 线程的
{
    log_info("wifi early init");

    return os_task_create(ext_wifi_app_task, NULL, 10, 1024, 64, WIFI_APP_TASK_NAME);
    //return os_task_create(WIFI_APP_TASK_NAME, 10, 0x1000, 64, 0, ext_wifi_app_task, NULL);
    //return thread_fork(WIFI_APP_TASK_NAME, 10, 0x1000, 64, 0, ext_wifi_app_task, NULL);
}
late_initcall(ext_wireless_net_init);

#endif

