#include "app_config.h"
#if TCFG_USER_BLE_ENABLE && (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
#include "system/includes.h"
#include "app_ble_spp_api.h"
#include "btstack_rcsp_user.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "zxh_vehicle_protocol.h"
#include "zxh_ble.h"
#include "zxh_ble_frame.h"

#define CLIENTS 2
#define TX_DEPTH 8
#define RX_DEPTH 4
#define TOKEN_TAG 0xb1000000u
static OS_MUTEX ble_mutex;
static int initialized;
static uint32_t nonce;
struct tx_message { char *data; uint16_t length, offset; uint8_t frame, index; uint32_t started; };
static struct client_state {
    void *hdl, *token;
    uint16_t connection;
    uint8_t subscribed, next_frame, tx_head, tx_count;
    struct zxh_ble_decoder rx;
    char *requests[RX_DEPTH];
    uint8_t rx_head, rx_count;
    struct tx_message tx[TX_DEPTH];
} clients[CLIENTS];

int zxh_ble_is_client(void *client) { return ((uintptr_t)client & 0xff000000u) == TOKEN_TAG; }
static struct client_state *find_token(void *token)
{
    int i;
    for (i = 0; i < CLIENTS; ++i) if (clients[i].token == token && clients[i].connection) return &clients[i];
    return NULL;
}
int zxh_ble_client_active(void *client)
{
    struct client_state *s; int active;
    if (!initialized) return 0;
    os_mutex_pend(&ble_mutex, 0); s = find_token(client); active = s && s->subscribed;
    os_mutex_post(&ble_mutex); return active;
}
uint16_t zxh_ble_client_connection(void *client)
{
    struct client_state *s; uint16_t con = 0;
    if (!initialized) return 0;
    os_mutex_pend(&ble_mutex, 0); s = find_token(client); if (s) con = s->connection;
    os_mutex_post(&ble_mutex); return con;
}
void zxh_ble_disconnected(void *hdl)
{
    int i, j; void *old[CLIENTS] = {0};
    if (!initialized) return;
    os_mutex_pend(&ble_mutex, 0);
    for (i = 0; i < CLIENTS; ++i) if (clients[i].hdl == hdl) {
        old[i] = clients[i].token;
        for (j = 0; j < RX_DEPTH; ++j) free(clients[i].requests[j]);
        for (j = 0; j < TX_DEPTH; ++j) free(clients[i].tx[j].data);
        memset(&clients[i], 0, sizeof(clients[i]));
    }
    os_mutex_post(&ble_mutex);
    /* Protocol sends acquire BLE second; disconnect must not invert that order. */
    for (i = 0; i < CLIENTS; ++i) if (old[i]) zxh_vehicle_disconnect(old[i]);
}
int zxh_ble_send(void *client, const char *json)
{
    struct client_state *s; struct tx_message *m; unsigned int n = strlen(json); int ret = -1;
    if (!initialized || !n || n > ZXH_BLE_MESSAGE_MAX) return -1;
    os_mutex_pend(&ble_mutex, 0); s = find_token(client);
    if (s && s->subscribed && s->tx_count < TX_DEPTH) {
        m = &s->tx[(s->tx_head + s->tx_count) % TX_DEPTH]; m->data = malloc(n + 1);
        if (m->data) {
            strcpy(m->data, json); m->length = n; m->offset = 0; m->index = 0;
            m->frame = ++s->next_frame; m->started = sys_timer_get_ms(); s->tx_count++; ret = 0;
        }
    }
    os_mutex_post(&ble_mutex); return ret;
}
static struct client_state *client_for_write(void *hdl, uint16_t connection)
{
    int i;
    for (i = 0; i < CLIENTS; ++i) if (clients[i].hdl == hdl && clients[i].connection == connection) return &clients[i];
    for (i = 0; i < CLIENTS; ++i) if (!clients[i].connection && nonce < 0xffffffu) {
        clients[i].hdl = hdl; clients[i].connection = connection;
        clients[i].token = (void *)(uintptr_t)(TOKEN_TAG | ++nonce); return &clients[i];
    }
    return NULL;
}
int zxh_ble_write(void *hdl, uint16_t connection, uint16_t handle, uint16_t mode,
                  uint16_t offset, const uint8_t *data, uint16_t size)
{
    struct client_state *s;
    if (handle != ZXH_BLE_RX_HANDLE && handle != ZXH_BLE_CCC_HANDLE) return 0;
    if (!initialized || !connection || mode || offset || !data ||
        (hdl != rcsp_server_ble_hdl && hdl != rcsp_server_ble_hdl1)) return 1;
    os_mutex_pend(&ble_mutex, 0); s = client_for_write(hdl, connection);
    if (s && handle == ZXH_BLE_CCC_HANDLE && size == 2 && data[1] == 0 && data[0] <= 1) {
        s->subscribed = data[0]; multi_att_set_ccc_config(connection, ZXH_BLE_CCC_HANDLE, data[0]);
        printf("[ZXH_BLE] subscribe con=%u enabled=%u\n", connection, data[0]);
        if (!s->subscribed) {
            int j; s->rx.receiving = 0;
            for (j = 0; j < RX_DEPTH; ++j) { free(s->requests[j]); s->requests[j] = NULL; }
            for (j = 0; j < TX_DEPTH; ++j) { free(s->tx[j].data); memset(&s->tx[j], 0, sizeof(s->tx[j])); }
            s->rx_count = s->rx_head = s->tx_count = s->tx_head = 0;
        }
    } else if (s && s->subscribed && handle == ZXH_BLE_RX_HANDLE) {
        if (zxh_ble_frame_feed(&s->rx, data, size, sys_timer_get_ms()) == 1 &&
            !memchr(s->rx.data, 0, s->rx.total) && s->rx_count < RX_DEPTH) {
            unsigned int slot = (s->rx_head + s->rx_count) % RX_DEPTH;
            s->requests[slot] = malloc(s->rx.total + 1);
            if (s->requests[slot]) { memcpy(s->requests[slot], s->rx.data, s->rx.total + 1); s->rx_count++; }
        }
    }
    os_mutex_post(&ble_mutex); return 1;
}
uint16_t zxh_ble_read(uint16_t connection, uint16_t offset, uint8_t *data, uint16_t size)
{
    uint16_t cfg = multi_att_get_ccc_config(connection, ZXH_BLE_CCC_HANDLE);
    uint8_t bytes[2] = { cfg, cfg >> 8 };
    if (!data) return 2;
    if (offset >= 2) return 0;
    if (size > 2 - offset) size = 2 - offset;
    memcpy(data, bytes + offset, size); return size;
}
static void ble_poll(void *unused)
{
    int i;
    for (i = 0; i < CLIENTS; ++i) {
        struct client_state *s = &clients[i]; char *request = NULL; void *token = NULL, *stale = NULL;
        os_mutex_pend(&ble_mutex, 0);
        if (s->connection && app_ble_get_hdl_con_handle(s->hdl) != s->connection) stale = s->hdl;
        else if (s->subscribed && s->rx_count) {
            request = s->requests[s->rx_head]; s->requests[s->rx_head] = NULL;
            s->rx_head = (s->rx_head + 1) % RX_DEPTH; s->rx_count--; token = s->token;
        }
        os_mutex_post(&ble_mutex);
        if (stale) { zxh_ble_disconnected(stale); continue; }
        /* Parse/dispatch in app_core, not in the Bluetooth write callback. */
        if (request) {
            struct json_object *r = json_tokener_parse(request), *t;
            if (r && json_object_is_type(r, json_type_object)) {
                t = json_object_object_get(r, "topic");
                if (t && json_object_is_type(t, json_type_string)) zxh_vehicle_receive(token, json_object_get_string(t), request);
            }
            if (r) json_object_put(r);
            free(request);
        }
        os_mutex_pend(&ble_mutex, 0);
        if (s->connection && s->subscribed && s->tx_count) {
            struct tx_message *m = &s->tx[s->tx_head]; uint8_t packet[ZXH_BLE_PACKET_MAX];
            unsigned int n = zxh_ble_frame_make(m->data, m->length, m->offset, m->frame, m->index, packet);
            int ret = app_ble_att_send_data(s->hdl, ZXH_BLE_TX_HANDLE, packet, n, ATT_OP_NOTIFY);
            if (!ret) { m->offset += n - ZXH_BLE_HEADER_SIZE; m->index++; }
            if (m->offset == m->length || (uint32_t)(sys_timer_get_ms() - m->started) > 5000u) {
                free(m->data); memset(m, 0, sizeof(*m)); s->tx_head = (s->tx_head + 1) % TX_DEPTH; s->tx_count--;
            }
        }
        os_mutex_post(&ble_mutex);
    }
}
void zxh_ble_profile_ready(void)
{
    if (rcsp_server_ble_hdl) app_ble_att_handle_enable(rcsp_server_ble_hdl, ZXH_BLE_SERVICE_HANDLE, ZXH_BLE_CCC_HANDLE, 1);
    if (rcsp_server_ble_hdl1) app_ble_att_handle_enable(rcsp_server_ble_hdl1, ZXH_BLE_SERVICE_HANDLE, ZXH_BLE_CCC_HANDLE, 1);
    printf("[ZXH_BLE] profile service=0x20 rx=0x22 tx=0x24 ccc=0x25\n");
}
static int zxh_ble_init(void)
{
    if (os_mutex_create(&ble_mutex) || !sys_timer_add(NULL, ble_poll, 20)) return -1;
    initialized = 1; return 0;
}
late_initcall(zxh_ble_init);
#else
#include "zxh_ble.h"
void zxh_ble_profile_ready(void) {}
void zxh_ble_disconnected(void *hdl) {}
int zxh_ble_is_client(void *client) { return 0; }
int zxh_ble_client_active(void *client) { return 0; }
uint16_t zxh_ble_client_connection(void *client) { return 0; }
int zxh_ble_send(void *client, const char *json) { return -1; }
int zxh_ble_write(void *hdl, uint16_t connection, uint16_t handle, uint16_t mode,
                  uint16_t offset, const uint8_t *data, uint16_t size) { return 0; }
uint16_t zxh_ble_read(uint16_t connection, uint16_t offset, uint8_t *data, uint16_t size) { return 0; }
#endif
