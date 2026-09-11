#include "app_config.h"

#if TCFG_WIFI_NET_ENABLE
#include "sock_api/sock_api.h"
#include "os/os_api.h"
#include "server/net_server.h"
#include "server/server_core.h"
#include "server/ctp_server.h"
#include "simple_mov_unpkg.h"
#include "server/rt_stream_pkg.h"
#include "fs/fs.h"
#include "generic/list.h"
#include "packet.h"
#define VIDEO_PLAYBACK_TASK_NAME "video_playback"
#define VIDEO_PLAYBACK_TASK_STK  0x300
#define VIDEO_PLAYBACK_TASK_PRIO 22


struct __playback {
    u32 id;
    u8 state;
    void *video_playback_sock_hdl;
    int (*cb)(void *priv, u8 *data, size_t len);
    u8 kill_flag;
    OS_MUTEX mutex;
    struct list_head cli_head;
};


struct __playback playback_info;
#define playback_info_hander (&playback_info)
static u32 cout = 0;//thread_fork name;
struct __playback_cli_info {
    struct list_head entry;
    u32 msec;
    u32 state;
    struct sockaddr_in remote_addr;
    int pid;
    u8 stop;
    u8 kill_flag;
    u16 fast_ctrl;
    u32 tmp_is_30fps;
    struct __packet_info pinfo;
    OS_SEM   sem;

};
//找I帧第2方式，根据帧数
#ifdef CONFIG_NET_H264
static int find_idr_frame2(struct __packet_info *pinfo, u32 offset)
{
    int i = 0;
    char buf[5];
    int  sample_offset;
    int sample_size;
    int ret;

    i = offset;

    while (1) {

        if (i >= pinfo->info.video_sample_count) {
            return pinfo->info.video_sample_count - 1;
        }

        sample_size = get_sample_size(pinfo->info.stsz_tab, i);
        if (sample_size == -1) {
            return -1;
        }
        sample_offset = get_chunk_offset(pinfo->info.stco_tab, i);
        if (sample_offset == -1) {
            return -1;
        }
        if (false == fseek(pinfo->fd, sample_offset, SEEK_SET)) {
            return -1;
        }
        ret = fread(buf, 1, 5, pinfo->fd);
        if (ret != 5) {
            return -1;
        }
        if (buf[4] != 0x67) {

            i++;
        } else {
            return i;
        }

    }

}
#endif
//offset : cnt
static int find_idr_frame(struct __packet_info *pinfo, u32 offset)
{
    int i = 0;
    char buf[5];
    int sample_offset;
    int sample_size;
    int ret;

    float  frame_per_ts = 0 ;

    /* printf(" pinfo->info.durition=%d   pinfo->info.scale=%d \n", pinfo->info.durition, pinfo->info.scale); */
    /* frame_per_ts = (pinfo->info.durition * 1000) / (pinfo->info.video_sample_count * pinfo->info.scale); */

    /* i = offset / frame_per_ts; */

#ifdef CONFIG_NET_H264
    i = offset * pinfo->info.scale / (pinfo->info.sample_duration * 1000);
    while (1) {

        if (i >= pinfo->info.video_sample_count) {
            return pinfo->info.video_sample_count - 1;
        }

        sample_size = get_sample_size(pinfo->info.stsz_tab, i);
        if (sample_size == -1) {
            return -1;
        }

        sample_offset = get_chunk_offset(pinfo->info.stco_tab, i);
        if (sample_offset == -1) {
            return -1;
        }

        if (false == fseek(pinfo->fd, sample_offset, SEEK_SET)) {
            return -1;
        }
        ret = fread(buf, 1, 5, pinfo->fd);
        if (ret != 5) {
            return -1;
        }

        if (buf[4] != 0x67) {
            i--;
        } else {
            return i;
        }

    }
#endif

#ifdef CONFIG_NET_JPEG
    return  avi_get_video_num(pinfo->fd, offset, pinfo->state); //返回seq
#endif
    return 0;
}

static int send_date_end_packet(struct __playback_cli_info *cli, u32 i)
{
    if (cli->tmp_is_30fps != i) {
        return send_date_packet(&cli->pinfo, 1);
    } else {
        return 0;
    }
}

static int send_date_per_fps(struct __playback_cli_info *cli, u32 i)
{
    int ret = 0;
    u32  is_30fps = 0;
    u32 sec = 0;
    int fps;
    int cnt = i + 1;

#ifdef CONFIG_NET_H264
    fps = cli->pinfo.info.scale / cli->pinfo.info.sample_duration;
#endif

#ifdef CONFIG_NET_JPEG
    fps = avi_get_fps(cli->pinfo.fd, cli->pinfo.state);
#endif

    sec = (cnt - cli->tmp_is_30fps) / fps;

    if (sec) {
        //printf("vd_cnt :  %d , all : %d , fp : %d \n", i + 1, cli->pinfo.info.video_sample_count, fps);
        ret = send_date_packet(&cli->pinfo, sec);
#if 0
        send_gps_data_packet(&cli->pinfo);
#endif
        cli->tmp_is_30fps = cnt - cnt % fps;//消除快进时间累计误差
    }
    return ret;
}
static void video_playback_cli_thread(void *arg)
{
    int ret;
    struct vfscan *fs;
    FILE *fd;
    u8 name[32];
    char buf[64];
    int i = 0, j = 0;
    u32 msec = 0;
    int last_video_num = 0;
    u32 fast_num = 0;
    struct __playback_cli_info *cli = (struct __playback_cli_info *)arg;

    puts("start  video_playback_cli_thread\n");
    cli->pinfo.data = (u8 *)zalloc(IMAGE_SIZE);
    cli->pinfo.len = IMAGE_SIZE;
    if (!cli->pinfo.data) {
        cli->pinfo.data = (u8 *)zalloc(IMAGE_SIZE / 2);
        cli->pinfo.len = IMAGE_SIZE / 2;
    }
    if (cli->pinfo.data == NULL) {
        printf("%s err , no mem !!!\n\n", __func__);
        CTP_CMD_COMBINED(NULL, CTP_OPEN_FILE, "TIME_AXIS_PLAY", "NOTIFY", buf);
        goto err1;
    }
    cli->pinfo.fd = fopen(cli->pinfo.file_name, "r");
    cli->fast_ctrl = 0;
    cli->pinfo.state = 1;//回放模式

    if (cli->pinfo.fd == NULL) {
        puts("open fail\n");
        fget_name(cli->pinfo.fd, name, 32);
        sprintf(buf, "path:%s", name);
        CTP_CMD_COMBINED(NULL, CTP_OPEN_FILE, "TIME_AXIS_PLAY", "NOTIFY", buf);
        goto err1;
    }

#ifdef CONFIG_NET_H264
    if (!is_vaild_mov_file(cli->pinfo.fd)) {
        puts("open fail\n");
        fget_name(cli->pinfo.fd, name, 32);
        sprintf(buf, "path:%s", name);
        CTP_CMD_COMBINED(NULL, CTP_OPEN_FILE, "TIME_AXIS_PLAY", "NOTIFY", buf);
        goto err1;
    }
#endif

#ifdef CONFIG_NET_JPEG
    avi_net_playback_unpkg_init(cli->pinfo.fd, cli->pinfo.state);
    if (!is_vaild_avi_file(cli->pinfo.fd, cli->pinfo.state)) {
        puts("open fail\n");
        fget_name(cli->pinfo.fd, name, 32);
        sprintf(buf, "path:%s", name);
        CTP_CMD_COMBINED(NULL, CTP_OPEN_FILE, "TIME_AXIS_PLAY", "NOTIFY", buf);
        goto err1;
    }
#endif

#if 0
    if (find_gps_data(&cli->pinfo) < 0) {
        puts("mov file no gps data\n");
    }
#endif

#ifdef CONFIG_NET_H264
    cli->pinfo.type = H264_TYPE_VIDEO;
#endif
#ifdef CONFIG_NET_JPEG
    cli->pinfo.type = JPEG_TYPE_VIDEO;
#endif

    //get video media info
    if (get_video_media_info(&cli->pinfo)) {
        puts("get video media fail\n");
        goto err1;
    }

#ifdef VIDEO_SCALE_ENABLE
    video_scaler_init(cli->pinfo.info.length, cli->pinfo.info.height, 640, 480);
#endif

    i = find_idr_frame(&cli->pinfo, cli->msec);

    if (send_media_packet(&cli->pinfo) <= 0) {
        printf("send media packet fail \n\n");
        CTP_CMD_COMBINED(NULL, CTP_NET_ERR, "TIME_AXIS_PLAY", "NOTIFY", CTP_NET_ERR_MSG);
        goto err1;

    }

    incr_date_time(&cli->pinfo.time, cli->msec / 1000);

    cli->tmp_is_30fps = i;

#ifdef CONFIG_NET_H264
    j = i / (cli->pinfo.info.video_sample_count / cli->pinfo.info.audio_chunk_num);
#endif

#ifdef CONFIG_NET_JPEG
    j = avi_video_base_to_get_audio_frame(i, cli->pinfo.state);
#endif
    while (1) {
        if (cli->stop) {
            os_sem_pend(&cli->sem, 0);
        }

        if (cli->kill_flag) {
            printf("--kill_flag---\n");
            goto err1;
        }

#ifdef CONFIG_NET_H264
        if (cli->pinfo.info.video_sample_count && cli->pinfo.info.audio_chunk_num) {
            if ((i % (cli->pinfo.info.video_sample_count / cli->pinfo.info.audio_chunk_num) == 0)
                && j < cli->pinfo.info.audio_chunk_num) {
                if (!cli->fast_ctrl) {
                    ret = send_audio_packet(&cli->pinfo, j);
                    if (ret <= 0) {
                        goto err1;
                    }
                }

                j++;
            }
        }
#endif
#ifdef CONFIG_NET_JPEG
        if (cli->pinfo.info.video_sample_count && (i >= last_video_num || fast_num > 0)) {
            if (!cli->fast_ctrl) {
                if (j < cli->pinfo.info.audio_chunk_num && cli->pinfo.info.audio_chunk_num) {
                    if (fast_num) {
                        j = avi_video_base_to_get_audio_frame(i, cli->pinfo.state);
                        last_video_num = avi_audio_base_to_get_video_frame(j - 1, cli->pinfo.state);
                        if (i < last_video_num) {
                            i = last_video_num;
                        }
                        /*printf("fast ad : %d , vd : %d , num :%d ,fast_num: %d\n",j,i,last_video_num,fast_num);*/
                        fast_num = 0;
                    }
                    /*printf("ad_cnt : %d \n",j);*/
                    ret = send_audio_packet(&cli->pinfo, j);
                    if (ret <= 0) {
                        goto send_video;
                    }
                    last_video_num = avi_audio_base_to_get_video_frame(j, cli->pinfo.state);
                    /*printf("ad : %d , vd : %d , num :%d \n",j,i,last_video_num);*/
                    j++;
                }
            }
        }
#endif

send_video:

        if (i >= cli->pinfo.info.video_sample_count) {
next:
            /*send_date_end_packet(cli, i); //发送最后一个时间包*/
            cli->pinfo.fd = play_next(&cli->pinfo);
            send_end_packet(&cli->pinfo);
            if (cli->pinfo.fd == NULL) {
                puts("video play back end\n\n");
                goto err1;
            }

            update_data(&cli->pinfo);
            cli->tmp_is_30fps = 0;
            i = 0;
            j = 0;
            last_video_num = 0;
#if 0
            if (find_gps_data(&cli->pinfo) < 0) {
                printf("mov gps data err \n");
            }
#endif
            send_media_packet(&cli->pinfo);
        }

        /*printf("vd_cnt :  %d , all : %d \n", i + 1, cli->pinfo.info.video_sample_count);*/
        ret = send_video_packet(&cli->pinfo, i);

        if (ret < 0) {
            /* CTP_CMD_COMBINED(NULL, CTP_NET_ERR, "TIME_AXIS_PLAY", "NOTIFY", CTP_NET_ERR_MSG); */
            puts("send vidoe err!\n");
            goto err1;
        } else if (ret == 0) {
            goto next;
        }

        if (!cli->fast_ctrl) {
            i++;
        } else {

#ifdef CONFIG_NET_H264
            i += cli->fast_ctrl;
            fast_num += cli->fast_ctrl / 16;
#endif
#ifdef CONFIG_NET_JPEG
            i += cli->fast_ctrl / 16;
            fast_num += cli->fast_ctrl / 16;
#endif

#ifdef CONFIG_NET_H264
            i = find_idr_frame2(&cli->pinfo, i);
#endif
        }

        send_date_per_fps(cli, i);
    }

err1:
#if 0
    unfind_gps_data();
#endif

#ifdef VIDEO_SCALE_ENABLE
    video_scaler_release();
#endif

#ifdef CONFIG_NET_JPEG
    avi_net_unpkg_exit(cli->pinfo.fd, cli->pinfo.state);
#endif
    if (cli->pinfo.info.stco_tab != NULL) {
        free(cli->pinfo.info.stco_tab);
        cli->pinfo.info.stco_tab = NULL;
    }

    if (cli->pinfo.info.stsz_tab != NULL) {
        free(cli->pinfo.info.stsz_tab);
        cli->pinfo.info.stsz_tab = NULL;
    }

    if (cli->pinfo.info.audio_stco_tab != NULL) {
        free(cli->pinfo.info.audio_stco_tab);
        cli->pinfo.info.audio_stco_tab = NULL;
    }

    if (cli->pinfo.fd != NULL) {
        fclose(cli->pinfo.fd);
        cli->pinfo.fd = NULL;
    }
    if (cli->pinfo.data) {
        free(cli->pinfo.data);
    }
    os_mutex_pend(&playback_info_hander->mutex, 0);
    list_del(&cli->entry);
    /* puts("third\n"); */
    sock_unreg(cli->pinfo.sock);
    /* puts("four\n"); */
    cli->pinfo.data = NULL;
    cli->kill_flag = 0;
    thread_kill(&cli->pid, KILL_WAIT);  /* clear task_tcb.pid before free(cli) */
    free(cli);
    os_mutex_post(&playback_info_hander->mutex);

    /* puts("end  video_playback_cli_thread\n"); */
}


int video_playback_req_handler2(void *msg)
{
    char buf[128];
    struct sockaddr_in remote_addr;
    void *ret;
    fd_set rdset;
    struct timeval tv = {5, 0};

    char name[32];
    int ret1 = 0;
    if (msg == NULL) {
        return -1;
    }

    struct net_req *req = (struct net_req *)msg;

    socklen_t addrlen = sizeof(remote_addr);
    /* sprintf(buf, "path:%s,offset:%d", req->playback.file_name, req->playback.msec); */

    sprintf(buf, "status:%d", 0);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "TIME_AXIS_PLAY", "NOTIFY", buf);
    while (1) {
        FD_ZERO(&rdset);
        FD_SET(sock_get_socket(playback_info_hander->video_playback_sock_hdl), &rdset);

        ret1 = sock_select(playback_info_hander->video_playback_sock_hdl, &rdset, NULL, NULL, &tv);
        if (ret1 < 0) {
            goto err1;
        } else if (ret1 == 0) {
            puts("accept time out\n");
            CTP_CMD_COMBINED(NULL, CTP_REQUEST, "TIME_AXIS_PLAY", "NOTIFY", CTP_REQUEST_MSG);
            free(req);
            return -1;
        } else {
            ret = sock_accept(playback_info_hander->video_playback_sock_hdl, (struct sockaddr *)&remote_addr, &addrlen, NULL, NULL);

            if (ret == NULL) {
                printf("%s ::%d sock_accept \n", __func__, __LINE__);
                goto err1;
            }


            //malloc cli info res
            struct __playback_cli_info *cli = zalloc(sizeof(struct __playback_cli_info));//&cli_playback_info;

            if (cli == NULL) {
                printf("%s ::%d calloc \n", __func__, __LINE__);
                goto err1;
            }

            //get req
            strcpy(cli->pinfo.file_name, req->playback.file_name);
            cli->msec = req->playback.msec;
            cli->pinfo.sock = ret;

            printf("old req->playback.filename : %s sec:%d\n", req->playback.file_name, req->playback.msec);
            printf("new req->playback.filename : %s sec:%d\n", cli->pinfo.file_name, cli->msec);

            //malloc data's bufffer
            cli->pinfo.data = NULL;//(u8 *)calloc(IMAGE_SIZE, 1);
            cli->pinfo.len = 0;//IMAGE_SIZE;



            memcpy(&cli->remote_addr, &remote_addr, sizeof(struct sockaddr_in));
            os_sem_create(&cli->sem, 0);
            os_mutex_pend(&playback_info_hander->mutex, 0);
            list_add_tail(&cli->entry, &playback_info_hander->cli_head);
            os_mutex_post(&playback_info_hander->mutex);

            printf("create cli thread\n");
            sprintf(name, "playback_cli_%d", cout++);
            ret1 = thread_fork(name, 19, 1024, 0, &cli->pid, video_playback_cli_thread, (void *)cli);
            if (ret1 != OS_NO_ERR) {
                free(cli);
                printf("thread fork err =%d\n", ret1);
                goto err1;
            }
            free(req);
            return 0;

        }
    }
err1:
    free(req);
    sock_unreg(playback_info_hander->video_playback_sock_hdl);
    return -1;
}
static void time_axis_play_thread(void *arg)
{
    video_playback_req_handler2(arg);
}

int video_playback_post_msg(struct net_req *_req)
{
    int ret = 0;
    static u32 count = 0;
    char buf[64];
    struct net_req *req = (struct net_req *)calloc(1, sizeof(struct net_req));

    if (req == NULL) {
        return -1;
    }

    memcpy(req, _req, sizeof(struct net_req));

    sprintf(buf, "time_axis_play_thread%d", count++);
    ret = thread_fork(buf, 18, 0x1000, 0, 0, time_axis_play_thread, (void *)req);
    if (ret != OS_NO_ERR) {
        free(req);
        return -1;
    }
    return 0;


}


int playback_init(u16 port, int callback(void *priv, u8 *data, size_t len))
{
    puts("playback_init\n");
    int ret;
    struct sockaddr_in dest_addr;
    playback_info_hander->video_playback_sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);

    if (playback_info_hander->video_playback_sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __func__, __LINE__);
        goto EXIT;
    }

    if (sock_set_reuseaddr(playback_info_hander->video_playback_sock_hdl)) {
        printf("%s %d->Error in sock_set_reuseaddr(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(port);
    puts("playback_init\n");
    ret = sock_bind(playback_info_hander->video_playback_sock_hdl, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    if (ret) {
        printf("%s %d->Error in bind(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }

    ret = sock_listen(playback_info_hander->video_playback_sock_hdl, 0xff);

    if (ret) {
        printf("%s %d->Error in listen()\n", __func__, __LINE__);
        goto EXIT;
    }

    puts("playback_init\n");

    if (callback == NULL) {
        playback_info_hander->cb = NULL;
    } else {
        playback_info_hander->cb = callback;
    }

    os_mutex_create(&playback_info_hander->mutex);
    INIT_LIST_HEAD(&playback_info_hander->cli_head);
    puts("create video_playback_thread\n");
    /* thread_fork(VIDEO_PLAYBACK_TASK_NAME, VIDEO_PLAYBACK_TASK_PRIO, VIDEO_PLAYBACK_TASK_STK, 40, &playback_info_hander->id, video_playback_thread, NULL); */


    return 0;
EXIT:
    sock_unreg(playback_info_hander->video_playback_sock_hdl);
    playback_info_hander->video_playback_sock_hdl = NULL;
    return -1;
}

static int playback_disconnect_all_cli(void)
{
    puts("---------------playback_disconnect_all_cli\n\n");
    struct __playback_cli_info *cli = NULL;
    struct list_head *pos = NULL, *node = NULL;
    os_mutex_pend(&playback_info_hander->mutex, 0);
    if (list_empty(&playback_info_hander->cli_head)) {
        puts("video_playback cli is emtry\n");
        os_mutex_post(&playback_info_hander->mutex);
        return -1;
    }
    list_for_each_safe(pos, node, &playback_info_hander->cli_head) {
        cli = list_entry(pos, struct __playback_cli_info, entry);
        cli->kill_flag = 1;
        if (cli->stop) {
            os_sem_post(&cli->sem);
        }

        sock_set_quit(cli->pinfo.sock);
    }

    os_mutex_post(&playback_info_hander->mutex);
    return -1;
}

void playback_uninit(void)
{
    playback_disconnect_all_cli();
    sock_unreg(playback_info_hander->video_playback_sock_hdl);
    playback_info_hander->video_playback_sock_hdl = NULL;
    /* playback_info_hander->kill_flag = 1; */
    /* thread_kill(&playback_info_hander->id, KILL_WAIT); */
    /* os_taskq_post_msg(VIDEO_PLAYBACK_TASK_NAME, 1, 0); */
    /* playback_info_hander->kill_flag = 0; */
}


int playback_disconnect_cli(struct sockaddr_in *dst_addr)
{
    /*puts("---------------playback_disconnect_cli\n\n"); */
    struct __playback_cli_info *cli = NULL;
    os_mutex_pend(&playback_info_hander->mutex, 0);
    struct list_head *pos = NULL, *node = NULL;
    if (list_empty(&playback_info_hander->cli_head)) {
        puts("video_playback cli is emtry\n");
        os_mutex_post(&playback_info_hander->mutex);
        return -1;
    }
    list_for_each_safe(pos, node, &playback_info_hander->cli_head) {
        cli = list_entry(pos, struct __playback_cli_info, entry);

        /* printf("remote_addr:%s:%d\n", inet_ntoa(cli->remote_addr.sin_addr.s_addr), ntohs(cli->remote_addr.sin_port)); */
        if (dst_addr != NULL
            && cli->remote_addr.sin_addr.s_addr == dst_addr->sin_addr.s_addr) {
            cli->kill_flag = 1;
            if (cli->stop) {
                os_sem_post(&cli->sem);
            }

            sock_set_quit(cli->pinfo.sock);
        } else {
            printf("not find dst_addr:%s:%d\n", inet_ntoa(dst_addr->sin_addr.s_addr), ntohs(dst_addr->sin_port));
        }

        os_mutex_post(&playback_info_hander->mutex);
        return 0;
        //break;
    }

    os_mutex_post(&playback_info_hander->mutex);
    return -1;
}

int playback_cli_pause(struct sockaddr_in *dst_addr)
{
    puts("-----------playback_cli_pause\n\n");
    struct __playback_cli_info *cli = NULL;
    struct list_head *pos = NULL, *node = NULL;

    os_mutex_pend(&playback_info_hander->mutex, 0);
    list_for_each_safe(pos, node, &playback_info_hander->cli_head) {
        cli = list_entry(pos, struct __playback_cli_info, entry);

        if (cli->remote_addr.sin_addr.s_addr == dst_addr->sin_addr.s_addr) {
            cli->stop = 1;
            os_mutex_post(&playback_info_hander->mutex);
            return 0;
            //		break;
        }
    }
    os_mutex_post(&playback_info_hander->mutex);
    return -1;
}

int playback_cli_continue(struct sockaddr_in *dst_addr)
{
    puts("---------------playback_cli_continue\n\n");
    struct __playback_cli_info *cli = NULL;
    struct list_head *pos = NULL, *node = NULL;
    os_mutex_pend(&playback_info_hander->mutex, 0);
    list_for_each_safe(pos, node, &playback_info_hander->cli_head) {
        cli = list_entry(pos, struct __playback_cli_info, entry);

        if (cli->remote_addr.sin_addr.s_addr == dst_addr->sin_addr.s_addr) {
            cli->stop = 0;

            os_sem_post(&cli->sem);
            os_mutex_post(&playback_info_hander->mutex);
            return 0;
            //break;
        }
    }
    os_mutex_post(&playback_info_hander->mutex);
    return -1;

}
//快放
int playback_cli_fast_play(struct sockaddr_in *dst_addr, u32 speed)
{
    puts("---------------playback_cli_fast_play\n\n");
    struct __playback_cli_info *cli = NULL;
    struct list_head *pos = NULL, *node = NULL;
    os_mutex_pend(&playback_info_hander->mutex, 0);
    list_for_each_safe(pos, node, &playback_info_hander->cli_head) {
        cli = list_entry(pos, struct __playback_cli_info, entry);

        if (cli->remote_addr.sin_addr.s_addr == dst_addr->sin_addr.s_addr) {
            cli->fast_ctrl = speed;
            os_mutex_post(&playback_info_hander->mutex);
            return 0;
            //break;
        }
    }
    os_mutex_post(&playback_info_hander->mutex);
    return -1;

}

#endif
