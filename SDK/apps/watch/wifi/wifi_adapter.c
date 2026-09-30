#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE
#include "system/includes.h"
#include "device/device.h"
#include "wireless/wifi_dev.h"
#include "wireless/wifi_connect.h"
#include "wireless/wifi_def.h"
#include "lwip.h"
#include "lwip/netif.h"
#include "wifi_adapter.h"

#if TCFG_AIC8800D40_ENABLE
#include "wifi_api.h"
#include "wifi_port.h"
#include "netif_port.h"
#endif

#define LOG_TAG             "[WIFI_ADAPTER]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#define AP_SSID             TCFG_EXT_WIFI_AP_SSID
#define AP_PWD              TCFG_EXT_WIFI_AP_PWD
#define STA_SSID            TCFG_EXT_WIFI_STA_SSID
#define STA_PWD             TCFG_EXT_WIFI_STA_PWD
#define P2P_SSID            TCFG_EXT_WIFI_P2P_SSID

#define WIFI_APP_ROLE_CYCLE_TEST       WIFI_ADAPTER_ROLE_CYCLE_TEST
#define WIFI_APP_ROLE_INTERVAL_MS      15000
#define WIFI_APP_STA_DHCP_TIMEOUT_MS   15000
#define WIFI_APP_SCAN_TEST_COUNT       3
#define WIFI_APP_MODULE_TIMEOUT_MS     15000
#define WIFI_APP_POWER_SETTLE_MS       2000
#define WIFI_APP_POWER_RETRY_MS        3000

static void *wifi_dev;
int wifi_work_flag;

extern int wifi_module_is_init(void);
#if TCFG_AIC8800D40_ENABLE
extern int ext_wifi_module_enter_ap_mode(char *ssid, char *pwd);
extern int ext_wifi_module_exit_ap_mode(void);
extern int ext_wifi_module_enter_p2p_mode(char role, const char *device_name);
extern int ext_wifi_module_exit_p2p_mode(void);
extern void wifi_module_set_user_cb(
    int (*cb)(void *, enum WIFI_EVENT, void *), void *priv);
extern int wifi_module_user_cb(void *priv, enum WIFI_EVENT event, void *arg);
extern int wlan_ap_is_running(void);

static OS_MUTEX wifi_adapter_role_mutex;
static u8 wifi_adapter_role_mutex_ready;
static volatile u8 wifi_adapter_sta_connected;
static u8 wifi_adapter_sta_active;
static u8 wifi_adapter_ap_running;
static u8 wifi_adapter_p2p_running;
static wifi_adapter_event_cb_t wifi_adapter_user_event_cb;

static int wifi_adapter_event_proxy(void *network_ctx,
                                    enum WIFI_EVENT event, void *priv)
{
    /*
     * The generic Wi-Fi callback passes an AP peer address in network_ctx,
     * while the AIC event bridge passes it in priv.  Keep the application
     * callback ABI unchanged and normalize the AIC arguments here.
     */
    if (event == WIFI_EVENT_AP_ON_ASSOC ||
        event == WIFI_EVENT_AP_ON_DISCONNECTED) {
        if (!network_ctx) {
            network_ctx = priv;
        }
        if (!network_ctx) {
            log_error("AP peer event %d has no MAC address", event);
            return -1;
        }
    }

    if (wifi_adapter_user_event_cb) {
        return wifi_adapter_user_event_cb(network_ctx, event, priv);
    }
    return 0;
}

static void wifi_adapter_sta_status_cb(aic_wifi_mac_status_e status)
{
    u8 was_connected = wifi_adapter_sta_connected;

    wifi_adapter_sta_connected = (status == WIFI_STA_STATUS_CONNECTED) ? 1 : 0;
    printf("[wifi-adapter][sta] link %s\n",
           wifi_adapter_sta_connected ? "connected" : "disconnected");

    if (wifi_adapter_sta_connected && !was_connected) {
        wifi_module_user_cb(NULL, WIFI_EVENT_STA_CONNECT_SUCC, NULL);
    } else if (!wifi_adapter_sta_connected && was_connected) {
        wifi_module_user_cb(NULL, WIFI_EVENT_STA_DISCONNECT, NULL);
    }
}

static int wifi_adapter_role_lock(void)
{
    if (!wifi_adapter_role_mutex_ready || !wifi_module_is_init()) {
        log_error("wifi role request rejected: module/mutex not ready");
        return -1;
    }
    return os_mutex_pend(&wifi_adapter_role_mutex, 0);
}

static void wifi_adapter_role_unlock(void)
{
    os_mutex_post(&wifi_adapter_role_mutex);
}

static int wifi_adapter_stop_sta_locked(void);

static int wifi_adapter_start_sta_locked(const char *ssid, const char *password)
{
    const char *sta_ssid = (ssid && ssid[0]) ? ssid : STA_SSID;
    const char *sta_password = password ? password : STA_PWD;
    net_if_t sta_netif;
    uint32_t ip = 0;
    int waited_ms;
    int ret;

    if (wifi_adapter_sta_active) {
        return 0;
    }
    if (wifi_adapter_ap_running || wlan_ap_is_running() || wifi_adapter_p2p_running) {
        log_error("stop AP/P2P before entering STA mode");
        return -1;
    }

    wifi_adapter_sta_connected = 0;
    aic_wifi_sta_status_cb_set(wifi_adapter_sta_status_cb);
    aic_wifi_mode_set(WIFI_MODE_STA);
    aicwf_wifi_dev.wifi_mode = WIFI_MODE_STA;

    printf("[wifi-adapter][sta] start ssid=%s\n", sta_ssid);
    ret = aic_wifi_start_sta((u8 *)sta_ssid, (u8 *)sta_password, 30000);
    if (ret != 0) {
        printf("[wifi-adapter][sta] association failed ret=%d\n", ret);
        wifi_module_user_cb(NULL, WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT,
                            NULL);
        aic_wifi_sta_status_cb_set(NULL);
        aic_wifi_mode_set(WIFI_MODE_UNKNOWN);
        aicwf_wifi_dev.wifi_mode = WIFI_MODE_UNKNOWN;
        return ret;
    }

    wifi_adapter_sta_active = 1;
    wifi_module_user_cb(NULL, WIFI_EVENT_STA_START, NULL);

    /* Full driver reinit reuses this lwIP netif after leaving its link down. */
    sta_netif = aicwf_net_if_get(0);
    if (sta_netif) {
        aicwf_net_if_down(sta_netif);
        aicwf_net_if_set_ip(sta_netif, 0, 0, 0);
        extern void aicwf_jieli_register_netif(struct netif * netif);
        lwip_register_ext_wireless_netif((struct netif *)sta_netif);
        aicwf_jieli_register_netif((struct netif *)sta_netif);
        aicwf_net_if_set_default(sta_netif);
        aicwf_net_if_up(sta_netif);
        netif_set_link_up((struct netif *)sta_netif);
    }
    aicwf_netif_mode_switch(1);

    for (waited_ms = 0; waited_ms < WIFI_APP_STA_DHCP_TIMEOUT_MS;
         waited_ms += 100) {
        if ((waited_ms % 1000) == 0) {
            extern int aic_jieli_p2p_dhcp_is_active(void);
            uint32_t dbg_ip, dbg_mask, dbg_gw;
            net_if_t dbg_nif = aicwf_net_if_get(0);
            if (dbg_nif) {
                (void)aicwf_net_if_get_ip(dbg_nif, &dbg_ip, &dbg_mask, &dbg_gw);
                printf("[wifi-adapter][sta][dhcp] t=%d p2p_active=%d ip=%u.%u.%u.%u\n",
                       waited_ms, aic_jieli_p2p_dhcp_is_active(),
                       ((u8 *)&dbg_ip)[0], ((u8 *)&dbg_ip)[1],
                       ((u8 *)&dbg_ip)[2], ((u8 *)&dbg_ip)[3]);
            } else {
                printf("[wifi-adapter][sta][dhcp] t=%d p2p_active=%d netif=NULL\n",
                       waited_ms, aic_jieli_p2p_dhcp_is_active());
            }
        }
        if (aicwf_net_if_dhcp_done(0)) {
            break;
        }
        aicwf_rtos_msleep(100);
    }

    if (waited_ms >= WIFI_APP_STA_DHCP_TIMEOUT_MS) {
        printf("[wifi-adapter][sta] DHCP timeout, link=%u\n",
               wifi_adapter_sta_connected);
        wifi_module_user_cb(NULL, WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT,
                            NULL);
        (void)wifi_adapter_stop_sta_locked();
        return -1;
    }

    sta_netif = aicwf_net_if_get(0);
    if (sta_netif) {
        (void)aicwf_net_if_get_ip(sta_netif, &ip, NULL, NULL);
    }
    printf("[wifi-adapter][sta] ready ip=%u.%u.%u.%u waited=%dms link=%u\n",
           ((u8 *)&ip)[0], ((u8 *)&ip)[1],
           ((u8 *)&ip)[2], ((u8 *)&ip)[3],
           waited_ms, wifi_adapter_sta_connected);
    wifi_module_user_cb(NULL, WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC, NULL);
    return 0;
}

static int wifi_adapter_stop_sta_locked(void)
{
    int ret;

    if (!wifi_adapter_sta_active) {
        return 0;
    }

    aicwf_netif_mode_switch(0);
    ret = aic_wifi_stop_sta();
    if (wifi_adapter_sta_connected) {
        /* Some driver revisions omit the final status callback on stop. */
        wifi_adapter_sta_connected = 0;
        wifi_module_user_cb(NULL, WIFI_EVENT_STA_DISCONNECT, NULL);
    }
    aic_wifi_sta_status_cb_set(NULL);
    wifi_adapter_sta_active = 0;
    wifi_adapter_sta_connected = 0;
    aic_wifi_mode_set(WIFI_MODE_UNKNOWN);
    aicwf_wifi_dev.wifi_mode = WIFI_MODE_UNKNOWN;
    printf("[wifi-adapter][sta] stop ret=%d\n", ret);
    wifi_module_user_cb(NULL, WIFI_EVENT_STA_STOP, NULL);
    return ret;
}

static int wifi_adapter_start_ap_locked(const char *ssid, const char *password)
{
    int ret;

    if (wifi_adapter_sta_active) {
        log_error("stop STA before entering AP mode");
        return -1;
    }
    if (wifi_adapter_ap_running || wlan_ap_is_running()) {
        wifi_adapter_ap_running = 1;
        return 0;
    }

    ret = ext_wifi_module_enter_ap_mode((char *)(ssid ? ssid : AP_SSID),
                                        (char *)(password ? password : AP_PWD));
    if (ret >= 0) {
        wifi_adapter_ap_running = 1;
        wifi_module_user_cb(NULL, WIFI_EVENT_AP_START, NULL);
    }
    printf("[wifi-adapter][role] AP start ret=%d running=%u\n",
           ret, wifi_adapter_ap_running);
    return ret;
}

static int wifi_adapter_stop_ap_locked(void)
{
    int ret;

    if (!wifi_adapter_ap_running && !wlan_ap_is_running()) {
        return 0;
    }

    ret = ext_wifi_module_exit_ap_mode();
    if (ret >= 0) {
        wifi_adapter_ap_running = 0;
        wifi_module_user_cb(NULL, WIFI_EVENT_AP_STOP, NULL);
    }
    printf("[wifi-adapter][role] AP stop ret=%d running=%u\n",
           ret, wifi_adapter_ap_running);
    return ret;
}

static int wifi_adapter_start_p2p_locked(const char *device_name)
{
    int ret;

    if (wifi_adapter_sta_active) {
        log_error("stop STA before entering P2P mode");
        return -1;
    }
    if (wifi_adapter_p2p_running) {
        return 0;
    }

    ret = ext_wifi_module_enter_p2p_mode(
              1, (device_name && device_name[0]) ? device_name : P2P_SSID);
    if (ret >= 0) {
        wifi_adapter_p2p_running = 1;
        wifi_module_user_cb(NULL, WIFI_EVENT_P2P_START, NULL);
    }
    printf("[wifi-adapter][role] P2P start ret=%d running=%u\n",
           ret, wifi_adapter_p2p_running);
    return ret;
}

static int wifi_adapter_stop_p2p_locked(void)
{
    int ret;

    if (!wifi_adapter_p2p_running) {
        return 0;
    }

    ret = ext_wifi_module_exit_p2p_mode();
    if (ret >= 0) {
        wifi_adapter_p2p_running = 0;
        wifi_module_user_cb(NULL, WIFI_EVENT_P2P_STOP, NULL);
    }
    printf("[wifi-adapter][role] P2P stop ret=%d running=%u\n",
           ret, wifi_adapter_p2p_running);
    return ret;
}

int wifi_adapter_start_sta(const char *ssid, const char *password)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_start_sta_locked(ssid, password);
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_stop_sta(void)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_stop_sta_locked();
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_start_ap(const char *ssid, const char *password)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_start_ap_locked(ssid, password);
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_stop_ap(void)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_stop_ap_locked();
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_start_p2p(const char *device_name)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_start_p2p_locked(device_name);
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_stop_p2p(void)
{
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }
    ret = wifi_adapter_stop_p2p_locked();
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_start_ap_p2p(const char *ap_ssid, const char *ap_password,
                              const char *p2p_device_name)
{
    int ap_started_here;
    int ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }

    ap_started_here = !wifi_adapter_ap_running && !wlan_ap_is_running();
    ret = wifi_adapter_start_ap_locked(ap_ssid, ap_password);
    if (ret >= 0) {
        ret = wifi_adapter_start_p2p_locked(p2p_device_name);
        if (ret < 0 && ap_started_here) {
            (void)wifi_adapter_stop_ap_locked();
        }
    }

    printf("[wifi-adapter][role] AP+P2P start ret=%d ap=%u p2p=%u\n",
           ret, wifi_adapter_ap_running, wifi_adapter_p2p_running);
    wifi_adapter_role_unlock();
    return ret;
}

int wifi_adapter_stop_ap_p2p(void)
{
    int p2p_ret;
    int ap_ret;

    if (wifi_adapter_role_lock() != 0) {
        return -1;
    }

    p2p_ret = wifi_adapter_stop_p2p_locked();
    ap_ret = wifi_adapter_stop_ap_locked();
    wifi_adapter_role_unlock();

    if (p2p_ret < 0) {
        return p2p_ret;
    }
    return ap_ret;
}
#else
int wifi_adapter_start_ap(const char *ssid, const char *password)
{
    struct cfg_info info = {0};

    info.mode = AP_MODE;
    info.ssid = ssid ? ssid : AP_SSID;
    info.pwd = password ? password : AP_PWD;
    info.force_default_mode = 0;
    return dev_ioctl(wifi_dev, DEV_AP_MODE, (u32)&info);
}

int wifi_adapter_start_sta(const char *ssid, const char *password)
{
    struct cfg_info info = {0};

    info.mode = STA_MODE;
    info.ssid = ssid ? ssid : STA_SSID;
    info.pwd = password ? password : STA_PWD;
    info.force_default_mode = 1;
    return dev_ioctl(wifi_dev, DEV_STA_MODE, (u32)&info);
}

int wifi_adapter_start_p2p(const char *device_name)
{
    struct cfg_info info = {0};

    info.p2p_role = 1;
    info.ssid = device_name ? device_name : P2P_SSID;
    info.force_default_mode = 1;
    return dev_ioctl(wifi_dev, DEV_P2P_MODE, (u32)&info);
}
#endif

/* AIC8800D40 role switching and power-cycle test. */
#if TCFG_AIC8800D40_ENABLE && WIFI_APP_ROLE_CYCLE_TEST

static wifi_ap_list_t wifi_adapter_scan_list;

static void wifi_adapter_role_interval(const char *next_role)
{
    printf("[wifi-cycle] keep current state for %u ms, next=%s\n",
           (unsigned)WIFI_APP_ROLE_INTERVAL_MS, next_role);
    aicwf_rtos_msleep(WIFI_APP_ROLE_INTERVAL_MS);
}

static int wifi_adapter_scan_once(unsigned int test_no)
{
    unsigned int count;
    unsigned int i;
    int ret;

    memset(&wifi_adapter_scan_list, 0, sizeof(wifi_adapter_scan_list));
    printf("[wifi-cycle][scan] %u/%u scan start\n", test_no,
           (unsigned)WIFI_APP_SCAN_TEST_COUNT);
    ret = aic_wifi_scan();
    if (ret != 0) {
        printf("[wifi-cycle][scan] %u/%u scan failed ret=%d\n", test_no,
               (unsigned)WIFI_APP_SCAN_TEST_COUNT, ret);
        return ret;
    }

    aic_wifi_getscan(&wifi_adapter_scan_list);
    count = wifi_adapter_scan_list.ap_count;
    if (count > MAX_AP_COUNT) {
        count = MAX_AP_COUNT;
    }
    printf("[wifi-cycle][scan] %u/%u getscan count=%u\n", test_no,
           (unsigned)WIFI_APP_SCAN_TEST_COUNT, count);
    for (i = 0; i < count; i++) {
        const wifi_ap_info_t *ap = &wifi_adapter_scan_list.ap_info[i];

        printf("[wifi-cycle][scan] %u/%u ap#%u rssi=%d ch=%u "
               "bssid=%02x:%02x:%02x:%02x:%02x:%02x ssid=%.*s\n",
               test_no, (unsigned)WIFI_APP_SCAN_TEST_COUNT, i + 1,
               (int)(int8_t)ap->rssi, ap->channel,
               ap->bssid[0], ap->bssid[1], ap->bssid[2],
               ap->bssid[3], ap->bssid[4], ap->bssid[5],
               (int)sizeof(ap->ssid), (const char *)ap->ssid);
    }
    return 0;
}

static void wifi_adapter_scan_test(void)
{
    unsigned int i;

    for (i = 1; i <= WIFI_APP_SCAN_TEST_COUNT; i++) {
        (void)wifi_adapter_scan_once(i);
        if (i != WIFI_APP_SCAN_TEST_COUNT) {
            aicwf_rtos_msleep(1000);
        }
    }
}

static void wifi_adapter_stop_all_roles(void)
{
    (void)wifi_adapter_stop_sta();
    (void)wifi_adapter_stop_ap_p2p();
}


/* The module power pin is active low. Network stop/start owns driver teardown. */
static int wifi_adapter_set_power_pin(int on)
{
    struct cfg_info info = {0};
    int ret;

    if (!wifi_dev) {
        printf("[wifi-cycle][power] wifi dev is not open\n");
        return -1;
    }

    info.port_status = on ? PORT_OUTPUT_LOW : PORT_OUTPUT_HIGH;
    ret = dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
    printf("[wifi-cycle][power] pin=%s ret=%d\n",
           on ? "ON (LOW)" : "OFF (HIGH)", ret);
    return ret;
}

static int wifi_adapter_wait_module_state(int expected_ready, unsigned int timeout_ms)
{
    unsigned int waited_ms;

    for (waited_ms = 0; waited_ms < timeout_ms; waited_ms += 100) {
        if (wifi_module_is_init() == expected_ready) {
            return 0;
        }
        aicwf_rtos_msleep(100);
    }

    printf("[wifi-cycle][power] wait module %s timeout=%ums current=%d\n",
           expected_ready ? "ready" : "down", timeout_ms,
           wifi_module_is_init());
    return -1;
}

static int wifi_adapter_power_off(void)
{
    int ret;

    printf("[wifi-cycle][power] network/driver/SDIO stop\n");
    ext_wifi_off();
    ret = wifi_adapter_wait_module_state(0, WIFI_APP_MODULE_TIMEOUT_MS);
    if (wifi_module_is_init() == 0) {
        wifi_adapter_sta_active = 0;
        wifi_adapter_sta_connected = 0;
        wifi_adapter_ap_running = 0;
        wifi_adapter_p2p_running = 0;
    }
    aicwf_rtos_msleep(WIFI_APP_POWER_SETTLE_MS);
    if (wifi_adapter_set_power_pin(0) != 0) {
        ret = -1;
    }
    return ret;
}

static int wifi_adapter_power_on(void)
{
    if (wifi_adapter_set_power_pin(1) != 0) {
        return -1;
    }

    aicwf_rtos_msleep(WIFI_APP_POWER_SETTLE_MS);
    printf("[wifi-cycle][power] SDIO/network/driver start\n");
    ext_wifi_on();
    return wifi_adapter_wait_module_state(1, WIFI_APP_MODULE_TIMEOUT_MS);
}

static void wifi_adapter_power_on_retry(void)
{
    unsigned int attempt = 0;

    while (wifi_adapter_power_on() != 0) {
        attempt++;
        printf("[wifi-cycle][power] start attempt=%u failed, retry in %ums\n",
               attempt, (unsigned)WIFI_APP_POWER_RETRY_MS);
        (void)wifi_adapter_power_off();
        aicwf_rtos_msleep(WIFI_APP_POWER_RETRY_MS);
    }

    printf("[wifi-cycle][power] module ready attempt=%u\n", attempt + 1);
}

static void wifi_adapter_round_memory_stats(unsigned int round)
{
    size_t ram_total = memory_get_size(P_MEMORY_TOTAL);
    size_t ram_used = memory_get_size(P_MEMORY_USED);
    size_t ram_free = memory_get_size(P_MEMORY_UNUSED);

    printf("[wifi-cycle][mem] ===== round %u after driver/SDIO unload and power-off =====\n",
           round);
    printf("[wifi-cycle][mem] RAM total=%u used=%u free=%u bytes\n",
           (unsigned)ram_total, (unsigned)ram_used, (unsigned)ram_free);
    mem_stats();

#if defined(TCFG_PSRAM_SIZE) && TCFG_PSRAM_SIZE
    printf("[wifi-cycle][mem] PSRAM configured=%u bytes\n",
           (unsigned)TCFG_PSRAM_SIZE);
    psram_heap_stats();
#else
    printf("[wifi-cycle][mem] PSRAM disabled\n");
#endif
    printf("[wifi-cycle][mem] ===== round %u memory stats end =====\n", round);
}

static void wifi_adapter_run_role_sequence(unsigned int round)
{
    int ret;

    /* Phase 1: STA association + DHCP, then three scan/getscan pairs. */
    ret = wifi_adapter_start_sta(STA_SSID, STA_PWD);
    printf("[wifi-cycle] round=%u STA start ret=%d\n", round, ret);
    if (ret == 0) {
        wifi_adapter_scan_test();
    } else {
        printf("[wifi-cycle] round=%u skip scan because STA is not ready\n",
               round);
    }
    wifi_adapter_role_interval("AP");
    (void)wifi_adapter_stop_sta();

    /* Phase 2: standalone SoftAP. */
    ret = wifi_adapter_start_ap(AP_SSID, AP_PWD);
    printf("[wifi-cycle] round=%u AP start ret=%d\n", round, ret);
    wifi_adapter_role_interval("P2P");
    (void)wifi_adapter_stop_ap();

    /* Phase 3: standalone P2P GO. */
    ret = wifi_adapter_start_p2p(P2P_SSID);
    printf("[wifi-cycle] round=%u P2P start ret=%d\n", round, ret);
    wifi_adapter_role_interval("AP+P2P/AP first");
    (void)wifi_adapter_stop_p2p();

    /* Phase 4: AP first, wait 30 s, then add P2P on the same channel. */
    ret = wifi_adapter_start_ap(AP_SSID, AP_PWD);
    printf("[wifi-cycle] round=%u coexist AP-first ret=%d\n", round, ret);
    wifi_adapter_role_interval("AP+P2P/add P2P");
    if (ret == 0) {
        ret = wifi_adapter_start_p2p(P2P_SSID);
    } else {
        ret = -1;
        printf("[wifi-cycle] round=%u skip coexist P2P: AP start failed\n",
               round);
    }
    printf("[wifi-cycle] round=%u coexist P2P-second ret=%d\n", round, ret);
    wifi_adapter_role_interval("power off");
}

void wifi_adapter_run_cycle_test(void)
{
    unsigned int round = 0;
    int ret;

    printf("[wifi-cycle] ===== application role/power cycle test start =====\n");
    printf("[wifi-cycle] sequence: STA -> AP -> P2P -> AP+P2P; interval=%ums\n",
           (unsigned)WIFI_APP_ROLE_INTERVAL_MS);

    wifi_adapter_power_on_retry();

    while (1) {
        round++;
        printf("[wifi-cycle] ===== round %u START =====\n", round);

        wifi_adapter_run_role_sequence(round);

        /* Stop roles, then DEV_NETWORK_STOP removes driver and SDIO host. */
        wifi_adapter_stop_all_roles();
        ret = wifi_adapter_power_off();
        printf("[wifi-cycle] ===== round %u END power-off ret=%d ready=%d =====\n",
               round, ret, wifi_module_is_init());
        wifi_adapter_round_memory_stats(round);

        /* Keep the module physically off between complete test rounds. */
        aicwf_rtos_msleep(WIFI_APP_ROLE_INTERVAL_MS);
        wifi_adapter_power_on_retry();
    }
}
#endif
void ext_wifi_on(void)
{
    if (wifi_dev) {
        dev_ioctl(wifi_dev, DEV_NETWORK_START, 0);
        wifi_work_flag = 1;
    }
}

void ext_wifi_off(void)
{
    if (wifi_dev) {
        dev_ioctl(wifi_dev, DEV_NETWORK_STOP, 0);
        wifi_work_flag = 0;
    }
}

int ext_wifi_work(void)
{
    return wifi_work_flag;
}

void *wifi_adapter_open(void)
{
    if (!wifi_dev) {
        wifi_dev = dev_open("wifi", NULL);
        if (!wifi_dev) {
            printf("[wifi-adapter] dev_open(wifi) failed\n");
            return NULL;
        }
    }
    return wifi_dev;
}

int wifi_adapter_set_event_callback(wifi_adapter_event_cb_t event_cb)
{
    struct cfg_info info = {0};
    int ret;

    if (!wifi_dev || !event_cb) {
        return -1;
    }
#if TCFG_AIC8800D40_ENABLE
    wifi_adapter_user_event_cb = event_cb;
    info.cb = wifi_adapter_event_proxy;
#else
    info.cb = event_cb;
#endif
    info.net_priv = NULL;
    ret = dev_ioctl(wifi_dev, DEV_SET_CB_FUNC, (u32)&info);

#if TCFG_AIC8800D40_ENABLE
    if (!wifi_adapter_role_mutex_ready) {
        os_mutex_create(&wifi_adapter_role_mutex);
        wifi_adapter_role_mutex_ready = 1;
    }
    wifi_module_set_user_cb(wifi_adapter_event_proxy, NULL);
#endif
    return ret;
}

int wifi_adapter_module_power_on(void)
{
    struct cfg_info info = {0};
    int ret;

    info.port_status = PORT_OUTPUT_LOW;
    ret = dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
    if (ret < 0) {
        return ret;
    }

    ext_wifi_on();

    return ret;
}

#endif /* TCFG_EXT_WIFI_ENABLE */
