/*

 * Copyright (C) 2018-2024 AICSemi Ltd.

 *

 * Jieli ext_wifi_dev bridge for AIC8800 new RTOS driver.
 * P2P concurrent: v70 os_realloc fix + GO assoc path split (2026-09-05)
 */



#include "app_config.h"
#if TCFG_EXT_WIFI_ENABLE
#include "wifi_jieli_ext.h"

#include "clock_manager/clock_manager.h"

#include "driver/cpu/periph/clock.h"

#include "driver/power/wdt.h"

#include "wireless/wifi_connect.h"

#include "wireless/wifi_module_event.h"

#include "lwip/port/lwip.h"



#include "rtos_port.h"

#include "sdio_port.h"

#include "plat_port.h"

#include "netif_port.h"

#include "wifi_api.h"

#include "wifi_if.h"

#include "wifi_def.h"

#include "wifi_port.h"

#include "aic_log.h"

#include "rtos_port.h"

#include "system/malloc.h"

#include "lwip/prot/ethernet.h"

#include "lwip/app/ctp/ctp.h"

#include "lwip/sockets.h"



#include <string.h>

#include <stdio.h>



extern int dhcps_get_ipaddr(u8 hwaddr[6], struct ip4_addr *ipaddr);
extern void dhcps_init(u8 lwip_netif);
extern void dhcps_uninit(void);



int ext_wifi_module_ap_on_disconnect(void *priv, u8 *mac, u8 reason);

int ext_wifi_module_ap_on_assoc(void *priv, u8 *mac);

int ext_wifi_module_p2p_go_sta_connected(void);

int ext_wifi_module_p2p_go_sta_disconnected(void);

int ext_wifi_module_enter_p2p_mode(char role, const char *device_name);

int ext_wifi_module_exit_p2p_mode(void);

int wifi_module_user_cb(void *priv, enum WIFI_EVENT event, void *arg);



extern void wifi_driver_init(void);
extern void wifi_driver_mgr_reset(void);

extern uint8_t GetWiFiInitStat(void);

extern void aicwf_pwrctl_enable_set(uint8_t enable);

extern int g_fhost_p2p_vif_idx;

extern int fhost_concurrent_go_vif_idx(void);

extern net_if_t fhost_to_net_if(uint8_t fhost_vif_idx);

extern struct aic_ap_cfg wifi_ap_cfg;

extern int aic_wifi_init_mac(void);

extern void aic_wifi_deinit(int mode);

extern int app_wlan_bss_config(unsigned int start_stop);
extern int wlan_ap_is_running(void);

extern void aic_wifi_event_callback(AIC_WIFI_EVENT enEvent, aic_wifi_event_data *enData);



static void aicwf_register_ext_netif(void);
static void aicwf_ap_netif_start(void);
static void aicwf_ap_netif_stop(void);

static void aicwf_enable_bcmc_tx_queues(void);

static u8 g_ap_dhcps_active;

static net_if_t aic_p2p_go_netif_get(void)
{
    int vif = g_fhost_p2p_vif_idx;
    int go_vif = fhost_concurrent_go_vif_idx();

    if (go_vif >= 0) {
        vif = go_vif;
    }

    return fhost_to_net_if((uint8_t)vif);
}



/*
 * SoftAP-only slot test (2026-09-11): align hostapd max + Host remote pool to 8.
 * See STA_POOL_LEAK.md §9.10 — expect pool-exhausted when LMAC climbs past idx=7.
 */
#define AIC_AP_STA_NUM_MAX  8

#define AIC_WIFI_SYS_CLK    (240 * MHz)

#define AIC_NX_REMOTE_STA_MAX  32
#define AIC_NX_FW_VIF_MAX      8
#define AIC_NX_VIRT_DEV_MAX    2

/*
 * Wi-Fi reinit stress (full module remove/init). The enabled mixed-role
 * sequence rotates AP-only, P2P-only and both concurrent start/stop orders.
 * Sequencer uses create_only so remove_all does not kill it.
 */
#define JIELI_AP_P2P_LIFECYCLE_TEST   0
#define JIELI_WIFI_REINIT_TEST        0
#define JIELI_WIFI_REINIT_WITH_P2P    1
#define JIELI_AP_P2P_CONCURRENT_TEST  0
#define JIELI_P2P_FIRST_THEN_AP       0
#define JIELI_P2P_ONLY_TEST           0

/*
 * BR33 does not reliably return task-stack MMU pages after a full Wi-Fi
 * create/delete cycle.  Keep the transport/firmware tasks resident and make
 * module remove a logical shutdown.  AP and P2P resources are still stopped
 * on every remove; the next init reuses the quiescent driver instance.
 */
#define JIELI_WIFI_RESIDENT_DRIVER    0

#define JIELI_P2P_GO_SSID   "DIRECT-JL380N-P2P"
#define JIELI_P2P_DEV_NAME  "JL380N-P2P"
#define JIELI_P2P_GO_PASS   "12345678"
#define JIELI_STA_TEST_SSID  "ZTE-5G"
#define JIELI_STA_TEST_PASS  "12345678"
#define JIELI_STA_CONNECT_TIMEOUT_MS  30000
#define JIELI_STA_DHCP_TIMEOUT_MS     15000
#define JIELI_STA_CALLBACK_SETTLE_MS   2000
#define JIELI_LIFECYCLE_SINGLE_MS       20000
#define JIELI_LIFECYCLE_P2P_SINGLE_MS   50000
#define JIELI_LIFECYCLE_CONCURRENT_MS   50000
#define JIELI_LIFECYCLE_REMAINING_MS    30000
#define JIELI_LIFECYCLE_IDLE_MS         20000
#define JIELI_LIFECYCLE_CONNECT_MS      120000
#define JIELI_AP_LOOP_ON_MS              30000
#define JIELI_AP_LOOP_OFF_MS             30000

/*
 * orig keeps TCFG_UI_ENABLE=1 (LVGL/jlgpu); testbuild used UI=0 for WiFi bring-up.
 * Concurrent GO malloc/free races with lv_jlgpu_task_list_free on the same
 * heap_mmu. lv_timer_enable(false) alone is NOT enough: jlgpu_run_disp_task
 * can still free attach buffers. Suspend LVGL task + drain GPU before GO.
 */
#if (defined(TCFG_UI_ENABLE) && TCFG_UI_ENABLE && \
     defined(CONFIG_LVGL_UI_ENABLE) && CONFIG_LVGL_UI_ENABLE)
#define JIELI_WIFI_UI_GUARD 1
#include "lvgl.h"
extern void lvgl_ui_task_suspend_resume(uint8_t state);
extern void lv_jlgpu_disp_wait_idle(uint32_t timeout_ms);
#else
#define JIELI_WIFI_UI_GUARD 0
#endif

#if JIELI_WIFI_UI_GUARD
/*
 * Short settle before first interface; SoftAP STA 4-way still gates via
 * g_ap_heap_busy_until when SoftAP-first path is used.
 */
#define JIELI_CONCURRENT_P2P_SETTLE_MS  800
#define JIELI_CONCURRENT_P2P_AP_QUIET_MS 2000
#define JIELI_WIFI_UI_PAUSE_ON_GO       1
#define JIELI_CONCURRENT_P2P_GO_HOLD_MS 1500
#define JIELI_CONCURRENT_AP_AFTER_P2P_MS 500
#else
#define JIELI_CONCURRENT_P2P_SETTLE_MS  500
#define JIELI_CONCURRENT_P2P_AP_QUIET_MS 1000
#define JIELI_WIFI_UI_PAUSE_ON_GO       0
#define JIELI_CONCURRENT_P2P_GO_HOLD_MS 1000
#define JIELI_CONCURRENT_AP_AFTER_P2P_MS 300
#endif

#ifndef AIC_JIELI_STA_ROOT_CAUSE_TEST
#define AIC_JIELI_STA_ROOT_CAUSE_TEST 0
#endif
#ifndef AIC_JIELI_P2P_GO_AUTO_RECYCLE
#define AIC_JIELI_P2P_GO_AUTO_RECYCLE 0
#endif

static u8 p2p_go_active;
static volatile u8 g_p2p_client_connected;
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
static volatile u8 g_lifecycle_ap_connected;
static volatile u8 g_lifecycle_p2p_connected;
static volatile u8 g_lifecycle_sta_connected;
static wifi_ap_list_t g_lifecycle_scan_list;
#endif



extern void fhost_tx_do_sta_add(uint8_t sta_id);
extern int8_t aic_ap_lookup_sta_vif_by_mac(const uint8_t mac[6]);
extern int8_t aic_ap_lookup_sta_vif_by_mac(const uint8_t mac[6]);



#define AIC_AP_EVT_Q_DEPTH  8
#define AIC_AP_EVT_P2P_RECYCLE  0x90
#define AIC_AP_EVT_P2P_CONNECTED  0x91
#define AIC_AP_EVT_P2P_DISCONNECTED  0x92



struct aic_ap_evt_msg {

    u8 type;

    u8 mac[6];

    s8 sta_idx;

};



static rtos_queue g_ap_evt_q;

static rtos_task_handle g_ap_evt_task;

static s8 g_tracked_sta_idx = -1;
static u8 g_ap_peer_mac[6];
static u8 g_p2p_peer_mac[6];
static u8 g_ap_peer_valid;
static u8 g_p2p_peer_valid;
static u8 g_ap_peer_mac[6];
static u8 g_p2p_peer_mac[6];
static u8 g_ap_peer_valid;
static u8 g_p2p_peer_valid;

static volatile u8 g_p2p_recycle_cancel;

static volatile u8 g_p2p_recycle_busy;

#if JIELI_AP_P2P_CONCURRENT_TEST
static volatile u8 g_concurrent_p2p_armed;
static rtos_task_handle g_concurrent_p2p_task;
/* SoftAP assoc/4-way/DHCP churn window — SoftAP-first path only. */
static volatile unsigned long g_ap_heap_busy_until;
#if JIELI_P2P_FIRST_THEN_AP
static volatile u8 g_concurrent_bringup_done;
static volatile int g_concurrent_bringup_ret;
#endif
#endif

#if JIELI_WIFI_UI_GUARD
static volatile u8 g_wifi_ui_paused;

static void aicwf_wifi_ui_pause(int pause)
{
#if !JIELI_WIFI_UI_PAUSE_ON_GO
    (void)pause;
    return;
#else
    if (pause) {
        if (g_wifi_ui_paused) {
            return;
        }
        /* Stop LVGL task so no new flush/malloc; drain in-flight GPU free. */
        lvgl_ui_task_suspend_resume(0);
        lv_lock();
        lv_timer_enable(false);
        lv_unlock();
        lv_jlgpu_disp_wait_idle(500);
        aicwf_rtos_msleep(100);
        g_wifi_ui_paused = 1;
        printf("[aic8800] LVGL suspended+GPU idle for WiFi bring-up\n");
    } else if (g_wifi_ui_paused) {
        lv_lock();
        lv_timer_enable(true);
        lv_unlock();
        lvgl_ui_task_suspend_resume(1);
        g_wifi_ui_paused = 0;
        printf("[aic8800] LVGL resumed after WiFi bring-up\n");
    }
#endif
}
#else
static void aicwf_wifi_ui_pause(int pause)
{
    (void)pause;
}
#endif



/*

 * Driver stores one MAC byte per unsigned int slot in auth_deauth_data.reserved[].

 * Casting reserved[] to u8* reads padding bytes (e.g. c2:00:00:00:52:00).

 */

static void aic_event_sta_mac(const aic_wifi_event_data *data, u8 mac[6])

{

    unsigned int i;



    if (!data || !mac) {

        return;

    }



    for (i = 0; i < 6; i++) {

        mac[i] = (u8)(data->data.auth_deauth_data.reserved[i] & 0xff);

    }

}



static void aic_ap_evt_post(u8 type, const u8 *mac, s8 sta_idx)

{

    struct aic_ap_evt_msg msg;



    if (!g_ap_evt_q || !mac) {

        return;

    }



    msg.type = type;

    msg.sta_idx = sta_idx;

    memcpy(msg.mac, mac, 6);

    if (aicwf_rtos_queue_write(g_ap_evt_q, sizeof(msg), &msg, 0, false) != 0) {

        printf("[aic8800] ap evt q full type=%u\n", (unsigned)type);

    }

}



static void aic_ap_evt_worker(void *arg)

{

    struct aic_ap_evt_msg msg;

    struct eth_addr ea;



    (void)arg;



    while (1) {

        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            printf("[aic8800] ap_evt exit (del_req)\n");
            os_task_del_res(OS_TASK_SELF);
            return;
        }

        if (aicwf_rtos_queue_read(g_ap_evt_q, sizeof(msg), &msg,

                                  100, false) != 0) {

            continue;

        }



        if (msg.type == AIC_AP_EVT_P2P_RECYCLE) {
#if AIC_JIELI_STA_ROOT_CAUSE_TEST
            printf("[aic8800] suppress P2P GO recycle (root-cause test)\n");
            continue;
#elif !AIC_JIELI_P2P_GO_AUTO_RECYCLE
            printf("[aic8800] suppress P2P GO recycle (AUTO_RECYCLE=0)\n");
            continue;
#else
            /*
             * WPS→PSK reassoc disconnects briefly; wait long enough for cancel.
             * Only bounce GO while peer is gone — AP vif0 stays up.
             */
            printf("[aic8800] P2P GO recycle armed, wait 2000ms (v154)\n");
            aicwf_rtos_msleep(2000);
            if (g_p2p_recycle_cancel) {
                g_p2p_recycle_cancel = 0;
                printf("[aic8800] P2P GO recycle canceled (sta back) (v154)\n");
                continue;
            }
            if (g_p2p_peer_valid) {
                printf("[aic8800] P2P GO recycle skip (peer still valid) (v154)\n");
                continue;
            }
            jieli_sta_fix_on_go_idle();
            if (!jieli_sta_fix_take_recycle_req() &&
                !jieli_sta_fix_pool_exhausted()) {
                printf("[aic8800] P2P GO idle settle, no recycle needed (v154)\n");
                continue;
            }
            if (g_p2p_recycle_busy) {
                continue;
            }
            g_p2p_recycle_busy = 1;
            printf("[aic8800] P2P GO recycle start (sta-pool v154)\n");
            ext_wifi_module_exit_p2p_mode();
            aicwf_rtos_msleep(1000);
            jieli_sta_fix_clear_exhausted();
            if (ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME) < 0) {
                printf("[aic8800] P2P GO recycle restart failed (v154)\n");
            } else {
                printf("[aic8800] P2P GO recycle done (v154)\n");
            }
            g_p2p_recycle_busy = 0;
            continue;
#endif
        }

        memcpy(ea.addr, msg.mac, 6);

        if (msg.type == AIC_AP_EVT_P2P_CONNECTED) {
            wifi_module_user_cb(NULL, WIFI_EVENT_P2P_GO_STA_CONNECTED, &ea);
            ext_wifi_module_p2p_go_sta_connected();
            continue;
        }

        if (msg.type == AIC_AP_EVT_P2P_DISCONNECTED) {
            wifi_module_user_cb(NULL, WIFI_EVENT_P2P_GO_STA_DISCONNECTED, &ea);
            ext_wifi_module_p2p_go_sta_disconnected();
            continue;
        }

        if (msg.type == DISASSOC_STA_IND_EVENT) {

            printf("[aic8800] STA disconnect %02x:%02x:%02x:%02x:%02x:%02x "

                   "(disconnect-fix-v4)\n",

                   msg.mac[0], msg.mac[1], msg.mac[2],

                   msg.mac[3], msg.mac[4], msg.mac[5]);

            wifi_module_user_cb(NULL, WIFI_EVENT_AP_ON_DISCONNECTED, &ea);

            ext_wifi_module_ap_on_disconnect(NULL, (u8 *)msg.mac, 0);

        } else if (msg.type == ASSOC_IND_EVENT) {

            /* L2 assoc only; WIFI_EVENT_AP_ON_ASSOC fires after 4-way */

        } else if (msg.type == EAPOL_STA_FIN_EVENT) {

            wifi_module_user_cb(NULL, WIFI_EVENT_AP_ON_ASSOC, &ea);

            ext_wifi_module_ap_on_assoc(NULL, (u8 *)msg.mac);

        }

    }

}



static int aic_ap_evt_worker_start(void)

{

    if (g_ap_evt_q) {

        return 0;

    }



    if (aicwf_rtos_queue_create((int)sizeof(struct aic_ap_evt_msg),

                                AIC_AP_EVT_Q_DEPTH, &g_ap_evt_q,

                                "aic_ap_evt") != 0) {

        printf("[aic8800] ap evt q create fail\n");

        return -1;

    }



    if (aicwf_rtos_task_create(aic_ap_evt_worker, "aic_ap_evt", 0,

                               2048, NULL, 5, &g_ap_evt_task) != 0) {

        printf("[aic8800] ap evt task create fail\n");

        aicwf_rtos_queue_delete(g_ap_evt_q);

        g_ap_evt_q = NULL;

        return -1;

    }



    return 0;

}



static int (*user_wifi_cb)(void *, enum WIFI_EVENT, void *);

static void *user_wifi_cb_priv;

/*
 * The generic wifi_connect layer installs its internal wifi_module_cb here.
 * Keep a separate callback for role/DHCP WIFI_EVENTs generated by this port.
 */
static int (*wifi_module_app_cb)(void *, enum WIFI_EVENT, void *);

static void *wifi_module_app_cb_priv;

static u8 wifi_module_init_flag;

static u8 wifi_driver_resident;

static enum WIFI_MODE cur_wifi_mode = NONE_MODE;

static struct aic_p2p_cfg jieli_p2p_cfg = {
    .enable_acs = 0,
    .band = 0,
    .channel = 6,
    .type = 1,
    .enable_he = 0,
    .aic_p2p_ssid = {
        .array = JIELI_P2P_GO_SSID,
        .length = sizeof(JIELI_P2P_GO_SSID) - 1,
    },
    .aic_ap_passwd = {
        .array = JIELI_P2P_GO_PASS,
        .length = sizeof(JIELI_P2P_GO_PASS) - 1,
    },
    .p2p_dev_name = JIELI_P2P_DEV_NAME,
};



void aic_platform_init(void)

{

    extern uint32_t rwnx_wifi_stack_size;

    extern uint32_t sdio_datrx_stack_size;

    extern uint32_t sdio_oobirq_stack_size;

    extern uint32_t fhost_cntrl_stack_size;

    extern uint32_t fhost_wpa_stack_size;

    extern uint32_t fhost_tx_stack_size;

    extern uint32_t fhost_rx_stack_size;

    extern uint32_t rwnx_apm_stack_size;

    extern uint32_t rwnx_timer_stack_size;



    rwnx_wifi_stack_size = 8192;

    sdio_datrx_stack_size = 5120;

    sdio_oobirq_stack_size = 3072;

    fhost_cntrl_stack_size = 8192;

    /* Must match task_info_table: PD/WPS needs ~16KB (8KB overflows). */
    fhost_wpa_stack_size = 16384;

    fhost_tx_stack_size = 4096;

    fhost_rx_stack_size = 4096;

    rwnx_apm_stack_size = 4096;

    rwnx_timer_stack_size = 2048;



    /*

     * BR33 A880S has no WiFi-module sleep/wakeup wiring for D80 SDIO pwrctrl.

     * Disable bus sleep until firmware is loaded.

     */

    aicwf_pwrctl_enable_set(0);

    aicwifi_sdio_rxdata_mode = 1;

    config_rxtask_insdio = 0;

    msgcfm_poll_en = 0;

    aicwf_rtos_heap_lock_init();

    /*

     * Hold SYS at 240MHz while AIC8800 is active. With UI off the clock

     * manager sees low CPU usage during blocking SDIO FW load and drops to

     * 48MHz mid-transfer, corrupting cmd_mgr callbacks (hmem exception).

     */

    if (clock_force_lock("aic8800", AIC_WIFI_SYS_CLK) == 0) {

        printf("[aic8800] clock locked at %lu Hz\n", (unsigned long)AIC_WIFI_SYS_CLK);

    } else {

        printf("[aic8800] clock lock fail (already locked?)\n");

    }

    printf("[aic8800] platform init: pwrctl=off rx_task=0 rxdata_poll=1 dbg=inf heap_lock=on\n");

}



extern char is_aic_driver;



static int wifi_module_event(enum WIFI_EVENT event, void *arg)

{

    if (user_wifi_cb) {

        return user_wifi_cb(user_wifi_cb_priv, event, arg);

    }

    return 0;

}



void wifi_module_set_cb(int (*cb)(void *, enum WIFI_EVENT, void *), void *priv)

{

    user_wifi_cb = cb;

    user_wifi_cb_priv = priv;

}



void wifi_module_set_user_cb(int (*cb)(void *, enum WIFI_EVENT, void *),
                             void *priv)

{

    wifi_module_app_cb = cb;

    wifi_module_app_cb_priv = priv;

}



int wifi_module_user_cb(void *priv, enum WIFI_EVENT event, void *arg)

{

    if (wifi_module_app_cb) {

        return wifi_module_app_cb(wifi_module_app_cb_priv, event, arg);

    }

    return wifi_module_event(event, arg);

}



int wifi_module_is_init(void)

{

    return wifi_module_init_flag ? 1 : 0;

}



enum WIFI_MODE ext_get_cur_wifi_mode(void)

{

    return cur_wifi_mode;

}



static int aicwf_wait_driver_ready(void)

{

    unsigned int waited_ms = 0;

    while (!GetWiFiInitStat()) {

        if (aicwf_wifi_dev.wifi_drive_op == 0) {

            printf("[aic8800] driver init completed with failure\n");

            return -1;

        }

        if (waited_ms >= 15000) {

            printf("[aic8800] driver init timeout after %u ms\n", waited_ms);

            return -1;

        }

        wdt_clear();

        aicwf_rtos_msleep(200);

        waited_ms += 200;

    }

    return 0;

}



static void aicwf_setup_ap_cfg(const char *ssid, const char *pwd)

{

    u32 ssid_len;

    u32 pwd_len;



    if (!ssid) {

        return;

    }



    /* Keep the product association limit at 8; the internal FW index space is 32. */

    wifi_ap_cfg.band = 1;

    wifi_ap_cfg.type = 0;

    wifi_ap_cfg.channel = 149;

    wifi_ap_cfg.hidden_ssid = 0;

    wifi_ap_cfg.max_inactivity = 60;

    wifi_ap_cfg.enable_he = 1;

    wifi_ap_cfg.enable_acs = 0;

    wifi_ap_cfg.bcn_interval = 100;

    wifi_ap_cfg.sta_num = AIC_AP_STA_NUM_MAX;



    ssid_len = (u32)strlen(ssid);

    if (ssid_len > sizeof(wifi_ap_cfg.aic_ap_ssid.array) - 1) {

        ssid_len = sizeof(wifi_ap_cfg.aic_ap_ssid.array) - 1;

    }



    wifi_ap_cfg.aic_ap_ssid.length = ssid_len;

    memcpy(wifi_ap_cfg.aic_ap_ssid.array, ssid, ssid_len);

    wifi_ap_cfg.aic_ap_ssid.array[ssid_len] = '\0';



    if (pwd) {

        pwd_len = (u32)strlen(pwd);

        if (pwd_len > sizeof(wifi_ap_cfg.aic_ap_passwd.array) - 1) {

            pwd_len = sizeof(wifi_ap_cfg.aic_ap_passwd.array) - 1;

        }

        wifi_ap_cfg.aic_ap_passwd.length = pwd_len;

        memcpy(wifi_ap_cfg.aic_ap_passwd.array, pwd, pwd_len);

        wifi_ap_cfg.aic_ap_passwd.array[pwd_len] = '\0';

        wifi_ap_cfg.sercurity_type = KEY_WPA2;

    } else {

        wifi_ap_cfg.aic_ap_passwd.length = 0;

        wifi_ap_cfg.sercurity_type = KEY_NONE;

    }

}



static void aicwf_register_ext_netif(void)

{

    net_if_t nif = fhost_to_net_if(0);



    if (nif) {

        lwip_register_ext_wireless_netif((struct netif *)nif);

        aicwf_jieli_register_netif((struct netif *)nif);

        printf("[aic8800] ext_netif registered %p\n", nif);

    } else {

        printf("[aic8800] ext_netif not ready (AP netif pending)\n");

    }

}

static void aicwf_ap_netif_start(void)
{
    net_if_t nif = fhost_to_net_if(0);

    if (!nif) {
        printf("[aic8800] AP netif start failed: vif0 netif missing\n");
        return;
    }

    aicwf_register_ext_netif();
    lwip_netif_set_up(EXT_WIFI_NETIF);
    lwip_renew(EXT_WIFI_NETIF, 0);

    if (!g_ap_dhcps_active) {
        dhcps_init(EXT_WIFI_NETIF);
        g_ap_dhcps_active = 1;
    }

    printf("[aic8800] AP netif/DHCP server started netif=%u\n",
           (unsigned)EXT_WIFI_NETIF);
}

static void aicwf_ap_netif_stop(void)
{
    if (!g_ap_dhcps_active) {
        return;
    }

    dhcps_uninit();
    g_ap_dhcps_active = 0;
    printf("[aic8800] AP DHCP server stopped\n");
}



/*
 * AP broadcast/multicast uses APM_START bcmc_idx =
 * NX_REMOTE_STA_MAX + fw_inst. FW IF inst can climb past fhost slots (v174).
 */
static void aicwf_enable_bcmc_tx_queues(void)
{
    uint8_t i;

    for (i = 0; i < AIC_NX_FW_VIF_MAX; i++) {
        fhost_tx_do_sta_add((uint8_t)(AIC_NX_REMOTE_STA_MAX + i));
    }
    printf("[aic8800] BCMC txq enabled for sta %u..%u\n",
           (unsigned)AIC_NX_REMOTE_STA_MAX,
           (unsigned)(AIC_NX_REMOTE_STA_MAX + AIC_NX_FW_VIF_MAX - 1));
}

#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
static void aicwf_lifecycle_peer_cb(const char *role, int connected,
                                    const u8 mac[6])
{
    printf("[aic8800][life-cb] %s %s %02x:%02x:%02x:%02x:%02x:%02x\n",
           role, connected ? "connected" : "disconnected",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void aicwf_lifecycle_sta_status_cb(aic_wifi_mac_status_e status)
{
    g_lifecycle_sta_connected =
        (status == WIFI_STA_STATUS_CONNECTED) ? 1 : 0;
    printf("[aic8800][life-cb] STA %s\n",
           g_lifecycle_sta_connected ? "connected" : "disconnected");
}

#endif



static void aic8800_wifi_event_proxy(AIC_WIFI_EVENT event, aic_wifi_event_data *data)

{

    u8 mac[6];

    int sta_id;



    aic_wifi_event_callback(event, data);



    if (!data) {

        return;

    }



    aic_event_sta_mac(data, mac);



    if (event == ASSOC_IND_EVENT) {
        int sta_vif = aic_ap_lookup_sta_vif_by_mac(mac);

        if (sta_vif == 1 ||
            (p2p_go_active && sta_vif == g_fhost_p2p_vif_idx)) {
            int kick;

            memcpy(g_p2p_peer_mac, mac, sizeof(g_p2p_peer_mac));
            g_p2p_peer_valid = 1;
            g_p2p_recycle_cancel = 1;
            for (kick = 0; kick < 8; kick++) {
                aic8800_sdio_kick_rx();
            }
            printf("[aic8800] P2P GO sta assoc vif=%d kick x8 (v314 role-split)\n",
                   sta_vif);
            if (jieli_sta_fix_pool_exhausted()) {
                u8 zmac[6] = {0};

                printf("[aic8800] P2P GO pool exhausted on assoc → recycle (v103)\n");
                aic_ap_evt_post(AIC_AP_EVT_P2P_RECYCLE, zmac, -1);
            }
            return;
        }

        memcpy(g_ap_peer_mac, mac, sizeof(g_ap_peer_mac));
        g_ap_peer_valid = 1;
#if JIELI_AP_P2P_CONCURRENT_TEST
        g_ap_heap_busy_until = aicwf_rtos_now(false) + JIELI_CONCURRENT_P2P_AP_QUIET_MS;
#endif
        printf("[aic8800] AP sta assoc vif=%d (v147 role-split)\n", sta_vif);

        aic8800_ensure_sta_tx_queues(mac);

        sta_id = aic8800_ap_lookup_sta_id(mac);

        if (sta_id >= AIC_AP_STA_NUM_MAX) {
            printf("[aic8800] WARN sta_idx=%d on assoc (sta-pool-fix)\n", sta_id);
        } else {
            printf("[aic8800] STA assoc sta_idx=%d\n", sta_id);
        }

        {
            int kick;
            for (kick = 0; kick < 8; kick++) {
                aic8800_sdio_kick_rx();
            }
            printf("[aic8800] STA assoc sdio rx kick x8 (v70)\n");
        }

        g_tracked_sta_idx = (s8)sta_id;

        aic_ap_evt_post(ASSOC_IND_EVENT, mac, sta_id);

    } else if (event == EAPOL_STA_FIN_EVENT &&
               !(p2p_go_active && g_p2p_peer_valid &&
                 memcmp(g_p2p_peer_mac, mac, sizeof(g_p2p_peer_mac)) == 0)) {

        aic8800_ensure_sta_tx_queues(mac);

        {
            int kick;

            for (kick = 0; kick < 8; kick++) {
                aic8800_sdio_kick_rx();
            }
            printf("[aic8800] AP 4-way done rx kick x8 (v69)\n");
        }

        aic_ap_evt_post(EAPOL_STA_FIN_EVENT, mac, g_tracked_sta_idx);
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
        g_lifecycle_ap_connected = 1;
        aicwf_lifecycle_peer_cb("AP", 1, mac);
#endif

    } else if (event == EAPOL_P2P_FIN_EVENT || event == EAPOL_STA_FIN_EVENT) {
        int notify_connected = !g_p2p_client_connected;

        {
            int kick;

            for (kick = 0; kick < 8; kick++) {
                aic8800_sdio_kick_rx();
            }
            printf("[aic8800] GO 4-way done rx kick x8 (v70)\n");
        }

        if (p2p_go_active) {
            net_if_t p2p_nif = aic_p2p_go_netif_get();

            if (p2p_nif) {
                jieli_p2p_dhcps_start((struct netif *)p2p_nif);
            }
        }
        g_p2p_client_connected = 1;
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
        g_lifecycle_p2p_connected = 1;
        aicwf_lifecycle_peer_cb("P2P", 1, mac);
#endif
        if (notify_connected) {
            aic_ap_evt_post(AIC_AP_EVT_P2P_CONNECTED, mac, g_tracked_sta_idx);
        }

    } else if (event == DISASSOC_STA_IND_EVENT) {
        int is_p2p_peer = g_p2p_peer_valid &&
                          memcmp(g_p2p_peer_mac, mac, sizeof(g_p2p_peer_mac)) == 0;

        if (is_p2p_peer) {
            printf("[aic8800] P2P GO sta disconnect %02x:%02x:%02x:%02x:%02x:%02x "
                   "(keep GO up, defer-reuse v156)\n",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            g_tracked_sta_idx = -1;
            g_p2p_peer_valid = 0;
            int notify_disconnected = g_p2p_client_connected;

            g_p2p_client_connected = 0;
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
            g_lifecycle_p2p_connected = 0;
#endif
            if (notify_disconnected) {
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
                aicwf_lifecycle_peer_cb("P2P", 0, mac);
#endif
                aic_ap_evt_post(AIC_AP_EVT_P2P_DISCONNECTED, mac, -1);
            }
#if AIC_JIELI_P2P_GO_AUTO_RECYCLE
            g_p2p_recycle_cancel = 0;
            aic_ap_evt_post(AIC_AP_EVT_P2P_RECYCLE, mac, -1);
#else
            /* Leave deferred ME_STA_DEL pending for WPS→PSK reuse. */
            jieli_sta_fix_on_go_idle();
#endif
            return;
        }

        /* Defer cleanup out of fhost_cntrl thread (CTP/DHCP/tcpip). */

        if (g_ap_peer_valid &&
            memcmp(g_ap_peer_mac, mac, sizeof(g_ap_peer_mac)) == 0) {
            g_ap_peer_valid = 0;
        }
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
        g_lifecycle_ap_connected = 0;
        aicwf_lifecycle_peer_cb("AP", 0, mac);
#endif
        printf("[aic8800] AP sta disconnect (v147 role-split)\n");

        aic_ap_evt_post(DISASSOC_STA_IND_EVENT, mac, g_tracked_sta_idx);

        g_tracked_sta_idx = -1;

    } else if (event == DISASSOC_P2P_IND_EVENT) {
        int notify_disconnected = g_p2p_client_connected;

        g_p2p_client_connected = 0;
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
        g_lifecycle_p2p_connected = 0;
#endif
        if (notify_disconnected) {
#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
            aicwf_lifecycle_peer_cb("P2P", 0, mac);
#endif
            aic_ap_evt_post(AIC_AP_EVT_P2P_DISCONNECTED, mac, -1);
        }
        g_p2p_peer_valid = 0;

    }

}


//#define PMC_PORTD         ((JL_PORT_TypeDef *)0x07E35200UL)
int ext_wifi_module_init(void)

{

    if (wifi_module_init_flag) {

        return 0;

    }

    g_ap_dhcps_active = 0;

#if JIELI_WIFI_RESIDENT_DRIVER
    if (wifi_driver_resident) {
        g_aic_wifi_event_cb = aic8800_wifi_event_proxy;
        is_aic_driver = 1;
        wifi_module_init_flag = 1;
        cur_wifi_mode = NONE_MODE;
        p2p_go_active = 0;
        printf("[aic8800] ext_wifi_module_init: reuse resident driver\n");
        return 0;
    }
#endif



    printf("[aic8800] ext_wifi_module_init build %s %s "
           "(D80_H fw v323 overnight-ap-p2p-reinit-cycle rx-node12 tx-aggr4 sta-limit=%d concurrent=%d)\n",
           __DATE__, __TIME__, AIC_AP_STA_NUM_MAX,
           JIELI_WIFI_REINIT_WITH_P2P);
#if JIELI_WIFI_REINIT_TEST
    printf("[aic8800] WiFi reinit test ENABLED (%s)\n",
           JIELI_WIFI_REINIT_WITH_P2P ? "four-phase AP/P2P" : "AP-only");
#elif JIELI_AP_P2P_LIFECYCLE_TEST
    printf("[aic8800] AP loop test ENABLED (on=%d off=%d ms)\n",
           JIELI_AP_LOOP_ON_MS, JIELI_AP_LOOP_OFF_MS);
#endif
#if AIC_JIELI_STA_ROOT_CAUSE_TEST
    printf("[aic8800] STA root-cause test=1 (defer/reuse/recycle disabled)\n");
#endif

    /* Cap remote STA across SoftAP+GO before either role starts (slot-leak dbg). */
    {
        extern void user_limit_sta_num_set(unsigned char num);
        user_limit_sta_num_set((unsigned char)AIC_AP_STA_NUM_MAX);
        printf("[aic8800] user_limit_sta_num=%d (sta-slot dbg)\n",
               AIC_AP_STA_NUM_MAX);
    }

    /* SDIO host is already initialized by wifi_dev during wifi_init() */

    aic_platform_init();


    // printf("get PD2 contrl before aic_wifi_init  --\n");
    // PMC_PORTD->SEL &= ~BIT(2);
    // __asm__ volatile("csync");
    if (aicwf_sdio_drv_register() != 0) {

        printf("[aic8800] sdio drv register fail\n");

        return -1;

    }



    /* UI off: short defer for SDIO host settle only */

    printf("[aic8800] defer driver init 2s (no UI)\n");

    aicwf_rtos_msleep(2000);



    printf("[aic8800] wifi_driver_init...\n");

    aicwf_wifi_dev.wifi_mode = WIFI_MODE_AP;

    // printf("get PD2 contrl before aic_wifi_init  --\n");
    // PMC_PORTD->SEL &= ~BIT(2);
    // __asm__ volatile("csync");
    wifi_driver_init();

    printf("[aic8800] wait driver ready...\n");

    if (aicwf_wait_driver_ready() != 0) {

        printf("[aic8800] wifi driver init failed\n");

        return -1;

    }

    printf("[aic8800] driver ready, init mac...\n");



    if (aic_wifi_init_mac() != 0) {

        printf("[aic8800] aic_wifi_init_mac fail\n");

        return -1;

    }



    g_aic_wifi_event_cb = aic8800_wifi_event_proxy;



    if (aic_ap_evt_worker_start() != 0) {

        printf("[aic8800] ap evt worker start fail\n");

    }



    aicwf_net_if_plat_init();



    is_aic_driver = 1;

    wifi_module_init_flag = 1;

#if JIELI_WIFI_RESIDENT_DRIVER
    wifi_driver_resident = 1;
#else
    wifi_driver_resident = 0;
#endif

    return 0;

}



int ext_wifi_module_remove(void)
{
    if (!wifi_module_init_flag) {
        return 0;
    }

    printf("[aic8800] ext_wifi_module_remove: stop roles + full deinit\n");

    /* Role stop before full teardown (safe if already stopped). */
    if (wlan_ap_is_running() || cur_wifi_mode == AP_MODE) {
        (void)app_wlan_bss_config(0);
    }
    aicwf_ap_netif_stop();
    if (p2p_go_active) {
        (void)ext_wifi_module_exit_p2p_mode();
    }

    /* Drop app refs before aic_wifi_deinit → net soft-release */
    lwip_register_ext_wireless_netif(NULL);
    aicwf_jieli_register_netif(NULL);

#if !JIELI_WIFI_RESIDENT_DRIVER
    printf("[aic8800] module_remove: before aic_wifi_deinit\n");
    aic_wifi_deinit(0);
    printf("[aic8800] module_remove: after aic_wifi_deinit\n");

    /*
     * ap_evt task is killed by remove_all_task; its queue is tracked and
     * already freed by remove_all IPC orphans. Only drop the app handle —
     * a second queue_delete is a UAF (v197 P33_WDT after deinit).
     */
    g_ap_evt_task = NULL;
    g_ap_evt_q = NULL;
    printf("[aic8800] module_remove: before wifi_driver_mgr_reset\n");
    wifi_driver_mgr_reset();
    printf("[aic8800] module_remove: after wifi_driver_mgr_reset\n");

    /*
     * Keep SDIO host up across driver deinit/re-init. Host was brought up
     * by wifi_dev; tearing it down here requires a matching re-init path
     * that is not yet wired. Driver/tasks are fully reset above.
     */
    /* ext_sdio_host_uninit(); */

    clock_unlock("aic8800");
#else
    /*
     * Do not delete fhost/SDIO/WPA worker tasks here.  All protocol roles are
     * already down and app netif references are detached, so the resident
     * driver is idle and can be reused without another MMU stack allocation.
     */
    printf("[aic8800] driver tasks kept resident (BR33 MMU leak workaround)\n");
#endif

    is_aic_driver = 0;
    wifi_module_init_flag = 0;
    cur_wifi_mode = NONE_MODE;
    p2p_go_active = 0;
    g_p2p_client_connected = 0;

    printf("[aic8800] ext_wifi_module_remove done (resident=%u)\n",
           wifi_driver_resident);
    return 0;
}



#if JIELI_AP_P2P_LIFECYCLE_TEST || JIELI_WIFI_REINIT_TEST
static volatile u8 g_lifecycle_started;
static rtos_task_handle g_lifecycle_task;

static int aicwf_lifecycle_start_ap(void)
{
    int ret;

    if (wlan_ap_is_running()) {
        printf("[aic8800][life] AP already running\n");
        return 0;
    }

    g_lifecycle_ap_connected = 0;
    aicwf_enable_bcmc_tx_queues();
    ret = app_wlan_bss_config(1);
    if (ret < 0) {
        printf("[aic8800][life] AP start fail ret=%d\n", ret);
        return ret;
    }

    aicwf_enable_bcmc_tx_queues();
    aicwf_ap_netif_start();
    cur_wifi_mode = AP_MODE;
    printf("[aic8800][life] AP started ready=%d mode=%d\n",
           wlan_ap_is_running(), cur_wifi_mode);
    return ret;
}

static int aicwf_lifecycle_stop_ap(void)
{
    int ret;

    if (!wlan_ap_is_running() && cur_wifi_mode != AP_MODE) {
        printf("[aic8800][life] AP already stopped\n");
        return 0;
    }

    ret = app_wlan_bss_config(0);
    aicwf_ap_netif_stop();
    g_lifecycle_ap_connected = 0;
    if (cur_wifi_mode == AP_MODE) {
        cur_wifi_mode = p2p_go_active ? P2P_MODE : NONE_MODE;
    }
    printf("[aic8800][life] AP stopped ret=%d ready=%d mode=%d p2p=%d\n",
           ret, wlan_ap_is_running(), cur_wifi_mode, p2p_go_active);
    return ret;
}

static void aicwf_lifecycle_wait(const char *tag, u32 wait_ms)
{
    printf("[aic8800][life] RUN %s %ums (ap=%d p2p=%d mode=%d)\n",
           tag, wait_ms, wlan_ap_is_running(),
           p2p_go_active, cur_wifi_mode);
    aicwf_rtos_msleep(wait_ms);
}

#if JIELI_WIFI_REINIT_TEST
/*
 * Four full module cycles:
 *   1. AP on/off twice -> deinit
 *   2. P2P on/off twice -> deinit
 *   3. AP on -> P2P on -> P2P off -> AP off -> deinit
 *   4. P2P on -> AP on -> AP off -> P2P off -> deinit
 * Every deinit is followed by init before advancing to the next phase.
 * Sequencer uses create_only so aicwf_rtos_remove_all does not kill it.
 */
static void aicwf_life_mem_snapshot(const char *tag)
{
    aicwf_rtos_wifi_heap_dump(tag);
}

static int aicwf_lifecycle_sta_preflight(void)
{
    const size_t target_len = sizeof(JIELI_STA_TEST_SSID) - 1;
    unsigned int i;
    int target_found = 0;
    int ret;
    int waited_ms;

    memset(&g_lifecycle_scan_list, 0, sizeof(g_lifecycle_scan_list));
    printf("[aic8800][life] STA preflight scan start\n");
    ret = aic_wifi_scan();
    if (ret != 0) {
        printf("[aic8800][life] STA preflight scan fail ret=%d\n", ret);
        return ret;
    }

    aic_wifi_getscan(&g_lifecycle_scan_list);
    for (i = 0; i < g_lifecycle_scan_list.ap_count; i++) {
        const wifi_ap_info_t *ap = &g_lifecycle_scan_list.ap_info[i];

        printf("[aic8800][scan] #%u rssi=%d ch=%u "
               "bssid=%02x:%02x:%02x:%02x:%02x:%02x ssid=%.*s\n",
               i + 1, (int)(int8_t)ap->rssi, ap->channel,
               ap->bssid[0], ap->bssid[1], ap->bssid[2],
               ap->bssid[3], ap->bssid[4], ap->bssid[5],
               (int)sizeof(ap->ssid), (const char *)ap->ssid);
        if (target_len < sizeof(ap->ssid) &&
            memcmp(ap->ssid, JIELI_STA_TEST_SSID, target_len) == 0 &&
            ap->ssid[target_len] == '\0') {
            target_found = 1;
        }
    }
    printf("[aic8800][scan] complete count=%u target=%s found=%d\n",
           g_lifecycle_scan_list.ap_count, JIELI_STA_TEST_SSID,
           target_found);

    g_lifecycle_sta_connected = 0;
    aic_wifi_sta_status_cb_set(aicwf_lifecycle_sta_status_cb);
    aic_wifi_mode_set(WIFI_MODE_STA);
    aicwf_wifi_dev.wifi_mode = WIFI_MODE_STA;
    printf("[aic8800][life] STA connect ssid=%s scan_found=%d\n",
           JIELI_STA_TEST_SSID, target_found);
    ret = aic_wifi_start_sta((u8 *)JIELI_STA_TEST_SSID,
                             (u8 *)JIELI_STA_TEST_PASS,
                             JIELI_STA_CONNECT_TIMEOUT_MS);
    if (ret != 0) {
        printf("[aic8800][life] STA connect fail ret=%d\n", ret);
        aic_wifi_mode_set(WIFI_MODE_UNKNOWN);
        aicwf_wifi_dev.wifi_mode = WIFI_MODE_UNKNOWN;
        return ret;
    }

    aicwf_netif_mode_switch(1);
    for (waited_ms = 0; waited_ms < JIELI_STA_DHCP_TIMEOUT_MS;
         waited_ms += 100) {
        if (aicwf_net_if_dhcp_done(0)) {
            break;
        }
        aicwf_rtos_msleep(100);
    }
    if (waited_ms >= JIELI_STA_DHCP_TIMEOUT_MS) {
        printf("[aic8800][life] STA DHCP timeout connected=%u\n",
               g_lifecycle_sta_connected);
        ret = -1;
    } else {
        uint32_t ip = 0;

        aicwf_net_if_get_ip(NULL, &ip, NULL, NULL);
        printf("[aic8800][life] STA ready ip=%u.%u.%u.%u waited=%dms cb=%u\n",
               ((u8 *)&ip)[0], ((u8 *)&ip)[1],
               ((u8 *)&ip)[2], ((u8 *)&ip)[3],
               waited_ms, g_lifecycle_sta_connected);
    }

    aicwf_rtos_msleep(JIELI_STA_CALLBACK_SETTLE_MS);
    aicwf_netif_mode_switch(0);
    if (aic_wifi_stop_sta() != 0) {
        printf("[aic8800][life] STA stop fail\n");
        ret = -1;
    }
    aicwf_wifi_dev.wifi_mode = WIFI_MODE_UNKNOWN;
    cur_wifi_mode = NONE_MODE;
    for (waited_ms = 0; waited_ms < 3000 && g_lifecycle_sta_connected;
         waited_ms += 100) {
        aicwf_rtos_msleep(100);
    }
    printf("[aic8800][life] STA preflight done ret=%d connected=%u\n",
           ret, g_lifecycle_sta_connected);
    return ret;
}

#if JIELI_WIFI_REINIT_WITH_P2P
static int aicwf_lifecycle_start_p2p(void)
{
    int ret;

    g_lifecycle_p2p_connected = 0;
    ret = ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME);

    printf("[aic8800][life] P2P start ret=%d active=%d mode=%d\n",
           ret, p2p_go_active, cur_wifi_mode);
    return ret;
}

static int aicwf_lifecycle_stop_p2p(void)
{
    int ret = ext_wifi_module_exit_p2p_mode();

    g_lifecycle_p2p_connected = 0;

    printf("[aic8800][life] P2P stop ret=%d active=%d mode=%d ap=%d\n",
           ret, p2p_go_active, cur_wifi_mode, wlan_ap_is_running());
    return ret;
}
#endif /* JIELI_WIFI_REINIT_WITH_P2P */

static void aicwf_wifi_reinit_lifecycle_worker(void *arg)
{
    u32 round = 0;
    u32 phase;
    u32 cycle;
    int ret;
    static size_t s_base_sram_free;
    size_t sram_free;

    (void)arg;

    printf("[aic8800][life] ===== START WiFi reinit test =====\n");
#if JIELI_WIFI_REINIT_WITH_P2P
    printf("[aic8800][life] schedule v235/q256: AP x2; P2P x2; "
           "AP->P2P/P2P->AP off; P2P->AP/AP->P2P off\n");
#else
    printf("[aic8800][life] schedule: AP on → AP off → deinit → init → AP on\n");
#endif
    aicwf_life_mem_snapshot("reinit_boot");
    aicwf_rtos_msleep(3000);

    do {
        ret = aicwf_lifecycle_sta_preflight();
        if (ret != 0) {
            printf("[aic8800][life] STA preflight FAIL ret=%d, retry in 5s\n",
                   ret);
            aicwf_rtos_msleep(5000);
        }
    } while (ret != 0);
    aicwf_lifecycle_wait("STA-to-role settle", 2000);

    while (1) {
        round++;
        phase = ((round - 1) % 4) + 1;
        cycle = ((round - 1) / 4) + 1;
        printf("[aic8800][life] === REINIT round=%u cycle=%u phase=%u START ===\n",
               round, cycle, phase);
        aicwf_life_mem_snapshot("round_start");

#if JIELI_WIFI_REINIT_WITH_P2P
        if (phase == 1) {
            int pass;

            for (pass = 1; pass <= 2; pass++) {
                ret = aicwf_lifecycle_start_ap();
                printf("[aic8800][life] round=%u phase=1 pass=%d AP start ret=%d ready=%d\n",
                       round, pass, ret, wlan_ap_is_running());
                aicwf_life_mem_snapshot("after_ap_on");
                aicwf_lifecycle_wait("P1 AP-only", JIELI_LIFECYCLE_SINGLE_MS);
                ret = aicwf_lifecycle_stop_ap();
                printf("[aic8800][life] round=%u phase=1 pass=%d AP stop ret=%d\n",
                       round, pass, ret);
                aicwf_life_mem_snapshot("after_ap_off");
                if (pass == 1) {
                    aicwf_lifecycle_wait("P1 AP restart gap", 2000);
                }
            }
        } else if (phase == 2) {
            int pass;

            for (pass = 1; pass <= 2; pass++) {
                ret = aicwf_lifecycle_start_p2p();
                printf("[aic8800][life] round=%u phase=2 pass=%d P2P start ret=%d active=%d\n",
                       round, pass, ret, p2p_go_active);
                aicwf_life_mem_snapshot("after_p2p_on");
                aicwf_lifecycle_wait("P2 P2P-only", JIELI_LIFECYCLE_P2P_SINGLE_MS);
                ret = aicwf_lifecycle_stop_p2p();
                printf("[aic8800][life] round=%u phase=2 pass=%d P2P stop ret=%d\n",
                       round, pass, ret);
                aicwf_life_mem_snapshot("after_p2p_off");
                if (pass == 1) {
                    aicwf_lifecycle_wait("P2 P2P restart gap", 2000);
                }
            }
        } else if (phase == 3) {
            ret = aicwf_lifecycle_start_ap();
            printf("[aic8800][life] round=%u phase=3 AP-first start ret=%d\n", round, ret);
            aicwf_lifecycle_wait("P3 AP settle", JIELI_CONCURRENT_P2P_SETTLE_MS);
            ret = aicwf_lifecycle_start_p2p();
            printf("[aic8800][life] round=%u phase=3 P2P-second start ret=%d\n", round, ret);
            aicwf_life_mem_snapshot("after_both_on");
            aicwf_lifecycle_wait("P3 AP+P2P", JIELI_LIFECYCLE_CONCURRENT_MS);
            ret = aicwf_lifecycle_stop_p2p();
            printf("[aic8800][life] round=%u phase=3 P2P-first stop ret=%d\n", round, ret);
            aicwf_lifecycle_wait("P3 AP remains", JIELI_LIFECYCLE_REMAINING_MS);
            ret = aicwf_lifecycle_stop_ap();
            printf("[aic8800][life] round=%u phase=3 AP-second stop ret=%d\n", round, ret);
        } else {
            ret = aicwf_lifecycle_start_p2p();
            printf("[aic8800][life] round=%u phase=4 P2P-first start ret=%d\n", round, ret);
            aicwf_lifecycle_wait("P4 P2P settle", JIELI_CONCURRENT_AP_AFTER_P2P_MS);
            ret = aicwf_lifecycle_start_ap();
            printf("[aic8800][life] round=%u phase=4 AP-second start ret=%d\n", round, ret);
            aicwf_life_mem_snapshot("after_both_on");
            aicwf_lifecycle_wait("P4 P2P+AP", JIELI_LIFECYCLE_CONCURRENT_MS);
            ret = aicwf_lifecycle_stop_ap();
            printf("[aic8800][life] round=%u phase=4 AP-first stop ret=%d\n", round, ret);
            aicwf_lifecycle_wait("P4 P2P remains", JIELI_LIFECYCLE_REMAINING_MS);
            ret = aicwf_lifecycle_stop_p2p();
            printf("[aic8800][life] round=%u phase=4 P2P-second stop ret=%d\n", round, ret);
        }
        aicwf_life_mem_snapshot("after_roles_off");
        aicwf_lifecycle_wait("roles OFF", 2000);
#else
        ret = aicwf_lifecycle_start_ap();
        printf("[aic8800][life] round=%u AP start ret=%d ready=%d\n",
               round, ret, wlan_ap_is_running());
        aicwf_life_mem_snapshot("after_ap_on");
        aicwf_lifecycle_wait("AP ON", JIELI_AP_LOOP_ON_MS);

        ret = aicwf_lifecycle_stop_ap();
        printf("[aic8800][life] round=%u AP stop ret=%d\n", round, ret);
        aicwf_life_mem_snapshot("after_ap_off");
        aicwf_lifecycle_wait("AP OFF", 2000);
#endif

        printf("[aic8800][life] round=%u module_remove...\n", round);
        ret = ext_wifi_module_remove();
        printf("[aic8800][life] round=%u remove ret=%d flag=%d\n",
               round, ret, wifi_module_init_flag);
        aicwf_life_mem_snapshot("after_deinit");
        sram_free = memory_get_size(P_MEMORY_UNUSED);
        if (round == 1) {
            s_base_sram_free = sram_free;
            printf("[aic8800][mem] leak_base sram_free=%u (after first deinit)\n",
                   (unsigned)s_base_sram_free);
        } else {
            int delta = (int)s_base_sram_free - (int)sram_free;
            printf("[aic8800][mem] leak_check round=%u sram_free=%u base=%u delta=%d"
                   " (positive => possible leak)\n",
                   round, (unsigned)sram_free, (unsigned)s_base_sram_free, delta);
        }
        aicwf_lifecycle_wait("DEINIT", 3000);

        printf("[aic8800][life] round=%u module_init...\n", round);
        ret = ext_wifi_module_init();
        printf("[aic8800][life] round=%u init ret=%d flag=%d\n",
               round, ret, wifi_module_init_flag);
        aicwf_life_mem_snapshot("after_reinit");
        if (ret != 0) {
            printf("[aic8800][life] init FAIL, abort loop\n");
            break;
        }
        aicwf_lifecycle_wait("INIT settle", 2000);
    }
}
#endif

#if JIELI_AP_P2P_LIFECYCLE_TEST
static int aicwf_lifecycle_start_p2p(void)
{
    int ret;

    g_lifecycle_p2p_connected = 0;
    ret = ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME);

    printf("[aic8800][life] P2P start ret=%d active=%d mode=%d\n",
           ret, p2p_go_active, cur_wifi_mode);
    return ret;
}

static int aicwf_lifecycle_stop_p2p(void)
{
    int ret = ext_wifi_module_exit_p2p_mode();

    g_lifecycle_p2p_connected = 0;

    printf("[aic8800][life] P2P stop ret=%d active=%d mode=%d ap=%d\n",
           ret, p2p_go_active, cur_wifi_mode, wlan_ap_is_running());
    return ret;
}

static int aicwf_lifecycle_wait_connected(const char *tag, int need_ap, int need_p2p)
{
    u32 waited_ms = 0;

    printf("[aic8800][life] WAIT CONNECT %s timeout=%ums need_ap=%d need_p2p=%d\n",
           tag, JIELI_LIFECYCLE_CONNECT_MS, need_ap, need_p2p);
    while (waited_ms < JIELI_LIFECYCLE_CONNECT_MS) {
        if ((!need_ap || g_lifecycle_ap_connected) &&
            (!need_p2p || g_lifecycle_p2p_connected)) {
            printf("[aic8800][life] CONNECTED %s waited=%ums ap_peer=%d p2p_peer=%d\n",
                   tag, waited_ms, g_lifecycle_ap_connected,
                   g_lifecycle_p2p_connected);
            return 0;
        }
        aicwf_rtos_msleep(100);
        waited_ms += 100;
    }

    printf("[aic8800][life] CONNECT TIMEOUT %s ap_peer=%d p2p_peer=%d\n",
           tag, g_lifecycle_ap_connected, g_lifecycle_p2p_connected);
    return -1;
}

static void aicwf_ap_p2p_lifecycle_worker(void *arg)
{
    u32 round = 0;
    int ret;

    (void)arg;

    printf("[aic8800][life] ===== START AP loop test =====\n");
    printf("[aic8800][life] schedule: AP on 30s, AP off 30s, repeat forever\n");
    printf("[aic8800][life] defer 3s (heap settle before first AP)\n");
    aicwf_rtos_msleep(3000);

    while (1) {
        round++;
        printf("[aic8800][life] === AP LOOP round=%u START ===\n", round);
        ret = aicwf_lifecycle_start_ap();
        printf("[aic8800][life] AP LOOP round=%u start_ret=%d ready=%d\n",
               round, ret, wlan_ap_is_running());
        aicwf_lifecycle_wait("AP loop ON", JIELI_AP_LOOP_ON_MS);

        printf("[aic8800][life] === AP LOOP round=%u STOP peer=%d ===\n",
               round, g_lifecycle_ap_connected);
        ret = aicwf_lifecycle_stop_ap();
        printf("[aic8800][life] AP LOOP round=%u stop_ret=%d ready=%d\n",
               round, ret, wlan_ap_is_running());
        aicwf_lifecycle_wait("AP loop OFF", JIELI_AP_LOOP_OFF_MS);
    }
}
#endif
#endif /* LIFECYCLE || REINIT */

#if JIELI_AP_P2P_CONCURRENT_TEST
#if JIELI_P2P_FIRST_THEN_AP
/*
 * P2P-first bring-up on a dedicated 8KB stack (wifi_app_task is only ~4KB).
 * enter_ap_mode waits until SoftAP is up so DEV_AP_MODE returns only after both.
 */
static void aicwf_concurrent_p2p_worker(void *arg)
{
    int ret;

    (void)arg;

    printf("[aic8800] P2P-first worker: settle %ums then GO\n",
           (unsigned)JIELI_CONCURRENT_P2P_SETTLE_MS);
    aicwf_rtos_msleep(JIELI_CONCURRENT_P2P_SETTLE_MS);

#if JIELI_WIFI_UI_PAUSE_ON_GO
    aicwf_wifi_ui_pause(1);
#else
    printf("[aic8800] P2P GO start (UI guard off, p2p_first)\n");
#endif

    if (ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME) < 0) {
        printf("[aic8800] P2P-first: GO start failed\n");
        g_concurrent_bringup_ret = -1;
        g_concurrent_bringup_done = 1;
        g_concurrent_p2p_armed = 0;
        aicwf_wifi_ui_pause(0);
        return;
    }

    aicwf_rtos_msleep(JIELI_CONCURRENT_P2P_GO_HOLD_MS);
    wifi_module_user_cb(NULL, WIFI_EVENT_P2P_START, NULL);
    printf("[aic8800] P2P-first: GO up, settle %ums then SoftAP\n",
           (unsigned)JIELI_CONCURRENT_AP_AFTER_P2P_MS);
    aicwf_rtos_msleep(JIELI_CONCURRENT_AP_AFTER_P2P_MS);

    aicwf_enable_bcmc_tx_queues();
    ret = app_wlan_bss_config(1);
    if (ret >= 0) {
        aicwf_enable_bcmc_tx_queues();
        aicwf_ap_netif_start();
        cur_wifi_mode = AP_MODE;
        printf("[aic8800] P2P-first: SoftAP started after GO (dhcp-fix-v3)\n");
    } else {
        printf("[aic8800] P2P-first: SoftAP fail ret=%d\n", ret);
    }

    g_concurrent_bringup_ret = ret;
    g_concurrent_bringup_done = 1;
    aicwf_wifi_ui_pause(0);
}
#else
static void aicwf_concurrent_p2p_worker(void *arg)
{
    int ap_wait;
    int quiet_wait;

    (void)arg;

    printf("[aic8800] concurrent P2P worker: settle SoftAP/services %ums\n",
           (unsigned)JIELI_CONCURRENT_P2P_SETTLE_MS);
    aicwf_rtos_msleep(JIELI_CONCURRENT_P2P_SETTLE_MS);

    for (ap_wait = 0; ap_wait < 100 && !wlan_ap_is_running(); ap_wait++) {
        aicwf_rtos_msleep(100);
    }
    printf("[aic8800] AP driver ready=%d (waited %d00ms)\n",
           wlan_ap_is_running(), ap_wait);

    if (!wlan_ap_is_running()) {
        printf("[aic8800] concurrent P2P skip: SoftAP not ready\n");
        g_concurrent_p2p_armed = 0;
        aicwf_wifi_ui_pause(0);
        return;
    }

    /*
     * SoftAP assoc/4-way/DHCP and GO GTK/beacon both hammer the heap.
     * Wait until SoftAP has been quiet (only if a STA just connected).
     */
    quiet_wait = 0;
    while ((long)(g_ap_heap_busy_until - aicwf_rtos_now(false)) > 0 &&
           quiet_wait < 200) {
        aicwf_rtos_msleep(100);
        quiet_wait++;
    }
    if (quiet_wait) {
        printf("[aic8800] concurrent P2P wait SoftAP quiet %d00ms\n", quiet_wait);
    }

#if JIELI_WIFI_UI_PAUSE_ON_GO
    aicwf_wifi_ui_pause(1);
#else
    printf("[aic8800] P2P GO start (UI guard off)\n");
#endif

    if (ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME) < 0) {
        printf("[aic8800] concurrent P2P GO start failed\n");
        g_concurrent_p2p_armed = 0;
        aicwf_wifi_ui_pause(0);
        return;
    }

    aicwf_rtos_msleep(JIELI_CONCURRENT_P2P_GO_HOLD_MS);

    wifi_module_user_cb(NULL, WIFI_EVENT_P2P_START, NULL);
    printf("[aic8800] concurrent P2P GO up (non-blocking path)\n");

    aicwf_wifi_ui_pause(0);
}
#endif
#endif

int ext_wifi_module_enter_ap_mode(char *ap_ssid, char *ap_pwd)

{

    int ret;



    if (!wifi_module_init_flag) {

        return -1;

    }

#if JIELI_P2P_ONLY_TEST
    (void)ap_ssid;
    (void)ap_pwd;
    printf("[aic8800] P2P-only test: redirect default AP request to P2P GO\n");
    aicwf_rtos_msleep(3000);
    return ext_wifi_module_enter_p2p_mode(1, JIELI_P2P_DEV_NAME);
#endif



    aicwf_setup_ap_cfg(ap_ssid, ap_pwd);

#if JIELI_WIFI_REINIT_TEST
    if (g_lifecycle_started) {
        printf("[aic8800][life] reinit sequencer already started\n");
        return 0;
    }
    g_lifecycle_started = 1;
    printf("[aic8800][life] spawn WiFi reinit sequencer (create_only)\n");
    /* create_only: not tracked by remove_all, survives module_remove. */
    if (aicwf_rtos_task_create_only(aicwf_wifi_reinit_lifecycle_worker,
                                    "wifi_reinit", 0, 3072, NULL, 5,
                                    &g_lifecycle_task) != 0) {
        g_lifecycle_started = 0;
        printf("[aic8800][life] reinit task create fail\n");
        return -1;
    }
    return 0;
#elif JIELI_AP_P2P_LIFECYCLE_TEST
    if (g_lifecycle_started) {
        printf("[aic8800][life] already started, ignore re-enter AP\n");
        return 0;
    }
    g_lifecycle_started = 1;
    printf("[aic8800][life] spawn sequencer (ignore normal AP keep-alive)\n");
    if (aicwf_rtos_task_create(aicwf_ap_p2p_lifecycle_worker, "ap_p2p_life",
                               0, 3072, NULL, 5, &g_lifecycle_task) != 0) {
        g_lifecycle_started = 0;
        printf("[aic8800][life] task create fail\n");
        return -1;
    }
    return 0;
#endif

#if JIELI_AP_P2P_CONCURRENT_TEST && JIELI_P2P_FIRST_THEN_AP
    /*
     * P2P-first: worker starts GO (wl1 + wl0 control), then SoftAP on wl0.
     * Block until SoftAP is ready so wifi_app_task services see AP up.
     */
    if (!g_concurrent_p2p_armed) {
        int wait;

        g_concurrent_p2p_armed = 1;
        g_concurrent_bringup_done = 0;
        g_concurrent_bringup_ret = -1;
        printf("[aic8800] arm P2P-first worker (GO then SoftAP)\n");
        if (aicwf_rtos_task_create(aicwf_concurrent_p2p_worker,
                                   "aic_p2p_go", 0, 2048, NULL, 5,
                                   &g_concurrent_p2p_task) != 0) {
            g_concurrent_p2p_armed = 0;
            printf("[aic8800] P2P-first worker create fail\n");
            return -1;
        }
        for (wait = 0; wait < 300 && !g_concurrent_bringup_done; wait++) {
            aicwf_rtos_msleep(100);
        }
        if (!g_concurrent_bringup_done) {
            printf("[aic8800] P2P-first bring-up timeout\n");
            return -1;
        }
        return g_concurrent_bringup_ret;
    }
    return wlan_ap_is_running() ? 0 : -1;
#endif

    /*
     * Historical 3s wait was to reclaim heap after SDIO FW load
     * (fhost_wpa_task Malloc fail). UI-off / large free heap no longer needs it.
     */
    aicwf_enable_bcmc_tx_queues();

    ret = app_wlan_bss_config(1);

    if (ret >= 0) {

        aicwf_enable_bcmc_tx_queues();

        aicwf_ap_netif_start();

        cur_wifi_mode = AP_MODE;

        printf("[aic8800] AP started, lwip renewed (dhcp-fix-v3)\n");

#if JIELI_AP_P2P_CONCURRENT_TEST
        /*
         * SoftAP-first path: return SoftAP first; start P2P in background.
         */
        if (!g_concurrent_p2p_armed) {
            g_concurrent_p2p_armed = 1;
            printf("[aic8800] arm concurrent P2P GO worker (+%ums after AP up)\n",
                   (unsigned)JIELI_CONCURRENT_P2P_SETTLE_MS);
            /* enter_p2p_mode runs on this task — 3K overflows into heap. */
            if (aicwf_rtos_task_create(aicwf_concurrent_p2p_worker,
                                       "aic_p2p_go", 0, 2048, NULL, 5,
                                       &g_concurrent_p2p_task) != 0) {
                g_concurrent_p2p_armed = 0;
                printf("[aic8800] concurrent P2P worker create fail\n");
            }
        }
#endif

    } else {

        printf("[aic8800] app_wlan_bss_config fail ret=%d\n", ret);

    }

    return ret;

}



int ext_wifi_module_exit_ap_mode(void)

{

    int ret;



    if (!wifi_module_init_flag) {

        return 0;

    }



    ret = app_wlan_bss_config(0);
    aicwf_ap_netif_stop();

    if (cur_wifi_mode == AP_MODE) {

        cur_wifi_mode = p2p_go_active ? P2P_MODE : NONE_MODE;

    }

    return ret;

}



int ext_wifi_module_enter_sta_mode(char *sta_ssid, char *sta_pwd)

{

    (void)sta_ssid;

    (void)sta_pwd;

    return 0;

}



int ext_wifi_module_exit_sta_mode(void)

{

    return 0;

}



int ext_wifi_module_enter_smp_mode(void)

{

    return 0;

}



int ext_wifi_module_exit_smp_mode(void)

{

    return 0;

}



int ext_wifi_module_enter_p2p_mode(char role, const char *device_name)

{

    int ret;

    u8 p2p_ip[4] = {
        TCFG_EXT_WIFI_P2P_IP0,
        TCFG_EXT_WIFI_P2P_IP1,
        TCFG_EXT_WIFI_P2P_IP2,
        TCFG_EXT_WIFI_P2P_IP3,
    };

    (void)role;



    if (!wifi_module_init_flag) {

        return -1;

    }



    if (p2p_go_active) {
        return 0;
    }

    g_p2p_client_connected = 0;

    /* P2P GO address is SDK-configurable (not sealed in libaic_platform.a). */
    (void)jieli_p2p_dhcps_set_addr(p2p_ip, NULL, 2, 20);



    if (device_name && device_name[0]) {

        aic_set_wps_DeviceName(device_name);

        memset(jieli_p2p_cfg.p2p_dev_name, 0, sizeof(jieli_p2p_cfg.p2p_dev_name));

        strncpy((char *)jieli_p2p_cfg.p2p_dev_name, device_name,

                sizeof(jieli_p2p_cfg.p2p_dev_name) - 1);

    }



    /* Same channel as AP for single-channel concurrent */
    if (wlan_ap_is_running()) {
        jieli_p2p_cfg.band = wifi_ap_cfg.band;
        jieli_p2p_cfg.channel = wifi_ap_cfg.channel;
        jieli_p2p_cfg.type = wifi_ap_cfg.type;
        jieli_p2p_cfg.type = wifi_ap_cfg.enable_he;
    }
    jieli_p2p_cfg.enable_acs = 0;



    printf("[aic8800] start P2P GO target=%s (mode=%d)\n",

           wlan_ap_is_running() ? "vif1 concurrent" : "vif0 standalone",
           cur_wifi_mode);



    ret = aic_wifi_start_p2p(&jieli_p2p_cfg);

    if (ret >= 0) {

        p2p_go_active = 1;

        if (!wlan_ap_is_running()) {
            cur_wifi_mode = P2P_MODE;
        }

        printf("[aic8800] P2P GO started net_id=%d vif=%d (fixed AP=wl0 P2P=wl1)\n",

               ret, g_fhost_p2p_vif_idx);

        {

            int p2p_vif = g_fhost_p2p_vif_idx ? g_fhost_p2p_vif_idx : 1;
            net_if_t p2p_nif = aic_p2p_go_netif_get();

            if (p2p_nif) {

                aicwf_jieli_register_p2p_netif((struct netif *)p2p_nif);
                jieli_p2p_dhcps_prepare((struct netif *)p2p_nif);

            } else {

                printf("[aic8800] P2P GO netif missing vif=%d\n", p2p_vif);

            }

        }

    } else {

        printf("[aic8800] aic_wifi_start_p2p fail ret=%d\n", ret);

    }

    return ret;

}



int ext_wifi_module_exit_p2p_mode(void)

{

    int ret;



    if (!p2p_go_active) {

        return 0;

    }



    jieli_p2p_dhcps_stop();

    ret = aic_wifi_stop_p2p();

    if (g_p2p_client_connected) {
        g_p2p_client_connected = 0;
        aic_ap_evt_post(AIC_AP_EVT_P2P_DISCONNECTED,
                        g_p2p_peer_mac, -1);
    }

    jieli_sta_fix_on_go_stop();

    p2p_go_active = 0;

    cur_wifi_mode = wlan_ap_is_running() ? AP_MODE : NONE_MODE;

    printf("[aic8800] P2P GO stopped ret=%d (AP vif0 kept)\n", ret);

    return ret;

}



int ext_wifi_module_enter_mp_test_mode(void)

{

    return 0;

}



int ext_wifi_module_exit_mp_test_mode(void)

{

    return 0;

}



int ext_wifi_module_ap_on_disconnect(void *priv, u8 *mac, u8 reason)

{

    (void)priv;

    (void)reason;



    if (mac) {

        printf("[aic8800] ap_on_disconnect %02x:%02x:%02x:%02x:%02x:%02x\n",

               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    }

    return 0;

}



int ext_wifi_module_ap_on_assoc(void *priv, u8 *mac)

{

    extern int aic_wifi_fixrate(int argc, int argv[]);
    int rate_argv[4] = {7, 0, 1, 2}; /* MCS7, 20 MHz, short GI, HT */

    (void)priv;

    if (mac) {

        printf("[aic8800] ap_on_assoc %02x:%02x:%02x:%02x:%02x:%02x\n",

               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    }

#if JIELI_AP_P2P_CONCURRENT_TEST
    /* DHCP / CTP after 4-way still touches heap — keep GO deferred. */
    g_ap_heap_busy_until = aicwf_rtos_now(false) + JIELI_CONCURRENT_P2P_AP_QUIET_MS;
#endif

    /* Airtime around MCS3 explains a 10-12 Mbps ceiling on both directions. */
    if (aic_wifi_fixrate(4, rate_argv) == 0) {
        printf("[aic8800] AP TX fixed HT MCS7 SGI 20M (v154 sta-pool-recycle)\n");
    }

    return 0;

}



int ext_wifi_module_pm_suspend_notify(void)

{

    return 0;

}



int ext_wifi_module_pm_resume_notify(void)

{

    return 0;

}



int ext_wifi_module_get_sta_ssid(void)

{

    return 0;

}



int ext_wifi_module_p2p_go_sta_connected(void)

{
    printf("[aic8800] P2P GO client connected\n");
    return 0;

}



int ext_wifi_module_p2p_go_sta_disconnected(void)
{
    printf("[aic8800] P2P GO sta disconnected (GO stays up / recycle v103)\n");
    return 0;
}

int ext_wifi_module_p2p_go_is_connected(void)
{
    return g_p2p_client_connected ? 1 : 0;
}



int ext_wifi_module_p2p_trigger_wsc_req(void)

{

    return 0;

}
#endif