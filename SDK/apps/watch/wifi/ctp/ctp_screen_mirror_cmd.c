#include "app_config.h"

#if TCFG_WIFI_NET_ENABLE
#include "server/ctp_server.h"
#include "server/net_server.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "server/server_core.h"
#include "ctp.h"
#include "cdp.h"
#include "os/os_api.h"
#include "ctp_cmd.h"
#include "screen_mirror.h"
#include "lcd_conf.h"
#include <ctype.h>
#include "wifi_connect.h"

static u8 scr_quality = 7;  //JPEG默认压缩率为70%, JLJPEG为Q值
struct __NET_SCR_CFG g_scr_cfg = {0};
static int g_scr_inited = 0;

#ifndef SCR_FRAME_TYPE
#define SCR_FRAME_TYPE 10 //jljpeg
#endif

#define __ALIGN_16(a)   (((a) + 15) / 16 * 16)

#if TCFG_NET_SCR_ENABLE
int net_scr_start(struct __NET_SCR_CFG *cfg)
{
    printf("net_scr start --\n");
    memcpy(&g_scr_cfg, cfg, sizeof(struct __NET_SCR_CFG));
    g_scr_inited = 1;
    return net_scr_init(&g_scr_cfg);
}


int net_scr_stop(void)
{
    if (!g_scr_inited) {
        return -1;
    }
    int ret = net_scr_uninit(&g_scr_cfg);
    g_scr_inited = 0;
    memset(&g_scr_cfg, 0, sizeof(g_scr_cfg));
    return ret;
}

int net_scr_ctp_disconnect_event_handle(u8 cli_num)
{
    if (get_net_scr_status()) {
        net_scr_stop();
    }
    /* CTP断开时主动重启P2P */
    struct WIFI_INIT_INFO *wifi_info = ext_get_cur_wifi_info();

    printf("cur mode : %d, role: %d, ssid: %s \n", wifi_info->mode, wifi_info->p2p_role, wifi_info->ssid[P2P_MODE]);

    if (ext_get_cur_wifi_info()->mode == P2P_MODE && cli_num == 0) {
        wifi_network_enter_p2p(wifi_info->p2p_role,
                               (const unsigned char *)wifi_info->ssid[P2P_MODE]);
    }
    return 0;

}

#endif

void set_jpg_quality_value(u8 qua)
{
    scr_quality = (qua > 10) ? 10 : qua;
}

#if TCFG_NET_SCR_ENABLE
static int cmd_get_net_scr(void *priv, char *content)
{
    char buf[128] = {0};
    u8 status;
    status = get_net_scr_status();
    snprintf(buf, sizeof(buf), "status:%d", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "NET_SCR", "NOTIFY", buf);
    return 0;
}

static void net_scr_ack_func(int seq)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "seq:%d", seq);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "NET_SCR_ACK", "NOTIFY", buf);
}

static int cmd_put_net_scr(void *priv, char *content)
{
    printf("%s %d ", __func__, __LINE__);
    char buf[128] = {0};
    struct __NET_SCR_CFG cfg = {0};

    json_object *new_obj = json_tokener_parse(content);
    json_object *parm = json_object_object_get(new_obj, "param");

    //以设备端为主的命令
#if	CONFIG_NET_PROTOCOL_TCP 	//TCP下默认需要回复app ack包
    cfg.prot = 0;
    cfg.ack = 1;
#elif CONFIG_NET_PROTOCOL_UDP   //UDP默认不需要ack
    cfg.prot = 1;
    cfg.ack = 0;
#endif

    if (cfg.ack) {
        cfg.ack_cb = net_scr_ack_func;
    }

    cfg.frame_type = SCR_FRAME_TYPE;
    const char *s_str = json_object_get_string(json_object_object_get(parm, "status"));
    if (!s_str) {
        printf("%s status string null\n", __func__);
        json_object_put(new_obj);
        return -1;
    }
    u8 status = atoi(s_str);

    //status: 1->打开  0->关闭
    if (1 == status) {
        cfg.src_w = __ALIGN_16(LCD_WIDTH);  //默认设备屏幕宽, 为了兼容YUV420格式, 如确认是YUV422/444格式, 可配置8对齐
        cfg.src_h =	__ALIGN_16(LCD_HEIGHT);
        cfg.fps = 15;       //初始状态fps
        //qua:图片质量 prot:协议类型 0->tcp 1->udp ack:规则类型 0->以帧率为准 1->以回包为准
        snprintf(buf, sizeof(buf), "status:%d,w:%d,h:%d,fps:%d,qua:%d,prot:%d,ack:%d,frm:%d", status, cfg.src_w, cfg.src_h, cfg.fps, scr_quality, cfg.prot, cfg.ack, cfg.frame_type);
        if (ctp_srv_get_cli_addr(priv)) {
            memcpy(&cfg.cli_addr, ctp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        } else {
            memcpy(&cfg.cli_addr, cdp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        }
        net_scr_start(&cfg);
    } else if (0 == status) {
        if (ctp_srv_get_cli_addr(priv)) {
            memcpy(&cfg.cli_addr, ctp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        } else {
            memcpy(&cfg.cli_addr, cdp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        }
        net_scr_stop();
    }
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "NET_SCR", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}
#endif

void extract_number_str(const char *src, char *dest, unsigned int dest_size)
{
    if (!src || !dest || dest_size == 0) {
        return;
    }

    size_t i = 0, j = 0;
    int dot_found = 0;
    const size_t MAX_SCAN_LEN = 16;

    while (i < MAX_SCAN_LEN && j < dest_size - 1) {
        unsigned char c = src[i++];
        if (isdigit(c)) {
            dest[j++] = c;
        } else if (c == '.' && !dot_found) {
            dest[j++] = c;
            dot_found = 1;
        } else {
            break;
        }
    }

    dest[j] = '\0';
}

int get_in_ui_navi_flag();
#ifdef CONFIG_UI_ENABLE
_WEAK_ void update_ui_remain_mileage_time_data(const char *mileage_buf, const char *time_buf)
{
    return;
}
#endif
/* 接收导航数据信息 */
static int cmd_notify_simple_navi_info(void *priv, char *content)
{
    printf("cmd_notify_simple_navi_info in --\n");
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *temp = NULL;
    const char remain_time_data[16],  remain_mileage_data[16];

    //printf("navi info change, content: %s\n", content);
    new_obj = json_tokener_parse(content);
    parm = json_object_object_get(new_obj, "param");

    temp = json_object_object_get(parm, "remain_mileage");
    const char *remain_mileage = json_object_get_string(temp);
    extract_number_str(remain_mileage, (char *)remain_mileage_data, sizeof(remain_mileage_data));

    temp = json_object_object_get(parm, "remain_time");
    const char *remain_time = json_object_get_string(temp);
    extract_number_str(remain_time, (char *)remain_time_data, sizeof(remain_time_data));
#ifdef CONFIG_UI_ENABLE
    if (get_in_ui_navi_flag()) {
        /* update_ui_remain_mileage_time_data(remain_mileage_data, remain_time); */
    }
#endif


    temp = json_object_object_get(parm, "arrival_time");
    const char *arrival_time = json_object_get_string(temp);

    temp = json_object_object_get(parm, "direction");
    const char *direction = json_object_get_string(temp);

    temp = json_object_object_get(parm, "guide_info");
    const char *guide_info = json_object_get_string(temp);

//	printf("remain_mileage: %s, remain_time: %s, arrival_time: %s, direction: %s, guide_info: %s, remain_mileage_data: %s, remain_time_data: %s", remain_mileage, remain_time, arrival_time, direction, guide_info, remain_mileage_data, remain_time_data);
    json_object_put(new_obj);
    return 0;
}



static int cmd_get_date_time(void *priv, char *content)
{
    struct sys_time time;
    char buf[128];
    void *rtc_fd = NULL;
    rtc_fd = dev_open("rtc", NULL);
    if (!rtc_fd) {
        printf("rtc open err !!\n\n");
        return 0;
    }
    dev_ioctl(rtc_fd, IOCTL_GET_SYS_TIME, (u32)&time);
    dev_close(rtc_fd);

    snprintf(buf, sizeof(buf), "date:%04d%02d%02d%02d%02d%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "DATE_TIME", "NOTIFY", buf);

    return 0;
}
int db_update(const char *table, u32 value);
static void date_time(const char *date, struct sys_time *tm)
{
    char *f = NULL;
    char *b = NULL;
    char buf[5] = {0};

    memcpy(buf, date, 4);
    tm->year = atoi(buf);
    memcpy(buf, date + 4, 2);
    buf[2] = '\0';
    tm->month = atoi(buf);
    memcpy(buf, date + 6, 2);
    buf[2] = '\0';
    tm->day = atoi(buf);
    memcpy(buf, date + 8, 2);
    buf[2] = '\0';
    tm->hour = atoi(buf);
    memcpy(buf, date + 10, 2);
    buf[2] = '\0';
    tm->min = atoi(buf);
    memcpy(buf, date + 12, 2);
    buf[2] = '\0';
    tm->sec = atoi(buf);

}
static int cmd_put_date_time(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    struct sys_time time;
    void *rtc_fd = NULL;


    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "date");
    const char *date = json_object_get_string(tmp);



    printf("date:%s\n", date);
    if (date != NULL) {
        date_time(date, &time);
    }
    printf("date->year:%04d month:%02d day:%02d hour:%02d min:%02d sec:%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    rtc_fd = dev_open("rtc", NULL);
    if (!rtc_fd) {
        printf("open rtd err \n\n");
        json_object_put(new_obj);
        return 0;
    }
    dev_ioctl(rtc_fd, IOCTL_SET_SYS_TIME, (u32)&time);
    dev_close(rtc_fd);
    snprintf(buf, sizeof(buf), "date:%04d%02d%02d%02d%02d%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "DATE_TIME", "NOTIFY", buf);
    json_object_put(new_obj);
    db_update("datey", time.year);
    db_update("datem", time.month);
    db_update("dated", time.day);
    db_update("dateh", time.hour);
    db_update("datemi", time.min);
    db_update("dates", time.sec);
    return 0;
}

static int cmd_put_ota_progress_notify(int download_len, int total, int status, int err_code)
{
    char buf[128];
    char error_field[32] = "";
    if (status == 2) {
        snprintf(error_field, sizeof(error_field), ",err_code:%d", err_code);
    }
    snprintf(buf, sizeof(buf), "downloaded:%d,total:%d,status:%d%s",
             download_len, total, status, error_field);
    return CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "OTA_DOWNLOAD_PROGRESS", "NOTIFY", buf);
}

int cmd_put_ota_download_progress(int download_len, int total, int status)
{
    return cmd_put_ota_progress_notify(download_len, total, status, 0);
}

int cmd_put_ota_download_error(int download_len, int total, int err_code)
{
    return cmd_put_ota_progress_notify(download_len, total, 2, err_code);
}


const struct ctp_map_entry ctp_screen_mirror_cmd_tab[] SEC_USED(.ctp_screen_mirror_cmd) = {
#if TCFG_NET_SCR_ENABLE
    {NULL, "NET_SCR", cmd_get_net_scr, cmd_put_net_scr},
#endif
    {NULL, "SIMPLE_NAVI_INFO", NULL, NULL, cmd_notify_simple_navi_info},
    {NULL, "DATE_TIME", cmd_get_date_time, cmd_put_date_time},
};

#endif

