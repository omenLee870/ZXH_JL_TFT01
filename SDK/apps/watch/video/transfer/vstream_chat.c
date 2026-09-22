

#include "system/includes.h"
#include "app_config.h"
#include "typedef.h"
#include "os/os_api.h"
#include "jlui_app/ui_style.h"
#include "ui.h"
#include "ui_api.h"
#include "task.h"
#include "lbuf.h"
#include "crc.h"
#include "vstream_common.h"
#include "vstream_cmd.h"
#include "data_storage/data_storage.h"
#include "transfer_stubs.h"
#include "dev_manager.h"   /* dev_manager_check_by_logo: 判 SD 在位 */

#define LOG_TAG             "[KWT_CMD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#include "kid_mem_dbg.h"

#define MSG_PEND_TIMEOUT	1000
#define MSG_PEND_START		2
/* 接收端文件类消息(CHAT_PHOTO/CHAT_VOICE)收完后的"改名保存 + 写聊天记录 + 回ACK"
 * 整段被包在 FILE_TRANS_CRC_ENABLE 里。若为 0, 接收端只存临时文件、不写聊天记录、
 * 不回 ACK -> 对端聊天列表不显示图片/语音。必须置 1。 */
#define FILE_TRANS_CRC_ENABLE 1
#define SEND_TIMEOUT		10*1000//MS
#define CACHE_BUF_SIZE		4096//优化写入效率
#ifndef UI_WINDOW_PREEMPTION_POP_DELAY
#define UI_WINDOW_PREEMPTION_POP_DELAY(id, ms)  /* stub — no-op when preemption framework unavailable */
#endif
/* 聊天页 / 空间不足提示页当前尚未实现: JLUI 的 ID_WINDOW_* 表中无对应项。
 * 未实现时这两页不能作为抢占目标, 故用 0 作哨兵值(PAGE_x 均自 0x800 起,
 * 不会与真实页面碰撞; 也不可用 -1, 那是 PAGE_NULL)。
 * 待 UI 侧补齐页面后删除本段兜底, 并补齐 ID_WINDOW_CHAT / ID_WINDOW_NOT_SPACE。 */
#ifndef ID_WINDOW_CHAT
#define ID_WINDOW_CHAT                  0
#endif
#ifndef ID_WINDOW_NOT_SPACE
#define ID_WINDOW_NOT_SPACE             0
#endif
//添加记录
struct chat_data_config {
    u8 type;
    u8 has_file;
    u16 data_len;
};
struct chat_send_list {
    struct list_head head;
    u16 fid;
    u8 sn;
    u8 status;
    u32 time;
};
struct chat_file_info {
    void (*refresh_cb)(void);
    //发送
    struct list_head head;
    OS_MUTEX list_lock;
    //接收
    FILE *fp;
    int save_file_len;
    u32 save_total_len;    /* 本次接收文件总长(由第一包记录) */
    u8  recv_drop_file;    /* 无处落盘(卡也没有), 已丢弃文件内容(仍回记录+ACK 兜底) */
    u8  recv_use_sd;       /* 本次接收文件写 SD(1) 还是 内置 virfat(0) */
    u32 file_crc;
    u16 file_cmd;
    u16 file_sn;
    u8 last_name[16];
    u8 init;
    u8 *cache_buf;
    u16 cache_buf_offset;
    /* u8 type; */
};
static struct chat_file_info _chat_file_info;
#define __info (&_chat_file_info)
// 文件接收期间置位，用于阻止拍照等高耗时操作打断接收流程。
static volatile u8 _chat_file_recv_busy = 0;
// 避免同一次文件接收过程中重复 push 同一个聊天页面。
static volatile u8 _chat_file_recv_preempt_active = 0;

/* 接收落盘辅助(定义在文件后部, 先声明给 vstream_cmd_ask_chat_with_file 用) */
static void chat_recv_root(char *buf, int size, u8 use_sd);
static int  chat_recv_sd_online(void);
static u32  chat_recv_free_space(const char *root);
static void chat_recv_pick_name(const char *root, const char *ext, char *out16);

static void kid_wt_chat_file_recv_preempt_push(void)
{
#if CONFIG_JL_UI_ENABLE
    if (UI_GET_WINDOW_ID() == ID_WINDOW_CHAT) {
        return;
    }
#endif
    _chat_file_recv_busy = 1;
#if CONFIG_JL_UI_ENABLE
    if (!_chat_file_recv_preempt_active) {
        _chat_file_recv_preempt_active = 1;
        // 接收文件时强制显示聊天页，防止停留在相机页继续触发拍照保存。
        UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_CHAT, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PHONE);
    }
#endif
}
static void kid_wt_chat_file_recv_preempt_pop(void)
{
#if CONFIG_JL_UI_ENABLE
    if (_chat_file_recv_preempt_active) {
        // 文件接收结束或失败后释放抢占，UI 返回原来的页面栈。
        UI_WINDOW_PREEMPTION_POP(ID_WINDOW_CHAT);
    }
    _chat_file_recv_preempt_active = 0;
#endif
    _chat_file_recv_busy = 0;
}
int kid_wt_chat_file_recv_busy_get(void)
{
    // 给相机模块查询，接收文件期间不允许启动拍照保存。
    return _chat_file_recv_busy;
}

//聊天发送抢占标志：被高优先级事件（如来电视频请求）置位时，
//正在进行的分包发送循环立刻中断，让出控制权给应答处理。
static volatile u8 _chat_send_abort_flag = 0;
int chat_send_abort_set(u8 abort)
{
    _chat_send_abort_flag = abort ? 1 : 0;
    return 0;
}
int chat_send_abort_get(void)
{
    return _chat_send_abort_flag;
}

const static struct chat_data_config data_config[CHAT_TYPE_MAX] = {
    {CHAT_TYPE_MEME, 	0,		4},			//index : sizeof(int)
    {CHAT_TYPE_TEXT, 	0,		4},			//index : sizeof(int)
    {CHAT_TYPE_MUSIC, 	0,		4},			//index : sizeof(int)

    {CHAT_TYPE_VOICE, 	1,		12},		//file_name : 8+3
    {CHAT_TYPE_PIC,	 	1,		12},		//file_name : 8+3
};
static void chat_data_update_refresh();
static int vstream_file_get_dirinfo(s8 *file, s8 *pre, int pre_len, s8 *ext, u32 *max, u32 *total);
int chat_send_list_init()
{
    if (!__info->init) {
        INIT_LIST_HEAD(&__info->head);
        os_mutex_create(&__info->list_lock);
        __info->init = 1;
    }
    return 0;
}
int chat_send_list_add(int fid, u8 sn)
{
    struct chat_send_list *p = malloc(sizeof(struct chat_send_list));
    if (!p) {
        log_error("%s malloc failed\n", __func__);
        KWT_MEM_STATS("chat.send_list malloc fail");
        return -1;
    }
    p->fid  = fid;
    p->sn = sn;
    p->status = FILE_STATUS_LOADING;
    p->time = jiffies_msec();
    //lock
    os_mutex_pend(&__info->list_lock, 0);
    list_add_tail(&p->head, &__info->head);
    //unlock
    os_mutex_post(&__info->list_lock);
    return 0;
}
void chat_send_list_cb(int fid, u8 status)
{
    int offset = 1;
    int total = small_file_get_size_by_id(F_TYPE_CHAT_REC, fid);
    small_file_update_by_id(F_TYPE_CHAT_REC, fid, offset, &status, 1, total);
    chat_data_update_refresh();
}
int chat_send_list_del_by_sn(u8 sn, void (*cb)(int fid, u8 status))
{
    //printf("%s sn:%d \n", __func__, sn);
    int ret = -1;
    struct chat_send_list *p, *n;
    list_for_each_entry_safe(p, n, &__info->head, head) {
        if (p->sn == sn) {
            //lock
            os_mutex_pend(&__info->list_lock, 0);
            list_del(&p->head);
            os_mutex_post(&__info->list_lock);
            //unlock
            int fid  = p->fid;
            u8 status = p->status;
            //todo
            cb(fid, status);

            free(p);
            ret = 0;
        }
    }
    return ret;
}
int chat_send_list_update_status_by_sn(u8 sn, u8 status)
{
    int ret = -1;
    struct chat_send_list *p;
    list_for_each_entry(p, &__info->head, head) {
        if (p->sn == sn) {
            p->status = status;
        }
    }
    return ret;
}
int chat_update_timeout()
{
    u32 time = jiffies_msec();
    struct chat_send_list *p, *n;
    list_for_each_entry_safe(p, n, &__info->head, head) {
        if (jiffies_msec2offset(p->time, time) >= SEND_TIMEOUT) {
            log_info("%s sn:%d timeout\n", __func__, p->sn);
            chat_send_list_update_status_by_sn(p->sn, FILE_STATUS_ERROR);
            chat_send_list_del_by_sn(p->sn, chat_send_list_cb);
        }
    }
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief chat_data_refresh_cb_register 注册刷新回调
 *
 * @param cb
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int chat_data_refresh_cb_register(void(*cb)(void))
{
    if (__info) {
        __info->refresh_cb = cb;
        return 0;
    }
    return -1;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief chat_data_update_refresh 刷新
 */
/* ------------------------------------------------------------------------------------*/
static void chat_data_update_refresh()
{
    if (__info->refresh_cb) {
        __info->refresh_cb();
    }

}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief chat_data_len_get 获取数据长度
 *
 * @param type
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int chat_data_len_get(int type)
{
    for (int i = 0; i < CHAT_TYPE_MAX; i++) {
        if (data_config[i].type == type) {
            return data_config[i].data_len;
        }
    }
    log_error("%s type:%d\n", __func__, type);
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief chat_data_has_file 是否以文件形式保存（附带文件）
 *
 * @param type
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int chat_data_has_file(int type)
{
    for (int i = 0; i < CHAT_TYPE_MAX; i++) {
        if (data_config[i].type == type) {
            return data_config[i].has_file;
        }
    }
    log_error("%s type:%d\n", __func__, type);
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_data_storage_read 读取记录
 *
 * @param index 索引
 * @param buf	数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int kid_wt_chat_data_storage_read(int index, u8 *buf)
{
    int file_id = small_file_get_id_by_index(F_TYPE_CHAT_REC, index);
    if (!file_id) {
        log_error("%s ,index:%d", __func__, index);
        return -1;
    }
    int len = small_file_get_size_by_index(F_TYPE_CHAT_REC, index);
    int ret = small_file_read(F_TYPE_CHAT_REC, file_id, 0, buf, len);
    log_debug("%s index:%d ret%d", __func__, index, ret);
    kid_wt_chat_save_dump((struct chat_data_storage *)buf);
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_data_storage_del 删除最旧的一组数据
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int kid_wt_chat_data_storage_del()
{
    u8 tmp_buf[20] = {0};
    struct chat_data_storage *save = (struct chat_data_storage *)tmp_buf;
    kid_wt_chat_data_storage_read(0, (u8 *)save);
    //先删记录，再删文件
    small_file_delete_by_index(F_TYPE_CHAT_REC, 0);

    int has_file =  chat_data_has_file(save->type);
    if (has_file) {
        /* 接收文件可能在 virfat 或 SD: 两个根目录都尝试删除 */
        int try_idx;
        for (try_idx = 0; try_idx < 2; try_idx++) {
            char root[64];
            char path[96] = {0};
            FILE *fp;
            chat_recv_root(root, sizeof(root), try_idx ? 1 : 0);
            snprintf(path, sizeof(path), "%s%s", root, (const char *)save->data);
            log_debug("%s path:%s", __func__, path);
            if (strstr(path, "CAM")) {
                //删除聊天记录时，不删除本地图片
                return 0;
            }
            fp = fopen(path, "r");
            if (fp) {
                fdelete(fp);
                log_info("%s path:%s del succ!!\n", __func__, path);
                break;
            }
        }
    }
    return 0;
}
void kid_wt_chat_delete_all()
{
    do {
        kid_wt_chat_data_storage_del();
    } while (kid_wt_char_get_rec_data_total());
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_data_storage_write 写入数据
 *
 * @param type			数据类型
 * @param id			文件id
 * @param buf			buf
 * @param len			长度
 * @param recv_flag		0:发送端消息，1：接收消息
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int kid_wt_chat_data_storage_write(u16 type, u32 *id, u8 *buf, int len, int recv_flag)
{
    ASSERT(len <= 18);
    u8 tmp_buf[20];
    struct chat_data_storage *save = (struct chat_data_storage *)tmp_buf;
    save->type = type;
    save->recv = recv_flag;
    memcpy(save->data, buf, len);
    kid_wt_chat_save_dump(save);
    int save_len = len + sizeof(u16);
    /* int id = 0; */
    int ret = small_file_write(F_TYPE_CHAT_REC, id, 0, save, save_len, save_len);
    log_debug("%s type:%d ret%d", __func__, type, ret);
    chat_data_update_refresh();
    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_char_get_rec_data_total 获取消息数量
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int kid_wt_char_get_rec_data_total()
{
    int total = small_file_get_count(F_TYPE_CHAT_REC);
    log_debug("%s :%d\n", __func__, total);
    return total;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_get_rec_data_by_index 通过索引获取消息内容
 *
 * @param index
 * @param save
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int kid_wt_chat_get_rec_data_by_index(int index, struct chat_data_storage *save)
{
    return kid_wt_chat_data_storage_read(index, (u8 *)save);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_pic_album_del_handle 处理相册删除图片时与聊天记录的协同
 *
 * 引用计数式删除策略：图片只有一份，所有引用方都删除后才真正删除。
 * 当用户从相册删除一张图片时，由相册侧调用此函数：
 *   - 若该图片仍被聊天记录引用：将其重命名为 S 前缀（脱离相册管理范围），
 *     并同步更新所有引用该文件的聊天记录指向新文件名。
 *     后续清除聊天记录时，由 kid_wt_chat_data_storage_del 按 S_ 文件正常 fdelete。
 *   - 若该图片未被聊天记录引用：返回 1，调用方执行正常物理删除。
 *
 * @param path        原文件完整路径（如 "storage/virfat_flash/C/CAM_0001.JPG"）
 * @param file_name   原文件名（12字节，如 "CAM_0001.JPG"）
 *
 * @return 0: 已重命名，调用方应跳过物理删除
 *         1: 无聊天引用，调用方应执行物理删除
 */
/* ------------------------------------------------------------------------------------*/
int kid_wt_chat_pic_album_del_handle(const char *path, const char *file_name)
{
    int total = kid_wt_char_get_rec_data_total();
    if (total <= 0) {
        return 1;
    }
    /* 第一遍扫描：检查是否有聊天记录引用该文件 */
    int ref_count = 0;
    u8 tmp_buf[20] = {0};
    struct chat_data_storage *save = (struct chat_data_storage *)tmp_buf;
    for (int i = 0; i < total; i++) {
        memset(tmp_buf, 0, sizeof(tmp_buf));
        kid_wt_chat_data_storage_read(i, tmp_buf);
        if (save->type == CHAT_TYPE_PIC && strncmp((char *)save->data, file_name, 12) == 0) {
            ref_count++;
        }
    }
    if (ref_count == 0) {
        return 1;
    }
    /* 生成新的 S 前缀文件名（与接收图片 R 前缀格式对称） */
    u32 s_max = 0, s_total = 0;
    vstream_file_get_dirinfo("S*******.JPG", "S", 1, "JPG", &s_max, &s_total);
    s_max++;
    char new_name[16] = {0};
    sprintf(new_name, "S%07d.JPG", s_max);
    /* 重命名物理文件 */
    FILE *fp = fopen(path, "r");
    if (!fp) {
        log_error("%s open failed: %s", __func__, path);
        return 1;
    }
    frename(fp, new_name);
    fclose(fp);
    /* 第二遍扫描：更新所有引用该文件的聊天记录指向新文件名。
     * chat_data_storage 内存布局: [type:1][recv:1][data:12]，
     * 偏移 2 起为 data 字段，total_len = 14 (sizeof type+recv+filename) */
    for (int i = 0; i < total; i++) {
        memset(tmp_buf, 0, sizeof(tmp_buf));
        kid_wt_chat_data_storage_read(i, tmp_buf);
        if (save->type == CHAT_TYPE_PIC && strncmp((char *)save->data, file_name, 12) == 0) {
            int fid = small_file_get_id_by_index(F_TYPE_CHAT_REC, i);
            if (fid) {
                small_file_update_by_id(F_TYPE_CHAT_REC, fid, 2, (void *)new_name, 12, 14);
            }
        }
    }
    log_info("%s renamed %s -> %s, updated %d chat records", __func__, file_name, new_name, ref_count);
    chat_data_update_refresh();
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_save_dump 打印接口
 *
 * @param save
 */
/* ------------------------------------------------------------------------------------*/
void kid_wt_chat_save_dump(struct chat_data_storage *save)
{
    return;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief vstream_cmd_ask_chat_with_index 接收消息处理(索引类)
 *
 * @param cmd
 * @param cmd_sn
 * @param data
 * @param len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int vstream_cmd_ask_chat_with_index(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    struct chat_data_mult_packet *packet = (struct chat_data_mult_packet *)data;
    int type = 0;
    //cmd_map_type
    switch (cmd) {
    case VSTREAM_CMD_CHAT_MEME:
        type = CHAT_TYPE_MEME;
        break;
    case VSTREAM_CMD_CHAT_TEXT:
        type = CHAT_TYPE_TEXT;
        break;
    case VSTREAM_CMD_CHAT_MUSIC:
        type = CHAT_TYPE_MUSIC;
        break;
    }
    int max  = small_file_get_max_item(F_TYPE_CHAT_REC);
    int total = small_file_get_count(F_TYPE_CHAT_REC);
    if (total == max) {
        //存满了，需要清除最旧的一条记录
        kid_wt_chat_data_storage_del();
    }

    log_debug("%s cmd:%d len:%d", __func__, cmd, len);
    /* put_buf(packet->data, packet->data_len); */
    //save
    u32 fid = 0;
    kid_wt_chat_data_storage_write(type, &fid, packet->data, packet->data_len, FILE_STATUS_RECV);
    //post msg to ui

    return 0;
}
void virfat_flash_get_dirinfo(void *file_buf, u32 *file_num);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief vstream_file_get_dirinfo
 *
 * @param file "R*******.ext" 文件格式
 * @param ext	"xxx"	后缀
 * @param max	最大索引
 * @param total 文件数
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int vstream_file_get_dirinfo(s8 *file, s8 *pre, int pre_len, s8 *ext, u32 *max, u32 *total)
{
    if ((max == NULL) || (total == NULL)) {
        return -1;
    }
    u32 all_total = 0;
    u32 ext_total = 0;
    u32 ext_max  = 0;
    virfat_flash_get_dirinfo(NULL, &all_total);
    u8 *file_name = zalloc(12 * all_total);
    if (!file_name) {
        log_error("%s zalloc(%d) failed\n", __func__, 12 * all_total);
        KWT_MEM_STATS("chat.dirinfo zalloc fail");
        return -1;
    }
    virfat_flash_get_dirinfo(file_name, &all_total);
    int star_start = -1;
    int star_end = -1;

    for (int i = 0; i < 12 - 1; i++) {
        if ((file[i] != '*') && (file[i + 1] == '*')) {
            star_start = i + 1;
        } else if ((file[i] == '*') && (file[i + 1] != '*')) {
            star_end = i;
        }
    }
    for (int i = 0; i < all_total; i++) {
        u8 tmp_name[13];
        memcpy(tmp_name, file_name + i * 12, 12);
        tmp_name[12] = 0;
        if ((!strncmp((const char *)tmp_name, pre, pre_len)) && strstr((const char *)tmp_name, ext)) {
            ext_total++;
            if ((star_start != -1) && (star_end != -1)) {
                u32 file_list = 0;
                for (int j = star_start; j <= star_end; j++) {
                    file_list *= 10;
                    /* log_debug("[check_max]j:%d tmp:%d fl:%d", j, tmp_name[j], file_list); */
                    file_list += (tmp_name[j] - '0');
                }
                if (file_list > ext_max) {
                    ext_max = file_list;
                }
            }
        }
    }
    free(file_name);
    *max  = ext_max;
    *total = ext_total;
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief vstream_cmd_ask_chat_with_file 接收带文件的消息
 *
 * @param cmd
 * @param cmd_sn
 * @param data
 * @param len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int vstream_cmd_ask_chat_with_file(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int type = 0;
    char *EXT;
    char root[64];   /* 本次接收文件所在根目录(virfat 或 SD), 由 file_save 首包选定 */
    chat_recv_root(root, sizeof(root), __info->recv_use_sd);
    //cmd_map_type
    switch (cmd) {
    case VSTREAM_CMD_CHAT_VOICE:
        type = CHAT_TYPE_VOICE;
        EXT = "VOI";
        break;
    case VSTREAM_CMD_CHAT_PHOTO:
        type = CHAT_TYPE_PIC;
        EXT = "JPG";
        break;
    }
    log_debug("%s cmd:%d sn:%d", __func__, cmd, cmd_sn);
    /* 两盘都没空间而丢弃文件内容: 仍写一条接收记录 + 回 ACK, 让对端能看到该图片/语音消息
     * (文件未落盘, 仅显示占位/名称)。 */
    if (__info->recv_drop_file) {
        char file_name[16] = {0};
        u32 fid = 0;
        chat_recv_pick_name(root, EXT, file_name);
        log_error("%s no space, record-only name:%s\n", __func__, file_name);
        kid_wt_chat_data_storage_write(type, &fid, (u8 *)file_name, strlen(file_name), FILE_STATUS_RECV);
        vstream_cmd_send_recv(cmd, cmd_sn, NULL, 0);
        __info->recv_drop_file = 0;
        __info->save_total_len = 0;
        __info->recv_use_sd = 0;
        memset(__info->last_name, 0, 16);
        __info->file_crc = 0;
        __info->file_cmd = 0;
        __info->file_sn = 0;
        kid_wt_chat_file_recv_preempt_pop();
        return 0;
    }
    if (__info->last_name[0]) {
        u32 crc = 0;
        memcpy(&crc, data, 4);
        char path[96] = {0};
        snprintf(path, sizeof(path), "%s%s", root, (const char *)__info->last_name);
        FILE *fp = fopen(path, "r");
        int file_crc  = 0;

        log_debug("%s fp:%x crc:%x path:%s ", __func__, fp, crc, path);
        if (fp) {
#if FILE_TRANS_CRC_ENABLE
            /* 整文件重算 CRC 与发送端逐包累计值比对。
             * 注意: 不能照旧用 "file_offset += file_len" 的写法(会只读一包/死循环),
             * 需从文件头完整读一遍。 */
            {
                int rlen;
                u8 *rbuf = zalloc(512);
                if (!rbuf)
                {
                    log_error("%s crc rbuf zalloc(512) failed\n", __func__);
                } else
                {
                    fseek(fp, 0, SEEK_SET);
                    file_crc = 0;
                    while ((rlen = fread(rbuf, 1, 512, fp)) > 0) {
                        file_crc = CRC16_with_initval(rbuf, rlen, file_crc);
                    }
                    free(rbuf);
                }
            }
            log_debug("%s fp:%x crc:%x file_crc:%x", __func__, fp, crc, file_crc);
#if VSTREAM_DATA_CRC_CHECK_EN
            if (crc == file_crc)
#endif
            {
                char file_name[16] = {0};
                u32 fid = 0;
                chat_recv_pick_name(root, EXT, file_name);
                log_info("%s file_name:%s", __func__, file_name);
                frename(fp, file_name);
                kid_wt_chat_data_storage_write(type, &fid, (u8 *)file_name, strlen(file_name),  FILE_STATUS_RECV);
                vstream_cmd_send_recv(cmd, cmd_sn, NULL, 0);
            }
#if VSTREAM_DATA_CRC_CHECK_EN
            else {
                /* CRC 不匹配: 删除临时文件, 避免残留 R*.TMP 占满 virfat;
                 * fdelete 会关掉句柄, 置 NULL 防下面重复 fclose */
                log_error("%s save err\n", __func__);
                fdelete(fp);
                fp = NULL;
            }
#endif
#endif
            if (fp) {
                fclose(fp);
            }
        }
        memset(__info->last_name, 0, 16);
        __info->file_crc = 0;
        __info->file_cmd = 0;
        kid_wt_chat_file_recv_preempt_pop();
    } else { //如cmd比file最后一包早到，则先保存
        u32 crc;
        memcpy(&crc, data, 4);
        __info->file_crc = crc;
        __info->file_cmd = cmd;
        __info->file_sn = cmd_sn;
        log_debug("%s crc:%x cmd:%x", __func__, __info->file_crc, __info->file_cmd);
    }
    return 0;
}

/* 删除 virfat 根目录残留的 R*.TMP 接收临时文件。
 * 旧固件 FILE_TRANS_CRC_ENABLE=0 / 接收中途失败 都会留下 .TMP 不清理,
 * 在很小的内置 virfat(4M 版仅 64K)上会很快塞满, 导致 fget_free_space 为 0,
 * 新图片收不下来(space:0)。 */
static void chat_recv_purge_leftover_tmp(void)
{
    u32 all_total = 0;
    u32 cap;
    u8 *file_name;
    int freed = 0;

    virfat_flash_get_dirinfo(NULL, &all_total);
    if (all_total == 0) {
        return;
    }
    /* 第一次只数数目, 第二次回填内容; 给点余量并封顶, 防两次查询间目录增长导致越界 */
    cap = all_total;
    if (cap > 512) {
        cap = 512;
    }
    file_name = zalloc(12 * (cap + 16));
    if (!file_name) {
        log_error("%s zalloc(%d) failed\n", __func__, 12 * (cap + 16));
        return;
    }
    virfat_flash_get_dirinfo(file_name, &all_total);
    if (all_total > cap + 16) {
        all_total = cap + 16;   /* 超出余量按余量遍历, 残余等下次再清 */
    }
    for (u32 i = 0; i < all_total; i++) {
        char nm[13];
        char path[64] = {0};
        FILE *fp;
        memcpy(nm, file_name + i * 12, 12);
        nm[12] = 0;
        if (strncmp(nm, "R", 1) || !strstr(nm, ".TMP")) {
            continue;
        }
        strcat(path, "storage/virfat_flash/C/");
        strcat(path, nm);
        fp = fopen(path, "r");
        if (fp) {
            fdelete(fp);
            freed++;
            log_info("%s del leftover tmp:%s\n", __func__, nm);
        }
    }
    free(file_name);
    if (freed) {
        log_info("%s freed %d tmp files\n", __func__, freed);
    }
}

/* 接收文件落盘根目录: SD 用 CONFIG_ROOT_PATH(如 storage/sd1/C/), 否则内置 virfat */
static void chat_recv_root(char *buf, int size, u8 use_sd)
{
    if (use_sd) {
        snprintf(buf, size, "%s", CONFIG_ROOT_PATH);
    } else {
        snprintf(buf, size, "storage/virfat_flash/C/");
    }
}

/* 外部卡(SD0/SD1, 以当前生效的 SDX_DEV 为准)是否在线(插入/挂载成功) */
static int chat_recv_sd_online(void)
{
#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    return dev_manager_check_by_logo(SDX_DEV) ? 1 : 0;
#else
    return 0;
#endif
}

/* 查询某根目录剩余空间(KB), 0 表示该盘不存在/不可写 */
static u32 chat_recv_free_space(const char *root)
{
    char path[96];
    u32 kb = 0;
    snprintf(path, sizeof(path), "%s%s", root, "R*******.TMP");
    fget_free_space(path, &kb);
    return kb;
}

/* 在某根目录选一个空闲 R 文件名(8.3, 探测 fopen 是否存在) */
static void chat_recv_pick_name(const char *root, const char *ext, char *out16)
{
    u32 i;
    for (i = 0; i < 9999999; i++) {
        char cand[24];
        char path[96];
        FILE *t;
        snprintf(cand, sizeof(cand), "R%07d.%s", i, ext);
        snprintf(path, sizeof(path), "%s%s", root, cand);
        t = fopen(path, "r");
        if (!t) {
            snprintf(out16, 16, "%s", cand);
            return;
        }
        fclose(t);
    }
    snprintf(out16, 16, "R%07d.%s", 0, ext);
}

int vstream_file_save(u8 *data, int len)
{
    struct chat_data_mult_packet *packet = (struct chat_data_mult_packet *)data;
    int type = 0;
    char path[96] = {0};
    u8  use_sd = 0;
    //第一包数据，创建文件
    if (packet->offset == 0) {
        // 新文件开始: 复位
        __info->recv_drop_file = 0;
        __info->recv_use_sd = 0;
        __info->save_total_len = 0;
        // 第一包即进入文件接收态，后续分包期间保持聊天页抢占。
        kid_wt_chat_file_recv_preempt_push();
        /* ASSERT(!__info->fp); */
        if (__info->fp) {
            fdelete(__info->fp);
            __info->fp = NULL;
        }
        u32 space;
        /* 1) 优先写内置 virfat */
        chat_recv_root(path, sizeof(path), 0);
        snprintf(path + strlen(path), sizeof(path) - strlen(path), "%s", "R*******.TMP");
        space = chat_recv_free_space("storage/virfat_flash/C/");
        space *= 1024;
        if (packet->total_len > space) {
            /* 先清理 virfat 残留 R*.TMP 再看一次 */
            chat_recv_purge_leftover_tmp();
            space = chat_recv_free_space("storage/virfat_flash/C/");
            space *= 1024;
        }
        /* 2) virfat 不够 -> 尝试写 SD(如果有卡) */
        if (packet->total_len > space && chat_recv_sd_online()) {
            char sd_path[96];
            u32 sd_space;
            chat_recv_root(sd_path, sizeof(sd_path), 1);
            sd_space = chat_recv_free_space(sd_path);
            sd_space *= 1024;
            if (packet->total_len <= sd_space) {
                use_sd = 1;
                space = sd_space;
                snprintf(path, sizeof(path), "%s%s", sd_path, "R*******.TMP");
            }
        }
        /* 3) 都没空间 -> 丢弃文件内容, 仅补记录+ACK(让消息框出现) */
        if (packet->total_len > space) {
            log_error("%s  no space anywhere (virfat+sd), drop file, keep chat record\n", __func__);
#if CONFIG_JL_UI_ENABLE
            UI_WINDOW_PREEMPTION_POSH(ID_WINDOW_NOT_SPACE, NULL, NULL, UI_WINDOW_PREEMPTION_TYPE_PHONE);
            UI_WINDOW_PREEMPTION_POP_DELAY(ID_WINDOW_NOT_SPACE, 2000);
#endif
            /* 放弃文件内容但仍接收/计数: 等 CRC 尾包到达时补一条"接收"记录并回 ACK,
             * 让对端聊天列表能显示这条图片(文件未能落盘)。 */
            __info->recv_drop_file = 1;
            __info->recv_use_sd = 0;
            __info->save_total_len = packet->total_len;
            return 0;
        }
        __info->recv_use_sd = use_sd;
        __info->fp = fopen(path, "w+");
        if (!__info->cache_buf) {
            __info->cache_buf = malloc(CACHE_BUF_SIZE);
            if (!__info->cache_buf) {
                log_error("%s cache_buf malloc(%d) failed\n", __func__, CACHE_BUF_SIZE);
                KWT_MEM_STATS("chat.cache_buf malloc fail");
            }
        }
        __info->cache_buf_offset = 0;
        __info->save_file_len = 0;
    }

    /* 丢弃文件模式(两盘都没空间): 只计数不写盘; 到最后一包且已收到 CRC cmd 时
     * 补记录+ACK(无真实文件)。 */
    if (__info->recv_drop_file) {
        __info->save_file_len += packet->data_len;
        if ((packet->offset + packet->data_len >= __info->save_total_len) &&
            __info->file_crc && __info->file_cmd) {
            u32 crc = __info->file_crc;
            vstream_cmd_ask_chat_with_file(__info->file_cmd, __info->file_sn, (u8 *)&crc, 4);
        }
        return 0;
    }

    //逐包写入
    if (!__info->fp) {
        // 文件句柄无效说明接收失败，释放聊天页抢占状态。
        kid_wt_chat_file_recv_preempt_pop();
        return -1;
    }
    __info->save_file_len += packet->data_len;

#if 1//4kcache
    if (__info->cache_buf_offset + packet->data_len >= CACHE_BUF_SIZE) {
        int copy_len  = CACHE_BUF_SIZE - __info->cache_buf_offset;
        /* printf("%s off:%d len:%d cp:%d \n",__func__,__info->cache_buf_offset,packet->data_len,copy_len); */
        memcpy(__info->cache_buf + __info->cache_buf_offset, packet->data, copy_len);
        fwrite(__info->cache_buf, 1, CACHE_BUF_SIZE, __info->fp);
        __info->cache_buf_offset = 0;
        if (copy_len != packet->data_len) {
            int left_len = packet->data_len - copy_len;
            /* printf("%s off:%d len:%d cp:%d left:%d\n",__func__,__info->cache_buf_offset,packet->data_len,copy_len,left_len); */
            memcpy(__info->cache_buf, packet->data + copy_len, left_len);
            __info->cache_buf_offset += left_len;
        }
    } else {
        /* printf("%s off:%d len:%d \n",__func__,__info->cache_buf_offset,packet->data_len); */
        memcpy(__info->cache_buf + __info->cache_buf_offset, packet->data, packet->data_len);
        __info->cache_buf_offset += packet->data_len;
    }
    if ((packet->offset + packet->data_len == packet->total_len) && (__info->cache_buf_offset != 0)) {
        fwrite(__info->cache_buf, 1, __info->cache_buf_offset, __info->fp);
    }
#else
    /* fseek(__info->fp, packet->offset, SEEK_SET); */
    fwrite(packet->data, 1, packet->data_len, __info->fp);
#endif

    log_debug("%s offset:%d data_len%d data_end:%d total:%d save:%d \n", __func__, packet->offset, packet->data_len, packet->offset + packet->data_len, packet->total_len, __info->save_file_len);
    /* put_buf(packet->data, packet->data_len); */
    //最后一包，关闭文件，保存记录
    if (packet->offset + packet->data_len == packet->total_len) {
        int file_name_len = 13;
        fget_name(__info->fp, __info->last_name, file_name_len);
        //文件数据校验
        if (__info->save_file_len == packet->total_len) {
            fclose(__info->fp);
        } else {
            fdelete(__info->fp);
            memset(__info->last_name, 0, 16);
        }
        log_debug("%s name:%s \n", __func__, __info->last_name);
        __info->fp = NULL;

        log_debug("%s crc:%x cmd:%x \n", __func__, __info->file_crc, __info->file_cmd);
        if ((__info->file_crc != 0) && (__info->file_cmd != 0)) {
            vstream_cmd_ask_chat_with_file(__info->file_cmd, __info->file_sn, (u8 *)&__info->file_crc, 4);
        }
    }
    return 0;
}
//接收数据
int vstream_cmd_ask_chat(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = 0;
    switch (cmd) {
    case VSTREAM_CMD_CHAT_MEME:
    case VSTREAM_CMD_CHAT_TEXT:
    case VSTREAM_CMD_CHAT_MUSIC:
        ret = vstream_cmd_ask_chat_with_index(cmd, cmd_sn, data, len);
        vstream_cmd_send_recv(cmd, cmd_sn, NULL, 0);
        break;
    case VSTREAM_CMD_CHAT_VOICE:
    case VSTREAM_CMD_CHAT_PHOTO:
        ret = vstream_cmd_ask_chat_with_file(cmd, cmd_sn, data, len);
        break;
    }

    return ret;
}
//应答数据
int vstream_cmd_recv_chat(u8 cmd, u8 cmd_sn, u8 *data, int len)
{
    int ret = 0;
    log_debug("%s sn:%d \n", __func__, cmd_sn);
    chat_send_list_update_status_by_sn(cmd_sn, FILE_STATUS_SEND);
    chat_send_list_del_by_sn(cmd_sn, chat_send_list_cb);
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief vstream_cmd_send_chat 发送消息
 *
 * @param type
 * @param data
 * @param data_len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int vstream_cmd_send_chat(u8 type, u8 *data, int data_len)
{
    FILE *fp = NULL;
    int total_len = data_len;
    int cmd = 0;
    switch (type) {
    case CHAT_TYPE_MEME:
        cmd = VSTREAM_CMD_CHAT_MEME;
        break;
    case CHAT_TYPE_TEXT:
        cmd = VSTREAM_CMD_CHAT_TEXT;
        break;
    case CHAT_TYPE_MUSIC:
        cmd = VSTREAM_CMD_CHAT_MUSIC;
        break;
    case CHAT_TYPE_VOICE:
        cmd = VSTREAM_CMD_CHAT_VOICE;
        fp  = fopen((const char *)data, "r");
        ASSERT(fp);
        total_len = flen(fp);
        log_debug("%s(len:%d) path:%s", __func__, total_len, data);
        break;
    case CHAT_TYPE_PIC:
        cmd =  VSTREAM_CMD_CHAT_PHOTO;
        fp  = fopen((const char *)data, "r");
        ASSERT(fp);
        total_len = flen(fp);
        break;
    }

    int packet_head = sizeof(struct chat_data_mult_packet);
    u8 *send_data = zalloc(FILE_PACKET_SIZE	+ packet_head);
    if (!send_data) {
        log_error("%s zalloc(%d) failed\n", __func__, FILE_PACKET_SIZE + packet_head);
        KWT_MEM_STATS("chat.send_data zalloc fail");
        return -1;
    }
    struct chat_data_mult_packet *packet = (struct chat_data_mult_packet *)send_data;
    int packet_max_len = FILE_PACKET_SIZE;
    int offset = 0;
    u32 crc = 0;
    u8 sn = 0;
    //本次发送开始前清空抢占标志
    _chat_send_abort_flag = 0;
    //分包发送
    int ret = 0;
    do {
        //每分包前检查是否被高优先级事件抢占（如对端视频请求）
        if (_chat_send_abort_flag) {
            log_info("%s aborted by preempt\n", __func__);
            ret = -1;
            break;
        }
        int packet_data_len = (offset + packet_max_len) > total_len ? total_len - offset : packet_max_len;
        packet->offset = offset;
        packet->total_len = total_len;
        packet->data_len = packet_data_len;
        log_debug("%s offset:%d data_len%d data_end:%d total:%d", __func__, packet->offset, packet->data_len, packet->offset + packet->data_len, packet->total_len);
        if (fp) {
            fread(packet->data, 1, packet_data_len, fp);
            extern int vstream_send_file(u8 cmd, u8 * data, int data_len);
            ret = vstream_send_file(cmd, send_data, packet_data_len + packet_head);
            //顺带卡一下发送时间，不校验crc也算
            crc = CRC16_with_initval(packet->data, packet_data_len, crc);
            /* put_buf(packet->data, packet->data_len); */
        } else {
            memcpy(packet->data, data, packet_data_len);
            ret = vstream_cmd_send_with_adk_sn(cmd, &sn, send_data, packet_data_len + packet_head);
        }
        /* os_time_dly(1); */
        if (ret) {
            log_error("%s ret:%d\n", __func__, ret);
            break;
        }
        offset += packet_data_len;
    } while (offset < total_len);
    free(send_data);
    if (fp) {
        //仅在未被抢占且未出错时发结束包，避免占用对端处理时间
        if (!ret && !_chat_send_abort_flag) {
            vstream_cmd_send_with_adk_sn(cmd, &sn, (u8 *)&crc, 4);
        }
        fclose(fp);
        /* fp = NULL; */
    }


    //save
    u32 fid = 0;
    u8 status = FILE_STATUS_LOADING;
    if (ret) {
        status = FILE_STATUS_ERROR;
    }
    if (fp) {
        u8 *save_data = (u8 *)strrchr((const char *)data, '/');
        save_data++;
        ret = kid_wt_chat_data_storage_write(type, &fid, save_data, 12, status);
    } else {
        ret = kid_wt_chat_data_storage_write(type, &fid, data, data_len, status);
    }
    log_debug("%s (sn:%d fid:%x)succ!!\n", __func__, sn, fid);
    //
    if (status ==  FILE_STATUS_LOADING) {
        chat_send_list_add(fid, sn);
    }
    return ret;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief vstream_cmd_send_chat_save 本地保存
 *
 * @param cmd
 * @param data
 * @param data_len
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int vstream_cmd_send_chat_save(u8 cmd, u8 *data, int data_len, u8 status)
{
    int type = 0;
    switch (cmd) {
    case VSTREAM_CMD_CHAT_MEME:
        type = CHAT_TYPE_MEME;
        break;
    case VSTREAM_CMD_CHAT_TEXT:
        type = CHAT_TYPE_TEXT;
        break;
    case VSTREAM_CMD_CHAT_MUSIC:
        type = CHAT_TYPE_MUSIC;
        break;
    case VSTREAM_CMD_CHAT_VOICE:
        type = CHAT_TYPE_VOICE;
        break;
    case VSTREAM_CMD_CHAT_PHOTO:
        type = CHAT_TYPE_PIC;
        break;
    }
    u32 fid  = 0;
    int ret = kid_wt_chat_data_storage_write(type, &fid, data, data_len, status);

    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief kid_wt_chat_init 初始化存储空间
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int kid_wt_chat_init()
{
    //初始化存储空间
    data_small_file_init();
    //初始化发送链表(链表头 + list_lock 互斥量)。必须最先执行：
    //否则 chat_send_list_add 会对未 INIT_LIST_HEAD 的 head 做 list_add_tail，
    //head->prev 为 NULL，prev->next=new 写地址 0，触发 hmem 写异常；
    //且 os_mutex_pend 会在未 os_mutex_create 的 list_lock 上报 OSEventType ERR。
    chat_send_list_init();
    return 0;

}

int kid_wt_chat_deinit()
{
    return 0;
}



#if  0
//测试代码
//与 vstream_cmd_send_chat一致
int __vstream_cmd_chat_test(u8 type, u8 *data, int data_len)
{
    int cmd = 0;
    switch (type) {
    case CHAT_TYPE_MEME:
        cmd =  VSTREAM_CMD_CHAT_MEME;
        break;
    case CHAT_TYPE_TEXT:
        cmd =  VSTREAM_CMD_CHAT_TEXT;
        break;
    case CHAT_TYPE_MUSIC:
        cmd =  VSTREAM_CMD_CHAT_MUSIC;
        break;
    case CHAT_TYPE_VOICE:
        cmd =  VSTREAM_CMD_CHAT_VOICE;
        break;
    case CHAT_TYPE_PIC:
        cmd =  VSTREAM_CMD_CHAT_PHOTO;
        break;
    }
    u8 *send_data = zalloc(512);
    struct chat_data_mult_packet *packet = (struct chat_data_mult_packet *)send_data;
    int packet_head = sizeof(struct chat_data_mult_packet);
    int packet_max_len = 512 - packet_head;
    int offset = 0;
    //分包发送
    int ret = 0;
    do {
        int packet_data_len = (offset + packet_max_len) > data_len ? data_len - offset : packet_max_len;
        packet->offset = offset;
        packet->total_len = data_len;
        packet->data_len = packet_data_len;
        log_debug("%s offset:%d data_len:%d total:%d", __func__, offset, packet_data_len, data_len);
        memcpy(packet->data, data, packet_data_len);
        ret = vstream_cmd_ask_chat(cmd, 0, send_data, packet_data_len + packet_head);
        if (ret) {
            break;
        }
        offset += packet_data_len;
    } while (offset < data_len);
    free(send_data);
    return ret;
}
void vstream_cmd_chat_test()
{
    //初始化
    kid_wt_chat_init();
#if 1
    //增
    for (int index = 0; index < 2; index++) {
        __vstream_cmd_chat_test(CHAT_TYPE_MEME, &index, sizeof(int));
    }
    FILE *fp = fopen("storage/virfat_flash/C/CAM_0000.JPG", "r");
    if (fp) {
        int file_len = flen(fp);
        log_debug("%s file_trans_test!!!(%d)", __func__, file_len);
        u8 *jpg = malloc(file_len);
        fread(jpg, 1, file_len, fp);
        if (jpg) {
            __vstream_cmd_chat_test(CHAT_TYPE_PIC, jpg, file_len);
        }
        free(jpg);
        fclose(fp);
    }
    for (int index = 2; index < 5; index++) {
        __vstream_cmd_chat_test(CHAT_TYPE_MEME, &index, sizeof(int));
    }

    //读取
    u8 tmp_buf[20];
    struct chat_data_storage *save = (struct chat_data_storage *)tmp_buf;
    int total =  kid_wt_char_get_rec_data_total();
    for (int i = 0; i < total; i++) {
        kid_wt_chat_get_rec_data_by_index(i, save);
        kid_wt_chat_save_dump(save);
    }
#if  0
    //删
    kid_wt_chat_data_storage_del();
    kid_wt_chat_data_storage_del();

    //读取
    total =  kid_wt_char_get_rec_data_total();
    for (int i = 0; i < total; i++) {
        kid_wt_chat_get_rec_data_by_index(i, save);
        kid_wt_chat_save_dump(save);
    }
#endif
#endif
}






#endif





