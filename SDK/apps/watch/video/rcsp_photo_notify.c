#include "typedef.h"
#include "app_config.h"
#include "rcsp_photo_notify.h"
#include "system/includes.h"
#include "audio_video_control.h"
#include "rcsp_browser.h"
#include "crc.h"

#define LOG_TAG_CONST       RCSP_PHOTO
#define LOG_TAG             "[RCSP-PHOTO]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

#if RCSP_MODE

/**
 * @brief 从完整落盘路径取文件名，拼成逻辑路径 "/video/{filename}"
 *
 * 落盘路径形如 "storage/sd0/C/video/IMG_0001.JPG"。
 * APP 读取照片依靠 storage handle + cluster 定位，file_path 中的文件名仅用于命名/展示。
 */
static const char *rcsp_photo_notify_build_file_path(char *buf, size_t buf_size,
        const char *photo_path)
{
    const char *name = strrchr(photo_path, '/');
    name = name ? name + 1 : photo_path;
    snprintf(buf, buf_size, "/video/%s", name);
    return buf;
}

/**
 * @brief 根据文件路径推导 RCSP 存储句柄
 *
 * 拍照文件统一保存在 REC_DIR = "storage/sd0/C/video/" 或 "storage/sd1/C/video/"，
 * 通过路径中的 "sd0"/"sd1" 反推存储句柄，与 rcsp_mix_record.c 的 logo 映射保持一致。
 */
static u32 rcsp_photo_notify_get_storage_handle(const char *path)
{
    if (!path) {
        return (u32) -1;
    }
    if (strstr(path, "sd0")) {
        return RCSPDevMapSD0;
    }
    if (strstr(path, "sd1")) {
        return RCSPDevMapSD1;
    }
    return (u32) -1;
}

/**
 * @brief 计算照片文件整包 CRC16
 *
 * 与 rcsp_common_info_res_file_handler.c 的 calculate_file_crc 一致：
 * 初值 0，按 256 字节分块累进 CRC16_with_initval，结果为大端写入的 2 字节 CRC。
 */
static u16 rcsp_photo_notify_calc_crc(FILE *file, u32 file_len)
{
    u8 *buf;
    u32 offset;
    u16 crc = 0;

    if (!file || !file_len) {
        return 0;
    }

    buf = malloc(256);
    if (!buf) {
        return 0;
    }

    fseek(file, 0, SEEK_SET);
    for (offset = 0; offset < file_len;) {
        u32 len = (file_len - offset) > 256 ? 256 : (file_len - offset);
        wdt_clear();
        if (len != fread(buf, len, 1, file)) {
            break;
        }
        crc = CRC16_with_initval(buf, len, crc);
        offset += len;
    }
    free(buf);
    return crc;
}

int rcsp_photo_notify_resource(const char *photo_path, u8 session_id)
{
    FILE *file;
    u32 storage_handle;
    char file_path[32];
    struct vfs_attr attr = {0};
    struct rcsp_audio_video_resource resource;

    if (!photo_path || !photo_path[0]) {
        log_error("photo notify: empty path\n");
        return -1;
    }

    storage_handle = rcsp_photo_notify_get_storage_handle(photo_path);
    if (storage_handle == (u32) -1) {
        log_error("photo notify: unknown storage: %s\n", photo_path);
        return -1;
    }

    file = fopen(photo_path, "r");
    if (!file) {
        log_error("photo notify: open fail: %s\n", photo_path);
        return -1;
    }
    if (fget_attrs(file, &attr)) {
        log_error("photo notify: attr fail: %s\n", photo_path);
        fclose(file);
        return -1;
    }

    if (!attr.sclust || !attr.fsize) {
        log_error("photo notify: invalid file, cluster=0x%x, size=%u\n",
                  attr.sclust, attr.fsize);
        fclose(file);
        return -1;
    }

    resource.operation = RCSP_AV_OP_TAKE_PHOTO;
    resource.session_id = session_id;
    resource.file_type = 0;
    resource.storage_handle = storage_handle;
    resource.cluster = attr.sclust;
    resource.file_size = attr.fsize;
    resource.crc = rcsp_photo_notify_calc_crc(file, attr.fsize);
    fclose(file);
    resource.file_path = (const u8 *)rcsp_photo_notify_build_file_path(
                             file_path, sizeof(file_path), photo_path);
    resource.file_path_len = strlen((const char *)resource.file_path);

    log_info("photo notify: handle=%u, cluster=0x%x, size=%u, crc=0x%04x, sess=%u, path=%s\n",
             storage_handle, attr.sclust, attr.fsize, resource.crc, session_id, photo_path);

    if (rcsp_audio_video_notify_resource(&resource) != JL_ERR_NONE) {
        log_error("photo notify: send fail: %s\n", photo_path);
        return -1;
    }
    return 0;
}

#endif /* RCSP_MODE */
