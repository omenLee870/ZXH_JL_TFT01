#include "app_config.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "system/includes.h"
#include "key_event_deal.h"

#include "health_manager/health_manager.h"
#include "data_storage/data_storage.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-FILE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".health_manager.data.bss")
#pragma data_seg(".health_manager.data")
#pragma const_seg(".health_manager.text.const")
#pragma code_seg(".health_manager.text")
#endif

#define SHM_FILE_MAGIC 0X5a
struct shm_file_hd {
    u8 magic;
    u8 type;
    u16 id;
};
#if  TCFG_DATA_STORAGE_ENABLE
static void *__sport_health_file_open(u8 type, u16 id)
{
    struct shm_file_hd *hd = zalloc(sizeof(struct shm_file_hd));
    hd->type = type;
    hd->id  = id;
    hd->magic = SHM_FILE_MAGIC;
    log_debug("[open] id:%d", id);
    return (void *)hd;
}
static int __sport_health_file_write(void *hd, u8 *buf, int offset, int len)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);
    u32 file_id  = fp->id;
    log_debug("[write]type:%d id:%d offset:%d len:%d", fp->type, file_id, offset, len);
    u32 ret = small_file_write(fp->type, &file_id, offset, (void *)buf, len, 0);
    fp->id = file_id;
    ret += offset;
    return ret;
}
static int __sport_health_file_update(void *hd, u8 *buf, int offset, int len)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);
    log_debug("[update]type:%d id:%d offset:%d len:%d", fp->type, fp->id, offset, len);
    u32 ret = small_file_update_by_id(fp->type, fp->id, offset, (void *)buf, len, 0);
    return ret;
}
static int __sport_health_file_read(void *hd, u8 *buf, int offset, int len)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);

    log_debug("[read]type:%d id:%d offset:%d len:%d", fp->type, fp->id, offset, len);
    return small_file_read(fp->type, fp->id, offset, (void *)buf, len);
}

static int __sport_health_file_close(void *hd)
{
    if (hd) {
        struct shm_file_hd *fp = (struct shm_file_hd *)hd;
        ASSERT(fp->magic == SHM_FILE_MAGIC);

        log_debug("[close]type:%d id:%d ", fp->type, fp->id);
        free(hd);
        /* hd = NULL; */
    }
    return 0;
}
static int __sport_health_file_get_len(void *hd)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);

    log_debug("[len]type:%d id:%d ", fp->type, fp->id);
    return small_file_get_size_by_id(fp->type, fp->id);
}
static int __sport_health_file_get_total(u8 type)
{

    log_debug("[total]type:%d ", type);
    return small_file_get_count(type);
}
static int __sport_health_file_delete(void *hd)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);

    log_debug("[delete]type:%d id:%d ", fp->type, fp->id);
    return  small_file_delete_by_id(fp->type, fp->id);
}
static int __sport_health_file_get_id(void *hd, u8 index)
{
    struct shm_file_hd *fp = (struct shm_file_hd *)hd;
    ASSERT(fp->magic == SHM_FILE_MAGIC);
    fp->id = small_file_get_id_by_index(fp->type, index);

    log_debug("[get_id]type:%d id:%d index:%d ", fp->type, fp->id, index);
    return  fp->id;
}
static void *__sport_health_file_open_by_time(u8 type, u16 year, u8 month, u8 day)
{
    log_debug("[open]type:%d [%d-%d-%d]", type, year, month, day);
    if ((type == F_TYPE_SLEEP) || (type == F_TYPE_HEART) || (type == F_TYPE_BLOOD_OXYGEN)) {
        struct shm_file_hd *fp = (struct shm_file_hd *)__sport_health_file_open(type, 0);
        if (!fp) {
            return NULL;
        }
        int total = __sport_health_file_get_total(type);
        for (int i = 0; i < total; i++) {
            fp->id = __sport_health_file_get_id((void *)fp, i);
            struct health_file_total_head total_head;
            __sport_health_file_read(fp, (u8 *)&total_head, 0, sizeof(struct health_file_total_head));
            sport_health_common_swapX((const u8 *)&total_head.year, (u8 *)&total_head.year, 2);
            if ((total_head.year == year) && (total_head.month == month) && (total_head.day == day)) {
                log_debug("[open]suc type:%d id:%d", fp->type, fp->id);
                return (void *)fp;
            }
        }
        __sport_health_file_close((void *)fp);
    }
    return NULL;
}
const struct shm_file_ops __shm_file_ops = {
    .open = __sport_health_file_open,
    .write = __sport_health_file_write,
    .open_by_time =  __sport_health_file_open_by_time,
    .read = __sport_health_file_read,
    .update = __sport_health_file_update,
    .close =  __sport_health_file_close,
    .delete = __sport_health_file_delete,
    .total =  __sport_health_file_get_total,
    .len  = __sport_health_file_get_len,
    .get_id = __sport_health_file_get_id,
};
#else

const struct shm_file_ops __shm_file_ops = {0};
#endif
void *sport_health_file_open_by_time(u8 type, u16 year, u8 month, u8 day)
{
    if (__shm_file_ops.open_by_time) {
        return __shm_file_ops.open_by_time(type, year, month, day);
    }
    return NULL;
}
void *sport_health_file_open(u8 type, u16 id)
{
    if (__shm_file_ops.open) {
        return __shm_file_ops.open(type, id);
    }
    return NULL;
}

int sport_health_file_write(void *hd, u8 *buf, int offset, int len)
{
    if (__shm_file_ops.write) {
        return __shm_file_ops.write(hd, buf, offset, len);
    }
    return 0;
}
int sport_health_file_update(void *hd, u8 *buf, int offset, int len)
{
    if (__shm_file_ops.update) {
        return __shm_file_ops.update(hd, buf, offset, len);
    }
    return 0;
}
int sport_health_file_read(void *hd, u8 *buf, int offset, int len)
{
    if (__shm_file_ops.read) {
        return __shm_file_ops.read(hd, buf, offset, len);
    }
    return 0;
}

int sport_health_file_close(void *hd)
{
    if (__shm_file_ops.close) {
        return __shm_file_ops.close(hd);
    }
    return 0;
}
int sport_health_file_get_len(void *hd)
{
    if (__shm_file_ops.len) {
        return __shm_file_ops.len(hd);
    }
    return 0;
}
int sport_health_file_get_total(u8 type)
{
    if (__shm_file_ops.total) {
        return __shm_file_ops.total(type);
    }
    return 0;
}
int sport_health_file_delete(void *hd)
{
    if (__shm_file_ops.delete) {
        return __shm_file_ops.delete(hd);
    }
    return 0;
}
int sport_health_file_get_id(void *hd, u8 index)
{
    if (__shm_file_ops.get_id) {
        return __shm_file_ops.get_id(hd, index);
    }
    return 0;
}
