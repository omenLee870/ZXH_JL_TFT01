/**
 * @file custom.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "key_driver.h"
#include "lvgl.h"
#include "lv_ui.h"
#include "custom.h"
#include "hardware_test.h"
#include "ui_update_page.h"
#include "youyibao_sif_ui.h"
#include "chongwuyibiao_yinyue.h"
#include <time.h>
#include "playback/app_playback_mpi.h"

#define SWIPE_DISTANCE 80 // 移动80像素
// 容器状态
typedef enum {
    PANEL_A_VISIBLE,
    PANEL_B_VISIBLE
} panel_state_t;

static lv_obj_t *scr;             // 当前屏幕
static lv_obj_t *cont_a;          // 容器 A
static lv_obj_t *cont_b;          // 容器 B
static lv_coord_t cont_a_base_y;   // 容器A的基准Y坐标
static lv_coord_t cont_b_base_y;   // 容器B的基准Y坐标
static panel_state_t current_panel = PANEL_A_VISIBLE;
static bool anim_running = false;  // 动画进行中标志
static lv_timer_t *roll_anim_timer = NULL; //滚动动画定时器

// 卡片滑动动画结束回调：将隐藏容器复位到其基准位置，并隐藏
static void hide_completed_cb(lv_anim_t *anim)
{
    lv_obj_t *obj = (lv_obj_t *)anim->var;
    // 根据对象确定其基准Y坐标
    lv_coord_t base_y = (obj == cont_a) ? cont_a_base_y : cont_b_base_y;
    lv_obj_set_y(obj, base_y);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    anim_running = false;
}

static void screen_switch_timer_cb(lv_timer_t *timer);
static void screen_switch_func(void);

static int hardware_test_is_adkey_value(u8 key_value)
{
    return key_value >= KEY_AD_NUM0 && key_value <= KEY_AD_NUM19;
}

/* OK 只在宠物仪表及其临时倒车页接管；一次短按只切换一次。 */
bool lvgl_key_event_consumed(struct key_event *event)
{
    if (event->value != KEY_AD_NUM1) return false;
    if (ui_update_page_is_active() || hardware_test_adkey_is_active()) {
        printf("[PET_KEY] OK blocked by update/test page\n");
        return false;
    }
    bool consumed = chongwuyibiao_daoche_toggle(event->event == KEY_ACTION_CLICK);
    printf("[PET_KEY] OK event=%u consumed=%u\n", event->event, consumed);
    return consumed;
}

int ui_scr_key_event_handler_ext(struct key_event *event)
{

    printf("key event: %d, value: %d, type: %d --- \n", event->event, event->value, event->type);

    /* 升级页(双备份升级)活动期间：吞掉全部按键，禁止切回仪表盘屏(其字库/图片正在被擦写) */
    if (ui_update_page_is_active()) {
        return 0;
    }

    /* 硬件测试界面: 拦截所有UI层按键(不切屏、不外发app)，只能通过 UI Effect 按钮退出 */
    gui_scr_t *act_scr = gui_scr_get_act();
    if (act_scr != NULL && act_scr->id == GUI_SCREEN_HARDWARE_TEST) {
        /* ADKEY 测试项激活(或尚无测试项在跑)时, AD 按键回显到弹窗 */
        if (event->event == KEY_ACTION_CLICK && hardware_test_is_adkey_value(event->value)) {
            hw_test_item_t item = hardware_test_get_active_item();
            if (item == HW_TEST_ADKEY || item == HW_TEST_NONE) {
                hardware_test_ui_adkey_down(event->value - KEY_AD_NUM0 + 1);
            }
        }
        return 0;
    }

    if (hardware_test_adkey_is_active() && hardware_test_is_adkey_value(event->value)) {
        return 0;
    }

    if (event->event == KEY_ACTION_CLICK && (event->value == KEY_AD_NUM0 || event->value == KEY_IO_NUM0)) {
        screen_switch_func();
    }
    //返回1时，则将按键信息交给外部处理
    //默认是所有的非DOWN和UP事件都会发回给app
    return 0;
}

// ---------- arcmenu 滚动辅助函数 ----------
static void scroll_arcmenu_to_index(int index)
{
    lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (!ui_scr) {
        return;
    }
    if (!ui_scr->page_app_select_arcmenu_1) {
        return;
    }
    lv_obj_t *arcmenu = ui_scr->page_app_select_arcmenu_1;
    int child_cnt = lv_obj_get_child_count(arcmenu);
    if (index < 0 || index >= child_cnt) {
        return;
    }
    lv_obj_t *target = lv_obj_get_child(arcmenu, index);
    lv_obj_scroll_to_view(target, LV_ANIM_ON);
}

// 卡片垂直方向微动切换 + arcmenu 同步滚动
static void switch_panels_vertical(int dir)
{
    if (anim_running) {
        return;
    }
    anim_running = true;

    // 确定隐藏和显示的容器
    lv_obj_t *hide_obj = (current_panel == PANEL_A_VISIBLE) ? cont_a : cont_b;
    lv_obj_t *show_obj = (current_panel == PANEL_A_VISIBLE) ? cont_b : cont_a;

    // 根据方向决定 arcmenu 滚动目标索引
    if (dir == 1) {  // A -> B：滚动到第三个子控件（索引 2）
        scroll_arcmenu_to_index(2);
    } else if (dir == -1) {  // B -> A：滚动到第二个子控件（索引 1）
        scroll_arcmenu_to_index(1);
    }

    // 获取基准位置
    lv_coord_t hide_base_y = (hide_obj == cont_a) ? cont_a_base_y : cont_b_base_y;
    lv_coord_t show_base_y = (show_obj == cont_a) ? cont_a_base_y : cont_b_base_y;

    // 计算移动目标位置（移动 SWIPE_DISTANCE 像素）
    lv_coord_t hide_target_y = hide_base_y - dir * SWIPE_DISTANCE;
    lv_coord_t show_start_y  = show_base_y + dir * SWIPE_DISTANCE;
    lv_coord_t show_target_y = show_base_y;

    // 准备显示容器：设置起始位置并取消隐藏
    lv_obj_set_y(show_obj, show_start_y);
    lv_obj_clear_flag(show_obj, LV_OBJ_FLAG_HIDDEN);

    // 隐藏容器动画
    lv_anim_t a_hide;
    lv_anim_init(&a_hide);
    lv_anim_set_var(&a_hide, hide_obj);
    lv_anim_set_exec_cb(&a_hide, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a_hide, hide_base_y, hide_target_y);
    lv_anim_set_time(&a_hide, 80);
    lv_anim_set_path_cb(&a_hide, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a_hide, hide_completed_cb);
    lv_anim_start(&a_hide);

    // 显示容器动画
    lv_anim_t a_show;
    lv_anim_init(&a_show);
    lv_anim_set_var(&a_show, show_obj);
    lv_anim_set_exec_cb(&a_show, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a_show, show_start_y, show_target_y);
    lv_anim_set_time(&a_show, 150);
    lv_anim_set_path_cb(&a_show, lv_anim_path_ease_out);
    lv_anim_start(&a_show);

    // 更新当前面板状态
    current_panel = (current_panel == PANEL_A_VISIBLE) ? PANEL_B_VISIBLE : PANEL_A_VISIBLE;
}

// 菜单自动切换定时器回调
static void auto_switch_timer_cb(lv_timer_t *timer)
{
    lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (!ui_scr) {
        return;
    }
    if (current_panel == PANEL_A_VISIBLE) {
        switch_panels_vertical(1);   // A -> B
    } else {
        switch_panels_vertical(-1);  // B -> A
    }
}


// 卡片滑动初始化入口
void ui_auto_vertical_switch_init(void)
{
    lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (!ui_scr) {
        return;
    }
    scr = ui_scr->page_app_select;
    cont_a = ui_scr->page_app_select_view_select_app_music;
    cont_b = ui_scr->page_app_select_view_select_app_map;
    cont_a_base_y = lv_obj_get_y(cont_a);   // 记录基准
    cont_b_base_y = lv_obj_get_y(cont_b);   // 记录基准

    roll_anim_timer = lv_timer_create(auto_switch_timer_cb, 1200, NULL);
    auto_switch_timer_cb(roll_anim_timer);
}

/* 升级窗口内: 暂停翻屏定时器, 停掉 app_select/meter 的 animimg(避免读 flash 图片) */
void dashboard_ui_freeze_background(void)
{
    if (roll_anim_timer != NULL) {
        lv_timer_pause(roll_anim_timer);
    }

    lv_ui_page_app_select *app = (lv_ui_page_app_select *)ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (app != NULL && app->page_app_select_animimg_1 != NULL) {
        lv_animimg_delete(app->page_app_select_animimg_1);
    }

    lv_ui_page_meter *meter = (lv_ui_page_meter *)ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
    if (meter != NULL) {
        if (meter->page_meter_animimg_1 != NULL) {
            lv_animimg_delete(meter->page_meter_animimg_1);
        }
        if (meter->page_meter_animimg_2 != NULL) {
            lv_animimg_delete(meter->page_meter_animimg_2);
        }
        if (meter->page_meter_animimg_3 != NULL) {
            lv_animimg_delete(meter->page_meter_animimg_3);
        }
    }
}

/* 升级失败/中止回退后，恢复后台自动翻屏与动画。 */
void dashboard_ui_resume_background(void)
{
    if (roll_anim_timer != NULL) {
        lv_timer_resume(roll_anim_timer);
    }

    lv_ui_page_app_select *app = (lv_ui_page_app_select *)ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (app != NULL && app->page_app_select_animimg_1 != NULL) {
        lv_animimg_start(app->page_app_select_animimg_1);
    }

    lv_ui_page_meter *meter = (lv_ui_page_meter *)ui_get_scr_ptr(&guider_ui, GUI_SCREEN_PAGE_METER);
    if (meter != NULL) {
        if (meter->page_meter_animimg_1 != NULL) {
            lv_animimg_start(meter->page_meter_animimg_1);
        }
        if (meter->page_meter_animimg_2 != NULL) {
            lv_animimg_start(meter->page_meter_animimg_2);
        }
        if (meter->page_meter_animimg_3 != NULL) {
            lv_animimg_start(meter->page_meter_animimg_3);
        }
    }
}

extern void screen_map_jpeg_buf_release(void *buf);

void screen_map_post_msg_to_lvgl_ui(const char *msg, ...)
{
    va_list argptr;

    /* printf("[chili] %s %s   \n", __func__, msg); */
    va_start(argptr, msg);

    if (strstr(msg, "screen_map_jljpg_update")) {
        char *data = va_arg(argptr, char *);
        int data_len = va_arg(argptr, int);
        if (!data || data_len <= 0) {
            if (data) {
                screen_map_jpeg_buf_release(data);
            }
            va_end(argptr);
            return;
        }
        lvgl_rpc_post_func(ui_jljpg_update_handler, 2, data, data_len);
        /* ui_jljpg_update_post_handle(data, data_len); */

    } else if (strstr(msg, "screen_map_jljpg_clear")) {
        lvgl_rpc_post_func(ui_jljpg_clear_handler, 1, NULL);
    }

    va_end(argptr);

}

uint32_t timer_get_ms(void);
static void screen_switch_func(void)
{
    gui_scr_t *cur_scr = gui_scr_get_act();
    if (cur_scr == NULL) {
        return;
    }

    gui_scr_t *next_scr = NULL;
    uint32_t cur_id = cur_scr->id;

    switch (cur_id) {
    case GUI_SCREEN_PAGE_METER:
        printf("cur scr is page_music!\n");

        next_scr = ui_get_scr(GUI_SCREEN_PAGE_MUSIC);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;

    case GUI_SCREEN_PAGE_MUSIC:
        printf("cur scr is page_map!\n");
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_MAP);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;

    case GUI_SCREEN_PAGE_MAP:
        printf("cur scr is page_video!\n");
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_VIDEO);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }

        break;

    case GUI_SCREEN_PAGE_APP_SELECT:
        printf("cur scr is page_meter!\n");

        if (roll_anim_timer != NULL) {
            lv_timer_pause(roll_anim_timer);
        }

        next_scr = ui_get_scr(GUI_SCREEN_PAGE_METER);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;

    case GUI_SCREEN_PAGE_VIDEO:
        next_scr = ui_get_scr(GUI_SCREEN_CHONGWUYIBIAO);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;

    case GUI_SCREEN_CHONGWUYIBIAO:
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr,
                LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;

    default:
        printf("cur scr is page_meter!\n");

        next_scr = ui_get_scr(GUI_SCREEN_PAGE_METER);
        if (next_scr != NULL && next_scr != cur_scr) {
            ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        }
        break;
    }
}


static void screen_switch_timer_cb(lv_timer_t *timer)
{
    /* 获取当前活动屏幕 */
    gui_scr_t *cur_scr = gui_scr_get_act();
    if (cur_scr == NULL) {
        return;
    }

    gui_scr_t *next_scr = NULL;
    uint32_t cur_id = cur_scr->id;

    /* 根据当前屏幕 ID 决定下一个屏幕 */
    switch (cur_id) {
    case GUI_SCREEN_PAGE_METER:
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_MUSIC);
        printf("cur scr is page_music!\n");
        break;
    case GUI_SCREEN_PAGE_MUSIC:
        printf("cur scr is page_map!\n");
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_MAP);
        break;
    case GUI_SCREEN_PAGE_MAP:
        printf("cur scr is page_video!\n");
        if (roll_anim_timer != NULL) {
            auto_switch_timer_cb(roll_anim_timer);
            lv_timer_resume(roll_anim_timer);
        }
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_VIDEO);

        break;
    case GUI_SCREEN_PAGE_VIDEO:
        printf("cur scr is page_app select!\n");
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_APP_SELECT);
        break;
    case GUI_SCREEN_PAGE_APP_SELECT: //AP:
        if (roll_anim_timer != NULL) {
            lv_timer_pause(roll_anim_timer);
        }
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_METER);
        printf("cur scr is page_meter!\n");
        break;

    default:
        next_scr = ui_get_scr(GUI_SCREEN_PAGE_METER);
        printf("cur scr is page_meter!\n");
        break;
    }

    /* 切换到下一个屏幕，无动画 */
    if (next_scr != NULL && next_scr != cur_scr) {
        ui_load_scr_anim(&guider_ui, next_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
    }
}

void start_screen_rotation(uint32_t period_ms)
{
    lv_timer_create(screen_switch_timer_cb, period_ms, NULL);
}



static void mpi_vdec_demo_wrapper(void *dummy, ...)
{
    (void)dummy; // 避免未使用警告
    ui_music_album_pic_handler();
}

void bt_music_post_msg_to_lvgl_ui(const char *msg, ...)
{
    va_list argptr;

    printf("[chili] %s %s   \n", __func__, msg);
    va_start(argptr, msg);

    if (strstr(msg, "music_state_off")) {
        //暂停
        ui_music_pause_handler();
    } else if (strstr(msg, "music_state_on")) {
        //播放音乐
        ui_music_on_handler();
    } else if (strstr(msg, "music_tol_time")) {
        //总时长
        ui_music_tol_time_handler(va_arg(argptr, int));
    } else if (strstr(msg, "music_cur_time")) {
        //当前时长
        ui_music_cur_time_handler(va_arg(argptr, int));
    } else if (strstr(msg, "music_lyrics")) {
        ui_music_lyrics_handler(va_arg(argptr, char *));
    } else if (strstr(msg, "music_artist")) {
        /* 歌手/歌名 要用“/”切割出来 */
        ui_music_artist_handler(va_arg(argptr, char *));
    } else if (strstr(msg, "music_album_pic")) {
        /*lvgl_rpc_post_func(mpi_vdec_demo, 1, NULL); //ui_music_album_pic_handler, 0);*/
        int ret = lvgl_rpc_post_func(mpi_vdec_demo_wrapper, 1, NULL);
        printf("[BIP] UI notify ret=%d\n", ret);
    } else if (strstr(msg, "music_volume")) {
        ui_music_volume_handler(va_arg(argptr, int));
    } else if (strstr(msg, "player_state")) {
        //播放器状态(单曲循环/...)
    } else if (strstr(msg, "music_spectrum")) {
        // 频谱数据 - 这里传递指针
        /*float (*db_data)[MAX_SPECTRUM_SECTION_NUM] = va_arg(argptr, float(*)[MAX_SPECTRUM_SECTION_NUM]);*/
        /*ui_music_spectrum_handler(db_data);*/
    } else {
        printf("[chili] %s your msg [%s] no callback! \n", __func__, msg);
    }

    va_end(argptr);
}
/**
 * Create a demo application
 */
void custom_init(lv_ui *ui)
{
    bip_mem_init(); //music bg lock init
    /* start_screen_rotation(5000); */
    ui_auto_vertical_switch_init();

    /* 注册双备份升级进度回调：升级开始写 flash 时自动切到内置字库的“升级进度页” */
    ui_update_page_init();
    youyibao_sif_ui_init();

#if !TCFG_DASHBOARD_HARDWARE_TEST_ENABLE
    /* 未开启硬件测试宏时，隐藏 app_select 上的 TEST 入口按钮 */
    lv_ui_page_app_select *ui_scr = ui_get_scr_ptr(ui, GUI_SCREEN_PAGE_APP_SELECT);
    if (ui_scr != NULL && ui_scr->page_app_select_btn_back != NULL) {
        lv_obj_add_flag(ui_scr->page_app_select_btn_back, LV_OBJ_FLAG_HIDDEN);
    }
#endif
}

void hw_input_focus_release(void)
{
    lv_group_t *grp = lv_group_get_default();
    if (grp) {
        lv_group_remove_all_objs(grp);
    }
    lv_indev_t *idv = NULL;
    while ((idv = lv_indev_get_next(idv)) != NULL) {
        lv_indev_reset(idv, NULL);
    }
}
#else
#include "custom.h"
#include "../generated/gui_guider.h"
#include "lvgl.h"
void hw_input_focus_release(void)
{
    lv_group_t *grp = lv_group_get_default();
    if (grp) {
        lv_group_remove_all_objs(grp);
    }
    lv_indev_t *idv = NULL;
    while ((idv = lv_indev_get_next(idv)) != NULL) {
        lv_indev_reset(idv, NULL);
    }
}
/**
 * Create a demo application
 */
void custom_init(lv_ui *ui)
{


}
#endif
