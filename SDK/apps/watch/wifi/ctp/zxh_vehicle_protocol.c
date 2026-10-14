#include "app_config.h"
#if TCFG_WIFI_NET_ENABLE || TCFG_USER_BLE_ENABLE
#ifdef ZXH_PROTOCOL_TEST
#include "zxh_vehicle_test_platform.h"
#else
#include "system/includes.h"
#include "server/ctp_server.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "ctp_cmd.h"
#include "zxh_ble.h"
#endif
#include "zxh_vehicle_protocol.h"
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 8
#define CACHE_MS 60000u
static const char *actions[] = { "LOCK", "UNLOCK", "OPEN_SEAT", "CLOSE_SEAT",
    "LIGHT_SET", "FIND", "MODE_SET", "CHARGE_TARGET_SET", "PROXIMITY_SET" };
static OS_MUTEX mutex;
static int initialized;
static uint32_t next_token;
static struct operation {
    struct zxh_request request;
    void *client;
    uint32_t token, started, finished;
    int used, pending, running, code, confirmed;
} cache[CACHE_SIZE];

#ifndef ZXH_PROTOCOL_TEST
__attribute__((weak)) int zxh_vehicle_authorize(void *client, const struct zxh_request *request)
{ return ZXH_EXECUTION_FAILED; }
__attribute__((weak)) int zxh_vehicle_authorize_read(void *client, const char *topic)
{ return ZXH_EXECUTION_FAILED; }
__attribute__((weak)) int zxh_vehicle_submit(uint32_t token, const struct zxh_request *request, int *confirmed)
{ return ZXH_EXECUTION_FAILED; }
__attribute__((weak)) int zxh_vehicle_read_state(struct zxh_state *state)
{ return ZXH_EXECUTION_FAILED; }
__attribute__((weak)) int zxh_vehicle_read_bms(struct zxh_bms *bms)
{ return ZXH_EXECUTION_FAILED; }
#endif

static void field(struct json_object *object, const char *key, const char *value)
{ json_object_object_add(object, key, json_object_new_string(value)); }
static void number(struct json_object *object, const char *key, int value)
{ char text[24]; snprintf(text, sizeof(text), "%d", value); field(object, key, text); }
static struct json_object *result(const char *id, const char *action, const char *phase, int code, int confirmed)
{
    struct json_object *p = json_object_new_object();
    field(p, "v", "1"); field(p, "id", id); field(p, "action", action);
    field(p, "phase", phase); number(p, "code", code); number(p, "confirmed", confirmed);
    return p;
}
static void send_reply(void *client, const char *topic, struct json_object *p)
{
    struct json_object *root = json_object_new_object();
    const char *text;
    char *copy;
    json_object_object_add(root, "errno", json_object_new_int(0));
    field(root, "op", "NOTIFY"); json_object_object_add(root, "param", p);
    field(root, "topic", topic);
    text = json_object_to_json_string(root);
#ifndef ZXH_PROTOCOL_TEST
    if (zxh_ble_is_client(client)) {
        int queued = zxh_ble_send(client, text);
        printf("[ZXH_BLE] reply topic=%s code=%s queued=%d\n", topic,
            json_object_get_string(json_object_object_get(p, "code")), queued);
        json_object_put(root); return;
    }
#endif
#if defined(ZXH_PROTOCOL_TEST) || (TCFG_WIFI_NET_ENABLE && TCFG_ZXH_CTP_COMPAT)
    copy = malloc(strlen(text) + 1);
    if (copy) {
        strcpy(copy, text);
        /* The CTP server takes ownership of a successfully submitted buffer. */
        if (!client || send_ctp_string(CTP_NOTIFY_COMMAND, copy, topic, client)) free(copy);
    }
#endif
    json_object_put(root);
}
static const char *string(struct json_object *p, const char *key)
{
    struct json_object *v = json_object_object_get(p, key);
    return v && json_object_is_type(v, json_type_string) ? json_object_get_string(v) : "";
}
static int safe_id(const char *id)
{
    unsigned int n = strlen(id), i;
    if (!n || n >= ZXH_ID_SIZE) return 0;
    for (i = 0; i < n; ++i) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
    }
    return 1;
}
static int parse_value(const char *text, enum zxh_action action, int *value)
{
    unsigned int i, n = strlen(text);
    long v;
    if (!n || n > 5) return 0;
    for (i = 0; i < n; ++i) if (text[i] < '0' || text[i] > '9') return 0;
    v = strtol(text, NULL, 10); *value = (int)v;
    switch (action) {
    case ZXH_LIGHT_SET: case ZXH_PROXIMITY_SET: return v <= 1;
    case ZXH_FIND: return v >= 1000 && v <= 10000;
    case ZXH_MODE_SET: return v <= 2;
    case ZXH_CHARGE_TARGET_SET: return v >= 70 && v <= 100;
    default: return v == 0;
    }
}
static void finish(struct operation *op, int code, int confirmed)
{
    if (code < 0 || (code != ZXH_OK && code < ZXH_VERSION_ERROR)) code = ZXH_EXECUTION_FAILED;
    if (code == ZXH_OK && !confirmed) code = ZXH_UNCONFIRMED;
    op->pending = 0; op->code = code; op->confirmed = code == ZXH_OK;
    op->finished = sys_timer_get_ms();
    if (op->client) send_reply(op->client, "ZXH_CONTROL", result(op->request.id,
        actions[op->request.action], "result", op->code, op->confirmed));
}
void zxh_vehicle_complete(uint32_t token, int code, int confirmed)
{
    int i;
    if (!initialized) return;
    os_mutex_pend(&mutex, 0);
    for (i = 0; i < CACHE_SIZE; ++i) if (cache[i].used && cache[i].running && cache[i].token == token) {
        cache[i].running = 0;
        if (cache[i].pending) {
            if ((uint32_t)(sys_timer_get_ms() - cache[i].started) >= ZXH_EXECUTION_MS)
                finish(&cache[i], ZXH_EXECUTION_TIMEOUT, 0);
            else finish(&cache[i], code, confirmed);
        } else cache[i].finished = sys_timer_get_ms();
        break;
    }
    os_mutex_post(&mutex);
}
void zxh_vehicle_disconnect(void *client)
{
    int i;
    if (!initialized) return;
    os_mutex_pend(&mutex, 0);
    /* Preserve deduplication across reconnects; never dereference a freed client. */
    for (i = 0; i < CACHE_SIZE; ++i) if (cache[i].client == client) cache[i].client = NULL;
    os_mutex_post(&mutex);
}
static void protocol_poll(void *unused)
{
    int i;
    uint32_t now = sys_timer_get_ms();
    os_mutex_pend(&mutex, 0);
    for (i = 0; i < CACHE_SIZE; ++i) {
        struct operation *op = &cache[i];
        if (!op->used) continue;
        if (op->pending && (uint32_t)(now - op->started) >= ZXH_EXECUTION_MS)
            finish(op, ZXH_EXECUTION_TIMEOUT, 0);
        else if (!op->pending && !op->running && (uint32_t)(now - op->finished) >= CACHE_MS) op->used = 0;
    }
    os_mutex_post(&mutex);
}
static int receive(void *client, char *content, const char *topic)
{
    struct json_object *root = json_tokener_parse(content), *p;
    const char *id = "", *action = "";
    int code = ZXH_PARAMETER_ERROR, value = 0, a, i, free_slot = -1;
    struct operation *op;
    if (!root || !json_object_is_type(root, json_type_object)) goto reject;
    p = json_object_object_get(root, "param");
    if (!p || !json_object_is_type(p, json_type_object)) goto reject;
    id = string(p, "id"); action = string(p, "action");
    if (!safe_id(id)) { id = ""; action = ""; goto reject; }
    if (strcmp(string(root, "op"), "PUT")) goto reject;
    if (strcmp(string(p, "v"), "1")) { code = ZXH_VERSION_ERROR; goto reject; }
    if (!initialized) { code = ZXH_EXECUTION_FAILED; goto reject; }
#ifndef ZXH_PROTOCOL_TEST
    if (zxh_ble_is_client(client) && !zxh_ble_client_active(client)) { json_object_put(root); return 0; }
#endif
    if (strcmp(topic, "ZXH_CONTROL")) {
        if (*action) goto reject;
        struct json_object *reply = result(id, "", "result", ZXH_OK, 0);
        os_mutex_pend(&mutex, 0);
        if (!strcmp(topic, "ZXH_CAPS")) {
            field(reply, "actions", "LOCK|UNLOCK|OPEN_SEAT|CLOSE_SEAT|LIGHT_SET|FIND|MODE_SET|CHARGE_TARGET_SET|PROXIMITY_SET");
            field(reply, "execution_ms", "8000"); field(reply, "dedup_ms", "60000");
        } else if (!strcmp(topic, "ZXH_STATE")) {
            struct zxh_state s;
            memset(&s, 0xff, sizeof(s)); code = zxh_vehicle_authorize_read(client, topic);
            if (code == ZXH_OK) code = zxh_vehicle_read_state(&s);
            if (code == ZXH_OK) {
                number(reply,"locked",s.locked); number(reply,"seat_open",s.seat_open);
                number(reply,"light_on",s.light_on); number(reply,"mode",s.mode);
                number(reply,"proximity",s.proximity); number(reply,"charge_target",s.charge_target);
                number(reply,"speed_dkph",s.speed_dkph); number(reply,"odometer_m",s.odometer_m); number(reply,"range_m",s.range_m);
            }
        } else {
            struct zxh_bms b;
            memset(&b, 0xff, sizeof(b)); b.current_ma = (-2147483647 - 1); b.temperature_dc = -32768;
            code = zxh_vehicle_authorize_read(client, topic);
            if (code == ZXH_OK) code = zxh_vehicle_read_bms(&b);
            if (code == ZXH_OK) {
                number(reply,"soc",b.soc); number(reply,"voltage_mv",b.voltage_mv);
                number(reply,"current_ma",b.current_ma); number(reply,"temperature_dc",b.temperature_dc);
                number(reply,"charging",b.charging); number(reply,"cycles",b.cycles);
            }
        }
        if (strcmp(topic, "ZXH_CAPS")) number(reply, "code", code < 0 ? ZXH_EXECUTION_FAILED : code);
        send_reply(client, topic, reply); os_mutex_post(&mutex);
        json_object_put(root); return 0;
    }
    for (a = 0; a < ZXH_ACTION_COUNT; ++a) if (!strcmp(action, actions[a])) break;
    if (a == ZXH_ACTION_COUNT) { code = ZXH_ACTION_ERROR; if (strlen(action) > 24) action = ""; goto reject; }
    if (!parse_value(string(p, "value"), (enum zxh_action)a, &value)) goto reject;
    os_mutex_pend(&mutex, 0);
    for (i = 0; i < CACHE_SIZE; ++i) {
        op = &cache[i];
        if (op->used && !op->pending && !op->running && (uint32_t)(sys_timer_get_ms() - op->finished) >= CACHE_MS) op->used = 0;
        if (!op->used) { free_slot = i; continue; }
        if (!strcmp(op->request.id, id)) {
            code = op->request.action != a || op->request.value != value ? ZXH_ID_CONFLICT : op->code;
            /* A pending replay only acknowledges, and never submits the action twice. */
            send_reply(client, topic, result(id, action, code == ZXH_ID_CONFLICT || !op->pending ? "result" : "ack",
                code == ZXH_ID_CONFLICT ? code : op->pending ? ZXH_OK : code,
                code == ZXH_ID_CONFLICT ? 0 : op->confirmed));
            if (code != ZXH_ID_CONFLICT) op->client = client;
            os_mutex_post(&mutex); json_object_put(root); return 0;
        }
    }
    /* Serialize physical mutations and retain all terminal IDs for the full TTL. */
    code = free_slot < 0 ? ZXH_BUSY : ZXH_OK;
    for (i = 0; i < CACHE_SIZE; ++i) if (cache[i].used && (cache[i].pending || cache[i].running)) code = ZXH_BUSY;
    if (code) {
        send_reply(client, topic, result(id, action, "result", code, 0));
        os_mutex_post(&mutex); json_object_put(root); return 0;
    }
    op = &cache[free_slot]; memset(op, 0, sizeof(*op));
    op->used = op->pending = 1; op->client = client; op->started = sys_timer_get_ms();
    op->token = ++next_token; if (!op->token) op->token = ++next_token;
    strcpy(op->request.id, id); op->request.action = (enum zxh_action)a; op->request.value = value;
    send_reply(client, topic, result(id, action, "ack", ZXH_OK, 0));
    code = zxh_vehicle_authorize(client, &op->request);
    if (code == ZXH_OK) code = zxh_vehicle_submit(op->token, &op->request, &op->confirmed);
    else if (code < 0) code = ZXH_EXECUTION_FAILED;
    if (code == ZXH_PENDING) op->running = 1;
    else finish(op, code, op->confirmed);
    os_mutex_post(&mutex); json_object_put(root); return 0;
reject:
    send_reply(client, topic, result(id, action, "result", code, 0));
    if (root) json_object_put(root);
    return 0;
}
int zxh_vehicle_receive(void *client, const char *topic, char *json)
{
    if (strcmp(topic, "ZXH_CAPS") && strcmp(topic, "ZXH_CONTROL") && strcmp(topic, "ZXH_STATE") && strcmp(topic, "ZXH_BMS")) return -1;
    return receive(client, json, topic);
}
#if TCFG_ZXH_CTP_COMPAT
static int caps(void *p, char *s) { return receive(p, s, "ZXH_CAPS"); }
static int control_cb(void *p, char *s) { return receive(p, s, "ZXH_CONTROL"); }
static int state(void *p, char *s) { return receive(p, s, "ZXH_STATE"); }
static int bms(void *p, char *s) { return receive(p, s, "ZXH_BMS"); }
/* PUT-only prevents legacy bootstrap's automatic GET walk from issuing RPCs. */
const struct ctp_map_entry zxh_vehicle_commands[] SEC_USED(.ctp_screen_mirror_cmd.zxh) = {
    { NULL, "ZXH_CAPS", NULL, caps }, { NULL, "ZXH_CONTROL", NULL, control_cb },
    { NULL, "ZXH_STATE", NULL, state }, { NULL, "ZXH_BMS", NULL, bms }
};
#endif
static int zxh_protocol_init(void)
{
    if (os_mutex_create(&mutex)) return -1;
    if (!sys_timer_add(NULL, protocol_poll, 200)) return -1;
    initialized = 1; return 0;
}
late_initcall(zxh_protocol_init);
#endif
