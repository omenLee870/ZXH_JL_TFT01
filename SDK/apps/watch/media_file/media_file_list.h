#ifndef MEDIA_FILE_LIST_H
#define MEDIA_FILE_LIST_H
#include "generic/typedef.h"

// ====== 目录配置 ======
#define MEDIA_VIDEO_DIR CONFIG_ROOT_PATH "DCIM/VIDEO/"
#define MEDIA_PHOTO_DIR CONFIG_ROOT_PATH "DCIM/PHOTO/"
#define MEDIA_DB_PATH CONFIG_ROOT_PATH "INDEX.DAT"
#define MEDIA_VF_LIST CONFIG_ROOT_PATH "VF_LIST.txt"

// ====== file_list_mgr API（给 video_dec 使用） ======
typedef struct {
    char path[128];
    char name[64];
    u8 type; // 0=PHOTO, 1=VIDEO
    u8 *thumbnail; // 128x128x2, 由调用方管理
} flm_file_t;

int flm_scan_and_load(void);
int flm_sd_online(void);  /* 拔卡检测唯一入口 */
int flm_sd_is_exfat(void);  /* 当前 SDK 不支持 exFAT, 返回 1 表示需要提示用户格式化为 FAT32 */
void flm_rebuild(void);  /* 插卡后强制重建索引, 不删文件 */
void flm_rebuild_async(void);  /* 插卡等场景: 重建放独立线程, app_core 只投递 (同步扫会栈溢出) */
void flm_reset(void);  /* 拔卡清索引状态, 不删文件 */
int flm_get_count(void);
int flm_get_entry(int index, flm_file_t *out);
int flm_delete_file(const char *path);
int flm_lock_file(const char *path);
int flm_unlock_file(const char *path);
int flm_is_locked(const char *path);
int flm_get_storage_info(u32 *total_kb, u32 *free_kb, u32 *photo_cnt, u32 *video_cnt);
int flm_free_all(void);  /* 返回真删文件数, 写保护文件跳过 */
int flm_dump_json(void);

/**
 * @brief Extract filename from open file, read attrs, add to database.
 *        Does NOT call fclose(fp) — the caller closes the file after this returns.
 * @param fp       Open file handle (void *, cast to FILE* internally)
 * @param dir_path Directory path (MEDIA_VIDEO_DIR or MEDIA_PHOTO_DIR)
 * @return 0 on success, -1 on failure
 */
int flm_add(void *fp, const char *dir_path);
int flm_add_by_name(const char *path, const char *filename);  /* 按文件名入库 (照片/录像共用) */

// ====== 现有 API（给 video_rec / video_photo / CTP 使用，保持不变） ======
void FILE_LIST_ADD(u32 status, const char *__path, u8 create_file);
void FILE_DELETE(char *__fname, u8 create_file);
void FILE_GEN(void);
void FILE_CHANGE_ATTR(const char *fname, char attr);

#endif
