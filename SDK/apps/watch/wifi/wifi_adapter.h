#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H

#include "app_config.h"
#include "wireless/wifi_def.h"

/* Set to 0 to disable the continuous role/power-cycle test. */
#define WIFI_ADAPTER_ROLE_CYCLE_TEST 0

typedef int (*wifi_adapter_event_cb_t)(void *network_ctx,
                                       enum WIFI_EVENT event,
                                       void *priv);

void *wifi_adapter_open(void);
int wifi_adapter_set_event_callback(wifi_adapter_event_cb_t event_cb);
int wifi_adapter_module_power_on(void);

#if TCFG_AIC8800D40_ENABLE && WIFI_ADAPTER_ROLE_CYCLE_TEST
void wifi_adapter_run_cycle_test(void);
#endif

void ext_wifi_on(void);
void ext_wifi_off(void);
int ext_wifi_work(void);

int wifi_adapter_start_sta(const char *ssid, const char *password);
int wifi_adapter_start_ap(const char *ssid, const char *password);
int wifi_adapter_start_p2p(const char *device_name);
#if TCFG_AIC8800D40_ENABLE
int wifi_adapter_stop_sta(void);
int wifi_adapter_stop_ap(void);
int wifi_adapter_stop_p2p(void);
int wifi_adapter_start_ap_p2p(const char *ap_ssid, const char *ap_password,
                              const char *p2p_device_name);
int wifi_adapter_stop_ap_p2p(void);
#endif

#endif
