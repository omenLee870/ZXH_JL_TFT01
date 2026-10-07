#if !LV_USE_GUIBUILDER_SIMULATOR
#include "lv_ui.h"
#include "custom.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "lv_image_dsc.h"
/* #include "stdio.h" */
#include <string.h>
#include "fs/fs.h"
#include "lv_draw_jlgpu.h"
#include "lcd_conf.h"

__attribute__((weak)) void screen_map_jpeg_buf_release(void *buf)
{
    (void)buf;
}
#define __ALIGN_16(a)   (((a) + 15) / 16 * 16)

static lv_image_dsc_t jljpeg_dsc = {
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_ARGB8888,
    .header.flags = 0,
    .header.w = __ALIGN_16(LCD_WIDTH),
    .header.h = __ALIGN_16(LCD_HEIGHT),
    .header.stride = 0,
    .header.res_compress = LV_COMPRESS_C3,
    .data_size = 0,
    .data = 0,
};

static jl_bin_user_data_t c3_user_data = {
    .c3_image_info.has_table = 1,
};

static lv_obj_t *map_jljpeg_img_obj = NULL;
//当前decoding数据
static void *map_jljpeg_cur_buf = NULL;

void ui_jljpg_update_handler(void *arg, ...)
{
    va_list ap;
    char *buf = (char *)arg;
    char *old_buf;
    int len;

    va_start(ap, arg);
    len = va_arg(ap, int);
    va_end(ap);

    if (!buf) {
        return;
    }

    if (len <= 0) {
        screen_map_jpeg_buf_release(buf);
        return;
    }

    if (map_jljpeg_img_obj == NULL) {
        lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);

        if (!ui_scr || !ui_scr->page_map) {
            screen_map_jpeg_buf_release(buf);
            return;
        }

        map_jljpeg_img_obj = lv_img_create(ui_scr->page_map);
        if (!map_jljpeg_img_obj) {
            screen_map_jpeg_buf_release(buf);
            return;
        }

        lv_obj_move_background(map_jljpeg_img_obj);
    }

#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_jlgpu_mutex_pend();
#endif


    old_buf = map_jljpeg_cur_buf;

    if (old_buf && old_buf != buf) {
        screen_map_jpeg_buf_release(old_buf);   //拿到new buf之后立即释放old_buf给scr_task继续接收下一帧数据
    }


    jljpeg_dsc.data = (const uint8_t *)buf;
    jljpeg_dsc.data_size = len;

    c3_user_data.c3_image_info.type = 1;
    c3_user_data.c3_image_info.has_table = 1;
    c3_user_data.c3_image_info.data_size = jljpeg_dsc.data_size;

    jljpeg_dsc.reserved = &c3_user_data;

    lv_image_set_src(map_jljpeg_img_obj, &jljpeg_dsc);

    map_jljpeg_cur_buf = buf;

#if LV_USE_DRAW_JLGPU_LIST == 2
    lv_jlgpu_mutex_post();
#endif

}

void ui_jljpg_clear_handler(void *arg, ...)
{
    if (map_jljpeg_img_obj) {
        lv_obj_del(map_jljpeg_img_obj);
        map_jljpeg_img_obj = NULL;
    }

    jljpeg_dsc.data = NULL;
    jljpeg_dsc.data_size = 0;

    if (map_jljpeg_cur_buf) {
        screen_map_jpeg_buf_release(map_jljpeg_cur_buf);
        map_jljpeg_cur_buf = NULL;
    }
}


//注册页面加载卸载回调
static int gui_src_action_page_map(int action)
{
    printf("--->%s()----->%d, action: %d\n", __func__, __LINE__, action);

    switch (action) {
    case GUI_SCREEN_ACTION_LOAD:
        break;
    case GUI_SCREEN_ACTION_UNLOAD:
        break;
    }
    return 0;
}

REGISTER_UI_SCREEN_ACTION_HANDLER(GUI_SCREEN_PAGE_MAP)
.onchange = gui_src_action_page_map,
};
#endif
