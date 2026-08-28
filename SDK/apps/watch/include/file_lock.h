/**
 * @file file_lock.h
 * @brief 文件加锁/解锁统一接口 (提取自 video_rec + playback)
 *
 * 使用文件系统只读属性实现保护锁:
 *   - br33_fs_lock_file()   设置 F_ATTR_RO
 *   - br33_fs_unlock_file() 清除 F_ATTR_RO
 *   - br33_fs_is_locked()   检查只读属性
 */
#ifndef BR33_FILE_LOCK_H
#define BR33_FILE_LOCK_H

#include "system/includes.h"
#include "fs/fs.h"

/*==============================================================================
 * 统一接口 (所有模块使用)
 *==============================================================================
 */

/** 加锁文件 (设置只读) */
static inline int br33_fs_lock_file(const char *path)
{
    if (!path || !*path) {
        return -1;
    }
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        return -1;
    }
    int ret = fset_attr(fp, F_ATTR_RO);
    fclose(fp);
    return ret;
}

/** 解锁文件 (清除只读) */
static inline int br33_fs_unlock_file(const char *path)
{
    if (!path || !*path) {
        return -1;
    }
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        return -1;
    }
    int ret = fset_attr(fp, 0);
    fclose(fp);
    return ret;
}

/** 检查文件是否加锁 (返回 1=加锁, 0=未加锁) */
static inline int br33_fs_is_locked(const char *path)
{
    if (!path || !*path) {
        return 0;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    struct vfs_attr attr = {0};
    int ret = fget_attrs(fp, &attr);
    fclose(fp);
    return (ret == 0 && (attr.attr & F_ATTR_RO)) ? 1 : 0;
}

#endif /* BR33_FILE_LOCK_H */
