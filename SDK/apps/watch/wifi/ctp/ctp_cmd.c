#include "app_config.h"

#if TCFG_WIFI_NET_ENABLE
#include "server/ctp_server.h"
#include "server/net_server.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "server/server_core.h"
#include "fs/fs.h"
#include "http/http_server.h"
#include "ctp.h"
#include "cdp.h"
#include "os/os_api.h"
#include "screen_mirror.h"
#include "zxh_vehicle_protocol.h"

#define CTP_CMD_HEADER "{\"errno\":%d,\"op\":\"%s\",\"param\":{"
#define CTP_CMD_HEADER_WITHOUT_ERR "{\"op\":\"%s\",\"param\":{"
#define CTP_CMD_END "}}"
#define CTP_ERR_MESSAGE "{\"errno\":%d}"

extern bool dev_online(const char *name);

static struct ctp_arg info;

int send_ctp_string(int cmd_type, char *buf, const char *_req, void *priv)
{
    struct ctp_req req;
    struct server *ctp = NULL;

    ctp = get_ctp_server_hander();
    if (ctp == NULL) {
        return -1;
    }

    req.parm = buf;
    req.topic = _req;
    req.cli = priv;
    if (!priv) {
        req.cli = info.cli;
    }
    printf("%s: buf:%s ", __func__, buf);
    if (server_request(ctp, cmd_type, (void *)&req)) {
        return -1;
    }

    return 0;
}

static inline int _CTP_CMD_COMBINED(int cmd_type, void *priv, u32 err, const char *_req, const char *mothod, char *str)
{
    char *buf = NULL;
    int ret;
    int id = 0;
    char tmp[64];
    char *tmp1 = NULL;
    buf = (char *)malloc(512);

    if (buf == NULL) {
        printf("%s %d mem is fail \n", __func__, __LINE__);
        return -1;
    }

    if (err) {
        ret = snprintf(buf, 512, CTP_CMD_HEADER, err, mothod);
    } else {
        ret = snprintf(buf, 512, CTP_CMD_HEADER_WITHOUT_ERR, mothod);
    }

    char *key;
    char *value;

    if (!err && str != NULL) {
        while (1) {
            key = strtok_r(str, ":", &tmp1); //这函数会改数组，不能用const
            value = strtok_r(NULL, ",", &tmp1);

            if (key == NULL || value == NULL) {
                break;
            }

            ret = sprintf(tmp, "\"%s\":\"%s\",", key, value);
            tmp[ret] = '\0';
            ret = snprintf(buf, 512, "%s%s", buf, tmp);
            str = NULL;
        }

        buf[ret - 1] = '\0';//主要是去掉最后的逗号
        ret = snprintf(buf, 512, "%s%s", buf, CTP_CMD_END);
    } else {
        snprintf(buf, 512, "{\"op\":\"NOTIFY\",\"errno\":%d,\"param\":{%s}}", err, str);
    }

    void *cli = NULL;
    if (!strcmp(_req, info.topic) || !priv) {
        cli = info.cli;
    } else {
        cli = priv;
    }


    if (send_ctp_string(cmd_type, buf, _req, cli) < 0) {
        free(buf);
        return -1;
    }
    return 0;
}

int CTP_CMD_COMBINED(void *priv, u32 err, const char *_req, const char *mothod, char *str)
{
    return _CTP_CMD_COMBINED(CTP_NOTIFY_COMMAND, priv, err, _req, mothod, str);
}

void ctp_all_get_cmd_run(void *priv, char *content)
{
    const struct ctp_map_entry *map = NULL;
    list_for_ctp_mapping_tab(map) {
        if (map->get != NULL) {
            map->get(priv, content);    //执行map中所有get命令
        }
    }
}

int ctp_cmd_analysis(const char *topic, char *content, void *priv)
{
    struct ctp_map_entry *map = NULL;
    char buf[128];
    int ret = -1;

    if (strlen(topic) <= 0 || strlen(content) <= 0) {
        printf("%s  %d err....\n", __func__, __LINE__);
        return -1;
    }

    strcpy(info.topic, topic);
    info.content = NULL;
    info.cli = priv;

    list_for_ctp_mapping_tab(map) {
        if (!strcmp(topic, map->ctp_command)) {
            if (strstr(content, "PUT") && map->put != NULL) {
                ret =  map->put(priv, content);
            } else if (strstr(content, "GET") && map->get != NULL) {
                ret = map->get(priv, content);
            } else if (strstr(content, "NOTIFY") && map->notify != NULL) {
                ret = map->notify(priv, content);
            } else {
                puts("content is error \n\n");
            }

        }

        if (ret != -1) {
            return ret;
        }
    }

    CTP_CMD_COMBINED(priv, CTP_NO_TOPIC, topic, "NOTIFY", CTP_NO_TOPIC_MSG);
    printf("%s %d topic:%s not find it or cb is NULL\n", __func__, __LINE__, topic);
    return -1;

}


static int cmd_put_ctp_cli_connected(void *priv, char *content)
{
    return 0;
}

static int cmd_put_ctp_cli_disconnect(void *priv, char *content)
{
    zxh_vehicle_disconnect(priv);
    char buf[32];
    struct sockaddr_in *dest_addr;
    struct ctp_map_entry *map;
    dest_addr = ctp_srv_get_cli_addr(priv);
    if (!dest_addr) {
        dest_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("app_accept_num = %d \n", info.num);
    if (info.num > 0) {
        info.num--;
        if (info.num > 0) {
            ctp_srv_free_cli(priv);
            /* cdp_srv_free_cli(priv); */
            return 0;
        }
    } else {
        return 0;
    }
    info.dest_addr = NULL;
    info.cli = NULL;

    /*extern void stream_media_server_ip_close(struct sockaddr_in * dest_addr);*/
    /*extern void stupid_ftpd_disconnect_cli(struct sockaddr_in * dest_addr);*/
    //ctp_cmd_socket_unregister(priv);
    printf("|CLI_DISCONNECT 0x%x, 0x%x\n", (u32)priv, (u32)dest_addr->sin_addr.s_addr);
#ifdef CONFIG_ENABLE_VLIST
    video_preview_and_thus_disconnect(dest_addr);
    playback_disconnect_cli(dest_addr);
#endif

#if TCFG_NET_SCR_ENABLE
    int net_scr_ctp_disconnect_event_handle(u8 cli_num);
    net_scr_ctp_disconnect_event_handle(info.num);
#endif
    http_get_server_discpnnect_cli(dest_addr);
    strcpy(buf, "status:1");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "CTP_CLI_DISCONNECT", "NOTIFY", buf);
    ctp_srv_free_cli(priv);
    /* cdp_srv_free_cli(priv); */

    return 0;
}

static int cmd_put_video_app_access(void *priv, char *content)
{
    char buf[64];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *key = NULL;
    int type_num;
    void *addr = NULL;

    addr = (void *)ctp_srv_get_cli_addr(priv);
    if (!addr) {
        addr = (void *)cdp_srv_get_cli_addr(priv);
    }
    if (addr != info.dest_addr) {
        info.num++;
    }

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    key =  json_object_object_get(parm, "type");

    const char *type = json_object_get_string(key);
    key =  json_object_object_get(parm, "ver");
    const char *ver = json_object_get_string(key);

    snprintf(buf, sizeof(buf), "type:%s,ver:%s", type, ver);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "APP_ACCESS", "NOTIFY", buf);

    //app_access命令完成后，随后发送所有get命令
    ctp_all_get_cmd_run(priv, content);

    json_object_put(new_obj);

    info.dest_addr = addr;
    info.cli = priv;

    return 0;
}

static int ctp_get_storage_info(u32 *total_kb, u32 *free_kb)
{
    char mnt_name[16] = {0};
#if TCFG_SD0_ENABLE
    sprintf(mnt_name, "%s", "mnt/sd0");
#elif TCFG_SD1_ENABLE
    sprintf(mnt_name, "%s", "mnt/sd1");
#else
#endif

    struct vfs_partition *p = fget_partition(CONFIG_ROOT_PATH);
    if (!p) {
        p = fget_partition(mnt_name);
    }
    if (!p) {
        return -1;
    }
    *total_kb = p->total_size;
    if (fget_free_space(CONFIG_ROOT_PATH, free_kb) != 0) {
        fget_free_space(mnt_name, free_kb);
    }
    return 0;
}

static int cmd_get_sd_status(void *priv, char *content)
{
    char buf[16];
    u32 total_kb = 0, free_kb = 0;
    int ret = ctp_get_storage_info(&total_kb, &free_kb);

    // snprintf(buf, sizeof(buf), "online:%d", ret == 0 ? 1 : 0);
    snprintf(buf, sizeof(buf), "online:1");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "SD_STATUS", "NOTIFY", buf);

    return 0;
}

static int cmd_get_sd_size(void *priv, char *content)
{
    char buf[32];
    u32 total_kb = 0, free_kb = 0;
    int ret = ctp_get_storage_info(&total_kb, &free_kb);

    if (ret != 0) {
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "TF_CAP", "NOTIFY", CTP_SD_OFFLINE_MSG);
    } else {
        snprintf(buf, sizeof(buf), "left:%u,total:%u", free_kb, total_kb);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TF_CAP", "NOTIFY", buf);
    }

    return 0;
}

static int cmd_get_keep_alive_interval(void *priv, char *content)
{
    int timeout;
    char buf[16];
    //分解content字段
    timeout = ctp_srv_get_keep_alive_timeout();
    snprintf(buf, sizeof(buf), "timeout:%d", timeout);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "KEEP_ALIVE_INTERVAL", "NOTIFY", buf);


    return 0;

}

__attribute__((weak)) int http_create_passive_update_task(const char *url)
{
    return -1;
}

static int cmd_get_ota_request(void *priv, char *content)
{
    char buf[32];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;

    printf("ota req content: %s \n ", content);
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "url");
    const char *url = json_object_get_string(tmp);

    if (url == NULL || url[0] == '\0') {
        printf("url is null \n");
        json_object_put(new_obj);
        return 0;
    }
    printf("url : %s\n", url);
    int ret = http_create_passive_update_task(url);
    snprintf(buf, sizeof(buf), "allow:%d", ret == 0);  //新建成功或复用同一升级任务
    printf("OTA_REQUEST  GET\n");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "OTA_REQUEST", "NOTIFY", buf);  //回复allow
    json_object_put(new_obj);  //下载任务已经复制URL

    return 0;
}


const struct ctp_map_entry ctp_system_cmd_tab[] SEC_USED(.ctp_system_cmd) = {
    {NULL, "CTP_CLI_CONNECTED", NULL, cmd_put_ctp_cli_connected},
    {NULL, "CTP_CLI_DISCONNECT", NULL, cmd_put_ctp_cli_disconnect},
    {NULL, "APP_ACCESS", NULL, cmd_put_video_app_access},
    {NULL, "SD_STATUS", cmd_get_sd_status, NULL},
    {NULL, "TF_CAP", cmd_get_sd_size, NULL},
    {NULL, "KEEP_ALIVE_INTERVAL", cmd_get_keep_alive_interval, NULL},
    {NULL, "OTA_REQUEST", cmd_get_ota_request, NULL, NULL},
};

#endif

