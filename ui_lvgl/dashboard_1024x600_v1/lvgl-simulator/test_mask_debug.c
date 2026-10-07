/**
 * 测试遮罩功能的调试版本
 */

#include "lvgl.h"

/**
 * 创建一个简单的测试遮罩（渐变效果）
 */
lv_image_dsc_t * create_test_mask(int32_t w, int32_t h)
{
    // 创建A8格式的遮罩数据
    uint32_t buf_size = w * h;
    uint8_t * mask_buf = lv_malloc(buf_size);
    if(!mask_buf) return NULL;

    // 创建一个从左到右的渐变遮罩
    for(int32_t y = 0; y < h; y++) {
        for(int32_t x = 0; x < w; x++) {
            // 从左到右，从完全透明到完全不透明
            uint8_t alpha = (x * 255) / (w - 1);
            mask_buf[y * w + x] = alpha;
        }
    }

    // 创建图像描述符
    lv_image_dsc_t * mask_dsc = lv_malloc(sizeof(lv_image_dsc_t));
    if(!mask_dsc) {
        lv_free(mask_buf);
        return NULL;
    }

    mask_dsc->header.cf = LV_COLOR_FORMAT_A8;
    mask_dsc->header.w = w;
    mask_dsc->header.h = h;
    mask_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    mask_dsc->header.reserved_2 = 0;
    mask_dsc->header.stride = w;
    mask_dsc->data = mask_buf;
    mask_dsc->data_size = buf_size;

    printf("Created test mask: %dx%d, first few values: %d %d %d %d\n",
           w, h, mask_buf[0], mask_buf[1], mask_buf[2], mask_buf[3]);

    return mask_dsc;
}

/**
 * 创建圆形遮罩
 */
lv_image_dsc_t * create_circle_mask(int32_t w, int32_t h)
{
    uint32_t buf_size = w * h;
    uint8_t * mask_buf = lv_malloc(buf_size);
    if(!mask_buf) return NULL;

    int32_t center_x = w / 2;
    int32_t center_y = h / 2;
    int32_t radius = LV_MIN(w, h) / 3;

    for(int32_t y = 0; y < h; y++) {
        for(int32_t x = 0; x < w; x++) {
            int32_t dx = x - center_x;
            int32_t dy = y - center_y;
            int32_t dist = lv_sqrt(dx * dx + dy * dy);

            if(dist <= radius) {
                mask_buf[y * w + x] = 255; // 完全不透明
            } else {
                mask_buf[y * w + x] = 0;   // 完全透明
            }
        }
    }

    lv_image_dsc_t * mask_dsc = lv_malloc(sizeof(lv_image_dsc_t));
    if(!mask_dsc) {
        lv_free(mask_buf);
        return NULL;
    }

    mask_dsc->header.cf = LV_COLOR_FORMAT_A8;
    mask_dsc->header.w = w;
    mask_dsc->header.h = h;
    mask_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    mask_dsc->header.reserved_2 = 0;
    mask_dsc->header.stride = w;
    mask_dsc->data = mask_buf;
    mask_dsc->data_size = buf_size;

    printf("Created circle mask: %dx%d, center at (%d,%d), radius %d\n",
           w, h, center_x, center_y, radius);

    return mask_dsc;
}

/**
 * 测试遮罩效果
 */
void test_mask_effects(void)
{
    lv_obj_t * scr = lv_screen_active();

    // 测试1：渐变遮罩
    lv_obj_t * img1 = lv_image_create(scr);
    lv_image_set_src(img1, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\qrcode.jpg");
    lv_obj_set_pos(img1, 10, 10);
    lv_obj_set_size(img1, 200, 200);

    lv_image_dsc_t * gradient_mask = create_test_mask(200, 200);
    if(gradient_mask) {
        lv_image_set_bitmap_map_src(img1, gradient_mask);
        printf("Applied gradient mask to image 1\n");
    }

    // 测试2：圆形遮罩
    lv_obj_t * img2 = lv_image_create(scr);
    lv_image_set_src(img2, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\qrcode.jpg");
    lv_obj_set_pos(img2, 220, 10);
    lv_obj_set_size(img2, 200, 200);

    lv_image_dsc_t * circle_mask = create_circle_mask(200, 200);
    if(circle_mask) {
        lv_image_set_bitmap_map_src(img2, circle_mask);
        printf("Applied circle mask to image 2\n");
    }

    // 测试3：全0遮罩（应该完全隐藏图像）
    lv_obj_t * img3 = lv_image_create(scr);
    lv_image_set_src(img3, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\qrcode.jpg");
    lv_obj_set_pos(img3, 430, 10);
    lv_obj_set_size(img3, 200, 200);

    // 创建全0遮罩
    uint32_t buf_size = 200 * 200;
    uint8_t * zero_mask_buf = lv_malloc(buf_size);
    if(zero_mask_buf) {
        lv_memzero(zero_mask_buf, buf_size); // 全部设为0

        lv_image_dsc_t * zero_mask = lv_malloc(sizeof(lv_image_dsc_t));
        if(zero_mask) {
            zero_mask->header.cf = LV_COLOR_FORMAT_A8;
            zero_mask->header.w = 200;
            zero_mask->header.h = 200;
            zero_mask->header.magic = LV_IMAGE_HEADER_MAGIC;
            zero_mask->header.reserved_2 = 0;
            zero_mask->header.stride = 200;
            zero_mask->data = zero_mask_buf;
            zero_mask->data_size = buf_size;

            lv_image_set_bitmap_map_src(img3, zero_mask);
            printf("Applied zero mask to image 3 (should be invisible)\n");
        }
    }

    // 测试4：全255遮罩（应该完全显示图像）
    lv_obj_t * img4 = lv_image_create(scr);
    lv_image_set_src(img4, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\qrcode.jpg");
    lv_obj_set_pos(img4, 10, 220);
    lv_obj_set_size(img4, 200, 200);

    uint8_t * full_mask_buf = lv_malloc(buf_size);
    if(full_mask_buf) {
        lv_memset(full_mask_buf, 255, buf_size); // 全部设为255

        lv_image_dsc_t * full_mask = lv_malloc(sizeof(lv_image_dsc_t));
        if(full_mask) {
            full_mask->header.cf = LV_COLOR_FORMAT_A8;
            full_mask->header.w = 200;
            full_mask->header.h = 200;
            full_mask->header.magic = LV_IMAGE_HEADER_MAGIC;
            full_mask->header.reserved_2 = 0;
            full_mask->header.stride = 200;
            full_mask->data = full_mask_buf;
            full_mask->data_size = buf_size;

            lv_image_set_bitmap_map_src(img4, full_mask);
            printf("Applied full mask to image 4 (should be fully visible)\n");
        }
    }

    printf("Test masks created. Expected results:\n");
    printf("- Image 1: Left-to-right gradient effect\n");
    printf("- Image 2: Circular visible area\n");
    printf("- Image 3: Should be invisible\n");
    printf("- Image 4: Should be fully visible\n");
}