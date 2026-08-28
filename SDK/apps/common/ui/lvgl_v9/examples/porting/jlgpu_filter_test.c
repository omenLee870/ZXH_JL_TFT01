/**
 * @file    jlgpu_filter_test.c
 * @brief   GPU 图像滤镜功能测试 (使用 Session API)
 *
 * 演示 jlgpu_image_filters.h 的 Session API 用法:
 *  init(一次) → process(每帧) → uninit(一次)
 */

#include "asm/dcache.h"
#include "jlgpu_framework.h"
#include "jlgpu_utils.h"
#include "jlgpu_image_filters.h"
#include "malloc.h"

/* ========================================================================
 *  SD 卡文件路径 (按需修改)
 * ======================================================================== */

#define TEST_WIDTH      320
#define TEST_HEIGHT     240
#define TEST_IN_FORMAT  JLGPU_FORMAT_YUV420_BT601
#define TEST_OUT_FORMAT JLGPU_FORMAT_YUV420_BT601

void my_line_done_cb(jlgpu_filter_session_t *sess, uint8_t *dst,
                     int w, int h, int line_start, int line_count, void *user_data)
{
    printf("%d x %d line_start=%d line_conunt=%d\n", w, h, line_start, line_count);
}
/**
 * @brief 滤镜测试: SD卡加载YUV → GPU滤镜处理 → 写回SD卡
 *
 * 使用 Session API 演示完整的 init → process → uninit 流程。
 */
void test_color_filter(void)
{
#define FILTER_SRC  "storage/sd0/C/nv12.yuv"
#define FILTER_DST  "storage/sd0/C/output_negative.bin"

    jlgpu_filter_session_t *sess = NULL;
    int             in_size    = TEST_WIDTH * TEST_HEIGHT * 3 / 2;
    int             out_size   = TEST_HEIGHT * TEST_WIDTH * 2;
    uint8_t        *src        = NULL;
    uint8_t        *dst        = NULL;
    FILE           *fp         = NULL;

    /* 分配缓冲区 */
    src = (uint8_t *)malloc_psram(in_size);
    dst = (uint8_t *)malloc_psram(out_size);
    if (!src || !dst) {
        goto cleanup;
    }

    /* 从 SD 卡读取 YUV 源图 */
    fp = fopen(FILTER_SRC, "rb");
    if (!fp) {
        printf("fopen %s failed\n", FILTER_SRC);
        goto cleanup;
    }
    fread(src, 1, in_size, fp);
    fclose(fp);
    fp = NULL;

    /* 初始化 session */
    sess = jlgpu_filter_session_init(TEST_WIDTH, TEST_HEIGHT,
                                     TEST_IN_FORMAT, TEST_OUT_FORMAT);
    if (!sess) {
        printf("session init failed\n");
        goto cleanup;
    }

    /* 执行颜色滤镜  */
    // jlgpu_filter_session_process(sess, src, src, JLGPU_FILTER_YELLOW, NULL);

    /* 执行贴纸合成  */
    jlgpu_sticker_t sticker = {
        .x = 10,
        .y = 10,
        .path = "storage/res_nor_mode/C/lvgl/4b0001af.zip",
        // .path = "storage/res_nor_mode/C/lvgl/2.zip",
        .dst_w = 0,
        .dst_h = 0,
        .angle = 0.0f,
    };
    jlgpu_filter_session_process(sess, src, src, JLGPU_FILTER_STICKER, (void *)&sticker);
    // jlgpu_filter_session_set_line_config(sess, 16, my_line_done_cb, NULL);
    // jlgpu_filter_session_process(sess, src, dst, JLGPU_FILTER_TILE_HORIZ, NULL);

    /* 释放 session  */
    jlgpu_filter_session_uninit(sess);
    sess = NULL;

    /* 写回 SD 卡 */
    fp = fopen(FILTER_DST, "wb");
    if (!fp) {
        goto cleanup;
    }
    DcuFlushRegion((void *)src, in_size);
    DcuFlushRegion((void *)dst, out_size);
    fwrite(dst, 1, out_size, fp);
    fclose(fp);
    fp = NULL;

    printf("filter ok: %s -> %s\n", FILTER_SRC, FILTER_DST);

cleanup:
    if (sess) {
        jlgpu_filter_session_uninit(sess);
    }
    if (fp) {
        fclose(fp);
    }
    free_psram(src);
    free_psram(dst);
}

static void test_color_filter_task(void)
{
    printf("test_color_filter_task\n");
    os_time_dly(500);
    test_color_filter();
}
static int test_color_filter_init(void)
{
    thread_fork("test_color_filter", 10, 4096, 0, 0,
                (void (*)(void *))test_color_filter_task, NULL);
    return 0;
}
// late_initcall(test_color_filter_init);
