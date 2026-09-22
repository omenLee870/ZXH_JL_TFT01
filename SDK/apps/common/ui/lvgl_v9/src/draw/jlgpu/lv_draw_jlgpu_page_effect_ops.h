#ifndef LV_DRAW_JLGPU_PAGE_EFFECT_OPS_H
#define LV_DRAW_JLGPU_PAGE_EFFECT_OPS_H

#include "lv_draw_jlgpu.h"

/** Ops table for sticky 3D page effects.
 *  Each effect type (cube, hexagon, ...) provides a static instance.
 *  Extend this struct when adding new sticky effects. */
typedef struct sticky_effect_ops {
    void (*init)(lv_draw_jlgpu_unit_t *unit, bool list_flag);
    void (*uninit)(void);
    int (*move)(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset);
    int (*get_face_scr_id)(int x, int y);
    int (*get_face_index)(int x, int y);
    void (*get_page_id)(uint32_t *page_id_arr);
    /* 可选: manager 在每个槽重放前调用, 用于按槽位设置 GPU 全局透明度(即 app 层
     * global_alpha_tab + jlgpu_task_list_copy_group_global_alpha_reset 那套阴影机制)。
     * ctx_index 为该槽的 ctx 下标(即 draw_order), page_num 为槽总数;
     * 返回 0~128 则该槽任务链被设为该 alpha(128=不透明), 返回 <0 表示保持任务自带值。
     * 倒影类特效用它把镜像槽压暗成阴影(hexagon_reflection / lrb_mirror); 不实现(NULL)
     * 时各槽均不透明, 行为不变。 */
    int (*slot_alpha)(int ctx_index, int page_num);
} sticky_effect_ops_t;

#endif
