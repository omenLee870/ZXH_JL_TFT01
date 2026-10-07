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

#define CONFIG_FONT_JPEG_PATH "mnt/sdfile/EXT_RESERVED/uipackres/ui/poweron.jpg"

#include "system/includes.h"
#include "pipeline_core.h"
#include "asm/jpeg_codec.h"
#include "app_config.h"
#include "server/video_server.h"//app_struct
#include "server/video_dec_server.h"//dec_struct

#ifdef CONFIG_VIDEO_ENABLE

struct _jpeg_disp_f_t {
    struct server *dec_server;
};
struct _jpeg_disp_f_t map_jpeg_disp_fh = {0};
#define __this   (&map_jpeg_disp_fh)
#define JPG_MAX_SIZE 100*1024

int map_jpeg_disp_one_frame(u16 width, u16 height, u8 *buf, int len)
{
    union video_dec_req dec_req = {0};
    dec_req.dec.fb = "fb1";
    dec_req.dec.left = 0;
    dec_req.dec.top = 0;
    dec_req.dec.width = width;//屏幕的宽高
    dec_req.dec.height = height;
    dec_req.dec.thm_first = 0;
    dec_req.dec.pctl = NULL;
    dec_req.dec.preview = 0;
    dec_req.dec.image.buf = buf;
    dec_req.dec.image.size = len;
    return server_request(__this->dec_server, VIDEO_REQ_DEC_IMAGE, &dec_req);
}

int map_jpeg_disp_frame_init(void)
{
    memset(__this, 0x00, sizeof(struct _jpeg_disp_f_t));

    if (!__this->dec_server) {
        struct video_dec_arg arg = {0};
        arg.dev_name = "video_dec";
        arg.video_buf_size = JPG_MAX_SIZE;
        __this->dec_server = server_open("video_dec_server", &arg);
        if (!__this->dec_server) {
            printf("open video_dec_server fail");
            return -EFAULT;
        }
    }

    return 0;
}

int map_jpeg_disp_frame_uninit(void)
{
    union video_dec_req dec_req = {0};
    server_request(__this->dec_server, VIDEO_REQ_DEC_STOP, &dec_req);
    server_close(__this->dec_server);
    __this->dec_server = NULL;
    return 0;
}

#endif

/* 简易导航信息控件 */
lv_obj_t *page_map_view_3;
lv_obj_t *page_map_lbl_1;
lv_obj_t *page_map_lbl_2;
lv_obj_t *page_map_lbl_3;
lv_obj_t *page_map_lbl_4;
lv_obj_t *page_map_img_1;

lv_obj_t *get_page_map_view_3_in_map(void)
{
    return page_map_view_3;
}

void lv_page_map_view_3_create(void)
{
    //Write codes page_map_view_3

    page_map_view_3 = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(page_map_view_3, 27, 125);
    lv_obj_set_size(page_map_view_3, 207, 117);
    lv_obj_set_scrollbar_mode(page_map_view_3, LV_SCROLLBAR_MODE_OFF);
    ui_style_set(page_map_view_3, GUI_CTRL_CONT);

    //Set style for page_map_view_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
    lv_obj_set_style_radius(page_map_view_3, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(page_map_view_3, lv_color_make(0x08, 0x17, 0x27), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(page_map_view_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Delete extra default style for page_map_view_3
    lv_obj_remove_style(page_map_view_3, &gui_cont_main_focus_key_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    //Write codes page_map_lbl_1
    page_map_lbl_1 = lv_label_create(page_map_view_3);
    lv_label_set_text(page_map_lbl_1, "328");
    lv_label_set_long_mode(page_map_lbl_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(page_map_lbl_1, 81, 17);
    lv_obj_set_size(page_map_lbl_1, 72, 38);
    lv_obj_set_scrollbar_mode(page_map_lbl_1, LV_SCROLLBAR_MODE_OFF);
    ui_style_set(page_map_lbl_1, GUI_CTRL_LABEL);

    //Set style for page_map_lbl_1. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
    lv_obj_set_style_bg_color(page_map_lbl_1, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(page_map_lbl_1, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(page_map_lbl_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(page_map_lbl_1, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Write codes page_map_lbl_2
    page_map_lbl_2 = lv_label_create(page_map_view_3);
    lv_label_set_text(page_map_lbl_2, "22");
    lv_label_set_long_mode(page_map_lbl_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(page_map_lbl_2, 4, 65);
    lv_obj_set_size(page_map_lbl_2, 72, 38);
    lv_obj_set_scrollbar_mode(page_map_lbl_2, LV_SCROLLBAR_MODE_OFF);
    ui_style_set(page_map_lbl_2, GUI_CTRL_LABEL);

    //Set style for page_map_lbl_2. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
    lv_obj_set_style_bg_color(page_map_lbl_2, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(page_map_lbl_2, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(page_map_lbl_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(page_map_lbl_2, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Write codes page_map_lbl_3
    page_map_lbl_3 = lv_label_create(page_map_view_3);
    lv_label_set_text(page_map_lbl_3, "45");
    lv_label_set_long_mode(page_map_lbl_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(page_map_lbl_3, 59, 65);
    lv_obj_set_size(page_map_lbl_3, 72, 38);
    lv_obj_set_scrollbar_mode(page_map_lbl_3, LV_SCROLLBAR_MODE_OFF);
    ui_style_set(page_map_lbl_3, GUI_CTRL_LABEL);

    //Set style for page_map_lbl_3. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
    lv_obj_set_style_bg_color(page_map_lbl_3, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(page_map_lbl_3, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(page_map_lbl_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(page_map_lbl_3, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Write codes page_map_lbl_4
    page_map_lbl_4 = lv_label_create(page_map_view_3);
    lv_label_set_text(page_map_lbl_4, "12:20");
    lv_label_set_long_mode(page_map_lbl_4, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(page_map_lbl_4, 124, 65);
    lv_obj_set_size(page_map_lbl_4, 72, 38);
    lv_obj_set_scrollbar_mode(page_map_lbl_4, LV_SCROLLBAR_MODE_OFF);
    ui_style_set(page_map_lbl_4, GUI_CTRL_LABEL);

    //Set style for page_map_lbl_4. Part: LV_PART_MAIN, State: LV_STATE_DEFAULT
    lv_obj_set_style_bg_color(page_map_lbl_4, lv_color_make(0x21, 0x95, 0xF6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(page_map_lbl_4, &lv_font_montserratMedium_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(page_map_lbl_4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(page_map_lbl_4, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    //Write codes page_map_img_1
    page_map_img_1 = lv_img_create(page_map_view_3);
    lv_img_set_src(page_map_img_1, gui_get_res_path(GUI_RES_LAMP_01_PNG));
    lv_img_set_pivot(page_map_img_1, 0, 0);
    lv_img_set_angle(page_map_img_1, 0);
    lv_img_set_zoom(page_map_img_1, 256);
    lv_obj_set_pos(page_map_img_1, 18, 15);
    lv_obj_set_size(page_map_img_1, 40, 40);
    lv_obj_add_flag(page_map_img_1, LV_OBJ_FLAG_CLICKABLE);
    ui_style_set(page_map_img_1, GUI_CTRL_IMG);
    lv_obj_add_flag(page_map_view_3, LV_OBJ_FLAG_HIDDEN);

}

// 简易导航信息
extern lv_ui guider_ui;// Screen page_map
void update_text_lbl_1(const char *str)
{
    /* lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP); */
    lvgl_rpc_post_func(lv_label_set_text, 2, page_map_lbl_1, str);
}
void update_text_lbl_2(const char *str)
{
    /* lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP); */
    lvgl_rpc_post_func(lv_label_set_text, 2, page_map_lbl_2, str);
}
void update_text_lbl_3(const char *str)
{
    /* lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP); */
    lvgl_rpc_post_func(lv_label_set_text, 2, page_map_lbl_3, str);
}
void update_text_lbl_4(const char *str)
{
    /* lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP); */
    lvgl_rpc_post_func(lv_label_set_text, 2, page_map_lbl_4, str);
}

//resolution
void update_text_lbl_10(const char *str)
{
    lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
    lvgl_rpc_post_func(lv_label_set_text, 2, ui_scr->page_map_lbl_10, str);
}
//fps
void update_text_lbl_7(const char *str)
{
    lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
    lvgl_rpc_post_func(lv_label_set_text, 2, ui_scr->page_map_lbl_7, str);
}
//bitrate
void update_text_lbl_9(const char *str)
{
    lv_ui_page_map *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_MAP);
    lvgl_rpc_post_func(lv_label_set_text, 2, ui_scr->page_map_lbl_9, str);
}

void update_ui_remain_mileage_time_data(const char *mileage_buf, const char *time_buf)
{
    update_text_lbl_2(mileage_buf);
//    update_text_lbl_3(time_buf);
}


void update_ui_fps_bitrate(const char *fps_buf, const char *bitrate_buf)
{
    update_text_lbl_7(fps_buf);
    update_text_lbl_9(bitrate_buf);
}

#endif
#endif