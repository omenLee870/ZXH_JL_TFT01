#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE
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
#include "ctp_cmd.h"
#include "mpi_video_net.h"
#include "media_file_list.h"

#if TCFG_HOST_UVC_ENABLE
extern u8 uvc_host_is_h264_format(void);
#endif

static int cmd_get_video_size(void *priv, char *content)
{
    char buf[128] = {0};
    char str_buf[128] = "NA";
    int count = mpi_video_net_get_reso_count();

    if (count > 0) {
        for (int i = 0; i < count && i < 6; i++) {
            int w = 0, h = 0;
            if (mpi_video_net_get_reso_by_index(i, &w, &h) == 0) {
                char label[16] = {0};
                if (h == 1080) {
                    snprintf(label, sizeof(label), ";1080P");
                } else if (h == 720) {
                    snprintf(label, sizeof(label), ";720P");
                } else if (h == 480) {
                    snprintf(label, sizeof(label), ";480P");
                } else if (h == 360) {
                    snprintf(label, sizeof(label), ";360P");
                } else if (h == 240) {
                    snprintf(label, sizeof(label), ";240P");
                } else {
                    snprintf(label, sizeof(label), ";%dx%d", w, h);
                }
                strcat(str_buf, label);
            }
        }
    }
    /* fallback: if no camera, append common defaults */
    if (count == 0) {
        strcat(str_buf, ";720P;480P");
    }

    snprintf(buf, sizeof(buf), "str:%s,val:%d", str_buf, 1);
    printf("%s: buf -> %s\n", __func__, buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_SIZE", "NOTIFY", buf);
    return 0;
}

static int cmd_put_video_size(void *priv,  char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *v = NULL;
    char buf[128];
    const char *val;
    int count = mpi_video_net_get_reso_count();
    u32 res;

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    v =  json_object_object_get(parm, "val");
    val = json_object_get_string(v);
    res = atoi(val);

    /* Build the same dynamic string as cmd_get_video_size */
    char str_buf[128] = "NA";
    if (count > 0) {
        for (int i = 0; i < count && i < 6; i++) {
            int w = 0, h = 0;
            if (mpi_video_net_get_reso_by_index(i, &w, &h) == 0) {
                char label[16];
                if (h == 1080) {
                    snprintf(label, sizeof(label), ";1080P");
                } else if (h == 720) {
                    snprintf(label, sizeof(label), ";720P");
                } else if (h == 480) {
                    snprintf(label, sizeof(label), ";480P");
                } else if (h == 360) {
                    snprintf(label, sizeof(label), ";360P");
                } else if (h == 240) {
                    snprintf(label, sizeof(label), ";240P");
                } else {
                    snprintf(label, sizeof(label), ";%dx%d", w, h);
                }
                strcat(str_buf, label);
            }
        }
    }
    if (count == 0) {
        strcat(str_buf, ";720P;480P");
    }

    snprintf(buf, sizeof(buf), "str:%s,val:%d", str_buf, res);
    printf("%s: buf -> %s\n", __func__, buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_SIZE", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}

static int get_stream_format(void)
{
#if TCFG_HOST_UVC_ENABLE
    return uvc_host_is_h264_format() ? 1 : 0;
#else
    return 0; /* DVP or no camera: JPEG */
#endif
}

static int cmd_get_video_param(void *priv, char *content)
{
    char buf[128];
    int width = 640, height = 480;
    int fmt = get_stream_format();

    /* query real camera resolution, fallback to 640x480 */
    mpi_video_net_get_uvc_resolution(&width, &height);

    snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", width, height, fmt, 30, 8000);

    printf("%s: buf -> %s\n", __func__, buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_PARAM", "NOTIFY", buf);
    return 0;
}

static int cmd_put_video_param(void *priv,  char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *h = NULL;
    json_object *w = NULL;
    char buf[128];
    const char *height, *width;
    int w_val = 640, h_val = 480;
    int fmt = get_stream_format();
    //分解content字段
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    h =  json_object_object_get(parm, "h");
    w =  json_object_object_get(parm, "w");
    width = json_object_get_string(w);
    height = json_object_get_string(h);

    printf("width :%s height : %s\n", width, height);

    /* query real camera resolution, fallback to 640x480 */
    mpi_video_net_get_uvc_resolution(&w_val, &h_val);

    snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", w_val, h_val, fmt, 25, 8000);

    printf("%s: buf -> %s\n", __func__, buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_PARAM", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}

static int cmd_put_video_ctrl(void *priv, char *content)
{
    char buf[128];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    struct json_object *sta = NULL;
    const char *status_str;
    int set_state;
    int ret = 0;

    printf("cmd_put_video_ctrl : %s \n", content);
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    sta =  json_object_object_get(parm, "status");
    status_str = json_object_get_string(sta);
    set_state = atoi(status_str);

    if (set_state) {
        /* 启动本地录像 */
        //todo 路径还没选好
        ret = mpi_video_net_start_rec("storage/sd1/C/uvc_jpeg_rec.avi");
    } else {
        /* 停止本地录像 */
        mpi_video_net_stop_rec();
    }

    snprintf(buf, sizeof(buf), "status:%d", set_state);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}

static int cmd_get_video_ctrl(void *priv, char *content)
{
    char buf[128];
    int status;

    status = mpi_video_net_get_rec_status();
    snprintf(buf, sizeof(buf), "status:%d", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);

    return 0;
}

static int cmd_put_open_rt_stream(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *val = NULL;
    const char *str;
    int width = 640, height = 480, fps = 25, abr_kbps = 2000;
    struct sockaddr_in *addr;
    char net_path[64];
    int ret;

    /* query real UVC camera resolution as default */
    mpi_video_net_get_uvc_resolution(&width, &height);

    printf("cmd_put_open_rt_stream : %s\n", content);
    new_obj = json_tokener_parse(content);
    if (!new_obj) {
        goto err;
    }
    parm = json_object_object_get(new_obj, "param");
    if (!parm) {
        goto err;
    }

    val = json_object_object_get(parm, "w");
    if (val) {
        str = json_object_get_string(val);
        if (str) {
            width = atoi(str);
        }
    }
    val = json_object_object_get(parm, "h");
    if (val) {
        str = json_object_get_string(val);
        if (str) {
            height = atoi(str);
        }
    }
    val = json_object_object_get(parm, "fps");
    if (val) {
        str = json_object_get_string(val);
        if (str) {
            fps = atoi(str);
        }
    }
    /* APP may use "abr" or "rate" key for bitrate */
    val = json_object_object_get(parm, "abr");
    if (!val) {
        val = json_object_object_get(parm, "rate");
    }
    if (val) {
        str = json_object_get_string(val);
        if (str) {
            abr_kbps = atoi(str);
        }
    }

    addr = ctp_srv_get_cli_addr(priv);
    if (!addr) {
        printf("OPEN_RT_STREAM: failed to get cli addr\n");
        goto err;
    }
    snprintf(net_path, sizeof(net_path), "udp://%s:%d",
             inet_ntoa(addr->sin_addr.s_addr), 2224);

    printf("OPEN_RT_STREAM: path=%s, %dx%d@%d, abr=%d\n", net_path, width, height, fps, abr_kbps);

    ret = mpi_video_net_open_stream(net_path, width, height, fps, abr_kbps);
    if (ret) {
        CTP_CMD_COMBINED(NULL, CTP_RT_OPEN_FAIL, "OPEN_RT_STREAM", "NOTIFY", CTP_RT_OPEN_FAIL_MSG);
    } else {
        char buf[128];
        int fmt = get_stream_format();
        snprintf(buf, sizeof(buf), "format:%d,w:%d,h:%d,fps:%d,rate:8000", fmt, width, height, fps);
        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "OPEN_RT_STREAM", "NOTIFY", buf);
    }

    json_object_put(new_obj);
    return 0;

err:
    if (new_obj) {
        json_object_put(new_obj);
    }
    CTP_CMD_COMBINED(NULL, CTP_RT_OPEN_FAIL, "OPEN_RT_STREAM", "NOTIFY", CTP_RT_OPEN_FAIL_MSG);
    return 0;
}

static int cmd_put_close_rt_stream(void *priv, char *content)
{
    int ret;
    char buf[64];
    json_object *new_obj = NULL;
    json_object *parm = NULL;

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    ret = mpi_video_net_close_stream();
    if (ret) {
        printf("CLOSE_RT_STREAM err!!!\n\n");
        strcpy(buf, "status:0");
    } else {
        strcpy(buf, "status:1");
    }

    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "CLOSE_RT_STREAM", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}

/* 设备能力集获取 */
//0-App决定，1-SDK，2-HTTP，3-SDK回放、HTTP下载，4-HTTP回放、SDK下载
static int cmd_get_camera_capability(void *priv, char *content)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "value:%s", "00100100002");

    printf("buf -> %s\n", buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "CAMERA_CAPABILITY", "NOTIFY", buf);
    return 0;
}

static int cmd_get_generic_cmd(void *priv, char *content)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "status:%d", 1);
    printf("GENERIC_CMD  GET\n");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "GENERIC_CMD", "NOTIFY", buf);
    return 0;
}

static int cmd_put_generic_cmd(void *priv, char *content)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "status:%d", 1);

    printf("GENERIC_CMD  PUT\n");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "GENERIC_CMD", "NOTIFY", buf);

    return 0;
}

static int cmd_put_make_forward_files_list(void *priv, char *content)
{

    char buf[128];
#ifdef CONFIG_ENABLE_VLIST
    flm_scan_and_load();
    flm_dump_json();
#endif // CONFIG_ENABLE_VLIST

    snprintf(buf, sizeof(buf), "type:1,path:%s", MEDIA_VF_LIST);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);

    return 0;
}

static int cmd_put_files_delete(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[256 ] = {0};
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    while (1) {
        sprintf(filename, "path_%d", i);
        tmp =  json_object_object_get(parm, filename);

        if (tmp == NULL) {
            break;
        }

        const char *tmp_value = json_object_get_string(tmp);
        printf("filename %s \n", tmp_value);

        if (fdelete_by_name(tmp_value)) {
            printf("fdelete by name\n");
            ret = snprintf(buf, sizeof(buf), "status:%d,path:%s", 0, tmp_value);
            CTP_CMD_COMBINED(priv, CTP_OPEN_FILE, "FILES_DELETE", "NOTIFY", buf);
        } else {

#if defined CONFIG_ENABLE_VLIST
            FILE_DELETE((void *)tmp_value, 1);
#endif
            snprintf(buf, sizeof(buf), "status:%d,path:%s", 1, tmp_value);
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FILES_DELETE", "NOTIFY", buf);

        }

        i++;
    }

    json_object_put(new_obj);
    return 0;

}

static int cmd_put_multi_cover_figure(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    char (*file_name_array)[64] = calloc(1, 51 * 64);
    if (file_name_array == NULL) {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
        return 0;
    }

    while (1) {
        sprintf(filename, "path_%d", i);
        tmp =  json_object_object_get(parm, filename);

        if (tmp == NULL) {
            break;
        }

        if (i > 50) {
            printf("path is too many\n");
            CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
            break;
        }

        const char *tmp_value = json_object_get_string(tmp);
        strcpy(file_name_array[i], tmp_value);
        /* printf("%d filename %s \n", i, file_name_array[i]); */
        i++;
    }

    if (!i) {
        free(file_name_array);
        file_name_array = NULL;
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
        json_object_put(new_obj);
        return 0;
    }

    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));
    req.pre.type = PREVIEW;
    req.pre.filename = file_name_array;
#ifdef CONFIG_ENABLE_VLIST
    if (video_preview_post_msg(&req)) {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
        json_object_put(new_obj);
        free(file_name_array);
        file_name_array = NULL;
        return 0;
    }
#endif
    json_object_put(new_obj);

    return 0;

}


static int cmd_put_thunbails(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char buf[128];
    u32 ret = 0;
    static char file_name[64];
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "path");
    const char *path = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "offset");
    const char *offset = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "num");
    const char *num = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "timeinv");
    const char *timeinv = json_object_get_string(tmp);

    printf("path -> %s    offset-> %s   num->%s   timeinv->%s\n"
           , path
           , offset
           , num
           , timeinv);
    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));

    strcpy(file_name, path);
    req.pre.type = THUS;

    char (*filename)[64];
    req.pre.filename = (char (*)[64])file_name;

    if (num == NULL) {
        req.pre.num = 0;
    } else {
        req.pre.num     = atoi(num);
    }

    req.pre.offset  = atoi(offset);
    req.pre.timeinv = atoi(timeinv);

#ifdef CONFIG_ENABLE_VLIST
    if (video_preview_post_msg(&req)) {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "THUMBNAILS", "NOTIFY", CTP_REQUEST_MSG);
        json_object_put(new_obj);
        return 0;
    }
#endif // CONFIG_ENABLE_VLIST
    json_object_put(new_obj);
    return 0;


}
static int cmd_put_thunbails_ctrl(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);
    printf("status -> %s\n", status);

    if (ctp_srv_get_cli_addr(priv)) {
#ifdef CONFIG_ENABLE_VLIST
        video_cli_slide(ctp_srv_get_cli_addr(priv), atoi(status));
#endif
    } else {
#ifdef CONFIG_ENABLE_VLIST
        video_cli_slide(cdp_srv_get_cli_addr(priv), atoi(status));
#endif
    }
    snprintf(buf, sizeof(buf), "status:%s", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "THUMBNAILS_CTRL", "NOTIFY", buf);
    json_object_put(new_obj);
    return 0;


}
static int cmd_put_time_axis_play(void *priv, char *content)
{
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "path");
    const char *file_name = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "offset");
    const char *msec = json_object_get_string(tmp);

    printf("filename :%s   msec:%s\n", file_name, msec);
    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));
    strcpy(req.playback.file_name, file_name);
    req.playback.msec = atoi(msec);

#ifdef CONFIG_ENABLE_VLIST
    if (video_playback_post_msg(&req)) {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_PLAY", "NOTIFY", CTP_REQUEST_MSG);
        json_object_put(new_obj);
        return 0;
    }
#endif
    json_object_put(new_obj);

    return 0;

}

#define PLAY_VIDEO_CONTINUE 0
#define PLAY_VIDEO_PAUSE    1
#define PLAY_VIDEO_STOP     2
static int cmd_put_time_axis_play_ctrl(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);

    struct sockaddr_in *dst_addr = ctp_srv_get_cli_addr(priv);
    if (!dst_addr) {
        dst_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("status:%s\n", status);

    switch (atoi(status)) {
    case PLAY_VIDEO_CONTINUE:
#ifdef CONFIG_ENABLE_VLIST
        ret = playback_cli_continue(dst_addr);
#endif
        break;

    case PLAY_VIDEO_PAUSE:
#ifdef CONFIG_ENABLE_VLIST
        ret = playback_cli_pause(dst_addr);
#endif
        break;

    case PLAY_VIDEO_STOP:
#ifdef CONFIG_ENABLE_VLIST
        ret = playback_disconnect_cli(dst_addr);
#endif
        break;

    default:
        ret = -1;
        break;
    }

    if (!ret) {

        snprintf(buf, sizeof(buf), "status:%s", status);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TIME_AXIS_PLAY_CTRL", "NOTIFY", buf);
    } else {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_PLAY_CTRL", "NOTIFY", CTP_REQUEST_MSG);
    }

    json_object_put(new_obj);

    return 0;

}

static int cmd_put_time_axis_fast_play(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    u32 speed = 0; // x2 x4 x8 x16 x32 x64
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "level");
    const char *level = json_object_get_string(tmp);

    struct sockaddr_in *dst_addr = ctp_srv_get_cli_addr(priv);
    if (!dst_addr) {
        dst_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("level:%s\n", level);
    if (atoi(level)) {
        speed = (1 << (atoi(level) - 1)) * 32;
    } else {
        speed = 0;
    }
#ifdef CONFIG_ENABLE_VLIST
    playback_cli_fast_play(dst_addr, speed);
#endif

    if (!ret) {
        snprintf(buf, sizeof(buf), "level:%s", level);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TIME_AXIS_FAST_PLAY", "NOTIFY", buf);
    } else {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_FAST_PLAY", "NOTIFY", CTP_REQUEST_MSG);
    }

    json_object_put(new_obj);

    return 0;

}

static int cmd_put_file_lock(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    const char *path;
    const char *status_str;
    int status;
    char buf[128];
    int ret;

    new_obj = json_tokener_parse(content);
    if (!new_obj) {
        goto err;
    }
    parm = json_object_object_get(new_obj, "param");
    if (!parm) {
        goto err;
    }
    tmp = json_object_object_get(parm, "path");
    if (!tmp) {
        goto err;
    }
    path = json_object_get_string(tmp);
    tmp = json_object_object_get(parm, "status");
    if (!tmp) {
        goto err;
    }
    status_str = json_object_get_string(tmp);
    status = atoi(status_str);

#ifdef CONFIG_ENABLE_VLIST
    if (status == 1) {
        ret = flm_lock_file(path);
    } else {
        ret = flm_unlock_file(path);
    }
#else
    ret = 0;
#endif // CONFIG_ENABLE_VLIST

    snprintf(buf, sizeof(buf), "status:%d,path:%s", ret == 0 ? 1 : 0, path);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FILE_LOCK", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;

err:
    if (new_obj) {
        json_object_put(new_obj);
    }
    CTP_CMD_COMBINED(priv, CTP_OPEN_FILE, "FILE_LOCK", "NOTIFY", "status:0");
    return 0;
}

static struct ctp_map_entry ctp_video_cmd_tab[] SEC_USED(.ctp_video_cmd) = {
    {NULL, "OPEN_RT_STREAM", NULL, cmd_put_open_rt_stream},
    {NULL, "CLOSE_RT_STREAM", NULL, cmd_put_close_rt_stream},
    {NULL, "VIDEO_SIZE", cmd_get_video_size, cmd_put_video_size},
    {NULL, "VIDEO_PARAM", cmd_get_video_param, cmd_put_video_param},
    {NULL, "VIDEO_CTRL", cmd_get_video_ctrl, cmd_put_video_ctrl},
    {NULL, "CAMERA_CAPABILITY", cmd_get_camera_capability, NULL},
    {NULL, "GENERIC_CMD", cmd_get_generic_cmd, cmd_put_generic_cmd},
    {NULL, "FORWARD_MEDIA_FILES_LIST", NULL, cmd_put_make_forward_files_list},
    {NULL, "FILES_DELETE", NULL, cmd_put_files_delete},
    {NULL, "MULTI_COVER_FIGURE", NULL, cmd_put_multi_cover_figure},
    {NULL, "THUMBNAILS", NULL, cmd_put_thunbails},
    {NULL, "THUMBNAILS_CTRL", NULL, cmd_put_thunbails_ctrl},
    {NULL,  "TIME_AXIS_PLAY", NULL, cmd_put_time_axis_play},
    {NULL,  "TIME_AXIS_PLAY_CTRL", NULL, cmd_put_time_axis_play_ctrl},
    {NULL,  "TIME_AXIS_FAST_PLAY", NULL, cmd_put_time_axis_fast_play},
    {NULL,  "FILE_LOCK", NULL, cmd_put_file_lock},
};

#endif
