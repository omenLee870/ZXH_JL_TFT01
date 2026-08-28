
/******************************************************************************
* File:             fb_out_dev.c
*
* Author:
* Created:          02/28/24
* Description:      1.fb 输出节点设备驱动
*****************************************************************************/
#include "asm/includes.h"
#include "lcd/lcd_conf.h"
#include "fb.h"

#include "sdk_config.h"
#if TCFG_UI_ENABLE

#define LOG_TAG_CONST       FB_DEV
#include "debug.h"

void fb_combine_init(void);
int fb_combine_mutex_enter(u8 id);
void fb_combine_mutex_exit(u8 id);
void fb_combine_list_del(struct fb_out_t *ep);
void fb_combine_prepare(struct fb_draw_info *info, u8 open_fb);
void fb_combine_list_add(struct fb_out_t *ep);
int fb_combine_updata(struct fb_out_t *ep, struct fb_map_user *map);
int fb_combine_close(u8 id);
void *fb_lcd_device_open(struct fb_out_info *out_info);

static u8 _open_fb; /* 打开fb的个数 */
static int fb_out_dev_init(const struct dev_node *node, void *_data)
{
    log_info(">>>>>> FB INIT <<<<<< ");
    _open_fb = 0;
    fb_combine_init(); /* fb合成模块初始化 */
    return 0;
}
static int fb_out_dev_open(const char *name, struct device **device, void *arg)
{
    int err = 0;
    int ret = 0;
    if (!arg) {
        return -1;
    }
    //申请一个fb输出节点
    struct fb_out_t *p = (struct fb_out_t *)zalloc(sizeof(struct fb_out_t));
    struct fb_draw_info *info = (struct fb_draw_info *)arg;
    u8 id = info->out_id;
    fb_combine_mutex_enter(id);
    if (p) {
        *device = &p->device;
        (*device)->private_data = p;
        strcpy(p->fb_name, info->name);
        p->fb = info->priv;
        p->z_order = info->z_order;
        p->out_id = info->out_id;
        err = dev_ioctl(p->fb, FBIOGET_ALLOC_FBUFFER, (u32)(p->buf_addr));
        if (err) {
            p->buf_num = err;
            for (int i = 0; i < p->buf_num; i++) {
                /* printf("fb get fbuffer %d : %x\n", i, p->buf_addr[i]); */
                p->buf_lock[i] = 0;
            }
        }
        if (err == 0) {
            if (!fb_lcd_device_open(NULL)) {
                ret = -1;
                goto __exit;
            }
            _open_fb++;
            fb_combine_prepare(info, _open_fb); /* fb 合成模块准备工作 */
            goto __exit;
        }
        //lcd 配置
        struct fb_out_info out_info = {0};
        out_info.name = "lcd";
        out_info.width = info->width; //宽度
        out_info.height = info->height; //高度
        out_info.format = info->format;
        out_info.out_id = info->out_id;
        out_info.out_addr = (err > 1) ? (u8 *)(p->buf_addr[1]) : (u8 *)(p->buf_addr[0]);
        p->out_id = out_info.out_id;
        if (err == 3) {
            out_info.out_addr = (u8 *)(p->buf_addr[2]);
        }
        if (!fb_lcd_device_open(&out_info)) {
            ret = -1;
            goto __exit;
        }

        _open_fb++;

        fb_combine_prepare(info, _open_fb); /* fb 合成模块准备工作 */

        fb_combine_list_add(p); /* 将fb_out节点加入到fb合成链表 */
    }

__exit:
    fb_combine_mutex_exit(id);
    return ret;
}

static int fb_out_dev_putmap(struct fb_out_t *ep, struct fb_map_user *map)
{

    /* //合成模块繁忙等待 */
    /* fb_combine_busy_wait(); */

    //数据更新到合成模块
    return fb_combine_updata(ep, map);
}

static int fb_out_dev_ioctl(struct device *device, unsigned int cmd,  u32 arg)
{
    int ret = 0;
    struct fb_out_t *ep = (struct fb_out_t *)device->private_data;
    switch (cmd) {
    case FBIOGETMAP:
        break;
    case FBIOPUTMAP:
        return fb_out_dev_putmap(ep, (struct fb_map_user *)arg);
    default:
        break;
    }

    return ret;
}

static int fb_out_dev_close(struct device *device)
{
    int err = 0;
    struct fb_out_t *p = (struct fb_out_t *)device->private_data;
    u8 id = p->out_id;

    fb_combine_mutex_enter(id);
    fb_combine_list_del(p);
    if (p) {
        free(p);
    }

    if (_open_fb) {
        _open_fb--;
    }
    if (_open_fb == 0) {
        /* fb_lcd_device_close(); */
        /* fb_lcd_buf_clear(); */

        fb_combine_close(id);
    }
    fb_combine_mutex_exit(id);
    return 0;
}
const struct device_operations fb_out_dev_ops = {
    .init = fb_out_dev_init,
    .open = fb_out_dev_open,
    .ioctl = fb_out_dev_ioctl,
    .close = fb_out_dev_close,
};
#endif
