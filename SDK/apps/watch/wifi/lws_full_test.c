#include "system/includes.h"

#if TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE
#include "os/os_api.h"
#include <libwebsockets.h>
#include <mbedtls/certs.h>

size_t xPortGetMinimumEverFreeHeapSize(void);

#define LWS_FULL_TEST_TIMEOUT_MS 15000
#define LWS_FULL_TEST_SERVICE_MS 50
#define LWS_FULL_TEST_LOCAL_PORT 7681

uint32_t timer_get_ms(void);
struct lws_full_test_case {
    const char *name;
    const char *address;
    const char *path;
    const char *host;
    const char *origin;
    const char *protocol;
    const char *method;
    int port;
    int ssl;
    int done;
    int ok;
    int sent;
    struct lws *wsi;
};

static volatile int lws_full_test_started;
static char lws_full_server_echo[64];
static int lws_full_server_echo_len;

static int lws_full_ws_cb(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len)
{
    unsigned char buf[LWS_PRE + 64];
    struct lws_full_test_case *tc = (struct lws_full_test_case *)lws_get_opaque_user_data(wsi);

    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("lws test %s: ws established\n", tc ? tc->name : "?");
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if (tc && !tc->sent) {
            const char *msg = "wl82 lws 4.5.8 smoke";
            int n = strlen(msg);
            memcpy(&buf[LWS_PRE], msg, n);
            tc->sent = 1;
            if (lws_write(wsi, &buf[LWS_PRE], n, LWS_WRITE_TEXT) < 0) {
                tc->done = 1;
            }
        }
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
        printf("lws test %s: ws recv %d\n", tc ? tc->name : "?", (int)len);
        if (tc) {
            tc->ok = 1;
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("lws test %s: ws error %s\n", tc ? tc->name : "?", in ? (char *)in : "");
        if (tc) {
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_CLIENT_CLOSED:
        if (tc) {
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_ESTABLISHED:
        puts("lws test local-server: ws established\n");
        break;
    case LWS_CALLBACK_RECEIVE:
        printf("lws test local-server: ws recv %d\n", (int)len);
        if (len > sizeof(lws_full_server_echo)) {
            len = sizeof(lws_full_server_echo);
        }
        memcpy(lws_full_server_echo, in, len);
        lws_full_server_echo_len = (int)len;
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (lws_full_server_echo_len > 0) {
            unsigned char echo[LWS_PRE + sizeof(lws_full_server_echo)];
            memcpy(&echo[LWS_PRE], lws_full_server_echo, lws_full_server_echo_len);
            lws_write(wsi, &echo[LWS_PRE], lws_full_server_echo_len, LWS_WRITE_TEXT);
            lws_full_server_echo_len = 0;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int lws_full_http_cb(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    struct lws_full_test_case *tc = (struct lws_full_test_case *)lws_get_opaque_user_data(wsi);

    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
        printf("lws test %s: http connected\n", tc ? tc->name : "?");
        break;
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
        char buf[LWS_PRE + 512];
        char *p = &buf[LWS_PRE];
        int n = sizeof(buf) - LWS_PRE;

        if (lws_http_client_read(wsi, &p, &n) < 0) {
            if (tc) {
                tc->done = 1;
            }
            return -1;
        }
        break;
    }
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
        printf("lws test %s: http body %d\n", tc ? tc->name : "?", (int)len);
        if (tc) {
            tc->ok = 1;
        }
        break;
    case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        printf("lws test %s: http complete\n", tc ? tc->name : "?");
        if (tc) {
            tc->ok = 1;
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("lws test %s: http error %s\n", tc ? tc->name : "?", in ? (char *)in : "");
        if (tc) {
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
    case LWS_CALLBACK_CLIENT_CLOSED:
        if (tc) {
            tc->done = 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static int lws_full_mqtt_cb(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    struct lws_full_test_case *tc = (struct lws_full_test_case *)lws_get_opaque_user_data(wsi);
    static lws_mqtt_topic_elem_t topic;
    static lws_mqtt_subscribe_param_t sub;
    static lws_mqtt_publish_param_t pub;
    static const char payload[] = "wl82 lws mqtt smoke";

    switch (reason) {
    case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED:
        printf("lws test %s: established\n", tc ? tc->name : "mqtt");
        topic.name = "jl/wl82/lws/test";
        topic.qos = QOS0;
        sub.num_topics = 1;
        sub.topic = &topic;
        lws_mqtt_client_send_subcribe(wsi, &sub);
        break;
    case LWS_CALLBACK_MQTT_SUBSCRIBED:
        printf("lws test %s: subscribed\n", tc ? tc->name : "mqtt");
        memset(&pub, 0, sizeof(pub));
        pub.topic = "jl/wl82/lws/test";
        pub.topic_len = strlen(pub.topic);
        pub.payload = payload;
        pub.payload_len = strlen(payload);
        pub.qos = QOS0;
        lws_mqtt_client_send_publish(wsi, &pub, payload, strlen(payload), LWS_MQTT_FINAL_PART);
        if (tc) {
            tc->ok = 1;
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_MQTT_CLIENT_CLOSED:
        if (tc) {
            tc->done = 1;
        }
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("lws test %s: error %s\n", tc ? tc->name : "mqtt", in ? (char *)in : "");
        if (tc) {
            tc->done = 1;
        }
        break;
    default:
        break;
    }

    return 0;
}

static const struct lws_protocols lws_full_protocols[] = {
    { "lws-full-ws", lws_full_ws_cb, 0, 512 },
    { "lws-full-http", lws_full_http_cb, 0, 512 },
    { "mqtt", lws_full_mqtt_cb, 0, 512 },
    LWS_PROTOCOL_LIST_TERM
};

static int lws_full_run_case(struct lws_context *context, struct lws_full_test_case *tc)
{
    struct lws_client_connect_info ccinfo;
    u32 start = timer_get_ms();

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = tc->address;
    ccinfo.port = tc->port;
    ccinfo.path = tc->path;
    ccinfo.host = tc->host ? tc->host : tc->address;
    ccinfo.origin = tc->origin ? tc->origin : ccinfo.host;
    ccinfo.protocol = tc->protocol;
    ccinfo.local_protocol_name = tc->protocol;
    ccinfo.method = tc->method;
    ccinfo.ssl_connection = tc->ssl;
    if (tc->ssl) {
        ccinfo.alpn = "http/1.1";
    }
    ccinfo.opaque_user_data = tc;
    ccinfo.pwsi = &tc->wsi;

    tc->done = 0;
    tc->ok = 0;
    tc->sent = 0;
    tc->wsi = lws_client_connect_via_info(&ccinfo);
    if (!tc->wsi) {
        printf("lws test %s: connect create failed\n", tc->name);
        return -1;
    }

    while (!tc->done && timer_get_ms() - start < LWS_FULL_TEST_TIMEOUT_MS) {
        lws_service(context, LWS_FULL_TEST_SERVICE_MS);
        os_time_dly(1);
    }

    printf("lws test %s: %s\n", tc->name, tc->ok ? "ok" : "fail");

    return tc->ok ? 0 : -1;
}

static int lws_full_run_mqtt_case(struct lws_context *context, struct lws_full_test_case *tc)
{
    struct lws_client_connect_info ccinfo;
    lws_mqtt_client_connect_param_t mqtt_cp;
    char mqtt_client_id[32];
    u32 start = timer_get_ms();

    memset(&mqtt_cp, 0, sizeof(mqtt_cp));
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "jl-%s-%08x", tc->name, timer_get_ms());
    mqtt_cp.client_id = mqtt_client_id;
    mqtt_cp.keep_alive = 30;
    mqtt_cp.clean_start = 1;
    mqtt_cp.client_id_nofree = 1;

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = tc->address;
    ccinfo.port = tc->port;
    ccinfo.path = tc->path;
    ccinfo.host = tc->host ? tc->host : tc->address;
    ccinfo.protocol = tc->protocol;
    ccinfo.local_protocol_name = tc->protocol;
    ccinfo.method = tc->method;
    ccinfo.ssl_connection = tc->ssl;
    ccinfo.opaque_user_data = tc;
    ccinfo.mqtt_cp = &mqtt_cp;
    ccinfo.pwsi = &tc->wsi;

    tc->done = 0;
    tc->ok = 0;
    tc->sent = 0;
    tc->wsi = lws_client_connect_via_info(&ccinfo);
    if (!tc->wsi) {
        printf("lws test %s: connect create failed\n", tc->name);
        return -1;
    }

    while (!tc->done && timer_get_ms() - start < LWS_FULL_TEST_TIMEOUT_MS) {
        lws_service(context, LWS_FULL_TEST_SERVICE_MS);
        os_time_dly(1);
    }

    printf("lws test %s: %s\n", tc->name, tc->ok ? "ok" : "fail");

    return tc->ok ? 0 : -1;
}


static int lws_full_run_local_server_case(void)
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws_full_test_case tc = {
        "local-ws-server", "127.0.0.1", "/", "127.0.0.1", "http://jl.com", "lws-full-ws", NULL,
        LWS_FULL_TEST_LOCAL_PORT, 0
    };
    int ret;

    memset(&info, 0, sizeof(info));
    info.port = LWS_FULL_TEST_LOCAL_PORT;
    info.protocols = lws_full_protocols;

    context = lws_create_context(&info);
    if (!context) {
        puts("lws test local-server: create context failed\n");
        return -1;
    }

    ret = lws_full_run_case(context, &tc);
    lws_context_destroy(context);

    return ret;
}

static void lws_full_test_task(void *priv)
{
    os_time_dly(100);
    struct lws_context_creation_info info;
    struct lws_context *context;
    int pass = 0;
    int total = 0;
    int heap_start = xPortGetMinimumEverFreeHeapSize();

    /* lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | */
    /* LLL_DEBUG | LLL_PARSER | LLL_HEADER | LLL_CLIENT, */
    /* NULL); */
    printf("lws test: start heap %d\n", heap_start);

    total++;
    if (!lws_full_run_local_server_case()) {
        pass++;
    }

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = lws_full_protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.client_ssl_ca_mem = mbedtls_test_cas_pem;
    info.client_ssl_ca_mem_len = mbedtls_test_cas_pem_len;

    context = lws_create_context(&info);
    if (!context) {
        printf("lws test: create context failed\n");
        lws_full_test_started = 0;
        return;
    }

    struct lws_full_test_case tests[] = {
        { "http", "example.com", "/", "example.com", NULL, "lws-full-http", "GET", 80, 0 },
        {
            "https", "ws.postman-echo.com", "/raw", "ws.postman-echo.com", NULL, "lws-full-http", "GET", 443,
            LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
        },
        {
            "wss", "ws.postman-echo.com", "/raw", "ws.postman-echo.com", "http://jl.com", "lws-full-ws", NULL, 443,
            LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
        },
    };

    for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
        total++;
        if (!lws_full_run_case(context, &tests[i])) {
            pass++;
        }
    }

    struct lws_full_test_case mqtt_tests[] = {
        { "mqtt", "broker-cn.emqx.io", "/", "broker-cn.emqx.io", NULL, "mqtt", "MQTT", 1883, 0 },
        {
            "mqtts", "broker-cn.emqx.io", "/", "broker-cn.emqx.io", NULL, "mqtt", "MQTT", 8883,
            LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
        },
    };

    for (int i = 0; i < (int)(sizeof(mqtt_tests) / sizeof(mqtt_tests[0])); i++) {
        total++;
        if (!lws_full_run_mqtt_case(context, &mqtt_tests[i])) {
            pass++;
        }
    }

    lws_context_destroy(context);
    printf("lws test summary: %d/%d passed, heap %d -> %d\n",
           pass, total, heap_start, (int)xPortGetMinimumEverFreeHeapSize());
    lws_full_test_started = 0;
}

int lws_full_test_start(void)
{
    if (lws_full_test_started) {
        puts("lws test: already running\n");
        return 0;
    }

    lws_full_test_started = 1;

    return thread_fork("lws_full_test", 10, 8 * 1024, 0, NULL, lws_full_test_task, NULL);
}

#endif
