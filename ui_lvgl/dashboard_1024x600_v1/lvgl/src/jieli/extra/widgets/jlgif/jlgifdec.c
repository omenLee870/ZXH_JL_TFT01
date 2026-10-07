/**
 * @file jlgifdec.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <string.h>
#include "../../../../../lvgl.h"
#include "jlgifdec.h"
#include "../../libs/rle/rle.h"

#if LV_USE_JLGIF

/*********************
 *      DEFINES
 *********************/
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define GIF_FLAG_HAS_COLOR_TABLE     (1 << 0)  // 是否有颜色表
#define GIF_FLAG_HAS_TRANS_COLOR     (1 << 1)  // 是否有透明色
#define GIF_FLAG_IS_D_FRAME          (1 << 2)  // 是否为 D_FRAME
#define GIF_FLAG_COMPRESS_MASK       (3 << 3)  // 压缩方式掩码
#define GIF_FLAG_COMPRESS_LZW_RLE    (0 << 3)  // LZW+RLE 压缩
#define GIF_FLAG_COMPRESS_RLE        (1 << 3)  // RLE 压缩
#define GIF_FLAG_COMPRESS_LZW        (2 << 3)  // LZW 压缩
#define GIF_FLAG_HAS_GLOBAL_TABLE    (1 << 5)  // 是否有全局颜色表
#define GIF_FLAG_TRANS_COLOR_MASK    (0xFF << 8) // 透明色掩码

/**********************
 *   STATIC PROTOTYPES
 **********************/
static jl_gif_t * gif_open(jl_gif_t * gif);
static bool f_gif_open(jl_gif_t * gif, const void * path, bool is_file, uint32_t size);
static void f_gif_read(jl_gif_t * gif, void * buf, size_t len);
static int f_gif_seek(jl_gif_t * gif, size_t pos, int k);
static void f_gif_close(jl_gif_t * gif);
static int decode_frame_data(jl_gif_t * gif, uint16_t frame_index);
static void render_frame_rect(jl_gif_t * gif, uint8_t * buffer);
static void update_palette_in_canvas(jl_gif_t * gif);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

jl_gif_t * jl_open_gif_file(const char * fname)
{
    jl_gif_t gif_base;
    memset(&gif_base, 0, sizeof(gif_base));

    bool res = f_gif_open(&gif_base, fname, true, 0);
    if(!res) return NULL;

    return gif_open(&gif_base);
}

jl_gif_t * jl_open_gif_data(const void * data, uint32_t size)
{
    jl_gif_t gif_base;
    memset(&gif_base, 0, sizeof(gif_base));

    bool res = f_gif_open(&gif_base, data, false, size);
    if(!res) return NULL;

    return gif_open(&gif_base);
}

void jl_render_frame(jl_gif_t * gif, uint8_t * buffer)
{
    if(gif == NULL || buffer == NULL) return;

    update_palette_in_canvas(gif);
    render_frame_rect(gif, buffer);
}

int jl_get_frame(jl_gif_t * gif)
{
    if(gif->curr_frame >= gif->header.frame_num - 1) {
        if(gif->loop_count == 1 || gif->loop_count < 0) {
            return 0;  // 到达结尾
        }
        else if(gif->loop_count > 1) {
            gif->loop_count--;
            gif->curr_frame = 0;
        }
        else {   // loop_count == 0, 无限循环
            gif->curr_frame = 0;
        }
    }
    else {
        gif->curr_frame++;
    }

    if(decode_frame_data(gif, gif->curr_frame) < 0) {
        return -1;
    }

    return 1;
}

int jl_get_specific_frame(jl_gif_t * gif, uint16_t frame_index)
{
    if(frame_index >= gif->header.frame_num) {
        LV_LOG_WARN("Frame index out of range");
        return -1;
    }

    gif->curr_frame = frame_index;
    if(decode_frame_data(gif, frame_index) < 0) {
        return -1;
    }

    return 1;
}

void jl_rewind(jl_gif_t * gif)
{
    gif->curr_frame = -1;
    gif->loop_count = -1;
}

void jl_close_gif(jl_gif_t * gif)
{
    if(gif == NULL) return;

    f_gif_close(gif);
    if(gif->frames) {
        lv_free(gif->frames);
        gif->frames = NULL;
    }
    if(gif->frame) {
        lv_free(gif->frame);
        gif->frame = NULL;
    }
    if(gif->base_frame) {
        lv_free(gif->base_frame);
        gif->base_frame = NULL;
    }
    lv_free(gif);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static jl_gif_t * gif_open(jl_gif_t * gif_base)
{
    jl_gif_t * gif = NULL;
    uint8_t * bgcolor;
    int i;

    /* 读取GIF头部 */
    f_gif_read(gif_base, &gif_base->header, sizeof(jl_gif_file_header_t));

    /* 验证版本号 */
    if(memcmp(gif_base->header.version, "JLG", 3) != 0) {
        LV_LOG_WARN("Invalid JLGIF signature");
        goto fail;
    }

    /* 分配内存并创建GIF结构 */
    uint32_t canvas_size = gif_base->header.width * gif_base->header.height +
                           JLGIF_COLOR_TABLE_LENGTH; // 索引数据 + 调色板
    gif = lv_malloc(sizeof(jl_gif_t) + canvas_size);

    if(!gif) {
        LV_LOG_ERROR("Failed to allocate memory for GIF");
        goto fail;
    }

    memcpy(gif, gif_base, sizeof(jl_gif_t));

    /* 分配帧信息数组 */
    gif->frames = lv_malloc(sizeof(jl_gif_file_frame_t) * gif->header.frame_num);
    if(!gif->frames) {
        LV_LOG_ERROR("Failed to allocate memory for frame info");
        lv_free(gif);
        goto fail;
    }

    /* 读取所有帧信息 */
    f_gif_read(gif, gif->frames, sizeof(jl_gif_file_frame_t) * gif->header.frame_num);

    /* 设置画布和索引数据区域 */
    gif->canvas = (uint8_t *) &gif[1];              // 画布起始位置
    gif->index_data = gif->canvas + JLGIF_COLOR_TABLE_LENGTH;   // 索引数据起始位置(调色板之后)

    /* 分配当前帧和参考帧缓冲区 */
    gif->frame = lv_malloc(gif->header.width * gif->header.height);
    if(!gif->frame) {
        LV_LOG_ERROR("Failed to allocate memory for frame buffer");
        lv_free(gif->frames);
        lv_free(gif);
        goto fail;
    }

    gif->base_frame = lv_malloc(gif->header.width * gif->header.height);
    gif->base_index = -1; // 参考帧索引
    if(!gif->base_frame) {
        LV_LOG_ERROR("Failed to allocate memory for base frame");
        lv_free(gif->frame);
        lv_free(gif->frames);
        lv_free(gif);
        goto fail;
    }

    /* 检查是否有全局颜色表 */
    if(gif->frames[0].flag & GIF_FLAG_HAS_GLOBAL_TABLE) {
        /* 读取全局颜色表 */
        gif->gct.size = JLGIF_COLOR_TABLE_SIZE;
        f_gif_read(gif, gif->gct.colors, JLGIF_COLOR_TABLE_LENGTH);
        gif->palette = &gif->gct;
    }

    /* 初始化索引数据区域为0 (透明) */
    memset(gif->index_data, 0, gif->header.width * gif->header.height);

    /* 初始化canvas中的调色板部分 */
    update_palette_in_canvas(gif);

    /* 设置初始状态 */
    gif->curr_frame = -1;
    gif->loop_count = -1; // 无限循环

    return gif;

fail:
    f_gif_close(gif_base);
    return NULL;
}

static bool f_gif_open(jl_gif_t * gif, const void * path, bool is_file, uint32_t size)
{
    gif->f_rw_p = 0;
    gif->data = NULL;
    gif->is_file = is_file;

    if(is_file) {
        lv_fs_res_t res = lv_fs_open(&gif->fd, path, LV_FS_MODE_RD);
        if(res != LV_FS_RES_OK) return false;
        else return true;
    }
    else {
        gif->data = path;
        return true;
    }
}

static void f_gif_read(jl_gif_t * gif, void * buf, size_t len)
{
    if(gif->is_file) {
        lv_fs_read(&gif->fd, buf, len, NULL);
    }
    else {
        memcpy(buf, &gif->data[gif->f_rw_p], len);
        gif->f_rw_p += len;
    }
}

static int f_gif_seek(jl_gif_t * gif, size_t pos, int k)
{
    if(gif->is_file) {
        lv_fs_seek(&gif->fd, pos, k);
        uint32_t x;
        lv_fs_tell(&gif->fd, &x);
        return x;
    }
    else {
        if(k == LV_FS_SEEK_CUR) gif->f_rw_p += pos;
        else if(k == LV_FS_SEEK_SET) gif->f_rw_p = pos;
        return gif->f_rw_p;
    }
}

static void f_gif_close(jl_gif_t * gif)
{
    if(gif->is_file) {
        lv_fs_close(&gif->fd);
    }
}

static void update_palette_in_canvas(jl_gif_t * gif)
{
    if(!gif || !gif->palette) return;

    /* 将调色板复制到canvas的开头部分 */
    memcpy(gif->canvas, gif->palette->colors, JLGIF_COLOR_TABLE_LENGTH);

    /* 如果有透明色，设置透明色的Alpha为0 */
    if(gif->has_trans) {
        gif->canvas[gif->trans_index * 4 + 3] = 0; // 设置Alpha通道为0
    }
}

static int decode_frame_data(jl_gif_t * gif, uint16_t frame_index)
{
    jl_gif_file_frame_t * frame = &gif->frames[frame_index];
    jl_gif_file_frame_t * base_frame = NULL;
    uint8_t * frame_data = NULL;
    uint8_t * curr_palette = NULL;
    uint32_t palette_size = 0;

    /* 定位到帧数据位置 */
    f_gif_seek(gif, frame->addr, LV_FS_SEEK_SET);

    /* 检查是否有帧的颜色表 */
    if(frame->flag & GIF_FLAG_HAS_COLOR_TABLE) {
        /* 读取帧的颜色表 */
        palette_size = JLGIF_COLOR_TABLE_LENGTH;
        curr_palette = lv_malloc(palette_size);
        if(!curr_palette) {
            LV_LOG_ERROR("Failed to allocate memory for palette");
            return -1;
        }
        f_gif_read(gif, curr_palette, palette_size);
    }
    else if(frame->flag & GIF_FLAG_HAS_GLOBAL_TABLE) {
        gif->palette = &gif->gct;
    }
    else {
        LV_LOG_ERROR("No color table available for frame");
        return -1;
    }

    /* 分配内存读取帧数据 */
    frame_data = lv_malloc(frame->size);
    if(!frame_data) {
        LV_LOG_ERROR("Failed to allocate memory for frame data");
        if(curr_palette) lv_free(curr_palette);
        return -1;
    }
    f_gif_read(gif, frame_data, frame->size);

    if(frame->base_index != gif->base_index) {
        /* 解码参考帧 */
        uint32_t curr_pos = f_gif_seek(gif, 0, LV_FS_SEEK_CUR);
        base_frame = &gif->frames[frame->base_index];

        /* 定位到参考帧数据位置 */
        f_gif_seek(gif, base_frame->addr, LV_FS_SEEK_SET);

        /* 跳过参考帧颜色表（如果有的话） */
        if(base_frame->flag & GIF_FLAG_HAS_COLOR_TABLE) {
            f_gif_seek(gif, JLGIF_COLOR_TABLE_LENGTH, LV_FS_SEEK_CUR);
        }

        /* 分配内存读取参考帧数据 */
        uint8_t * base_data = lv_malloc(base_frame->size);
        if(!base_data) {
            LV_LOG_ERROR("Failed to allocate memory for base frame data");
            lv_free(frame_data);
            if(curr_palette) lv_free(curr_palette);
            return -1;
        }

        /* 读取参考帧数据 */
        f_gif_read(gif, base_data, base_frame->size);

        /* 解码参考帧数据 */
        if((base_frame->flag & GIF_FLAG_COMPRESS_MASK) == GIF_FLAG_COMPRESS_RLE) {
            /* RLE解压缩 */
            memset(gif->base_frame, 0, gif->header.width * gif->header.height);
            Rle_Decode(base_data, base_frame->size, gif->base_frame,
                       gif->header.width * gif->header.height, 1);
        }
        else {
            LV_LOG_ERROR("Unsupported compression type for base frame");
            lv_free(base_data);
            lv_free(frame_data);
            if(curr_palette) lv_free(curr_palette);
            return -1;
        }

        lv_free(base_data);

        f_gif_seek(gif, curr_pos, LV_FS_SEEK_SET);
        gif->base_index = frame->base_index;
    }

    /* 解码当前帧数据 */
    if((frame->flag & GIF_FLAG_COMPRESS_MASK) == GIF_FLAG_COMPRESS_RLE) {
        /* RLE解压缩 */
        memset(gif->frame, 0, gif->header.width * gif->header.height);
        Rle_Decode(frame_data, frame->size, gif->frame,
                   gif->header.width * gif->header.height, 1);
    }
    else {
        LV_LOG_ERROR("Unsupported compression type");
        lv_free(frame_data);
        if(curr_palette) lv_free(curr_palette);
        return -1;
    }

    lv_free(frame_data);

    /* 更新当前颜色表 */
    if(curr_palette) {
        memcpy(gif->gct.colors, curr_palette, palette_size);
        lv_free(curr_palette);
        gif->palette = &gif->gct;
    }

    /* 保存透明色信息 */
    gif->has_trans = (frame->flag & GIF_FLAG_HAS_TRANS_COLOR) ? 1 : 0;
    gif->trans_index = (gif->has_trans) ? ((frame->flag & GIF_FLAG_TRANS_COLOR_MASK) >> 8) : 0;

    /* 将解码后的帧数据复制到索引数据区域 */
    if(frame->flag & GIF_FLAG_IS_D_FRAME) {
        memcpy(gif->index_data, gif->base_frame, gif->header.width * gif->header.height);
        uint32_t i, j, src_idx, dst_idx;
        for(j = 0; j < frame->height; j++) {
            src_idx = j * frame->width;
            dst_idx = (j + frame->y) * gif->header.width + frame->x;
            memcpy(gif->index_data + dst_idx, gif->frame + src_idx, frame->width);
        }
    }
    else {
        memcpy(gif->index_data, gif->frame, frame->width * frame->height);
    }

    return 0;
}

static void render_frame_rect(jl_gif_t * gif, uint8_t * buffer)
{
    if(buffer != gif->canvas) {
        memcpy(buffer, gif->canvas, gif->header.width * gif->header.height + JLGIF_COLOR_TABLE_LENGTH);
    }
}

#endif /* LV_USE_JLGIF */
