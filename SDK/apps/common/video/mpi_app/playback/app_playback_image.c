#include "app_playback_mpi_priv.h"
#include "fs/fs.h"

#define APP_PLAYBACK_VDEC_HW_DEV          0
#define APP_PLAYBACK_VDEC_LINE_CNT        16
#define APP_PLAYBACK_VDEC_CACHE_LINES     32
#define APP_PLAYBACK_VDEC_FRAME_EXTRA     1024
#define APP_PLAYBACK_MAX_SRC_WIDTH        1280
#define APP_PLAYBACK_MAX_SRC_HEIGHT       720

/* 无缩略图时安全整读上限改为动态获取, 见 app_playback_max_full_image_bytes() */

/*==============================================================================
 * 内嵌 EXIF 缩略图提取
 *
 * 拍照侧会把 320x240 JPEG 缩略图写入 APP1/IFD1。回放时优先提取这段缩略图
 * 送 VDEC 解码，避免直接解码整张 720P/大图导致内存/耗时风险。
 *==============================================================================*/
#define APP_PLAYBACK_EXIF_APP1_MARKER         0xE1u
#define APP_PLAYBACK_EXIF_HEADER_LEN          6u  /* "Exif\0\0" */
#define APP_PLAYBACK_EXIF_TAG_JPEG_OFFSET     0x0201u
#define APP_PLAYBACK_EXIF_TAG_JPEG_LENGTH     0x0202u
#define APP_PLAYBACK_EXIF_TYPE_SHORT          3u
#define APP_PLAYBACK_EXIF_TYPE_LONG           4u

static uint16_t app_playback_get_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint16_t app_playback_tiff_get16(const uint8_t *p, int little_endian)
{
    if (little_endian) {
        return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    }
    return app_playback_get_be16(p);
}

static uint32_t app_playback_tiff_get32(const uint8_t *p, int little_endian)
{
    if (little_endian) {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t app_playback_exif_value_u32(const uint8_t *val, uint16_t type,
        int little_endian)
{
    if (type == APP_PLAYBACK_EXIF_TYPE_SHORT) {
        return app_playback_tiff_get16(val, little_endian);
    }
    if (type == APP_PLAYBACK_EXIF_TYPE_LONG) {
        return app_playback_tiff_get32(val, little_endian);
    }
    return 0;
}

static int app_playback_parse_exif_tiff(const uint8_t *tiff, uint32_t tiff_len,
                                        const uint8_t **thumb, uint32_t *thumb_len)
{
    int little_endian;
    uint16_t magic;
    uint16_t count;
    uint16_t tag;
    uint16_t type;
    uint32_t ifd0;
    uint32_t next_ifd;
    uint32_t entry;
    uint32_t i;
    uint32_t thumb_off = 0;
    uint32_t thumb_size = 0;
    int have_off = 0;
    int have_len = 0;
    int pass;
    uint32_t ifd;

    if (tiff == NULL || thumb == NULL || thumb_len == NULL || tiff_len < 8) {
        return 0;
    }

    if (tiff[0] == 'I' && tiff[1] == 'I') {
        little_endian = 1;
    } else if (tiff[0] == 'M' && tiff[1] == 'M') {
        little_endian = 0;
    } else {
        return 0;
    }

    magic = app_playback_tiff_get16(tiff + 2, little_endian);
    if (magic != 42) {
        return 0;
    }

    ifd0 = app_playback_tiff_get32(tiff + 4, little_endian);
    if (ifd0 == 0 || ifd0 + 2 > tiff_len) {
        return 0;
    }

    next_ifd = 0;
    for (pass = 0; pass < 2; pass++) {
        ifd = (pass == 0) ? ifd0 : next_ifd;

        if (ifd == 0 || ifd + 2 > tiff_len) {
            break;
        }

        count = app_playback_tiff_get16(tiff + ifd, little_endian);
        if (ifd + 2 + (uint32_t)count * 12u + 4u > tiff_len) {
            break;
        }

        for (i = 0; i < count; i++) {
            entry = ifd + 2u + i * 12u;
            tag = app_playback_tiff_get16(tiff + entry, little_endian);
            type = app_playback_tiff_get16(tiff + entry + 2, little_endian);

            if (tag == APP_PLAYBACK_EXIF_TAG_JPEG_OFFSET) {
                thumb_off = app_playback_exif_value_u32(tiff + entry + 8, type, little_endian);
                have_off = 1;
            } else if (tag == APP_PLAYBACK_EXIF_TAG_JPEG_LENGTH) {
                thumb_size = app_playback_exif_value_u32(tiff + entry + 8, type, little_endian);
                have_len = 1;
            }
        }

        if (pass == 0) {
            next_ifd = app_playback_tiff_get32(tiff + ifd + 2u + (uint32_t)count * 12u,
                                               little_endian);
        }
    }

    if (have_off && have_len && thumb_size > 0 &&
        thumb_off < tiff_len && thumb_size <= tiff_len - thumb_off &&
        tiff[thumb_off] == 0xFF && tiff[thumb_off + 1] == 0xD8) {
        *thumb = tiff + thumb_off;
        *thumb_len = thumb_size;
        return 1;
    }

    return 0;
}

static int app_playback_extract_exif_thumb(const uint8_t *data, uint32_t len,
        const uint8_t **thumb, uint32_t *thumb_len)
{
    uint32_t p = 2;
    uint16_t seg_len;
    const uint8_t *payload;
    uint8_t marker;

    if (data == NULL || len < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        return 0;
    }

    while (p + 4 <= len) {
        if (data[p] != 0xFF) {
            p++;
            continue;
        }

        marker = data[p + 1];
        if (marker == 0xFF || marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7)) {
            p += 2;
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            return 0;
        }

        seg_len = app_playback_get_be16(data + p + 2);
        if (seg_len < 2 || p + 2u + seg_len > len) {
            return 0;
        }

        payload = data + p + 4;
        if (marker == APP_PLAYBACK_EXIF_APP1_MARKER &&
            seg_len >= 2u + APP_PLAYBACK_EXIF_HEADER_LEN &&
            payload + APP_PLAYBACK_EXIF_HEADER_LEN <= data + len &&
            memcmp(payload, "Exif\0\0", APP_PLAYBACK_EXIF_HEADER_LEN) == 0) {
            if (app_playback_parse_exif_tiff(payload + APP_PLAYBACK_EXIF_HEADER_LEN,
                                             seg_len - 2u - APP_PLAYBACK_EXIF_HEADER_LEN,
                                             thumb, thumb_len)) {
                return 1;
            }
        }

        p += 2u + seg_len;
    }

    return 0;
}


static uint32_t app_playback_frame_size(uint16_t width,
                                        uint16_t height,
                                        mpi_pixel_format_t format)
{
    switch (format) {
    case MPI_PIXEL_FMT_YUV420SP:
        return (uint32_t)width * height * 3u / 2u;
    case MPI_PIXEL_FMT_YUV444_PACKED:
        return (uint32_t)width * height * 3u;
    case MPI_PIXEL_FMT_YUV422_PACKED:
    default:
        return (uint32_t)width * height * 2u;
    }
}

static uint8_t app_playback_pick_factor(uint16_t value, uint16_t limit)
{
    uint8_t factor;

    if (value <= limit) {
        return 0;
    }

    for (factor = 2; factor <= 15; factor++) {
        if ((value % factor) == 0 && (value / factor) <= limit) {
            return factor;
        }
    }

    return 0xff;
}

static app_playback_ret_t app_playback_calc_prescale(const app_playback_image_info_t *image_info,
        uint8_t *pre_scale_enable,
        uint8_t *width_scale_factor,
        uint8_t *height_scale_factor)
{
    uint8_t width_factor;
    uint8_t height_factor;

    if (image_info == NULL || pre_scale_enable == NULL ||
        width_scale_factor == NULL || height_scale_factor == NULL ||
        image_info->align_width == 0 || image_info->align_height == 0) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    *pre_scale_enable = 0;
    *width_scale_factor = 0;
    *height_scale_factor = 0;

    if (image_info->align_width <= APP_PLAYBACK_MAX_SRC_WIDTH &&
        image_info->align_height <= APP_PLAYBACK_MAX_SRC_HEIGHT) {
        return APP_PLAYBACK_OK;
    }

    width_factor = app_playback_pick_factor(image_info->align_width, APP_PLAYBACK_MAX_SRC_WIDTH);
    height_factor = app_playback_pick_factor(image_info->align_height, APP_PLAYBACK_MAX_SRC_HEIGHT);
    if (width_factor == 0xff) {
        printf("failed to calc VPSS pre-scale factor, align=%ux%u\n",
               image_info->align_width,
               image_info->align_height);
        return APP_PLAYBACK_ERR_NOT_SUPPORT;
    }
    if (height_factor != 2 && height_factor != 4) {
        height_factor = 0;
    }
    if (width_factor == 0 && height_factor == 0) {
        return APP_PLAYBACK_OK;
    }

    *pre_scale_enable = 1;
    *width_scale_factor = width_factor;
    *height_scale_factor = height_factor;

    return APP_PLAYBACK_OK;
}

static void app_playback_prepare_vo_attr(mpi_vo_chn_attr_t *vo_attr,
        const app_playback_display_attr_t *display)
{
    memset(vo_attr, 0, sizeof(*vo_attr));
    memcpy(vo_attr->fb_name, display->fb_name, sizeof(vo_attr->fb_name));
    vo_attr->fb_num = display->fb_num;
    vo_attr->fb_width = display->width;
    vo_attr->fb_height = display->height;
    vo_attr->window_width = display->width;
    vo_attr->window_height = display->height;
    vo_attr->left = display->left;
    vo_attr->top = display->top;
    vo_attr->fb_format = FB_COLOR_FORMAT_YUV420P;
    vo_attr->rotate = display->rotate;
    vo_attr->mirror = display->mirror;
    vo_attr->combine = 1;
}

static void app_playback_fill_chn(int mod_id, int dev_id, int chn_id, mpi_chn_t *chn)
{
    chn->mod_id = mod_id;
    chn->dev_id = dev_id;
    chn->chn_id = chn_id;
}

/* 在文件里定位内嵌 EXIF 缩略图: 只扫描 JPEG 段头, 不整读大图。
 * 成功返回 APP_PLAYBACK_OK, 并给出缩略图在文件中的绝对偏移和长度。 */
static app_playback_ret_t app_playback_find_exif_thumb_in_file(FILE *fp,
        uint8_t **thumb_buf, uint32_t *thumb_len)
{
    uint8_t hdr[4];
    uint8_t skip[64];
    uint8_t *payload = NULL;
    uint8_t *thumb = NULL;
    const uint8_t *thumb_in_payload;
    uint32_t payload_len;
    uint32_t thumb_size = 0;
    uint32_t remain;
    uint16_t seg_len;
    uint8_t marker;
    long fsize;
    int ret = APP_PLAYBACK_ERR_NOT_FOUND;

    if (fp == NULL || thumb_buf == NULL || thumb_len == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    *thumb_buf = NULL;
    *thumb_len = 0;

    fsize = flen(fp);
    if (fsize < 4) {
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    /* 顺序读定位缩略图, 全程不使用 fseek: 该文件系统 fseek 后可能清空后续内容 */
    if (fread(hdr, 1, 2, fp) != 2 || hdr[0] != 0xFF || hdr[1] != 0xD8) {
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    while (1) {
        if (fread(hdr, 1, 2, fp) != 2) {
            break;
        }
        if (hdr[0] != 0xFF) {
            break;
        }

        marker = hdr[1];
        if (marker == 0xFF || marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7)) {
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            break;
        }

        if (fread(hdr, 1, 2, fp) != 2) {
            break;
        }
        seg_len = (uint16_t)(((uint16_t)hdr[0] << 8) | hdr[1]);
        if (seg_len < 2) {
            break;
        }
        payload_len = seg_len - 2u;

        if (marker == APP_PLAYBACK_EXIF_APP1_MARKER &&
            payload_len >= APP_PLAYBACK_EXIF_HEADER_LEN) {
            payload = (uint8_t *)malloc(payload_len);
            if (payload == NULL) {
                ret = APP_PLAYBACK_ERR_NOMEM;
                break;
            }
            if (fread(payload, 1, payload_len, fp) != payload_len) {
                free(payload);
                ret = APP_PLAYBACK_ERR_IO;
                break;
            }

            if (memcmp(payload, "Exif\0\0", APP_PLAYBACK_EXIF_HEADER_LEN) == 0 &&
                app_playback_parse_exif_tiff(payload + APP_PLAYBACK_EXIF_HEADER_LEN,
                                             payload_len - APP_PLAYBACK_EXIF_HEADER_LEN,
                                             &thumb_in_payload, &thumb_size)) {
                thumb = (uint8_t *)malloc(thumb_size);
                if (thumb == NULL) {
                    free(payload);
                    ret = APP_PLAYBACK_ERR_NOMEM;
                    break;
                }
                memcpy(thumb, thumb_in_payload, thumb_size);
                *thumb_buf = thumb;
                *thumb_len = thumb_size;
                free(payload);
                return APP_PLAYBACK_OK;
            }
            free(payload);
            payload = NULL;
        } else {
            /* 非 APP1/非缩略图段: 顺序读丢弃, 不用 fseek */
            remain = payload_len;
            while (remain > 0) {
                uint32_t n = remain > sizeof(skip) ? (uint32_t)sizeof(skip) : remain;
                if (fread(skip, 1, n, fp) != n) {
                    ret = APP_PLAYBACK_ERR_IO;
                    break;
                }
                remain -= n;
            }
            if (ret != APP_PLAYBACK_ERR_NOT_FOUND) {
                break;
            }
        }
    }

    return ret;
}

/* 公开接口：读取 JPG 文件内嵌 EXIF 缩略图。
 * 成功返回 APP_PLAYBACK_OK，*buf 为 malloc 出来的缩略图 JPEG，*len 为长度；
 * 调用方使用完必须 free(*buf)。无缩略图返回 APP_PLAYBACK_ERR_NOT_FOUND。 */
app_playback_ret_t app_playback_read_embedded_thumb(const char *path,
        uint8_t **buf, uint32_t *len)
{
    FILE *fp;
    app_playback_ret_t ret;

    if (path == NULL || path[0] == '\0' || buf == NULL || len == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    *buf = NULL;
    *len = 0;

    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("failed to open file: %s\n", path);
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    ret = app_playback_find_exif_thumb_in_file(fp, buf, len);
    fclose(fp);

    if (ret == APP_PLAYBACK_OK) {
        printf("[APP_PLAYBACK] read embedded thumbnail len=%u\n", *len);
    }
    return ret;
}

/* 动态计算无缩略图时的整读安全上限:
 * 取当前剩余可分配内存(ram0+ram1)的 1/4, 至少保留给解码/显示等后续流程足够余量。
 * 若获取失败/内存很小, 返回保守下限 256KB。 */
static uint32_t app_playback_max_full_image_bytes(void)
{
    size_t free_ram0 = memory_get_size(P_MEMORY_UNUSED);
    size_t free_ram1 = memory_get_size(P_VLT_MEMORY_UNUSED);
    uint32_t threshold;

    if (free_ram0 == 0 && free_ram1 == 0) {
        return 256u * 1024u;
    }

    threshold = (uint32_t)((free_ram0 + free_ram1) / 4u);
    if (threshold < 256u * 1024u) {
        threshold = 256u * 1024u;
    }

    return threshold;
}


static app_playback_ret_t app_playback_load_file(const char *path, uint8_t **buf, uint32_t *len)
{
    FILE *fp;
    uint8_t *data;
    uint8_t *thumb_buf = NULL;
    long file_len;
    uint32_t thumb_len = 0;
    app_playback_ret_t find_ret;

    if (path == NULL || path[0] == '\0' || buf == NULL || len == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    /* SDK FS 只支持 "r"/"w"/"w+", 不要用 "rb" */
    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("failed to open file: %s\n", path);
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    file_len = flen(fp);
    if (file_len <= 0) {
        printf("invalid file length: %s, len=%ld\n", path, file_len);
        fclose(fp);
        return APP_PLAYBACK_ERR_IO;
    }

    /* 优先只读内嵌 EXIF 缩略图, 避免整张大图 malloc 导致物理内存不足死机。
     * 全程顺序读, 不使用 fseek, 防止该文件系统 fseek 清空后续内容。 */
    find_ret = app_playback_find_exif_thumb_in_file(fp, &thumb_buf, &thumb_len);
    if (find_ret == APP_PLAYBACK_OK) {
        fclose(fp);
        *buf = thumb_buf;
        *len = thumb_len;
        printf("[APP_PLAYBACK] read embedded thumbnail len=%u\n", thumb_len);
        return APP_PLAYBACK_OK;
    }

    /* 无缩略图: 只有小图才允许整读, 超过安全上限直接失败, 不触发 OOM ASSERT */
    if (find_ret != APP_PLAYBACK_ERR_NOT_FOUND) {
        fclose(fp);
        return find_ret;
    }
    if ((uint32_t)file_len > app_playback_max_full_image_bytes()) {
        printf("photo too large without thumbnail, skip full load: %s len=%ld\n",
               path, file_len);
        fclose(fp);
        return APP_PLAYBACK_ERR_NOMEM;
    }

    /* 整读前重新打开文件: 内嵌缩略图扫描已把指针走到尾部, 该文件系统 fseek 不可用,
     * 不重开则 fread 在 EOF 必然短读(全部无缩略图照片回放失败)。 */
    fclose(fp);
    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("failed to reopen file: %s\n", path);
        return APP_PLAYBACK_ERR_IO;
    }

    data = (uint8_t *)malloc((size_t)file_len);
    if (data == NULL) {
        fclose(fp);
        return APP_PLAYBACK_ERR_NOMEM;
    }

    if (fread(data, 1, (size_t)file_len, fp) != (size_t)file_len) {
        printf("failed to read file: %s\n", path);
        free(data);
        fclose(fp);
        return APP_PLAYBACK_ERR_IO;
    }

    fclose(fp);
    *buf = data;
    *len = (uint32_t)file_len;

    return APP_PLAYBACK_OK;
}

static app_playback_ret_t app_playback_image_probe(const mpi_stream_t *stream,
        app_playback_image_info_t *image_info)
{
    mpi_ret_t ret;
    mpi_vdec_probe_info_t pic_info = {0};

    if (stream == NULL || image_info == NULL ||
        stream->addr == NULL || stream->len == 0) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    memset(image_info, 0, sizeof(*image_info));

    ret = mpi_vdec_probe_pic_info(stream, &pic_info);
    if (ret != MPI_SUCCESS) {
        printf("failed to probe VDEC pic info, ret=%d\n", ret);
        return APP_PLAYBACK_ERR_IO;
    }

    image_info->align_width = pic_info.align_width;
    image_info->align_height = pic_info.align_height;
    image_info->format = pic_info.format;

    return APP_PLAYBACK_OK;
}

static int app_playback_image_info_match(app_playback_handle_t ctx,
        const app_playback_image_info_t *image_info)
{
    if (ctx == NULL || image_info == NULL) {
        return 0;
    }

    return memcmp(&ctx->image_info, image_info, sizeof(*image_info)) == 0;
}

static app_playback_ret_t app_playback_image_pipeline_open(app_playback_handle_t ctx,
        const app_playback_image_info_t *image_info)
{
    mpi_ret_t ret;
    app_playback_ret_t err_ret = APP_PLAYBACK_ERR_IO;
    mpi_vdec_chn_attr_t vdec_attr = {0};
    mpi_vpss_grp_attr_t grp_attr = {0};
    mpi_vpss_chn_attr_t vpss_attr = {0};
    mpi_vo_chn_attr_t vo_attr = {0};
    mpi_chn_t vdec_src;
    mpi_chn_t vpss_dst;
    mpi_chn_t vpss_out;
    mpi_chn_t vo_dst;
    uint8_t pre_scale_enable = 0;
    uint8_t width_scale_factor = 0;
    uint8_t height_scale_factor = 0;
    uint32_t vdec_frame_buf_size;

    if (ctx == NULL || image_info == NULL ||
        ctx->display_attr.width == 0 || ctx->display_attr.height == 0 ||
        image_info->align_width == 0 || image_info->align_height == 0) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    err_ret = app_playback_calc_prescale(image_info,
                                         &pre_scale_enable,
                                         &width_scale_factor,
                                         &height_scale_factor);
    if (err_ret != APP_PLAYBACK_OK) {
        return err_ret;
    }
    err_ret = APP_PLAYBACK_ERR_IO;
    vdec_frame_buf_size = app_playback_frame_size(image_info->align_width,
                          APP_PLAYBACK_VDEC_CACHE_LINES,
                          image_info->format) +
                          APP_PLAYBACK_VDEC_FRAME_EXTRA;

    ctx->vdec_dev = APP_PLAYBACK_VDEC_HW_DEV;
    ret = mpi_vdec_find_free_chn(&ctx->vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VDEC channel\n");
        return APP_PLAYBACK_ERR_NOT_FOUND;
    }

    vdec_attr.hw_dev = ctx->vdec_dev;
    vdec_attr.type = MPI_VDEC_TYPE_JPEG;
    vdec_attr.out_yuv_order = MPI_YUV_ORDER_YUYV;
    vdec_attr.output_mode = MPI_VDEC_OUTPUT_LINE;
    vdec_attr.line_cnt = APP_PLAYBACK_VDEC_LINE_CNT;
    vdec_attr.frame_buf_size = vdec_frame_buf_size;
    ret = mpi_vdec_create_chn(ctx->vdec_chn, &vdec_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VDEC channel\n");
        return APP_PLAYBACK_ERR_IO;
    }

    ret = mpi_vpss_find_free_grp(&ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS group\n");
        goto err_destroy_vdec;
    }

    if (pre_scale_enable && ctx->vpss_grp != 0) {
        printf("pre-scale only supported for  VPSS group[0]\n");
        err_ret = APP_PLAYBACK_ERR_NOT_SUPPORT;
        goto err_destroy_vdec;
    }

    grp_attr.src_width = image_info->align_width;
    grp_attr.src_height = image_info->align_height;
    grp_attr.src_format = MPI_VPSS_SRC_FMT_YUV;
    grp_attr.pre_scale_enable = pre_scale_enable;
    grp_attr.width_scale_factor = width_scale_factor;
    grp_attr.height_scale_factor = height_scale_factor;
    grp_attr.src_pixel_format = image_info->format;
    grp_attr.src_yuv_order = MPI_YUV_ORDER_YUYV;
    grp_attr.replay_isp_mode = 0;
    ret = mpi_vpss_create_grp(ctx->vpss_grp, &grp_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VPSS group\n");
        goto err_destroy_vdec;
    }

    ret = mpi_vpss_start_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VPSS group\n");
        goto err_destroy_grp;
    }

    ret = mpi_vpss_find_free_chn(ctx->vpss_grp, &ctx->vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VPSS channel\n");
        goto err_stop_grp;
    }

    vpss_attr.width = ctx->display_attr.width;
    vpss_attr.height = ctx->display_attr.height;
    vpss_attr.format = MPI_PIXEL_FMT_YUV420SP;
    vpss_attr.output_mode = MPI_VPSS_OUTPUT_OFFLINE;
    ret = mpi_vpss_create_chn(ctx->vpss_grp, ctx->vpss_chn, &vpss_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to set VPSS channel attributes\n");
        goto err_stop_grp;
    }
    ret = mpi_vpss_enable_chn(ctx->vpss_grp, ctx->vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to enable VPSS channel\n");
        goto err_destroy_vpss_chn;
    }

    app_playback_prepare_vo_attr(&vo_attr, &ctx->display_attr);

    ret = mpi_vo_find_free_chn(&ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to find free VO channel\n");
        goto err_disable_vpss_chn;
    }
    ret = mpi_vo_create_chn(ctx->vo_chn, &vo_attr);
    if (ret != MPI_SUCCESS) {
        printf("failed to create VO channel\n");
        goto err_disable_vpss_chn;
    }

    ret = mpi_vo_start_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VO channel\n");
        goto err_destroy_vo;
    }

    app_playback_fill_chn(MPI_MOD_ID_VDEC, ctx->vdec_dev, ctx->vdec_chn, &vdec_src);
    app_playback_fill_chn(MPI_MOD_ID_VPSS, ctx->vpss_grp, 0, &vpss_dst);
    app_playback_fill_chn(MPI_MOD_ID_VPSS, ctx->vpss_grp, ctx->vpss_chn, &vpss_out);
    app_playback_fill_chn(MPI_MOD_ID_VO, 0, ctx->vo_chn, &vo_dst);

    ret = mpi_sys_bind(&vpss_out, &vo_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VPSS channel and VO channel\n");
        goto err_stop_vo;
    }

    ret = mpi_vdec_start_chn(ctx->vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to start VDEC channel\n");
        goto err_unbind_vpss_vo;
    }

    ret = mpi_sys_bind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS) {
        printf("failed to bind VDEC and VPSS group\n");
        goto err_stop_vdec;
    }

    memcpy(&ctx->image_info, image_info, sizeof(ctx->image_info));
    return APP_PLAYBACK_OK;

err_stop_vdec:
    mpi_vdec_stop_chn(ctx->vdec_chn);
err_unbind_vpss_vo:
    mpi_sys_unbind(&vpss_out, &vo_dst);
err_stop_vo:
    mpi_vo_stop_chn(ctx->vo_chn);
err_destroy_vo:
    mpi_vo_destroy_chn(ctx->vo_chn);
err_disable_vpss_chn:
    mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn);
err_destroy_vpss_chn:
    mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn);
err_stop_grp:
    mpi_vpss_stop_grp(ctx->vpss_grp);
err_destroy_grp:
    mpi_vpss_destroy_grp(ctx->vpss_grp);
err_destroy_vdec:
    mpi_vdec_destroy_chn(ctx->vdec_chn);

    return err_ret;
}

app_playback_ret_t app_playback_image_pipeline_close(app_playback_handle_t ctx)
{
    app_playback_ret_t app_ret = APP_PLAYBACK_OK;
    mpi_ret_t ret;
    mpi_chn_t vdec_src;
    mpi_chn_t vpss_dst;
    mpi_chn_t vpss_out;
    mpi_chn_t vo_dst;

    if (ctx == NULL) {
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    if (!ctx->pipeline_started) {
        return APP_PLAYBACK_OK;  /* 管线未打开, 无需关闭, 避免重复关闭产生 MPI 错误日志 */
    }

    app_playback_fill_chn(MPI_MOD_ID_VDEC, ctx->vdec_dev, ctx->vdec_chn, &vdec_src);
    app_playback_fill_chn(MPI_MOD_ID_VPSS, ctx->vpss_grp, 0, &vpss_dst);
    app_playback_fill_chn(MPI_MOD_ID_VPSS, ctx->vpss_grp, ctx->vpss_chn, &vpss_out);
    app_playback_fill_chn(MPI_MOD_ID_VO, 0, ctx->vo_chn, &vo_dst);

    /*
     * 关闭顺序: 自下游往上游逐级关闭。
     * 1. 先断 VPSS→VO bind (断开 IMC dst 与 VO 的连接)
     * 2. 关闭 VO
     * 3. 释放 VPSS chn 的 IMC dst 硬件资源 (imc_dst_free)
     * 4. 再断 VDEC→VPSS bind (此时 IMC dst 已释放, replay cancel 的 vbus_stop 不会影响它)
     * 5. 清理 VPSS grp / VDEC
     */

    /* Step 1: 先解下游 VPSS_CHN→VO */
    ret = mpi_sys_unbind(&vpss_out, &vo_dst);
    if (ret != MPI_SUCCESS && ret != MPI_ERR_NOT_FOUND) {
        printf("failed to unbind VPSS and VO\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    /* Step 2: 关闭 VO */
    ret = mpi_vo_stop_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VO channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }
    ret = mpi_vo_destroy_chn(ctx->vo_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VO channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    /* Step 3: 释放 VPSS chn 的 IMC dst 硬件 (必须先于 replay cancel) */
    ret = mpi_vpss_disable_chn(ctx->vpss_grp, ctx->vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to disable VPSS channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }
    ret = mpi_vpss_destroy_chn(ctx->vpss_grp, ctx->vpss_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    /* Step 4: 再解上游 VDEC→VPSS_GRP (此时 IMC dst 已释放, vbus_stop 安全) */
    ret = mpi_sys_unbind(&vdec_src, &vpss_dst);
    if (ret != MPI_SUCCESS && ret != MPI_ERR_NOT_FOUND) {
        printf("failed to unbind VDEC and VPSS\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    /* Step 5: 清理 VPSS grp */
    ret = mpi_vpss_stop_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VPSS group\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }
    ret = mpi_vpss_destroy_grp(ctx->vpss_grp);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VPSS group\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    /* Step 6: 清理 VDEC */
    ret = mpi_vdec_stop_chn(ctx->vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to stop VDEC channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }
    ret = mpi_vdec_destroy_chn(ctx->vdec_chn);
    if (ret != MPI_SUCCESS) {
        printf("failed to destroy VDEC channel\n");
        app_ret = APP_PLAYBACK_ERR_IO;
    }

    memset(&ctx->image_info, 0, sizeof(ctx->image_info));
    ctx->pipeline_started = 0;

    return app_ret;
}

app_playback_ret_t app_playback_image_decode_stream(app_playback_handle_t handle,
        const mpi_stream_t *stream)
{
    mpi_ret_t ret;
    app_playback_ret_t app_ret;
    app_playback_image_info_t image_info = {0};
    mpi_stream_t decode_stream;
    const uint8_t *thumb = NULL;
    uint32_t thumb_len = 0;
    int use_thumb;
    int need_reopen;

    if (handle == NULL || stream == NULL || stream->addr == NULL || stream->len == 0) {
        printf("handle or image stream is invalid\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }

    /* 优先解码 JPG 内嵌 EXIF 缩略图: 小图解码更快更省内存, 避免大图解码风险 */
    use_thumb = app_playback_extract_exif_thumb(stream->addr, stream->len,
                &thumb, &thumb_len);
    if (use_thumb) {
        memset(&decode_stream, 0, sizeof(decode_stream));
        decode_stream.addr = thumb;
        decode_stream.len = thumb_len;
        app_ret = app_playback_image_probe(&decode_stream, &image_info);
        if (app_ret == APP_PLAYBACK_OK) {
            printf("[APP_PLAYBACK] decode embedded thumbnail len=%u (full=%u)\n",
                   thumb_len, stream->len);
        } else {
            printf("[APP_PLAYBACK] embedded thumbnail probe failed, fallback full image\n");
            use_thumb = 0;
        }
    }

    if (!use_thumb) {
        decode_stream = *stream;
        app_ret = app_playback_image_probe(&decode_stream, &image_info);
        if (app_ret != APP_PLAYBACK_OK) {
            printf("failed to probe image playback pipeline\n");
            return app_ret;
        }
    }

    need_reopen = !handle->pipeline_started || !app_playback_image_info_match(handle, &image_info);

    if (need_reopen && handle->pipeline_started) {
        app_ret = app_playback_image_pipeline_close(handle);
        if (app_ret != APP_PLAYBACK_OK) {
            printf("failed to close old image playback pipeline\n");
            return app_ret;
        }
    }

    if (need_reopen) {
        app_ret = app_playback_image_pipeline_open(handle, &image_info);
        if (app_ret != APP_PLAYBACK_OK) {
            printf("failed to open image playback pipeline\n");
            return app_ret;
        }
    }

    ret = mpi_vdec_send_stream(handle->vdec_chn, &decode_stream);
    if (ret != MPI_SUCCESS) {
        printf("failed to send VDEC stream, ret=%d\n", ret);
        if (need_reopen) {
            app_playback_image_pipeline_close(handle);
        }
        return APP_PLAYBACK_ERR_IO;
    }

    memcpy(&handle->image_info, &image_info, sizeof(handle->image_info));
    handle->pipeline_started = 1;

    return APP_PLAYBACK_OK;
}

app_playback_ret_t app_playback_one_image_path(app_playback_handle_t handle,
        const char *path)
{
    app_playback_ret_t app_ret;
    mpi_stream_t stream = {0};
    uint8_t *buf = NULL;
    uint32_t len = 0;

    if (handle == NULL || path == NULL || path[0] == '\0') {
        printf("handle or image path is invalid\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (handle->video_started) {
        printf("playback is busy\n");
        return APP_PLAYBACK_ERR_BUSY;
    }

    app_ret = app_playback_load_file(path, &buf, &len);
    if (app_ret != APP_PLAYBACK_OK) {
        printf("failed to load image file\n");
        return app_ret;
    }

    stream.addr = buf;
    stream.len = len;

    app_ret = app_playback_image_decode_stream(handle, &stream);
    if (app_ret != APP_PLAYBACK_OK) {
        printf("failed to send image stream\n");
    }
    free(buf);

    return app_ret;
}

app_playback_ret_t app_playback_one_image_data(app_playback_handle_t handle,
        const uint8_t *data,
        uint32_t len)
{
    app_playback_ret_t app_ret;
    mpi_stream_t stream = {0};

    if (handle == NULL || data == NULL || len == 0) {
        printf("handle or image data is invalid\n");
        return APP_PLAYBACK_ERR_INVALID_ARG;
    }
    if (handle->video_started) {
        printf("playback is busy\n");
        return APP_PLAYBACK_ERR_BUSY;
    }

    stream.addr = data;
    stream.len = len;

    app_ret = app_playback_image_decode_stream(handle, &stream);
    if (app_ret != APP_PLAYBACK_OK) {
        printf("failed to send image stream\n");
    }

    return app_ret;
}

