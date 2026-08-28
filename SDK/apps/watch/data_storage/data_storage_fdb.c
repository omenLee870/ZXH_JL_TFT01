#include "app_config.h"
#include "asm/cpu.h"
#include "device/device.h"
#include "flashdb.h"
#include "data_storage.h"
#include "os/os_api.h"


#define LOG_TAG_CONST       DATA_STORAGE
#define LOG_TAG     		"[FDB-DATA]"
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

#if TCFG_DATA_STORAGE_ENABLE && (TCFG_DATA_STORAGE_FDB_ENABLE||TCFG_DATA_STORAGE_FDB_EXFLASH_ENABLE)


static struct small_file_hd *hd;


struct small_file_info {
    u8  type;           /*文件类型*/
    u8  max_item;       /*文件数量最大值*/
    u8  cur_item;       /*当前文件数量*/
    u8  storage_sel;    /*对应数据库*/
    u16 *size_table;    /*每个文件对应的大小*/
    u32 *table;         /*每个文件对应的id*/
};



#define SMALL_FILE_DB_NAME          "small_file_kvdb"
#define SMALL_FILE_PARTITION_NAME   "small_file_partition"


#define USED_KVDB_MAX   1
#define USED_TSDB_MAX   1

#define USED_KVDB_0     0
#define USED_KVDB_1     1
#define USED_KVDB_2     2
#define USED_KVDB_3     3

#define USED_TSDB_0     4
#define USED_TSDB_1     5
#define USED_TSDB_2     6
#define USED_TSDB_3     7

#define NAME_FILE_LEN 13

struct small_file_hd {
    struct small_file_info *child;
    struct fdb_kvdb *kvdb[USED_KVDB_MAX];
    struct fdb_tsdb *tsdb[USED_TSDB_MAX];
    OS_MUTEX db_mutex;
    OS_MUTEX mutex;
};




const struct small_file data_small_file_info[] = {
    {F_TYPE_PHONEBOOK,     1,     USED_KVDB_0},
    {F_TYPE_MESSAGE,       10,    USED_KVDB_0},
    {F_TYPE_SPORTRECORD,   1,     USED_KVDB_0},
    {F_TYPE_HEART,         1,     USED_KVDB_0},
    {F_TYPE_BLOOD_OXYGEN,  1,     USED_KVDB_0},
    {F_TYPE_SLEEP,         1,     USED_KVDB_0},
    {F_TYPE_WEATHER,       1,     USED_KVDB_0},
    {F_TYPE_CALL_LOG,      10,    USED_KVDB_0},
    {F_TYPE_STEP,		   1,     USED_KVDB_0},
    {F_TYPE_CHAT_REC,      10,    USED_KVDB_0},
};




extern int small_file_table_init(u8 small_file_type);

#define VM_MASK       0x55
#define VM_MASK_BIT   40//24
#define TYPE_BIT      32//16


static u32  __is_name2mask(char *name)
{
    char *end;
    char temp[9];
    snprintf(temp, NAME_FILE_LEN - sizeof(temp) + 1, "%s", name);
    long intValue = strtol(temp, &end, 16);
    if (*end == '\0') { // 确保整个字符串都被成功转换
    } else {
        /* return false; */
    }
    u8  mask = (intValue >> (VM_MASK_BIT - 32)) & 0xff;
    if (mask == VM_MASK) {
        return true;
    }
    return false;
}


static u32  __type_name2id(char *name)
{
    char *end;
    char temp[9];
    snprintf(temp, NAME_FILE_LEN - sizeof(temp) + 1, "%s", name);
    long intValue = strtol(temp, &end, 16);
    if (*end == '\0') { // 确保整个字符串都被成功转换
        /* printf("The integer value of %s is %lx\n", name, intValue); */
    } else {
        /* printf("Invalid hexadecimal string\n"); */
        /* return 0; */
    }
    u8  mask = (intValue >> (VM_MASK_BIT - 32)) & 0xff;
    if (mask == VM_MASK) {
        snprintf(temp, sizeof(temp), "%s", name + (NAME_FILE_LEN - sizeof(temp)));
        intValue = strtol(temp, &end, 16);
        return (intValue) & 0xffffffff;
    }
    return 0;
}


static u32 __type_name2type(char *name)
{
    char *end;
    char temp[9];
    snprintf(temp, NAME_FILE_LEN - sizeof(temp) + 1, "%s", name);
    long intValue = strtol(temp, &end, 16);
    if (*end == '\0') { // 确保整个字符串都被成功转换
        /* printf("The integer value of %s is %lx\n", name, intValue); */
    } else {
        /* printf("Invalid hexadecimal string\n"); */
        /* return 0; */
    }
    u8  mask = (intValue >> (VM_MASK_BIT - 32)) & 0xff;
    if (mask == VM_MASK) {
        return (intValue >> (TYPE_BIT - 32)) & 0xff;
    }
    return 0;
}

static char  *__type_id2name(char *name, u8 file_type, u32 id)
{
    sprintf(name, "%02x%02x%08x", (int)VM_MASK, file_type, (u32)id);
    return name;
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


static u32 __find_id_index(struct small_file_info *info, u32 id)
{
    for (int index = 0; index < info->cur_item; index++) {
        if (info->table[index] == id) {
            return index;
        }
    }
    return info->cur_item;
}






static fdb_time_t get_time(void)
{
    static  int counts = 0;
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}




void flashdb_lock(fdb_db_t db)
{
    if (hd) {
        os_mutex_pend(&hd->db_mutex, 0);
    }
}




void flashdb_unlock(fdb_db_t db)
{
    if (hd) {
        os_mutex_post(&hd->db_mutex);
    }
}





int data_small_file_init(void)
{

    fdb_err_t result;

    if (hd) {
        /* 已初始化过，直接返回。重复调用会重复创建互斥量并重新 zalloc 各表，
           而 child[].cur_item 未清零，导致 small_file_table_init 越界写坏堆内存 */
        return true;
    }

    hd = zalloc(sizeof(struct small_file_hd) \
                + F_TYPE_COUNT * sizeof(struct small_file_info));

    if (!hd) {
        goto __err;
    }

    hd->child    = (struct small_file_info *)(hd + 1);


    os_mutex_create(&hd->mutex);
    os_mutex_create(&hd->db_mutex);


    int flag = 0;
    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file *file = (struct small_file *)&data_small_file_info[i];

        if (!file->type) {
            continue;
        }

        flag |= BIT(file->storage_sel);
        hd->child[file->type].type = file->type;
        hd->child[file->type].max_item = file->max_item;
        hd->child[file->type].storage_sel = file->storage_sel;
        hd->child[file->type].table = (u32 *)zalloc(sizeof(u32) * file->max_item);
        hd->child[file->type].size_table = (u16 *)zalloc(sizeof(u16) * file->max_item);

    }


    //先支持单个
    if (flag & 0x0f) {
        hd->kvdb[0] = (struct fdb_kvdb *)zalloc(sizeof(struct fdb_kvdb) * USED_KVDB_MAX);
        result = fdb_kvdb_init(hd->kvdb[0], SMALL_FILE_DB_NAME, SMALL_FILE_PARTITION_NAME, NULL, NULL);

        fdb_kvdb_control(hd->kvdb[0], FDB_KVDB_CTRL_SET_LOCK, flashdb_lock);
        fdb_kvdb_control(hd->kvdb[0], FDB_KVDB_CTRL_SET_UNLOCK, flashdb_unlock);


        if (result != FDB_NO_ERR) {
            log_error("kvdb init fail\n");
            goto __err;
        }

    }



    if (flag & 0xf0) {
        hd->tsdb[0] = (struct fdb_tsdb *)zalloc(sizeof(struct fdb_tsdb) * USED_TSDB_MAX);
        result = fdb_tsdb_init(hd->tsdb[0], SMALL_FILE_DB_NAME, SMALL_FILE_PARTITION_NAME, get_time, 128, NULL);
        fdb_tsdb_control(hd->tsdb[0], FDB_TSDB_CTRL_SET_LOCK, flashdb_lock);
        fdb_tsdb_control(hd->tsdb[0], FDB_TSDB_CTRL_SET_UNLOCK, flashdb_unlock);

        if (result != FDB_NO_ERR) {
            log_error("tsdb init fail\n");
            goto __err;
        }
    }


    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file *file = (struct small_file *)&data_small_file_info[i];
        small_file_table_init(file->type);

    }


    return true;

__err:

    return false;
}




u32  small_file_get_id_by_index(u8 small_file_type, int index)
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

int small_file_info_delete_by_index(struct small_file_info *file, int index)
{
    fdb_err_t result;

    struct fdb_kvdb *kvdb;
    if (!hd) {
        return 0;
    }

    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];

    if (!file->cur_item || index >= file->cur_item) {
        return 0;
    }

    u32 id = file->table[index];

    for (; index < file->cur_item - 1; index++) {
        file->table[index] = file->table[index + 1];
        file->size_table[index] = file->size_table[index + 1];
    }

    --file->cur_item;
    file->table[file->cur_item] = 0;
    file->size_table[file->cur_item] = 0;
    char name[NAME_FILE_LEN];

    __type_id2name(name, file->type, id);

    printf(">>>>>>>>>>>>>>>>> %s %s %d\n", __FUNCTION__, name, id);

    result = fdb_kv_del(kvdb, name);

    if (result != FDB_NO_ERR) {
        goto __err;
    }

    log_info("<%s> id:%d succ!", __func__, id);
    return true;

__err:
    return false;
}

int small_file_del_all(void)
{
    int flag = 0;
    fdb_err_t result;

    if (!hd) {
        return false;
    }

    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file *file = (struct small_file *)&data_small_file_info[i];

        if (!file->type) {
            continue;
        }

        flag |= BIT(file->storage_sel);
    }

    os_mutex_pend(&hd->mutex, 0);

    //先支持单个
    if (flag & 0x0f) {
        result = fdb_kv_set_default(hd->kvdb[0]);
        if (result != FDB_NO_ERR) {
            log_error("kvdb deinit fail\n");
            goto __err;
        }
    }

    for (int i = 0; i < ARRAY_SIZE(data_small_file_info); i++) {
        struct small_file *file = (struct small_file *)&data_small_file_info[i];

        if (!file->type) {
            continue;
        }

        hd->child[file->type].cur_item = 0;
        memset(hd->child[file->type].table, 0, (sizeof(u32) * file->max_item));
        memset(hd->child[file->type].size_table, 0, (sizeof(u16) * file->max_item));
    }

    log_info("<%s> succ!", __func__);
    os_mutex_post(&hd->mutex);
    return true;

__err:
    log_error("<%s> del small_file error", __func__);
    os_mutex_post(&hd->mutex);
    return false;
}


int small_file_delete_by_index(u8 small_file_type, int index)
{
    struct small_file_info *file;
    if (!hd) {
        return 0;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);
    file = &hd->child[small_file_type];

    os_mutex_pend(&hd->mutex, 0);
    int ret = small_file_info_delete_by_index(file, index);
    os_mutex_post(&hd->mutex);
    return ret;

}


u32 small_file_write(u8 small_file_type, u32 *id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    fdb_err_t result;
    struct fdb_blob blob;
    struct small_file_info *file;
    struct fdb_kv kv;
    struct fdb_kvdb *kvdb;
    u8 is_new_id = 1;
    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);

    file = &hd->child[small_file_type];
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];

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

    char name[NAME_FILE_LEN];
    __type_id2name(name, file->type, *id);

__agin:

    if (buf_offset) {
        if (fdb_kv_get_obj(kvdb, name, &kv)) {
            /* if (kv.value_len >=  buf_offset) { */
            u8 *temp_data = zalloc(buf_offset + len);
            fdb_kv_get_blob(kvdb, name, fdb_blob_make(&blob, temp_data, kv.value_len));
            memcpy(&temp_data[buf_offset], buf, len);
            result = fdb_kv_set_blob(kvdb, name, fdb_blob_make(&blob, temp_data, buf_offset + len));
            free(temp_data);
            /* } else { */
            /* goto __err; */
            /* } */
        } else {
            goto __err;
        }
    } else {
        result = fdb_kv_set_blob(kvdb, name, fdb_blob_make(&blob, buf, len));
    }

    if (result != FDB_NO_ERR) {
        small_file_info_delete_by_index(file, 0);
        goto __agin;
    }

    if (is_new_id) {
        file->table[file->cur_item] = *id;
        file->size_table[file->cur_item] = len;
        file->cur_item++;
    } else {
        u32 cur_item = __find_id_index(file, *id);
        file->table[cur_item] = *id;
        file->size_table[cur_item] = buf_offset + len;
        if (cur_item == file->cur_item) {
            file->cur_item++;
            log_info("<%s> id:%d del rwrite", __func__, *id);
        }
    }



    log_info("<%s> id:%d succ!", __func__, *id);
    os_mutex_post(&hd->mutex);
    return len;
__err:
    os_mutex_post(&hd->mutex);
    return 0;
}

int small_file_update_by_id(u8 small_file_type, u32 id, u32 buf_offset, void *buf, u32 len, u32 total_len)
{
    fdb_err_t result;
    struct fdb_blob blob;
    struct small_file_info *file;
    struct fdb_kv kv;
    struct fdb_kvdb *kvdb;
    u8 is_new_id = 1;
    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);

    file = &hd->child[small_file_type];
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];

    if (!id) {
        goto __err;
    }

    char name[NAME_FILE_LEN];
    __type_id2name(name, file->type, id);

__agin:

    if (fdb_kv_get_obj(kvdb, name, &kv)) {
        if (kv.value_len < (buf_offset + len)) {
            goto __err;
        }
        u8 *temp_data = zalloc(kv.value_len);
        fdb_kv_get_blob(kvdb, name, fdb_blob_make(&blob, temp_data, kv.value_len));
        memcpy(&temp_data[buf_offset], buf, len);
        result = fdb_kv_set_blob(kvdb, name, fdb_blob_make(&blob, temp_data, kv.value_len));
        free(temp_data);
    } else {
        goto __err;
    }

    if (result != FDB_NO_ERR) {
        goto __agin;
    }

    log_info("<%s> id:%d succ!", __func__, id);
    os_mutex_post(&hd->mutex);
    return len;
__err:
    os_mutex_post(&hd->mutex);
    return 0;
}


int small_file_read(u8 small_file_type, u32 id, u32 offset, void *buf, u32 len)
{
    int ret = len;
    struct fdb_blob blob;
    struct fdb_kvdb *kvdb;


    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    os_mutex_pend(&hd->mutex, 0);

    struct small_file_info *file = &hd->child[small_file_type];
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];

    if (!file->cur_item) {
        goto __err;
    }


    u8 *temp_data = zalloc(offset + len);

    char name[NAME_FILE_LEN];
    __type_id2name(name, file->type, id);
    /* printf("%s %d %s \n", __FUNCTION__, __LINE__, name); */
    fdb_kv_get_blob(kvdb, name, fdb_blob_make(&blob, temp_data, offset + len));
    if (blob.saved.len > offset) {
        if (blob.saved.len < offset + len) {
            ret = blob.saved.len - offset;
        }
        memcpy(buf, &temp_data[offset], ret);
    } else {
        ret = 0;
    }
    free(temp_data);
    os_mutex_post(&hd->mutex);
    return ret;
__err:
    os_mutex_post(&hd->mutex);
    return 0;
}




int small_file_delete_by_id(u8 small_file_type, u32 id)
{
    fdb_err_t result;
    struct small_file_info *file;
    struct fdb_kvdb *kvdb;
    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);

    file = &hd->child[small_file_type];
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];

    os_mutex_pend(&hd->mutex, 0);
    if (!file->cur_item) {
        goto __err;
    }


    u32 index = __find_id_index(file, id);
    if (index == file->cur_item) {
        goto __err;
    }

    for (; index < file->cur_item - 1; index++) {
        file->table[index] = file->table[index + 1];
        file->size_table[index] = file->size_table[index + 1];
    }

    --file->cur_item;
    file->table[file->cur_item] = 0;
    file->size_table[file->cur_item] = 0;

    char name[NAME_FILE_LEN];


    __type_id2name(name, file->type, id);

    printf(">>>>>>>>>>>>>>>>> %s %s\n", __FUNCTION__, name);

    result = fdb_kv_del(kvdb, name);

    if (result != FDB_NO_ERR) {
        goto __err;
    }

    log_info("<%s> id:%d succ!", __func__, id);
    os_mutex_post(&hd->mutex);
    return true;

__err:
    os_mutex_post(&hd->mutex);
    return false;

}


int small_file_get_id_table_len(u8 small_file_type)
{
    return small_file_get_id_table(small_file_type, NULL, 0);
}


int small_file_table_init(u8 small_file_type)
{
    int index = 0;
    struct small_file_info *file;
    struct fdb_kvdb *kvdb;
    struct fdb_kv_iterator iterator;
    fdb_kv_t cur_kv;

    if (!hd) {
        return 0;
    }

    ASSERT(small_file_type < F_TYPE_COUNT);
    os_mutex_pend(&hd->mutex, 0);
    file = &hd->child[small_file_type];
    file->cur_item = 0;
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];
    flashdb_lock(NULL);
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(kvdb, &iterator)) {
        cur_kv = &(iterator.curr_kv);

        if (!__is_name2mask(cur_kv->name)) {
            printf("del name %s \n", cur_kv->name);
            fdb_kv_del(kvdb, cur_kv->name);
        }

        if (__type_name2type(cur_kv->name) == file->type) {
            u32 id  = __type_name2id(cur_kv->name);
            printf("name %s %d \n", cur_kv->name, id);
            if (index < file->max_item) {
                file->size_table[file->cur_item] = cur_kv->value_len;
                file->table[file->cur_item] = id;
                file->cur_item++;
                index++;
            } else {
                printf("file over del name %s \n", cur_kv->name);
                fdb_kv_del(kvdb, cur_kv->name);
            }
        }
    }

    flashdb_unlock(NULL);
    os_mutex_post(&hd->mutex);
    return index ;

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
            id = (u16)file->table [i];
            file_len  = (u16)file->size_table [i];
            memcpy(table_data, &id, sizeof(u16));
            table_data += 2;
            memcpy(table_data, &file_len, sizeof(u16));
            table_data += 2;
        }
    }

    os_mutex_post(&hd->mutex);
    return count * 4;

}


u32  small_file_get_size_by_index(u8 small_file_type, int index)
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
u32  small_file_get_size_by_id(u8 small_file_type, int id)
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

int small_file_del_by_file_type(u8 small_file_type)
{
    fdb_err_t result;
    struct small_file_info *file;
    u32 id;
    char name[NAME_FILE_LEN];
    struct fdb_kvdb *kvdb;

    if (!hd) {
        return false;
    }
    ASSERT(small_file_type < F_TYPE_COUNT);

    file = &hd->child[small_file_type];
    kvdb = hd->kvdb[file->storage_sel % USED_KVDB_MAX];


    os_mutex_pend(&hd->mutex, 0);
    if (!file->cur_item) {
        log_error("<%s> file->cur_item is 0", __func__);
        goto __err;
    }

    while (file->cur_item) {
        id = file->table[file->cur_item - 1];
        __type_id2name(name, file->type, id);

        result = fdb_kv_del(kvdb, name);

        if (result != FDB_NO_ERR) {
            goto __err;
        }

        file->table[file->cur_item - 1] = 0;
        file->size_table[file->cur_item - 1] = 0;
        --file->cur_item;
    }
    log_info("<%s> succ!", __func__);
    os_mutex_post(&hd->mutex);
    return true;

__err:
    log_error("<%s> del small_file error", __func__);
    os_mutex_post(&hd->mutex);
    return false;
}

/************************************************
 *              注册flashDB的分区
 ***********************************************/

REGISTER_FLASHDB_PARTITION(phonebook_partition) = {
    .name = SMALL_FILE_PARTITION_NAME,
    .dev_name = TCFG_DATA_DEV_NAME,
    .offset = 0,
    .len = TCFG_DATA_FLASH_DEV_PATY_SIZE,
};

#endif /* if TCFG_WATCH_DATA_STORAGE_ENABLE */

