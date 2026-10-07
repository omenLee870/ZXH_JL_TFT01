#include "lvgl.h"
#include "generated/gui_guider.h"
#include "generated/gui_events/events_init.h"
#include "stdio.h"
#include "custom/neicun_zhuizong.h"

lv_ui guider_ui;

void jl_gui_init(void)
{
    neicun_zhuizong_kaishi();
#if !LV_USE_GUIBUILDER_SIMULATOR
    /* 上次HTTP OTA预留区(UI资源)升级掉电中断: 直接进内置字库升级页等待重新升级 */
    extern int ui_update_page_should_boot_recover(void);
    if (ui_update_page_should_boot_recover()) {
        printf("[OTA] UI res(reserve) upgrade incomplete -> boot into upgrade page, wait re-upgrade\n");
        extern void ui_update_page_boot_upgrade_only(void);
        ui_update_page_boot_upgrade_only();
        return;
    }
#endif

#if 0
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
        puts("waitting sd on... ");
    }
#endif

#if LV_USE_MONKEY != 0
    /*Create pointer monkey test*/
    lv_monkey_config_t monkey_pointer_config;
    monkey_pointer_config.type = LV_INDEV_TYPE_POINTER;
    monkey_pointer_config.period_range.min = 20;
    monkey_pointer_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_pointer_config), true);

    /*Create keypad monkey test*/
    lv_monkey_config_t monkey_keypad_config;
    monkey_keypad_config.type = LV_INDEV_TYPE_KEYPAD;
    monkey_keypad_config.period_range.min = 20;
    monkey_keypad_config.period_range.max = 20;
    lv_monkey_set_enable(lv_monkey_create(&monkey_keypad_config), true);

    /*Create encoder monkey test*/
    lv_monkey_config_t monkey_encode_config;
    monkey_encode_config.type = LV_INDEV_TYPE_ENCODER;
    monkey_encode_config.period_range.min = 20;
    monkey_encode_config.period_range.max = 20;
    monkey_encode_config.input_range.min = -5;
    monkey_encode_config.input_range.max = 5;
    lv_monkey_set_enable(lv_monkey_create(&monkey_encode_config), true);
#endif

    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);
    neicun_zhuizong_jieduan("ui_chushihua_hou");

}
