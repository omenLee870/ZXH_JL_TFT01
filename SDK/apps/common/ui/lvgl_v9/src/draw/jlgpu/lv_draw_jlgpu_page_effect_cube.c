#include "src/core/lv_refr_private.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "jieli/extra/libs/cube/lv_cube.h"
#include "lv_draw_jlgpu_page_effect_cube.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#define CUBE_FACE_COUNT     6
#define CUBE_VIEW_DISTANCE  4.5f

struct cube_priv {
    lv_draw_jlgpu_unit_t *unit;
    int init : 1;
    u32 page_id[CUBE_FACE_COUNT];
    int width_tab[CUBE_FACE_COUNT];
    int height_tab[CUBE_FACE_COUNT];
};

static struct cube_priv cube_priv_t = {0};
#define __this (&cube_priv_t)


#if LV_USE_DRAW_JLGPU_LIST


void lv_cube_effect_set_page_index(uint32_t page_id, uint32_t page_index)
{
    if (page_index >= CUBE_FACE_COUNT) {
        __this->page_id[CUBE_FACE_COUNT - 1] = page_id;
    } else {
        __this->page_id[page_index] = page_id;
    }
}

void lv_cube_effect_get_page_id(uint32_t *page_id_arr)
{
    for (int i = 0; i < CUBE_FACE_COUNT; i++) {
        page_id_arr[i] = __this->page_id[i];
    }
}

int lv_cube_effect_get_face_scr_id(int x, int y)
{
    int face_index = lv_cube_get_face_index(x, y);
    if (face_index == -1) {
        return -1;
    }
    return __this->page_id[face_index];
}


void lv_cube_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag)
{
    if (__this->init) {
        return;
    }

    lv_display_t *disp = lv_refr_get_disp_refreshing();
    int width  = lv_display_get_horizontal_resolution(disp);
    int height = lv_display_get_vertical_resolution(disp);

    for (int i = 0; i < CUBE_FACE_COUNT; i++) {
        __this->width_tab[i] = width;
        __this->height_tab[i] = height;
    }

    __this->unit = unit;

    if (lv_cube_init(__this->width_tab, __this->height_tab,
                     0.996f, 0.996f, CUBE_VIEW_DISTANCE,
                     0, 0, width, height) != 0) {
        return;
    }

    __this->init = 1;
}


void lv_cube_effect_uninit(void)
{
    if (__this->init) {
        lv_cube_uninit();
        __this->init = 0;
    }
}


int lv_ui_cube_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset)
{
    lv_display_t *disp = lv_refr_get_disp_refreshing();
    int page_width  = lv_display_get_horizontal_resolution(disp);
    int page_height = lv_display_get_vertical_resolution(disp);

    (void)page_width;
    (void)page_height;

    if (__this->init) {
        uint8_t draw_order[CUBE_FACE_COUNT] = {0};
        lv_cube_draw(unit, (float)xoffset, (float)yoffset, draw_order);

        /* Map face index draw order to the effect state draw_order */
        for (int i = 0; i < CUBE_FACE_COUNT; i++) {
            g_page_effect_state.draw_order[i] = draw_order[i];
        }
    }

    return 0;
}

static const sticky_effect_ops_t cube_effect_ops = {
    .init            = lv_cube_effect_init,
    .uninit          = lv_cube_effect_uninit,
    .move            = lv_ui_cube_move,
    .get_face_scr_id = lv_cube_effect_get_face_scr_id,
    .get_face_index  = lv_cube_get_face_index,
    .get_page_id     = lv_cube_effect_get_page_id,
};

const sticky_effect_ops_t *lv_cube_effect_get_ops(void)
{
    return &cube_effect_ops;
}

#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
