/**
 * @file lcd_lvgl_api.h
 *
 */

#ifndef LCD_LVGL_API_H
#define LCD_LVGL_API_H

#ifdef __cplusplus
extern "C" {
#endif

void lv_screen_manage_init(void);
int lv_screen_recover(void);
void lv_ui_auto_shut_down_re_run(void);
uint8_t lv_get_screen_saver_status();
void lv_set_lcd_keep_open_flag(uint8_t flag);
uint32_t lv_get_phy_addr(void);

#endif
