#include "custom.h"
#include "lvgl.h"
#include "gui_guider.h"

#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_core.h"
#include "app_config.h"
#include "action.h"
#include "ui.h"
#endif

#if !LV_USE_GUIBUILDER_SIMULATOR

#if 0

//专辑图片显示
//static lv_img_dsc_t image_dsc = {0};
lv_obj_t *img = NULL;
static lv_img_dsc_t image_dsc = {0};

lv_img_dsc_t *lv_get_page_music_image_dsc(void)
{
    return &image_dsc;
}

void lv_update_ui_music_img(lv_img_dsc_t *image_dsc);
void lv_album_img_show(const uint8_t *jpg_buf, uint32_t jpg_buf_len)
{
    if (!jpg_buf || jpg_buf_len == 0) {
        return;
    }

    image_dsc.header.always_zero = 0;
    image_dsc.header.w = 200;
    image_dsc.header.h = 200;
    image_dsc.data = jpg_buf;
    image_dsc.data_size = jpg_buf_len;
    image_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;

    lv_img_dsc_t *get_dsc =   lv_get_page_music_image_dsc();

    memcpy(get_dsc, &image_dsc, sizeof(lv_img_dsc_t));

    lv_update_ui_music_img(&image_dsc);
}

void update_ui_music_img(lv_img_dsc_t *image_dsc)
{
    if (ui_is_act_scr(GUI_SCREEN_PAGE_MUSIC)) {
        lv_ui_page_music *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MUSIC);
        lv_img_t *img = (lv_img_t *)ui_scr->page_music_img_3;
        lv_img_set_src(img, image_dsc);
    }
}


void lv_update_ui_music_img(lv_img_dsc_t *image_dsc)
{
    lvgl_rpc_post_func(update_ui_music_img, 1, image_dsc);
}
#endif
#endif