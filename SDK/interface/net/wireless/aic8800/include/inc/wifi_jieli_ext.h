/*
 * Copyright (C) 2018-2024 AICSemi Ltd.
 *
 * Public Jieli-side helpers for apps that compile wireless_main.c against
 * the prebuilt AIC8800 libraries. Layout mirrors release port headers
 * (port/inc/ → cpu/br33/liba/aic8800/include/inc/).
 *
 * Private headers (jieli_port.h, jieli_sta_fix.h, jieli_p2p_dhcps.h) stay
 * inside platform/jieli/ and are not part of the customer SDK surface.
 */

#ifndef WIFI_JIELI_EXT_H_
#define WIFI_JIELI_EXT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- RTOS / netdev glue (implemented in libaic_platform.a) --- */
void aicwf_rtos_heap_lock_init(void);

struct netif;
void aicwf_jieli_register_netif(struct netif *netif);
void aicwf_jieli_register_p2p_netif(struct netif *netif);

void aic8800_sdio_kick_rx(void);
void aic8800_ensure_sta_tx_queues(const uint8_t mac[6]);
int aic8800_ap_lookup_sta_id(const uint8_t mac[6]);

/* True after a P2P GO client completes the 4-way handshake. */
int ext_wifi_module_p2p_go_is_connected(void);

/* --- STA pool helpers (internal logic; API for wireless_main lifecycle) --- */
void jieli_sta_fix_disassoc(void);
void jieli_sta_fix_scrub_pool(void);
int jieli_sta_fix_pool_exhausted(void);
int jieli_sta_fix_take_recycle_req(void);
void jieli_sta_fix_on_go_idle(void);
void jieli_sta_fix_on_go_stop(void);
void jieli_sta_fix_clear_exhausted(void);

/**
 * P2P GO IPv4 before jieli_p2p_dhcps_start().
 * Default: 192.168.49.1 / 255.255.255.0, client hosts .2 .. .20
 */
int jieli_p2p_dhcps_set_addr(const uint8_t ip4[4], const uint8_t netmask[4],
                             uint8_t host_start, uint8_t host_end);
void jieli_p2p_dhcps_get_addr(uint8_t ip4[4]);

/* go_netif: lwIP struct netif * for the P2P GO interface */
int jieli_p2p_dhcps_start(void *go_netif);
int jieli_p2p_dhcps_prepare(void *go_netif);
void jieli_p2p_dhcps_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_JIELI_EXT_H_ */
