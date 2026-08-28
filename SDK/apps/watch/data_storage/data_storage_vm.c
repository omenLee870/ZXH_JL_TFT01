#include "app_config.h"
#include "asm/cpu.h"
#include "device/device.h"
#include "flashdb.h"
#include "data_storage.h"
#include "os/os_api.h"
#include "syscfg_id.h"

#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[VM-DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".storage.data.bss")
#pragma data_seg(".storage.data")
#pragma const_seg(".storage.text.const")
#pragma code_seg(".storage.text")
#endif

#if TCFG_DATA_STORAGE_ENABLE && TCFG_DATA_STORAGE_VM_ENABLE

//================================================//
//                      说明
//目前使用vm id数量限制在SMALL_FILE_VM_ITEM_MAX
//
//每条数据存储格式为 |id|len|用户数据|
//
//================================================//



#define SMALL_FILE_VM_ITEM_MAX  (VM_SMALL_FILE_END - VM_SMALL_FILE_START + 1)

struct small_file_vm {
    u8 type;        /*文件类型*/
    u8 max_item;    /*文件数量最大值*/
    u8 vm_max_item; /*使用vm id数量最大值*/
    u16 vm_id_table[SMALL_FILE_VM_ITEM_MAX]; /*对应vm id*/
};

struct small_file_vm_storage_info {
    u32 id;
    u32 len;
};

struct small_file_info {
    u8  type;           /*文件类型*/
    u8  max_item;       /*文件数量最大值*/
    u8  cur_item;       /*当前文件数量*/
    u16 *size_table;    /*每个文件对应的大小*/
    u32 *table;         /*每个文件对应的id*/
    u8 vm_max_item;     /*使用vm id数量最大值*/
    u16 *vm_id_table;   /*文件用于存储的id号*/
};

struct small_file_hd {
    struct small_file_info *child;
    OS_MUTEX mutex;
};

/*当前max_item未使用*/
const struct small_file_vm data_small_file_info[] = {
    {F_TYPE_PHONEBOOK,      10,     1,   {VM_SMALL_FILE_PHONEBOOK}},
    {F_TYPE_CALL_LOG,       10,     1,   {VM_SMALL_FILE_CALL_LOG}},
    {F_TYPE_WEATHER,        1,      1,   {VM_SMALL_FILE_WEATHER}},
    {F_TYPE_HEART,        1,      1,   {VM_SMALL_FILE_HEART}},
    {
        F_TYPE_MESSAGE,        5,      5,   {
            VM_SMALL_FILE_MESSAGE_0, VM_SMALL_FILE_MESSAGE_1, \
            VM_SMALL_FILE_MESSAGE_2, VM_SMALL_FILE_MESSAGE_3, VM_SMALL_FILE_MESSAGE_4
        }
    },
    {F_TYPE_SPORTRECORD,    2,      2,   {VM_SMALL_FILE_SPORTRECORD_0, VM_SMALL_FILE_SPORTRECORD_1}},
    {
        F_TYPE_BLOOD_OXYGEN,   4,      4,   {
            VM_SMALL_FILE_BLOOD_OXYGEN_0, VM_SMALL_FILE_BLOOD_OXYGEN_1, \
            VM_SMALL_FILE_BLOOD_OXYGEN_2, VM_SMALL_FILE_BLOOD_OXYGEN_3
        }
    },
    {
        F_TYPE_SLEEP,          4,      4,   {
            VM_SMALL_FILE_SLEEP_0, VM_SMALL_FILE_SLEEP_1, \
            VM_SMALL_FILE_SLEEP_2, VM_SMALL_FILE_SLEEP_3
        }
    },
};


static int small_file_info_delete_by_index(struct small_file_info *file, int index);
static int small_file_table_init(u8 small_file_type);
static u32 __is_new_id(struct small_file_info *info, u32 id);
static u32 __new_id_create(void *priv, int min, int max);
static u32 __find_id_index(struct small_file_info *info, u32 id);
static int __get_vm_saved_info(u16 vm_id, struct small_file_vm_storage_info *storage_info);
static int __read_vm_saved_data(u16 vm_id, u8 *buf, u32 len);
static int __write_vm_saved_data(u16 vm_id, u32 stored_id, u8 *buf, u32 len);
static int __del_vm_saved_item(u16 vm_id);
static int __data_small_file_info_init(void);
static int __data_small_file_info_update(void);

static struct small_file_hd *hd;

int data_small_file_init(void)
{
    int ret;

    if (hd) {
        /* 已初始化过，直接返回，避免重复创建互斥量并重复 zalloc 表导致越界 */
        return true;
    }

    hd = zalloc(sizeof(struct small_file_hd) \
                + F_TYPE_COUNT * sizeof(struct small_file_info));

    if (!hd) {
        goto __err;
    }

    hd->child = (struct small_file_info *)(hd + 1);

    os_mutex_create(&hd->mutex);

    __data_small_file_info_init();

    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file_vm *file = (struct small_file_vm *)&data_small_file_info[i];
        small_file_table_init(file->type);
    }

    return true;

__err:
    return false;
}

int small_file_get_id_table_len(u8 small_file_type)
{
    return small_file_get_id_table(small_file_type, NULL, 0);
}

int small_file_get_id_table(u8 small_file_type, u8 *table_data, u16 data_len)
{
    int count;
    struct small_file_info *file;

    if (!hd) {
        return 0;
    }
    u16 id;
    u16 file_len;

    ASSERT(small_file_type < F_TYPE_COUNT);
    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];
    count = file->cur_item;
    for (int i = 0; (i < count) && (i * 4 < data_len); i++) {
        if (table_data) {
            id = (u16)file->table[i];
            file_len  = (u16)file->size_table[i];
            memcpy(table_data, &id, sizeof(u16));
            table_data += 2;
            memcpy(table_data, &file_len, sizeof(u16));
            table_data += 2;
        }
    }

    os_mutex_post(&hd->mutex);
    return count * 4;
}

int small_file_delete_by_id(u8 small_file_type, u32 id)
{
    int ret;
    u16 vm_id = 0;
    u32 index;
    struct small_file_info *file;

    if (!hd) {
        return false;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    file = &hd->child[small_file_type];

    os_mutex_pend(&hd->mutex, 0);

    if (!file->cur_item) {
        goto __err;
    }
    if (id == 0) {
        goto __err;
    }

    index = __find_id_index(file, id);
    if (index >= file->cur_item) {
        log_error("<%s> index:%d id:%d", __func__, index, id);
        goto __err;
    }
    /*检查找到index的对应id是否正确*/
    if (file->table[index] != id) {
        log_error("<%s> index:%d id:%d find_id:%d", __func__, index, id, file->table[index]);
        goto __err;
    }

    vm_id = file->vm_id_table[index];

    ret = __del_vm_saved_item(vm_id);
    if (ret == false) {
        log_error("<%s> line:%d vm write err", __func__, __LINE__);
        goto __err;
    }

    for (; index < file->cur_item - 1; index++) {
        file->table[index] = file->table[index + 1];
        file->size_table[index] = file->size_table[index + 1];
        file->vm_id_table[index] = file->vm_id_table[index + 1];
    }

    --file->cur_item;
    file->table[file->cur_item] = 0;
    file->size_table[file->cur_item] = 0;
    file->vm_id_table[file->cur_item] = vm_id;

    log_info("<%s> file_type:%d id:%d succ!", __func__, file->type, id);

    ret = __data_small_file_info_update();
    if (ret == false) {
        log_error("<%s> __data_small_file_info_update error", __func__);
    }
    os_mutex_post(&hd->mutex);
    return true;

__err:
    log_error("<%s> file_type:%d id:%d error!", __func__, file->type, id);
    os_mutex_post(&hd->mutex);
    return false;
}

int small_file_delete_by_index(u8 small_file_type, int index)
{
    struct small_file_info *file;
    if (!hd) {
        return false;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    file = &hd->child[small_file_type];

    os_mutex_pend(&hd->mutex, 0);
    int ret = small_file_info_delete_by_index(file, index);
    os_mutex_post(&hd->mutex);
    return ret;

}

int small_file_del_all(void)
{
    int ret;
    if (!hd) {
        return false;
    }

    os_mutex_pend(&hd->mutex, 0);

    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file_vm *file = (struct small_file_vm *)&data_small_file_info[i];

        if (!file->type) {
            continue;
        }

        for (int j = 0; j < hd->child[file->type].cur_item; j++) {
            ret = __del_vm_saved_item(hd->child[file->type].vm_id_table[j]);
            if (ret == false) {
                log_error("<%s> line:%d vm del err", __func__, __LINE__);
                goto __err;
            }
            hd->child[file->type].table[j] = 0;
            hd->child[file->type].size_table[j] = 0;
        }
        hd->child[file->type].cur_item = 0;
    }

    log_info("<%s> succ!", __func__);
    ret = __data_small_file_info_update();
    if (ret == false) {
        log_error("<%s> __data_small_file_info_update error", __func__);
    }
    os_mutex_post(&hd->mutex);
    return true;

__err:
    log_error("<%s> del small_file error", __func__);
    os_mutex_post(&hd->mutex);
    return false;
}

int small_file_read(u8 small_file_type, u32 id, u32 offset, void *buf, u32 len)
{
    int read_len = len;
    u32 vm_id = 0;
    int saved_len = 0;
    u32 cur_item = 0;
    struct small_file_info *file;

    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);

    file = &hd->child[small_file_type];
    if (!file->cur_item) {
        goto __err;
    }

    cur_item = __find_id_index(file, id);
    if (cur_item >= file->cur_item) {
        log_error("<%s> id:%d error", __func__, id);
        goto __err;
    }
    vm_id = file->vm_id_table[cur_item];

    u8 *temp_data = zalloc(offset + len);
    if (!temp_data) {
        log_error("%s temp_data is null", __func__);
        goto __err;
    }
    saved_len = __read_vm_saved_data(vm_id, temp_data, offset + len);
    if (saved_len > offset) {
        if (saved_len < offset + len) {
            read_len = saved_len - offset;
        }
        memcpy(buf, &temp_data[offset], read_len);
    } else {
        read_len = 0;
    }

    free(temp_data);
    log_info("<%s> file_type:%d id:%d read succ!", __func__, file->type, id);
    os_mutex_post(&hd->mutex);
    return read_len;

__err:
    log_error("<%s> file_type:%d id:%d read error!", __func__, file->type, id);
    os_mutex_post(&hd->mutex);
    return 0;
}


u32 small_file_write(u8 small_file_type, u32 *id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    int ret;
    u16 vm_id;
    u8 *temp_data = NULL;
    u8 is_new_id = 1;
    u32 cur_item = 0;
    struct small_file_info *file;
    struct small_file_vm_storage_info stored_data_info;

    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];
    if (!file->max_item) {
        goto __err;
    }

    if (!*id) {
        if (buf_offset) {
            ASSERT(file->cur_item);
            *id =  file->table[file->cur_item - 1];
            //临时特殊处理一下app 不发id号
        }
    }
    is_new_id = __is_new_id(file, *id);

    while (is_new_id && file->cur_item >= file->max_item) {
        small_file_info_delete_by_index(file, 0);
    }

    if (!*id) {
        *id = __new_id_create(file, 0x1, 0xffff);
    }
    cur_item = __find_id_index(file, *id);
    if (cur_item >= file->cur_item) {
        log_error("<%s> id:%d error", __func__, *id);
        goto __err;
    }
    vm_id = file->vm_id_table[cur_item];


    if (buf_offset) {
        if (__get_vm_saved_info(vm_id, &stored_data_info) == true) {
            temp_data = zalloc(buf_offset + len);
            if (!temp_data) {
                log_error("%s temp_data is null", __func__);
                goto __err;
            }
            ret = __read_vm_saved_data(vm_id, temp_data, stored_data_info.len);
            if (ret != stored_data_info.len) {
                goto __err;
            }
            memcpy(&temp_data[buf_offset], buf, len);
            ret = __write_vm_saved_data(vm_id, stored_data_info.id, temp_data, buf_offset + len);
            if (ret != (buf_offset + len)) {
                log_error("<%s> line:%d vm write err", __func__, __LINE__);
                goto __err;
            }
        } else {
            goto __err;
        }
    } else {
        ret = __write_vm_saved_data(vm_id, *id, buf, len);
        if (ret != len) {
            goto __err;
        }
    }


    if (is_new_id) {
        file->table[file->cur_item] = *id;
        file->size_table[file->cur_item] = len;
        file->cur_item++;
    } else {
        cur_item = __find_id_index(file, *id);
        file->table[cur_item] = *id;
        file->size_table[cur_item] = buf_offset + len;
        if (cur_item == file->cur_item) {
            file->cur_item++;
            log_info("<%s> id:%d del rwrite", __func__, *id);
        }
    }

    if (temp_data) {
        free(temp_data);
    }


    log_info("<%s> file_type:%d id:%d succ!", __func__, file->type, *id);

    ret = __data_small_file_info_update();
    if (ret == false) {
        log_error("<%s> __data_small_file_info_update error", __func__);
    }
    os_mutex_post(&hd->mutex);
    return len;

__err:
    if (temp_data) {
        free(temp_data);
    }
    log_info("<%s> file_type:%d id:%d error!", __func__, file->type, *id);
    os_mutex_post(&hd->mutex);
    return 0;
}

int small_file_update_by_id(u8 small_file_type, u32 id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    int ret;
    u16 vm_id;
    u32 cur_item = 0;
    u8 *temp_data = NULL;
    struct small_file_info *file;
    struct small_file_vm_storage_info stored_data_info;

    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];

    cur_item = __find_id_index(file, id);
    if (cur_item >= file->cur_item) {
        log_error("<%s> id:%d error", __func__, id);
        goto __err;
    }
    vm_id = file->vm_id_table[cur_item];

    if (__get_vm_saved_info(vm_id, &stored_data_info) == true) {
        if (stored_data_info.len < (buf_offset + len)) {
            goto __err;
        }
        temp_data = zalloc(stored_data_info.len);
        if (!temp_data) {
            log_error("%s temp_data is null", __func__);
            goto __err;
        }
        ret = __read_vm_saved_data(vm_id, temp_data, stored_data_info.len);
        if (ret != stored_data_info.len) {
            goto __err;
        }
        memcpy(&temp_data[buf_offset], buf, len);
        ret = __write_vm_saved_data(vm_id, stored_data_info.id, temp_data, stored_data_info.len);
        if (ret != stored_data_info.len) {
            log_error("<%s> line:%d vm write err", __func__, __LINE__);
            goto __err;
        }
    } else {
        goto __err;
    }

    free(temp_data);
    log_info("<%s> file_type:%d id:%d succ!", __func__, file->type, id);
    ret = __data_small_file_info_update();
    if (ret == false) {
        log_error("<%s> __data_small_file_info_update error", __func__);
    }
    os_mutex_post(&hd->mutex);
    return len;

__err:
    if (temp_data) {
        free(temp_data);
    }
    os_mutex_post(&hd->mutex);
    return 0;
}

u32 small_file_get_size_by_index(u8 small_file_type, int index)
{
    struct small_file_info *file;
    u32 size;

    if (!hd) {
        return 0;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    file = &hd->child[small_file_type];
    if (!file->cur_item || index >= file->cur_item) {
        return 0;
    }
    os_mutex_pend(&hd->mutex, 0);
    size = file->size_table[index];
    os_mutex_post(&hd->mutex);
    return size;
}

u32 small_file_get_size_by_id(u8 small_file_type, int id)
{
    struct small_file_info *file;
    u32 size = 0;

    if (!hd) {
        return 0;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    file = &hd->child[small_file_type];
    os_mutex_pend(&hd->mutex, 0);
    for (int index = 0; index < file->cur_item; index++) {
        if (file->table[index] == id) {
            size = file->size_table[index];
        }
    }
    os_mutex_post(&hd->mutex);
    return size;
}

u32 small_file_get_id_by_index(u8 small_file_type, int index)
{
    struct small_file_info *file;
    u32 id;

    if (!hd) {
        return 0;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    file = &hd->child[small_file_type];
    if (!file->cur_item || index >= file->cur_item) {
        return 0;
    }
    os_mutex_pend(&hd->mutex, 0);
    id = file->table[index];
    os_mutex_post(&hd->mutex);
    return id;
}

int small_file_get_count(u8 small_file_type)
{
    int count;
    struct small_file_info *file;
    if (!hd) {
        return 0;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];
    count = file->cur_item;
    os_mutex_post(&hd->mutex);
    return count;
}

static int small_file_info_delete_by_index(struct small_file_info *file, int index)
{
    u16 vm_id = 0;
    int ret;

    if (!hd) {
        return false;
    }

    if (!file->cur_item || index >= file->cur_item) {
        return false;
    }

    vm_id = file->vm_id_table[index];

    for (; index < file->cur_item - 1; index++) {
        file->table[index] = file->table[index + 1];
        file->size_table[index] = file->size_table[index + 1];
        file->vm_id_table[index] = file->vm_id_table[index + 1];
    }

    --file->cur_item;
    file->table[file->cur_item] = 0;
    file->size_table[file->cur_item] = 0;
    file->vm_id_table[file->cur_item] = vm_id;

    ret = __del_vm_saved_item(vm_id);
    if (ret == false) {
        log_error("<%s> line:%d vm write err", __func__, __LINE__);
        goto __err;
    }

    log_info("<%s> file_type:%d index:%d succ!", __func__, file->type, index);

    ret = __data_small_file_info_update();
    if (ret == false) {
        log_error("<%s> __data_small_file_info_update error", __func__);
    }
    return true;

__err:
    return false;
}

static int small_file_table_init(u8 small_file_type)
{
    int ret;
    int count = 0;
    u32 id;
    struct small_file_info *file;
    struct small_file_vm_storage_info file_storage_info;

    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);
    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];
    file->cur_item = 0;

    for (int i = 0; i < file->vm_max_item; i++) {
        ret = __get_vm_saved_info(file->vm_id_table[i], &file_storage_info);
        if (ret == false) {
            continue;
        }
        file->size_table[file->cur_item] = file_storage_info.len;
        file->table[file->cur_item] = file_storage_info.id;
        file->cur_item++;
        ++count;
    }

    os_mutex_post(&hd->mutex);
    return count;
}

static u32 __is_new_id(struct small_file_info *info, u32 id)
{
    if (!id) {
        return true;
    }

    for (int index = 0; index < info->cur_item; index++) {
        if (info->table[index] == id) {
            return false;
        }
    }
    return true;
}

static u32 __new_id_create(void *priv, int min, int max)
{
    struct small_file_info *file = (struct small_file_info *)priv;
    u32 id = min;
    /*最小不重复id*/
__again:
    for (int index = 0; index < file->cur_item; index++) {
        if (file->table[index] == id) {
            id ++;
            if (!id || id > max) {
                id = min;
            }
            goto __again;
        }
    }
    return id;
}

static u32 __find_id_index(struct small_file_info *info, u32 id)
{
    for (int index = 0; index < info->cur_item; index++) {
        if (info->table[index] == id) {
            return index;
        }
    }
    return info->cur_item;
}

static int __get_vm_saved_info(u16 vm_id, struct small_file_vm_storage_info *storage_info)
{
    int ret;
    ret = syscfg_read(vm_id, storage_info, sizeof(struct small_file_vm_storage_info));
    if (ret != sizeof(struct small_file_vm_storage_info)) {
        return false;
    }
    return true;
}

static int __read_vm_saved_data(u16 vm_id, u8 *buf, u32 len)
{
    int saved_len = 0;
    u32 read_len = sizeof(struct small_file_vm_storage_info) + len;
    u8 *read_buf = zalloc(read_len);
    if (!read_buf) {
        log_error("%s read_buf is null", __func__);
        return 0;
    }
    saved_len = syscfg_read(vm_id, read_buf, read_len);
    if (saved_len <= sizeof(struct small_file_vm_storage_info)) {
        free(read_buf);
        return 0;
    }
    saved_len -= sizeof(struct small_file_vm_storage_info);
    memcpy(buf, &read_buf[sizeof(struct small_file_vm_storage_info)], saved_len);
    free(read_buf);
    return saved_len;
}

static int __write_vm_saved_data(u16 vm_id, u32 stored_id, u8 *buf, u32 len)
{
    int ret;
    struct small_file_vm_storage_info *storage_info;
    u32 saved_len = sizeof(struct small_file_vm_storage_info) + len;
    u8 *saved_buf = zalloc(saved_len);
    if (!saved_buf) {
        log_error("%s saved_buf is null", __func__);
        return 0;
    }
    storage_info = (struct small_file_vm_storage_info *)saved_buf;
    storage_info->id = stored_id;
    storage_info->len = len;
    memcpy(&saved_buf[sizeof(struct small_file_vm_storage_info)], buf, len);

    ret = syscfg_write(vm_id, saved_buf, saved_len);
    if (ret != saved_len) {
        free(saved_buf);
        return 0;
    }
    free(saved_buf);
    return len;
}

static int __del_vm_saved_item(u16 vm_id)
{
    int ret;
    u8 data = 0;
    ret = syscfg_write(vm_id, &data, sizeof(data));
    if (ret != sizeof(data)) {
        return false;
    }
    return true;
}

static int __data_small_file_info_init(void)
{
    int ret;
    if (!hd) {
        return false;
    }

    struct small_file_vm *temp_data = (struct small_file_vm *)zalloc(sizeof(data_small_file_info));
    if (!temp_data) {
        log_error("%s temp_data is null", __func__);
        return false;
    }

    ret = syscfg_read(VM_SMALL_FILE_MANAGER, temp_data, sizeof(data_small_file_info));
    if (ret == sizeof(data_small_file_info)) {
        for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
            struct small_file_vm *file = (struct small_file_vm *)&temp_data[i];

            if (!file->type) {
                continue;
            }

            hd->child[file->type].type = file->type;
            hd->child[file->type].max_item = file->max_item;
            hd->child[file->type].vm_max_item = file->vm_max_item;
            hd->child[file->type].table = (u32 *)zalloc(sizeof(u32) * file->vm_max_item);
            hd->child[file->type].size_table = (u16 *)zalloc(sizeof(u16) * file->vm_max_item);
            hd->child[file->type].vm_id_table = (u16 *)zalloc(sizeof(u16) * file->vm_max_item);
            ASSERT(hd->child[file->type].table && hd->child[file->type].size_table && hd->child[file->type].vm_id_table);
            memcpy((char *)hd->child[file->type].vm_id_table, (const char *)file->vm_id_table, sizeof(u16) * file->vm_max_item);
        }
    } else {
        for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
            struct small_file_vm *file = (struct small_file_vm *)&data_small_file_info[i];

            if (!file->type) {
                continue;
            }

            hd->child[file->type].type = file->type;
            hd->child[file->type].max_item = file->max_item;
            hd->child[file->type].vm_max_item = file->vm_max_item;
            hd->child[file->type].table = (u32 *)zalloc(sizeof(u32) * file->vm_max_item);
            hd->child[file->type].size_table = (u16 *)zalloc(sizeof(u16) * file->vm_max_item);
            hd->child[file->type].vm_id_table = (u16 *)zalloc(sizeof(u16) * file->vm_max_item);
            ASSERT(hd->child[file->type].table && hd->child[file->type].size_table && hd->child[file->type].vm_id_table);
            memcpy((char *)hd->child[file->type].vm_id_table, (const char *)file->vm_id_table, sizeof(u16) * file->vm_max_item);
        }
    }

    free(temp_data);
    return true;
}

static int __data_small_file_info_update(void)
{
    int ret;
    if (!hd) {
        return false;
    }

    struct small_file_vm *temp_file;
    struct small_file_vm *temp_data = (struct small_file_vm *)zalloc(sizeof(data_small_file_info));
    if (!temp_data) {
        log_error("%s temp_data is null", __func__);
        return false;
    }
    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file_vm *file = (struct small_file_vm *)&data_small_file_info[i];
        temp_file = &temp_data[i];

        if (!file->type) {
            continue;
        }

        temp_file->type = hd->child[file->type].type;
        temp_file->max_item = hd->child[file->type].max_item;
        temp_file->vm_max_item = hd->child[file->type].vm_max_item;
        memcpy((char *)temp_file->vm_id_table, (const char *)hd->child[file->type].vm_id_table, sizeof(u16) * hd->child[file->type].vm_max_item);
    }

    ret = syscfg_write(VM_SMALL_FILE_MANAGER, temp_data, sizeof(data_small_file_info));
    free(temp_data);
    if (ret != sizeof(data_small_file_info)) {
        return false;
    }
    return true;
}

#endif /* if TCFG_DATA_STORAGE_ENABLE */

