/******************************************************************************
* File:             fb_combine.c
*
* Author:
* Created:          02/28/24
* Description:      1.fb 多图层合成功能
*****************************************************************************/
#include "asm/includes.h"
#include "fb.h"
#include "sdk_config.h"
#include "lcd/lcd_conf.h"

#if TCFG_UI_ENABLE

#define LOG_TAG_CONST       FB_COMB
#include "debug.h"

#define FB_COMBINE_TIME_DEBUG_EN         0   //打印数据合成链路的时间消耗
#define FB_COMBINE_FRAME_DEBUG_EN        1   //打印FB合成丢帧帧数

#define FB_COMBINE_MAX_NUM  (1)
struct fb_combine_t {
    OS_SEM wait_sem;
    OS_MUTEX mutex;
    volatile u8 combine_task_run;
    volatile u8 combine_busy;
    u32 combine_out_buf[2];

    u8  fb_number;
    u16 ui_timer_id;
    u8 *map_backup_baddr;
    int fb_frame_cnt[FB_COMBINE_MAX_NUM];
};

static struct fb_combine_t _fb_combine[FB_MAX_OUT_NUM];
#define  __this   (&_fb_combine[id])

static spinlock_t fb_lock[FB_MAX_OUT_NUM] = {0};
static struct list_head head[FB_MAX_OUT_NUM]; //合成fb 链表
static u32 fb_combine_async_run[FB_MAX_OUT_NUM];
int (*fb_combine_out_cb_func)(void *, u32, u32);

#define _LVGL_UI_ENTER_FLUSH_TIMEOUT  (500)

enum {
    FB_COMBINE_STOP = 0, //合成停止
    FB_COMBINE_TASK_RUN, //以单独线程方式合成
    FB_COMBINE_FUNC_RUN, //独立函数,外部调用合成
};

enum {
    FB_COMBINE_OUT_USE_MAX_IMGBUF = 0x5a,
    FB_COMBINE_OUT_USE_ROTATE = 0x5b,
};


void fb_lcd_frame_buf_update(u8 id, u8 *frame_buffer);

int fb_combine_mutex_enter(u8 id)
{
    return os_mutex_pend(&__this->mutex, 0);
}
void fb_combine_mutex_exit(u8 id)
{
    os_mutex_post(&__this->mutex);
}

/**
 * @brief   fb 合成模块输出设置
 * @param:  none
 * @return: none
 **/
static void fb_combine_set_outbuf(u8 id)
{
}

/**
 * @brief   fb 合成模块初始化
 * @param:  none
 * @return: none
 **/
void fb_combine_init(void)
{
    u8 id = 0;
    for (int i = 0; i < sizeof(_fb_combine) / sizeof(_fb_combine[0]); i++) {
        id = i;
        memset(__this, 0, sizeof(struct fb_combine_t));
        INIT_LIST_HEAD(&head[i]);
        os_mutex_create(&__this->mutex);
        os_sem_create(&__this->wait_sem, 0);
        for (int j = 0; j < FB_COMBINE_MAX_NUM; j++) {
            __this->fb_frame_cnt[j] = -1;
        }
    }
}


/**
 * @brief   fb 合成模块准备
 * @param:  info:当前fb信息 open_fb:已经打开的fb个数
 * @return: none
 **/
void fb_combine_prepare(struct fb_draw_info *info, u8 open_fb)
{
    char fb_combine_task_name[20];
    u8 id = info->out_id;
    fb_combine_mutex_enter(id);
#if 0
    if (__is_need_create_combine_task(open_fb, info)) {
        if (__this->combine_task_run == FB_COMBINE_STOP) {
            __this->combine_task_run = FB_COMBINE_TASK_RUN;
            sprintf(fb_combine_task_name, "fb_combine_task%d", id);
            thread_fork(fb_combine_task_name, 25, 4096, 0, 0, __fb_task, (void *)id);
        }
    }
#endif

    fb_combine_set_outbuf(id); //fb 合成输出buf配置

    if (!strcmp(info->name, "fb0")) {
        if (info->fb_num == 1) { //单buffer
            __this->combine_task_run = FB_COMBINE_FUNC_RUN;
        } else if (info->fb_num == 0) {
#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
            u8 mode = 1; //使用lcd显存双buffer
            log_info("lvgl_set_ui_flush_mode: %x %x", __this->combine_out_buf[0], __this->combine_out_buf[1]);
            if (__this->combine_out_buf[0] == 0 && __this->combine_out_buf[1] == 0) {
                mode = 2;
            }
            lvgl_set_ui_flush_mode(mode, (void *)__this->combine_out_buf[0], (void *)__this->combine_out_buf[1]); /* lvgl UI自己刷新 */
#endif

        }
    } else {

#if (defined CONFIG_UI_ENABLE && defined USE_LVGL_V8_UI_DEMO)
#if (LV_DISP_UI_FB_NUM <= 1)
        if (__this->ui_timer_id == 0) {
            char *task_name;
            if (os_task_get_handle(LVGL_TASK_NAME) == NULL) {
                task_name = "app_core";
            } else {
                task_name = LVGL_TASK_NAME;
            }
            /* 创建自刷模式检测定时器 */
            __this->ui_timer_id = sys_timer_add_to_task(task_name, (void *)id, ui_set_flush_mode_timer_cb, _LVGL_UI_ENTER_FLUSH_TIMEOUT);
        }
#endif
#endif
    }

    fb_combine_mutex_exit(id);
}

/**
 * @brief   fb 合成模块链表追加
 * @param:  ep:当前fb 输出节点
 * @return: none
 **/
void fb_combine_list_add(struct fb_out_t *ep)
{
    struct fb_out_t *p0 = NULL;
    u8 id = ep->out_id;
    spin_lock(&fb_lock[id]);
    __this->fb_frame_cnt[ep->fb_name[2] - '0'] = 0;
    list_for_each_entry(p0, &head[id], entry) {
        if (p0->z_order <= ep->z_order) { //按照由小到大排序,数字越大越顶层
            __list_add(&ep->entry, p0->entry.prev, &p0->entry);
            spin_unlock(&fb_lock[id]);
            return;
        }
    }
    list_add_tail(&ep->entry, &head[id]);
    spin_unlock(&fb_lock[id]);

}

__attribute__((weak)) int fb_data_updata(u8 fb_index, u8 *baddr)
{
    /* printf("fb updata[%d]=0x%x\n",fb_index, (u32)baddr); */
    return 0;
}

/**
 * @brief   fb 合成模块数据更新
 * @param:  ep:当前fb 输出节点 map: 需要更新的数据
 * @return: 0:更新成功 非0: 更新失败
 **/
int fb_combine_updata(struct fb_out_t *ep, struct fb_map_user *map)
{
    struct fb_out_t *p = NULL;
    int err = 0;
    u8 need_combine = 0;
    u8 fb_index;
    u8 id = ep->out_id;

    if (map == NULL && ep->ready_combine == 0xff) {
        need_combine = 1;
    }

    spin_lock(&fb_lock[id]);
    list_for_each_entry(p, &head[id], entry) {
        if (p == ep) {
            p->map.baddr_bk = p->map.baddr;
            p->map.baddr = map->baddr;
            p->map.width = map->width;
            p->map.height = map->height;
            p->map.real_width = map->real_width;
            p->map.real_height = map->real_height;
            p->map.rotate = map->rotate;
            p->map.mirror = map->mirror;
            p->map.format = map->format;
            p->map.xoffset = map->xoffset;
            p->map.yoffset = map->yoffset;
            p->ready_combine = 1;
            need_combine = 1;
            for (u8 i = 0; i < sizeof(p->buf_addr) / sizeof(p->buf_addr[0]); i++) {
                if ((u32)p->map.baddr == p->buf_addr[i]) {
                    p->map.buf_index = i;
                    break;
                }
            }
            break;
        }
    }
    spin_unlock(&fb_lock[id]);

    if (need_combine) {
        fb_index = ep->fb_name[2] - '0';
        if (fb_index != 0) {
            err = fb_data_updata(fb_index, map ? map->baddr : NULL);
            __this->fb_frame_cnt[fb_index]++;

        }
        os_sem_set(&__this->wait_sem, 0);
        os_sem_post(&__this->wait_sem);
    }

    if (err == 1 && __this->combine_task_run == FB_COMBINE_STOP) {
        if (map) {
            fb_lcd_frame_buf_update(id, map->baddr);
        }
    }

    return 0;
}

/**
 * @brief   fb 合成模块链表节点删除
 * @param:  ep:当前fb 输出节点
 * @return: none
 **/
void fb_combine_list_del(struct fb_out_t *ep)
{
    struct fb_out_t _ep = {0};
    u8 id = ep->out_id;
    spin_lock(&fb_lock[id]);
    memcpy(&_ep, ep, sizeof(struct fb_out_t));
    ep->map.baddr = 0;
    ep->map.baddr_bk = 0;
    __this->fb_frame_cnt[ep->fb_name[2] - '0'] = -1;
    if (ep->buf_addr[0] || ep->buf_addr[1]) {
        list_del(&ep->entry);
    }
    spin_unlock(&fb_lock[id]);
    _ep.ready_combine = 0xff;
    fb_combine_updata(&_ep, NULL); //当前fb图层删除了应该触发一次合成
}

/**
 * @brief   fb 合成模块关闭
 * @param:  none
 * @return: 0:关闭成功 非0: 关闭失败
 **/
int fb_combine_close(u8 id)
{
    if (__this->combine_task_run) {
        __this->combine_task_run = FB_COMBINE_STOP;
        __this->combine_busy = 0;
        os_sem_set(&__this->wait_sem, 0);
        os_sem_post(&__this->wait_sem);

    }
    return 0;
}

#endif
