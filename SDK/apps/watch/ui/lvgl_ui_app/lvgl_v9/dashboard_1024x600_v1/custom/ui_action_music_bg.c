/**
 * @file ui_action_music_bg.c
 * @brief 蓝牙 BIP 专辑图片接收与显示（无文件系统依赖）
 * @note  JPEG 数据全程在内存中操作
 */
#if !LV_USE_GUIBUILDER_SIMULATOR
#include <string.h>
#include <stdlib.h>
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "mpi_common.h"
#include "mpi_vdec.h"
#include "custom.h"
#include "lvgl.h"
#include "ui.h"
#include "fs/fs.h"

// ======================= 配置宏 =======================
#define MAX_DISP_W              208                 // 显示最大宽度
#define MAX_DISP_H              208                 // 显示最大高度

// ======================= 枚举定义 =======================
enum {
    BIP_DATA_STATUS_START = 0X01,   // 开始包
    BIP_DATA_STATUS_CONTINUE,       // 中间包
    BIP_DATA_STATUS_STOP,           // 结束包
    BIP_DATA_STATUS_ERR,            // 错误包
    BIP_DATA_STATUS_ERR_GET,        // 重复获取
    BIP_DATA_STATUS_GET_NULL,       // 获取无图片歌曲
};

enum {
    BIP_FILE_STATUS_ERR,            // 文件不存在/错误
    BIP_FILE_STATUS_OK,             // 数据有效
    BIP_FILE_STATUS_UPDATE,         // 更新中
};

// ======================= 内存管理结构体 =======================
struct bip_file_info {
    OS_MUTEX mutex;                 // 互斥锁
    u8 *buffer;                     // JPEG 数据缓冲区
    u32 buffer_size;                // 已分配缓冲区大小（字节）
    u32 buffer_offset;              // 实际数据长度
    u8 bt_addr[6];                  // 蓝牙地址（用于请求图片更新）
    u16 timer;                      // 延迟请求定时器
    u16 tid;					    //延时更新专辑图片
    volatile s8 file_status;        // 数据状态
    void (*callback)(void);         // 完成回调
};

static struct bip_file_info bip_file;
#define __bip_info (&bip_file)

static uint32_t music_angle_idx = 0;
static bool music_rotation_enabled = true;

// ======================= 互斥锁操作 =======================
static int bip_file_mutex_init(void)
{
    return os_mutex_create(&__bip_info->mutex);
}

static int bip_file_mutex_lock(void)
{
    return os_mutex_pend(&__bip_info->mutex, 0);
}

static int bip_file_mutex_unlock(void)
{
    return os_mutex_post(&__bip_info->mutex);
}

// ======================= 内部数据更新函数 =======================
static void bip_file_data_update(u8 *data, u32 length)
{
    if (!__bip_info->buffer) {
        return;
    }
    if (__bip_info->buffer_offset + length > __bip_info->buffer_size) {
        u32 new_size = __bip_info->buffer_size * 2;
        u8 *new_buf = (u8 *)realloc(__bip_info->buffer, new_size);
        if (!new_buf) {
            printf("realloc failed, drop data\n");
            return;
        }
        __bip_info->buffer = new_buf;
        __bip_info->buffer_size = new_size;
    }
    memcpy(__bip_info->buffer + __bip_info->buffer_offset, data, length);
    __bip_info->buffer_offset += length;
}

// 对外接口：获取数据指针和大小
u8 *bip_file_data_get(void)
{
    return __bip_info->buffer;
}

u32 bip_file_data_size(void)
{
    return __bip_info->buffer_offset;
}

// ======================= BIP 回调处理（内存版本） =======================
void bip_rx_data_handle(u8 *packet, u16 body_len, u32 length, u8 bip_data_status)
{
    printf("[BIP] status=%d, len=%d\n", bip_data_status, body_len);

    switch (bip_data_status) {
    case BIP_DATA_STATUS_START:
        printf("BIP_START\n");
        bip_file_mutex_lock();

        // 释放旧数据
        if (__bip_info->buffer) {
            free(__bip_info->buffer);
            __bip_info->buffer = NULL;
        }
        // 初始分配 50KB，若 length 有效则按 length 分配
        __bip_info->buffer_size = (length > 0 && length < 2 * 1024 * 1024) ? length : 50 * 1024;
        __bip_info->buffer = (u8 *)malloc(__bip_info->buffer_size);
        if (__bip_info->buffer) {
            __bip_info->file_status = BIP_FILE_STATUS_UPDATE;
            __bip_info->buffer_offset = 0;
            memcpy(__bip_info->buffer, packet, body_len);
            __bip_info->buffer_offset = body_len;
        } else {
            __bip_info->file_status = BIP_FILE_STATUS_ERR;
            printf("malloc failed\n");
        }
        bip_file_mutex_unlock();
        break;

    case BIP_DATA_STATUS_CONTINUE:
        if (__bip_info->file_status == BIP_FILE_STATUS_UPDATE && __bip_info->buffer) {
            bip_file_mutex_lock();
            bip_file_data_update(packet, body_len);
            bip_file_mutex_unlock();
        }
        break;

    case BIP_DATA_STATUS_STOP:
        printf("BIP_STOP\n");
        bip_file_mutex_lock();
        if (__bip_info->buffer) {
            if (body_len > 0) {
                bip_file_data_update(packet, body_len);
            }
            __bip_info->file_status = BIP_FILE_STATUS_OK;
            printf("Image received, total size = %u bytes\n", __bip_info->buffer_offset);
        } else {
            __bip_info->file_status = BIP_FILE_STATUS_ERR;
        }
        bip_file_mutex_unlock();

        // 通知 UI 线程更新图片
        bt_music_post_msg_to_lvgl_ui("music_album_pic");
        if (__bip_info->callback) {
            __bip_info->callback();
        }
        break;

    case BIP_DATA_STATUS_ERR:
        printf("BIP_ERR\n");
        bip_file_mutex_lock();
        if (__bip_info->buffer) {
            free(__bip_info->buffer);
            __bip_info->buffer = NULL;
        }
        __bip_info->file_status = BIP_FILE_STATUS_ERR;
        bip_file_mutex_unlock();
        break;

    default:
        break;
    }
}

// ======================= 状态和回调设置 =======================
u8 bip_file_status_get(void)
{
    return __bip_info->file_status;
}

void bip_file_set_callback(void (*callback)(void))
{
    __bip_info->callback = callback;
}

// ======================= 图像解码与显示 =======================
static rgb_color_t fast_avg_color_from_yuv420_nv12(const uint8_t *y, const uint8_t *uv,
        uint32_t width, uint32_t height,
        uint32_t stride_y, uint32_t stride_uv,
        uint32_t sample_step)
{
    if (sample_step < 1) {
        sample_step = 8;
    }
    uint32_t r_sum = 0, g_sum = 0, b_sum = 0;
    uint32_t sample_cnt = 0;

    for (uint32_t y_idx = 0; y_idx < height; y_idx += sample_step) {
        const uint8_t *y_row = y + y_idx * stride_y;
        for (uint32_t x_idx = 0; x_idx < width; x_idx += sample_step) {
            uint8_t Y = y_row[x_idx];
            uint32_t uv_row = (y_idx / 2) * stride_uv;
            uint32_t uv_col = (x_idx / 2) * 2;
            uint8_t U = uv[uv_row + uv_col];
            uint8_t V = uv[uv_row + uv_col + 1];

            int16_t C = Y - 16;
            int16_t D = U - 128;
            int16_t E = V - 128;

            int16_t R = (298 * C + 409 * E + 128) >> 8;
            int16_t G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int16_t B = (298 * C + 516 * D + 128) >> 8;

            R = (R < 0) ? 0 : (R > 255 ? 255 : R);
            G = (G < 0) ? 0 : (G > 255 ? 255 : G);
            B = (B < 0) ? 0 : (B > 255 ? 255 : B);

            r_sum += R;
            g_sum += G;
            b_sum += B;
            sample_cnt++;
        }
    }

    rgb_color_t avg;
    if (sample_cnt > 0) {
        avg.r = r_sum / sample_cnt;
        avg.g = g_sum / sample_cnt;
        avg.b = b_sum / sample_cnt;
    } else {
        avg.r = avg.g = avg.b = 0x80;
    }
    return avg;
}

static void yuv420_nv12_to_rgb565_scaled(const uint8_t *y, const uint8_t *uv,
        uint32_t src_w, uint32_t src_h,
        uint32_t stride_y, uint32_t stride_uv,
        uint32_t out_w, uint32_t out_h,
        uint16_t *rgb)
{
    for (uint32_t out_y = 0; out_y < out_h; out_y++) {
        uint32_t src_y = (out_y * src_h) / out_h;
        const uint8_t *y_row = y + src_y * stride_y;
        for (uint32_t out_x = 0; out_x < out_w; out_x++) {
            uint32_t src_x = (out_x * src_w) / out_w;
            uint8_t Y = y_row[src_x];

            uint32_t uv_row = (src_y / 2) * stride_uv;
            uint32_t uv_col = (src_x / 2) * 2;
            uint8_t U = uv[uv_row + uv_col];
            uint8_t V = uv[uv_row + uv_col + 1];

            int16_t C = Y - 16;
            int16_t D = U - 128;
            int16_t E = V - 128;

            int16_t R = (298 * C + 409 * E + 128) >> 8;
            int16_t G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int16_t B = (298 * C + 516 * D + 128) >> 8;

            R = (R < 0) ? 0 : (R > 255 ? 255 : R);
            G = (G < 0) ? 0 : (G > 255 ? 255 : G);
            B = (B < 0) ? 0 : (B > 255 ? 255 : B);

            rgb[out_y * out_w + out_x] = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
        }
    }
}

static void set_dynamic_background_color(lv_obj_t *bg_obj, const rgb_color_t *avg_color)
{
    if (!bg_obj || !lv_obj_is_valid(bg_obj)) {
        return;
    }

    lv_color_t bg_color = lv_color_make(avg_color->r, avg_color->g, avg_color->b);
    lv_obj_set_style_bg_color(bg_obj, bg_color, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bg_obj, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bg_obj, LV_GRAD_DIR_NONE, LV_STATE_DEFAULT);
}

/**
 * @brief 从内存缓冲区解码并显示专辑图片
 * @note  由 UI 消息 "music_album_pic" 触发调用
 */
void ui_music_album_pic_handler(void)
{
    // 获取当前活动屏幕ID
    lv_obj_t *bg_obj = NULL;
    lv_obj_t *img_obj = NULL;
    int32_t current_screen_id = gui_scr_get_id(gui_scr_get_act()); // 假设有该函数

    if (current_screen_id == GUI_SCREEN_PAGE_MUSIC) {
        lv_ui_page_music *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
        if (!ui_scr) {
            return;
        }
        bg_obj = ui_scr->page_music;
        img_obj = ui_scr->page_music_img_3;
        music_rotation_enabled = false;
        page_music_reset_rotation();
    } else if (current_screen_id == GUI_SCREEN_PAGE_APP_SELECT) {
        lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
        if (!ui_scr) {
            return;
        }
        bg_obj = NULL;
        img_obj = ui_scr->page_app_select_img_3;
    } else {
        // 不是目标页面，不解码不显示，直接返回
        return;
    }

    // 确保图像控件有效
    if (!img_obj || !lv_obj_is_valid(img_obj)) {
        return;
    }

    mpi_ret_t ret;
    mpi_vdec_chn_t chn = 0;
    mpi_vdec_chn_attr_t attr = {
        .hw_dev = 0,
        .type = MPI_VDEC_TYPE_JPEG,
        .output_mode = MPI_VDEC_OUTPUT_FRAME,
        .frame_buf_size = 1 * 1024 * 1024,
    };
    mpi_vdec_pic_info_t pic_info = {0};
    mpi_stream_t stream = {0};
    mpi_video_frame_info_t frame = {0};
    uint8_t *jpeg_buf = NULL;
    uint32_t jpeg_len = 0;

    // 从全局内存中获取JPEG数据
    bip_file_mutex_lock();
    if (__bip_info->buffer == NULL || __bip_info->file_status != BIP_FILE_STATUS_OK) {
        bip_file_mutex_unlock();
        printf("No valid BIP image in memory\n");
        return;
    }
    jpeg_len = __bip_info->buffer_offset;
    jpeg_buf = (uint8_t *)malloc(jpeg_len);
    if (!jpeg_buf) {
        bip_file_mutex_unlock();
        printf("malloc jpeg buffer failed\n");
        return;
    }
    memcpy(jpeg_buf, __bip_info->buffer, jpeg_len);
    bip_file_mutex_unlock();

    stream.addr = jpeg_buf;
    stream.len = jpeg_len;

    ret = mpi_vdec_create_chn(chn, &attr);
    if (ret != MPI_SUCCESS) {
        printf("vdec create failed: %d\n", ret);
        goto error;
    }

    ret = mpi_vdec_get_pic_info(chn, &stream, &pic_info);
    if (ret != MPI_SUCCESS) {
        printf("get pic info failed: %d\n", ret);
        goto error;
    }

    ret = mpi_vdec_start_chn(chn);
    if (ret != MPI_SUCCESS) {
        printf("vdec start failed: %d\n", ret);
        goto error;
    }

    ret = mpi_vdec_send_stream(chn, &stream);
    if (ret != MPI_SUCCESS) {
        printf("send stream failed: %d\n", ret);
        goto error;
    }

    memset(&frame, 0, sizeof(frame));
    ret = mpi_vdec_get_frame(chn, &frame, 5000);
    if (ret != MPI_SUCCESS || frame.y_addr == NULL) {
        printf("get frame failed: %d\n", ret);
        goto error;
    }

    // 计算平均颜色并设置背景
    rgb_color_t avg = fast_avg_color_from_yuv420_nv12(
                          frame.y_addr, frame.c_addr,
                          pic_info.width, pic_info.height,
                          pic_info.align_width, pic_info.align_width, 8);
    set_dynamic_background_color(bg_obj, &avg);   // bg_obj 可能为 NULL，函数内部会判断
    // 缩放并转换 RGB565
    uint32_t disp_w = pic_info.width;
    uint32_t disp_h = pic_info.height;
    if (disp_w > MAX_DISP_W) {
        disp_w = MAX_DISP_W;
    }
    if (disp_h > MAX_DISP_H) {
        disp_h = MAX_DISP_H;
    }
    uint32_t rgb_size = disp_w * disp_h * 2;

    static void *s_rgb_buf = NULL;
    static uint32_t s_rgb_size = 0;
    if (s_rgb_buf == NULL || s_rgb_size < rgb_size) {
        if (s_rgb_buf) {
#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
            free_psram(s_rgb_buf);
#else
            free(s_rgb_buf);
#endif
        }

#if defined TCFG_PSRAM_DEV_ENABLE && TCFG_PSRAM_DEV_ENABLE
        s_rgb_buf = malloc_psram(rgb_size);
#else
        s_rgb_buf = malloc(rgb_size);
#endif
        if (!s_rgb_buf) {
            printf("rgb buffer alloc failed\n");
            goto error;
        }
        s_rgb_size = rgb_size;
    }

    yuv420_nv12_to_rgb565_scaled(frame.y_addr, frame.c_addr,
                                 pic_info.width, pic_info.height,
                                 pic_info.align_width, pic_info.align_width,
                                 disp_w, disp_h,
                                 (uint16_t *)s_rgb_buf);

    // 更新已有图像控件的图片源
    static lv_image_dsc_t img_dsc;
    img_dsc.header.cf = LV_COLOR_FORMAT_NATIVE;
    img_dsc.header.w = disp_w;
    img_dsc.header.h = disp_h;
    img_dsc.data_size = rgb_size;
    img_dsc.data = s_rgb_buf;

    lv_image_set_src(img_obj, &img_dsc);
    music_rotation_enabled = true;
    lv_obj_invalidate(img_obj);
    printf("Updated image on screen %d, size %ux%u\n", current_screen_id, disp_w, disp_h);

    mpi_vdec_release_frame(chn, &frame);
    mpi_vdec_stop_chn(chn);
    mpi_vdec_destroy_chn(chn);
    free(jpeg_buf);
    return;  // 正常退出

error:
    if (frame.y_addr) {
        mpi_vdec_release_frame(chn, &frame);
    }
    mpi_vdec_stop_chn(chn);
    mpi_vdec_destroy_chn(chn);
    if (jpeg_buf) {
        free(jpeg_buf);
    }
    return;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief bip_update_pic_file 获取文件
 */
/* ------------------------------------------------------------------------------------*/
void bip_update_pic_file()
{
    bt_cmd_prepare(USER_CTRL_BIP_GET_IMAGE, 0, NULL);
}


// ======================= 初始化函数（放在系统启动时调用） =======================
void bip_mem_init(void)
{
    bip_file_mutex_init();
}

void page_music_reset_rotation(void)
{
    music_angle_idx = 0;
    lv_ui_page_music *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
    if (ui_scr) {
        lv_img_set_angle(ui_scr->page_music_img_1, 0);
        lv_img_set_angle(ui_scr->page_music_img_3, 0);
        lv_obj_invalidate(ui_scr->page_music_img_1);
        lv_obj_invalidate(ui_scr->page_music_img_3);
    }
}

void page_music_rotate_timer_cb(lv_timer_t *src)
{
    if (!music_rotation_enabled) {
        return;
    }

    lv_ui_page_music *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
    if (!ui_scr) {
        return;
    }

    // 动态设置旋转中心
    lv_coord_t w1 = lv_obj_get_width(ui_scr->page_music_img_1);
    lv_coord_t h1 = lv_obj_get_height(ui_scr->page_music_img_1);
    lv_img_set_pivot(ui_scr->page_music_img_1, w1 / 2, h1 / 2);
    lv_img_set_angle(ui_scr->page_music_img_1, music_angle_idx);

    lv_coord_t w3 = lv_obj_get_width(ui_scr->page_music_img_3);
    lv_coord_t h3 = lv_obj_get_height(ui_scr->page_music_img_3);
    lv_img_set_pivot(ui_scr->page_music_img_3, w3 / 2, h3 / 2);
    lv_img_set_angle(ui_scr->page_music_img_3, music_angle_idx);

    music_angle_idx += 10;
    if (music_angle_idx >= 3600) {
        music_angle_idx = 0;
    }
}
#endif