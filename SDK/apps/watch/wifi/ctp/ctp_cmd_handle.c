#include "app_config.h"

#if TCFG_WIFI_NET_ENABLE

#include "ctp_cmd.h"
#include "server/ctp_server.h"
#include "ctp_cmd.h"
#include "event.h"
#include "ctp_cmd_handle.h"

#define LOG_TAG_CONST       CTP_CMD
#define LOG_TAG             "[CTP_CMD]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#define CTP_SERVER_TASK_NAME       "ctp_server_msg_handler"
#define CTP_CTRL_PORT   	3333
#define CDP_CTRL_PORT   	2228

static void net_msg_handler(struct sys_event *event);

static struct ctp_server_info g_server_info = {
    .ctp_vaild = true,
    .ctp_port = CTP_CTRL_PORT,
    .cdp_vaild = true,
    .cdp_port = CDP_CTRL_PORT,
    .k_alive_type = NOT_USE_ALIVE,
    /*.k_alive_type = CDP_ALIVE,*/
};

struct ctp_cmd_handler {
    struct server *ctp_server;
    int server_msg_handler_pid;
    int server_msg_handler_exit;
};

static struct ctp_cmd_handler g_ctp_hdl;

static void ctp_server_msg_handler(void *priv);

int ctp_init(u8 use_alive)
{
    int ret = 0;
    if (use_alive) {
        g_server_info.k_alive_type = CTP_ALIVE;
    } else {
        g_server_info.k_alive_type = NOT_USE_ALIVE;
    }
    g_ctp_hdl.ctp_server = server_open("ctp_server", (void *)&g_server_info);
    if (!g_ctp_hdl.ctp_server) {
        log_error("ctp server open err \n");
        return -1;
    }

    register_sys_event_handler(SYS_NET_EVENT, 0, 0, net_msg_handler);

    ret = thread_fork(CTP_SERVER_TASK_NAME, 15, 1024, 512,
                      &g_ctp_hdl.server_msg_handler_pid,
                      ctp_server_msg_handler, &g_ctp_hdl);
    if (ret) {
        log_error("ctp server msg handler thread fork err \n");
        server_close(g_ctp_hdl.ctp_server);
        g_ctp_hdl.ctp_server = NULL;
        return -1;
    }

    //ctp_cli_init();

    return 0;
}

int ctp_is_init(void)
{
    return g_ctp_hdl.ctp_server != NULL;
}

int ctp_exit(void)
{
    if (g_ctp_hdl.ctp_server) {
        server_close(g_ctp_hdl.ctp_server);
        g_ctp_hdl.ctp_server = NULL;
    }

    if (g_ctp_hdl.server_msg_handler_pid) {
        os_taskq_post_type(CTP_SERVER_TASK_NAME, Q_USER, 0, NULL);
        thread_kill(&g_ctp_hdl.server_msg_handler_pid, KILL_WAIT);
    }

    //ctp_cli_uninit();

    return 0;
}

static void ctp_server_msg_handler(void *priv)
{
    struct ctp_cmd_handler *hdl = (struct ctp_cmd_handler *)priv;
    int res;
    int msg[8];
    hdl->server_msg_handler_exit = 0;
    log_debug("ctp server msg handler start\n");

    while (!hdl->server_msg_handler_exit) {
        res = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_MSG:
                struct ctp_arg *ctp_arg = (struct ctp_arg *)msg[1];
                if (!ctp_arg) {
                    log_error("ctp server msg handler ctp_arg null\n");
                    break;
                }
                void *cli = ctp_arg->cli;
                char *topic = ctp_arg->topic;
                char *content = ctp_arg->content;
                log_debug("ctp server msg handler recv topic:%s, content:%s cli:%p\n", topic, content, cli);
                ctp_cmd_analysis(topic, content, cli);
                if (content) {
                    free(content);
                }
                free(ctp_arg);
                break;
            case Q_USER:
                //kill task
                hdl->server_msg_handler_exit = 1;
                break;
            default:
                log_debug("ctp server msg handler recv msg:%d\n", msg[0]);
                break;
            }
            break;
        default:
            break;
        }
    }

    log_debug("ctp server msg handler exit\n");
}



static void net_msg_handler(struct sys_event *event)
{
    int msg[2];
    struct net_event *net_event = (struct net_event *)event->payload;
    struct ctp_arg *event_arg = (struct ctp_arg *)net_event->arg;

    switch (net_event->event) {
    case NET_EVENT_CMD:
        log_debug("IN NET_EVENT_CMD Topic:%s\n", (char *)event_arg->topic);
        msg[0] = (int)event_arg;
        os_taskq_post_type(CTP_SERVER_TASK_NAME, Q_MSG, ARRAY_SIZE(msg), msg);
        break;
    default:
        break;
    }
}

#endif
