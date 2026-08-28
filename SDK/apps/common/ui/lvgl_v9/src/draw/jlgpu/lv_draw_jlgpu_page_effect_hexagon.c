#include "src/core/lv_refr_private.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "jieli/extra/libs/jl_hexagon/lv_hexagon.h"
#include "asm/math_fast_function.h"
#include "lv_draw_jlgpu_page_effect_hexagon.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if (POLYTOPE_CONFIG == POLYTOPE_SIX)
const u8 polytope = POLYTOPE_SIX;
#elif (POLYTOPE_CONFIG == POLYTOPE_EIGHT)
const u8 polytope = POLYTOPE_EIGHT;
#elif (POLYTOPE_CONFIG == POLYTOPE_ALL)
const u8 polytope = POLYTOPE_ALL;
#endif
struct hexagon_anim0 {
    float dist_x;
    float dist_y;
    int limit_v;	// 超过后按该值运行
    int limit_base;	// 最小值限制
    u8  negative_x;	// 负方向标记
    u8  negative_y;	// 负方向标记
    float hypotenuse; // 直角三角形斜边
    float sin_x;
};

struct hexagon_anim1 {
    /*ui_anim_t anim;*/
    u8 status;			//1 run
};

struct hexagon_anim2 {
    /*ui_anim_t anim;*/
    u8 status;			//1 run
    int face_index;
};

struct hexagon_anim3 {
    /*ui_anim_t anim;*/
    int last_val;
};

struct lv_hexagon_priv {
    lv_draw_jlgpu_unit_t *unit;
    void *curr_elm;//页面
    u8 hold_flag;
    u8 move_flag;
    u8 touch_flag;
    u8 polytope_type; // 多面体的数量配置，多个多面体共存时使用
    struct hexagon_anim0 *p_anim0;   // 转场动画 : 惯性动画
    struct hexagon_anim1 *p_anim1;   // 其他界面进入的转场动画前的入场动画
    struct hexagon_anim2 *p_anim2;   // 转场动画退出进入其他页面的离场动画
    struct hexagon_anim3 *p_anim3;   // 配合离场动画执行的角度修正
    int curr_win;
    s16 pos_x;
    s16 pos_y;
#if (POLYTOPE_CONFIG == POLYTOPE_SIX)
    u32 page_id[POLYTOPE_SIX];
    u8 reset_sep[POLYTOPE_SIX];
    u8 reset_sep_last[POLYTOPE_SIX];
#else //八面体或六面体和八面体共存
    u32 page_id[POLYTOPE_EIGHT];
    u8 reset_sep[POLYTOPE_EIGHT];
    u8 reset_sep_last[POLYTOPE_EIGHT];
#endif
    int init : 1;
    int ignore_first_move : 1;
    int cur_left;
    int dial_page_index;
    unsigned long last_time;
    s16 x_curr_angle;
    s16 y_curr_angle;
    //u8 x_dir;
    //u8 y_dir;
    int width_tab[8];
    int height_tab[8];
};

static struct lv_hexagon_priv hexagon_priv_t = {0};
#define __this (&hexagon_priv_t)

// 说明：以下参数是经过调整的，不要修改
#define X_ANGLE_ADJ 0.0f
#define X_ANGLE 14.85f // 14.85f //俯仰视角

#define VIEX_DISTANCE_MIN   4.7f
#define VIEX_DISTANCE_MAX   2.575f
#define VIEX_DISTANCE_DIFF  (VIEX_DISTANCE_MIN - VIEX_DISTANCE_MAX)

//缩放大小
#define W_SCALE     1.285f
#define H_SCALE     1.195f


#define SCALE_ANIM_SCOPE    100.0f
#define ANGLE_ANIM_SCOPE_MAX 180

struct hexagon_face_preview {
    int page_id;
    int image_id;
};

// 以下页面id优先使用图片作为页面预览
static const struct hexagon_face_preview preview[] = {
    /* {ID_WINDOW_CALENDAR, PAGE66_b35e_CALENDAR}, */
    /* {ID_WINDOW_SLEEP, PAGE66_ebb4_SLEEP}, */
    /* {-1, PAGE66_eb12_PAGE_ADD}, */
};


#if LV_USE_DRAW_JLGPU_LIST


static int get_face_preview(int page_id)
{
    int i;
    for (i = 0; i < sizeof(preview) / sizeof(preview[0]); i++) {
        if (preview[i].page_id == page_id) {
            return preview[i].image_id;
        }
    }
    return -1;
}

#if 0
/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim0_exec_cb 惯性动画回调
 *
 * @param var
 * @param v
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim0_exec_cb(int var, int32_t v)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim0_stop 结束惯性动结束惯性动画 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim0_stop(void)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim0_start 开始惯性动画
 *
 * @param e
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim0_start(struct element_touch_event *e)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim1_exec_cb 进入动画回调
 *
 * @param var
 * @param v
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim1_exec_cb(int var, int32_t v)
{

}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim1_ready_cb 入场动画接入回调
 *
 * @param _anim
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim1_ready_cb(struct _ui_anim_t *_anim)
{

}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim1_stop 结束入场动画
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim1_stop(void)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim1_start 开始入场动画
 *
 * @param curr_win
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim1_start(void)
{

}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim1_is_playing 入场动画是否播放
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int hexagon_anim1_is_playing()
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim2_exec_cb 离场动画回调
 *
 * @param var
 * @param v
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim2_exec_cb(int var, int32_t v)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim2_ready_cb 离场动画结束回调
 *
 * @param _anim
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim2_ready_cb(struct _ui_anim_t *_anim)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim2_stop 结束离场动画
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim2_stop(void)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim2_start
 *
 * @Params face_index   离场切换的页面
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim2_start(int face_index)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim2_is_playing 离场动画是否播放
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int hexagon_anim2_is_playing()
{

}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim3_exec_cb 匀速平移回调
 *
 * @param var
 * @param v
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim3_exec_cb(int var, int32_t v)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim3_stop 结束匀速平移动画 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim3_stop(void)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_anim3_start 开始匀速平移动画
 *
 * @param e
 */
/* ------------------------------------------------------------------------------------*/
static void hexagon_anim3_start(int angle_ofs, int run_time)
{

}

#endif

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_update_task_list_order 重排
 *
 * @param reset_sep
 */
/* ------------------------------------------------------------------------------------*/
static void jlgpu_update_task_list_order(u8 *reset_sep)
{

    for (int i = 0; i < __this->polytope_type; i++) {
        if (__this->reset_sep_last[i] != reset_sep[i]) {
            goto update_list;   // 贴图顺序发生了改变，需要更新链表
        }
    }

    return; // 跑到这里意味着链表不需要更新

update_list:
#if (POLYTOPE_CONFIG == POLYTOPE_SIX)
    u8 order_tab[] = {0, reset_sep[0], reset_sep[1], reset_sep[2], reset_sep[3], reset_sep[4], reset_sep[5]};
    u16 order_tab_size = sizeof(order_tab);
#elif (POLYTOPE_CONFIG == POLYTOPE_EIGHT)
    u8 order_tab[] = {0, reset_sep[0], reset_sep[1], reset_sep[2], reset_sep[3], reset_sep[4], reset_sep[5], reset_sep[6], reset_sep[7]};
    u16 order_tab_size = sizeof(order_tab);
#elif (POLYTOPE_CONFIG == POLYTOPE_ALL)
    u8 order_tab0[] = {0, reset_sep[0], reset_sep[1], reset_sep[2], reset_sep[3], reset_sep[4], reset_sep[5]};
    u8 order_tab1[] = {0, reset_sep[0], reset_sep[1], reset_sep[2], reset_sep[3], reset_sep[4], reset_sep[5], reset_sep[6], reset_sep[7]};
    u8 *order_tab = NULL;
    u16 order_tab_size = 0;
    if (__this->polytope_type == POLYTOPE_SIX) {
        order_tab = order_tab0;
        order_tab_size = sizeof(order_tab0);
    } else if (__this->polytope_type == POLYTOPE_EIGHT) {
        order_tab = order_tab1;
        order_tab_size = sizeof(order_tab1);
    } else {
        /*log_error("__this->polytope_type config err\n");*/
        return;
    }
#endif

    // 暂存上一次的输出
    for (int i = 0; i < __this->polytope_type; i++) {
        __this->reset_sep_last[i] = reset_sep[i];
    }

    return;
}

/* 动态设置多面体的数量，在配置成多面体共存是启用 */
static u8 custom_polytope_type = POLYTOPE_SIX;
void hexagon_custom_polytope_type_set(u8 type)
{
    custom_polytope_type = type;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_effect_set_page_index 配置3d灯笼页面
 *
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_effect_set_page_index(uint32_t page_id, uint32_t page_index)
{
    if (page_index >= POLYTOPE_CONFIG) {
        __this->page_id[POLYTOPE_CONFIG - 1] = page_id;
    } else {
        __this->page_id[page_index] = page_id;
    }
}

void lv_hexagon_effect_get_page_id(uint32_t *page_id_arr)
{
    for (int i = 0; i < POLYTOPE_CONFIG ; i++) {
        page_id_arr[i] = __this->page_id[i];
    }
}

int lv_hexagon_effect_get_face_scr_id(int x, int y)
{
    int face_index = lv_hexagon_get_face_index(x, y);
    if (face_index == -1) {
        return face_index;
    }
    return __this->page_id[face_index];
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_effect_init 3d灯笼初始化
 *
 * @param curr_elm
 * @param list_flag
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag)
{
    if (__this->init) {
        return;
    }
    if (polytope == POLYTOPE_SIX) {
        __this->polytope_type = POLYTOPE_SIX;
    } else if (polytope == POLYTOPE_EIGHT) {
        __this->polytope_type = POLYTOPE_EIGHT;
    } else if (polytope == POLYTOPE_ALL) {
        __this->polytope_type = custom_polytope_type; //用户自己定义，默认6面体
        lv_hexagon_polytope_type_set(__this->polytope_type);
    } else {
        /*log_error("polytope config err\n");*/
        return;
    }

    lv_display_t *disp =  lv_refr_get_disp_refreshing();
    int width  = lv_display_get_horizontal_resolution(disp);
    int height = lv_display_get_vertical_resolution(disp);

    for (int i = 0; i < __this->polytope_type; i++) {
        __this->width_tab[i] = width;
        __this->height_tab[i] = height;
    }

    __this->unit = unit;

    lv_hexagon_init(__this->width_tab, __this->height_tab, ((float)height / (float)width) * W_SCALE, H_SCALE, VIEX_DISTANCE_MIN, 0, 0, width, height, (X_ANGLE - X_ANGLE_ADJ));

    if (list_flag == false) {
        for (int i = 0; i < __this->polytope_type; i++) {
            __this->reset_sep[i] = __this->reset_sep_last[i] = i + 1;
        }
    }

    __this->init = 1;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief hexagon_effect_update 特效更新，用于刷新时间
 *
 * @param timeout
 */
/* ------------------------------------------------------------------------------------*/
void hexagon_effect_update(int timeout)
{

}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_hexagon_effect_uninit 反初始化
 */
/* ------------------------------------------------------------------------------------*/
void lv_hexagon_effect_uninit(void)
{
    if (__this->init) {
        /*hexagon_anim0_stop();*/
        /*hexagon_anim1_stop();*/
        /*hexagon_anim2_stop();*/
        /*hexagon_anim3_stop();*/

        if (__this->dial_page_index != -1) {
            __this->dial_page_index = -1;
        }

        lv_hexagon_uninit();

        __this->init = 0;
        __this->cur_left = 0;
        __this->x_curr_angle = 0;
        __this->hold_flag = 0;

    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_hexagon_status 特效执行状态
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_hexagon_status()
{
    return __this->init;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_ui_hexagon_move 滑动接口
 *
 * @param curr_win
 * @param xoffset
 * @param yoffset
 * @param mode
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int lv_ui_hexagon_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset)
{
    lv_display_t *disp =  lv_refr_get_disp_refreshing();

    void *curr_elm  = NULL;
    int page_width  = lv_display_get_horizontal_resolution(disp);
    int page_height = lv_display_get_vertical_resolution(disp);

    __this->cur_left += xoffset;
    __this->cur_left = (__this->cur_left >= page_width) ? (__this->cur_left = page_width) : __this->cur_left;
    __this->cur_left = (__this->cur_left <= -page_width) ? (__this->cur_left = -page_width) : __this->cur_left;

    if ((__this->cur_left != 0) && (__this->init == 0)) {
        lv_hexagon_effect_init(curr_elm, false);
    }

    if (__this->init) {
        __this->x_curr_angle -= xoffset;
        lv_hexagon_draw(unit, __this->x_curr_angle, __this->y_curr_angle, X_ANGLE, (X_ANGLE - X_ANGLE_ADJ), __this->reset_sep, 1);
    }

    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_hexagon_zoom_out 入场动画: 缩小
 *
 * @param curr_win
 * @param v
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_hexagon_zoom_out(int curr_win, int v)
{
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_hexagon_zoom_in 离场动画: 放大
 *
 * @param curr_win
 * @param v
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_hexagon_zoom_in(int curr_win, int v)
{
    return 0;
}

static const sticky_effect_ops_t hexagon_effect_ops = {
    .init            = lv_hexagon_effect_init,
    .uninit          = lv_hexagon_effect_uninit,
    .move            = lv_ui_hexagon_move,
    .get_face_scr_id = lv_hexagon_effect_get_face_scr_id,
    .get_face_index  = lv_hexagon_get_face_index,
    .get_page_id     = lv_hexagon_effect_get_page_id,
};

const sticky_effect_ops_t *lv_hexagon_effect_get_ops(void)
{
    return &hexagon_effect_ops;
}
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
