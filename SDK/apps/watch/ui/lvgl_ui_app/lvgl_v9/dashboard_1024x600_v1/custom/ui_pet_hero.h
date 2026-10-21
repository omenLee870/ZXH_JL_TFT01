/**
 * @file ui_pet_hero.h
 * @brief 宠物仪表的小英雄动作接口，仅允许在 LVGL 线程调用。
 * @note 只接收已经校验的显示快照，不解码车辆协议、不控制车辆。
 */
#ifndef UI_PET_HERO_H
#define UI_PET_HERO_H

#include "../generated/gui_guider.h"

enum ui_pet_hero_flag {
    UI_PET_HERO_BRAKE = 1u << 0, /**< 实际刹车或电子刹车有效。 */
    UI_PET_HERO_LEFT  = 1u << 1, /**< 实际左转向有效。 */
    UI_PET_HERO_RIGHT = 1u << 2  /**< 实际右转向有效。 */
};

void ui_pet_hero_bind(lv_ui_chongwuyibiao *ui);
void ui_pet_hero_update(uint16_t speed_dkph, bool valid, uint32_t flags);
void ui_pet_hero_speed(uint16_t speed_dkph);
void ui_pet_hero_visible(bool visible);
void ui_pet_hero_tick(void);
bool ui_pet_hero_active(void);
void ui_pet_hero_boot_request(void);

#endif
