
#ifndef __CUSTOM_H_
#define __CUSTOM_H_
#ifdef __cplusplus
extern "C" {
#endif
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "./../generated/gui_guider.h"
#define CONFIG_FONT_TTF_NAME                "30100000.ttf"
#define CONFIG_FONT_TTF_PATH                CONFIG_ROOT_PATH\
                                            CONFIG_FONT_TTF_NAME
#define MAX_SPECTRUM_SECTION_NUM    32
#define USE_SPECTRUM_SECTION_NUM    16
#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

void custom_init(lv_ui *ui);
void bt_music_post_msg_to_lvgl_ui(const char *msg, ...);

void ui_music_pause_handler(void);
void ui_music_on_handler(void);
void ui_music_tol_time_handler(u32 tol_time);
void ui_music_cur_time_handler(u32  cur_time);
void ui_music_artist_handler(char *song_info);
void ui_music_album_pic_handler(void);
void ui_music_volume_handler(int volume);
void ui_music_process_handler(u32 cur_time);
void ui_music_lyrics_handler(char *song_lyrics);
void bip_mem_init(void);
void page_music_reset_rotation(void);
void page_music_rotate_timer_cb(lv_timer_t *src);
void lv_example_lyrics_text_input(void *dummy, ...);//char *new_text)

//×¨¼­jpegÕÕÆ¬½âÂëÆ÷
int ui_action_jpeg_decoder(u8 **buf, u32 *length, rgb_color_t *avg_color, u16 *rgb_width, u16 *rgb_length);
void ui_jljpg_update_handler(void *arg, ...);
void ui_jljpg_clear_handler(void *arg, ...);
//ÆµÆ×
void ui_music_draw_spectrum(void);
void ui_music_spectrum_handler(float db_data[2][MAX_SPECTRUM_SECTION_NUM]);
void ui_music_update_spectrum_with_magnitude(void);
void ui_music_spectrum_set_bg_avg_color(uint8_t r, uint8_t g, uint8_t b);
#endif
#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */