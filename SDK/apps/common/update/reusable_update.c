#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".reusable_update.data.bss")
#pragma data_seg(".reusable_update.data")
#pragma const_seg(".reusable_update.text.const")
#pragma code_seg(".reusable_update.text")
#endif

#include "update.h"
#include "update_loader_download.h"
#include "app_config.h"
#include "cpu.h"
#include "system/includes.h"
#include "app_main.h"
#include "fs/sdfile.h"
#include "norflash.h"
#include "rec_nor/nor_interface.h"
#include "user_cfg_id.h"

#if TCFG_UPDATE_ENABLE

#define LOG_TAG "[REUSABLE-UPDATE]"
#define LOG_INFO_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

/*
                         REUSABLE RESERVED AREA
    #-----------------------------------------------------------------+  ->4KB aligned
    #                                                                 |
    #                          EXIF (4KB)                             |
    #                                                                 |
    #-----------------------------------------------------------------|  ->4KB aligned
    #                                                                 |
    #                          loader.bin                             |
    #                            vm bak                               |
    #                         update_param                            |
    #                                                                 |
    #-----------------------------------------------------------------+  ->end
*/

#define SECTOR_SIZE 4096
#define ALIGN_SECTOR(x) (((x + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE)

#define LOADER_RESERVE_SIZE				(100) // 单位是K

#define REUSABLE_RESERVED                   "REUSABLE"
#define SDFILE_EXT_RESERVED_ROOT_PATH       "mnt/sdfile/EXT_RESERVED/"

struct reuable_info_record {
    union {
        struct {
            u32 reserve;
        };
        struct {
            u8 app_update_flag;
        };
    };
    u32 update_again_flag;
};

struct reusable_reserv {
    u8 init_flag;

    u32 base_addr;
    u32 total_size;

    u32 exif_addr;
    u32 exif_size;

    u32 loader_addr;
    u32 loader_size;
};
struct reusable_reserv reusable_region = {0};

#define __this (&reusable_region)

static u8 new_sdk_update_again_flag = 0;

static void reusable_param_dump(void)
{
    printf("init_flag = %d\n", __this->init_flag);
    printf("base_addr = 0x%x\n", __this->base_addr);
    printf("total_size = 0x%x\n", __this->total_size);
    printf("exif_addr = 0x%x\n", __this->exif_addr);
    printf("exif_size = 0x%x\n", __this->exif_size);
    printf("loader_addr = 0x%x\n", __this->loader_addr);
    printf("loader_size = 0x%x\n", __this->loader_size);
}



extern u32 sfc_read(u8 *buf, u32 addr, u32 len);
extern u32 sfc_write(const u8 *buf, u32 addr, u32 len);
extern bool sfc_erase(int cmd, u32 addr);
extern bool reserved_area_special_handle(u8 opt);
extern const int support_reusable_special_update;
extern u32 config_update_features;
extern char *watch_get_root_path();

static u32 reusable_region_write(const u8 *buf, u32 addr, u32 len)
{
    if (__this->init_flag == 0) {
        return 0;
    }
    return sfc_write(buf, addr, len);
}

static u32 reusable_region_read(u8 *buf, u32 addr, u32 len)
{
    if (__this->init_flag == 0) {
        return 0;
    }
    return sfc_read(buf, addr, len);
}

static bool reusable_region_erase(int cmd, u32 addr)
{
    if (__this->init_flag == 0) {
        return false;
    }
    return sfc_erase(cmd, addr);
}


int reusable_region_init(void)
{
    int ret;
    FILE *fp = NULL;
    struct vfs_attr file_attr;
    const char *fpth = SDFILE_EXT_RESERVED_ROOT_PATH;

    u8 file_path_len = sizeof(SDFILE_EXT_RESERVED_ROOT_PATH) + strlen((const char *)REUSABLE_RESERVED);
    u8 *file_path = zalloc(file_path_len);
    if (NULL == file_path) {
        log_error("%s, alloc fail\n", __func__);
        ret = -1;
        goto err1;
    }

    // 如果SDFILE_APP_ROOT_PATH + REUSABLE_RESERVED不行，则还需要尝试打开扩展预留区域(mnt/sdfile/EXT_RESERVED)
    memcpy(file_path, SDFILE_APP_ROOT_PATH, strlen(SDFILE_APP_ROOT_PATH));
    memcpy(file_path + strlen(SDFILE_APP_ROOT_PATH), REUSABLE_RESERVED, strlen((const char *)REUSABLE_RESERVED));
    fp = fopen((const char *)file_path, "r");

    if (!fp) {
        memset((void *)file_path, 0, file_path_len);
        memcpy((void *)file_path, SDFILE_EXT_RESERVED_ROOT_PATH, strlen(SDFILE_EXT_RESERVED_ROOT_PATH));
        memcpy((void *)(file_path + strlen(SDFILE_EXT_RESERVED_ROOT_PATH)), REUSABLE_RESERVED, strlen((const char *)REUSABLE_RESERVED));
        fp = fopen((const char *)file_path, "r");
    }

    if (!fp) {
        log_error("open %s fail\n", fpth);
        ret = -2;
        goto err2;
    }

    fget_attrs(fp, &file_attr);
    __this->base_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    __this->total_size = file_attr.fsize;

    __this->exif_addr = ALIGN_SECTOR(__this->base_addr);
    __this->exif_size = SECTOR_SIZE;
    __this->loader_addr = __this->exif_addr + __this->exif_size;
    __this->loader_size = __this->base_addr + __this->total_size - __this->loader_addr;
    __this->loader_size = __this->loader_size / SECTOR_SIZE * SECTOR_SIZE;

    if (__this->loader_size < SECTOR_SIZE) {
        log_error("loader region too small\n");
        ret = -3;
        goto err2;
    }

    __this->init_flag = 1;

    reusable_param_dump();
    free(file_path);

    return 0;

err2:
    free(file_path);
err1:
    __this->init_flag = 0;
    return ret;
}

static bool reusable_update_flag_handle(u32 *value, u8 type, u8 dir)
{
    struct reuable_info_record reuable_info = {0};
    // 先读
    bool ret = (sizeof(reuable_info) == syscfg_read(VM_REUSABLE_SPECIAL_UPDATE_INFO, &reuable_info, sizeof(reuable_info)));
    if (dir) {
        // 写前判断当前值是否一致
        switch (type) {
        case 0:	// 对app下发的标志位进行操作
            u8 app_update_flag = ((u8 *)value)[0];
            if (ret && reuable_info.app_update_flag == app_update_flag) {
                break;
            }
            reuable_info.app_update_flag = app_update_flag;
            syscfg_write(VM_REUSABLE_SPECIAL_UPDATE_INFO, &reuable_info, sizeof(reuable_info));
            break;
        case 1:	// 对升级标志位进行操作
            u32 update_again_flag = *value;
            if (ret && reuable_info.update_again_flag == update_again_flag) {
                break;
            }
            reuable_info.update_again_flag = update_again_flag;
            syscfg_write(VM_REUSABLE_SPECIAL_UPDATE_INFO, &reuable_info, sizeof(reuable_info));
            break;
        }
    } else {
        switch (type) {
        case 0:
            if (ret) {
                ((u8 *)value)[0] = reuable_info.app_update_flag;
            }
            break;
        case 1:
            if (ret) {
                *value = reuable_info.update_again_flag;
            }
            break;
        }
    }
    return ret;
}

static bool reusable_update_is_push_loader_only(void)
{
    bool ret = reusable_update_flag_handle(&config_update_features, 1, 0);
    if (ret) {
        ret = (~BIT(CONFIG_UPDATE_FEATRUES_CONTENT_COMPARE_EN) & config_update_features) == BIT(CONFIG_UPDATE_FEATRUES_PUSH_LOADER_ONLY_EN);
    }
    return ret;
}

static int reusable_update_specail_handle(int update_type)
{
    if (0 == reserved_area_special_handle(0)) {
        // 如果蓝牙升级，则必须带资源
        if (BT_UPDATA == update_type || BLE_TEST_UPDATA == update_type) {
            return -1;
        }
        return 0;
    }

    if (reusable_update_is_push_loader_only()) {
        config_update_features |= BIT(CONFIG_UPDATE_FEATRUES_UPDATE_RES_ONLY_EN);
        config_update_features &= ~BIT(CONFIG_UPDATE_FEATRUES_PUSH_LOADER_ONLY_EN);
    } else if (0 == (~BIT(CONFIG_UPDATE_FEATRUES_CONTENT_COMPARE_EN) & config_update_features)) {
        config_update_features |= BIT(CONFIG_UPDATE_FEATRUES_PUSH_LOADER_ONLY_EN);
        config_update_features &= ~BIT(CONFIG_UPDATE_FEATRUES_UPDATE_RES_ONLY_EN);
        reserved_area_special_handle(1);
        new_sdk_update_again_flag = 3;
    }

    reusable_update_flag_handle(&config_update_features, 1, 1);
    return 0;
}

bool reusable_update_result_deal(bool result)
{
    if (result) {
        if (reusable_update_flag_handle(&config_update_features, 1, 0) &&
            config_update_features & BIT(CONFIG_UPDATE_FEATRUES_UPDATE_RES_ONLY_EN)) {
            u8 state = 0;
            reusable_update_flag_handle((u32 *)&state, 0, 1);
        }
        config_update_features = 0;
        reusable_update_flag_handle(&config_update_features, 1, 1);
    }
    return result;
}

int reusable_loader_region(loader_area_t *loader_area)
{
    int ret;
    int update_type = 0;

    if (loader_area == NULL) {
        return -1;
    }

    update_type = loader_area->type;

    if (__this->init_flag == 0) {
        ret = reusable_region_init();
        if (ret) {
            ret = -2;
            goto end;
        }
    }


    loader_area->type = 0;
    loader_area->start_addr = __this->loader_addr;
    loader_area->usable_size = __this->loader_size;
    loader_area->write_hdl = (void *)reusable_region_write;
    loader_area->read_hdl = (void *)reusable_region_read;
    loader_area->erase_hdl = (void *)reusable_region_erase;

    if (support_reusable_special_update) {
        loader_area->result = reusable_update_specail_handle(update_type);
    }

    return 0;

end:
    return ret;
}

int reusable_exif_region(u32 *base, u32 *len)
{
    int ret = 0;

    if (__this->init_flag == 0) {
        ret = reusable_region_init();
        if (ret) {
            ret = -1;
            goto end;
        }
    }

    if (base != NULL) {
        *base = __this->exif_addr;
    }
    if (len != NULL) {
        *len = __this->exif_size;
    }

end:
    return ret;
}

u8 new_sdk_update_again_flag_get(void)
{
    return new_sdk_update_again_flag;
}

void reusable_update_flag_clear(void)
{
    if (__this->loader_size && reusable_update_is_push_loader_only()) {
        reusable_update_result_deal(true);
    }
    memset(__this, 0, sizeof(struct reusable_reserv));
    new_sdk_update_again_flag = 0;
}

u8 reusable_update_app_flag_handle(u8 state, u8 opt)
{
    u8 ret = 0;
    if (opt) {
        // 写入
        if (2 == state || 0 == state) {
            if (state) {
                new_sdk_update_again_flag = 2;
            }
            reusable_update_flag_handle((u32 *)&state, 0, 1);
        }
    } else {
        // 从vm中获取对标志位，结合升级标志位确定返回值
        u32 update_flag = 0;
        if (reusable_update_flag_handle(&update_flag, 1, 0) &&
            ((update_flag & BIT(CONFIG_UPDATE_FEATRUES_PUSH_LOADER_ONLY_EN)) || (update_flag & BIT(CONFIG_UPDATE_FEATRUES_UPDATE_RES_ONLY_EN)))) {
            ret = 3;
        } else if (reusable_update_flag_handle((u32 *)&state, 0, 0)) {
            ret = state;
        }
    }
    return ret;
}

u32 reusable_area_flash_space(u32 *start_addr, u32 *free_space)
{
    u32 size = 0;
    FILE *fp = NULL;
    struct vfs_attr file_attr;
    const char *fpth = SDFILE_EXT_RESERVED_ROOT_PATH;

    u8 file_path_len = 0;
    u8 *file_path = NULL;

    if (!support_reusable_special_update) {
        goto _ERR_RET;
    }
    file_path_len = sizeof(SDFILE_EXT_RESERVED_ROOT_PATH) + strlen((const char *)REUSABLE_RESERVED);
    file_path = zalloc(file_path_len);
    if (NULL == file_path) {
        log_error("%s, alloc fail\n", __func__);
        goto _ERR_RET;
    }

    // 如果SDFILE_APP_ROOT_PATH + REUSABLE_RESERVED不行，则还需要尝试打开扩展预留区域(mnt/sdfile/EXT_RESERVED)
    memcpy(file_path, SDFILE_APP_ROOT_PATH, strlen(SDFILE_APP_ROOT_PATH));
    memcpy(file_path + strlen(SDFILE_APP_ROOT_PATH), REUSABLE_RESERVED, strlen((const char *)REUSABLE_RESERVED));

    u32 space = 0;
    fget_free_space(watch_get_root_path(), &space);
    fp = fopen((const char *)file_path, "r");

    if (!fp) {
        memset((void *)file_path, 0, file_path_len);
        memcpy((void *)file_path, SDFILE_EXT_RESERVED_ROOT_PATH, strlen(SDFILE_EXT_RESERVED_ROOT_PATH));
        memcpy((void *)(file_path + strlen(SDFILE_EXT_RESERVED_ROOT_PATH)), REUSABLE_RESERVED, strlen((const char *)REUSABLE_RESERVED));
        fp = fopen((const char *)file_path, "r");
    }

    if (!fp) {
        log_error("%s, open %s fail\n", __func__, fpth);
        goto _ERR_RET;
    }

    fget_attrs(fp, &file_attr);
    if (start_addr) {
        *start_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    }
    if (free_space) {
        *free_space = space > LOADER_RESERVE_SIZE ? (space - LOADER_RESERVE_SIZE) : 0;
    }
    size = file_attr.fsize / 1024;
_ERR_RET:
    if (file_path) {
        free(file_path);
    }
    if (fp) {
        fclose(fp);
    }
    return size;
}

#endif // TCFG_UPDATE_ENABLE

