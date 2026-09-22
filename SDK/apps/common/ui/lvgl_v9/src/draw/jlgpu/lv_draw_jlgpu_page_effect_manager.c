/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "jieli/extra/libs/jl_hexagon/lv_hexagon.h"
#include "lv_draw_jlgpu_page_effect_hexagon.h"
#include "lv_draw_jlgpu_page_effect_hexagon_reflection.h"
#include "lv_draw_jlgpu_page_effect_lrb_mirror.h"
#include "jieli/extra/libs/cube/lv_cube.h"
#include "lv_draw_jlgpu_page_effect_cube.h"
#include "lv_draw_jlgpu_page_effect_ops.h"
#include "lv_page_flip.h"
#include "lv_page_scale.h"
#include "lv_page_edge_flip.h"
#include "lv_page_cube_flip.h"
#include "lv_port_disp.h"
#include "jieli/common/gui/scr/gui_scr.h"
#include "app_config.h"
#include "gxdma_driver.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if LV_USE_DRAW_JLGPU_LIST

/*********************
 *      DEFINES
 *********************/
#define LV_PAGE_CLICK_SWIPE_THRESHOLD  5  /* 滑动/点击区分阈值(像素)，超过此距离视为滑动 */
#define LV_PAGE_RIPPLE_MAX_AMP_DIV   1     /* 水波纹最大振幅 = 屏宽/DIV(除数越小浪越高) */
#define LV_PAGE_RIPPLE_SPEED_GAIN    15    /* 振幅增益: target = 分界线速度(px/帧)*GAIN, clamp到最大振幅 */
#define LV_PAGE_RIPPLE_STOP_DECAY_DIV 3    /* 分界线停下时衰减: 越小回直越快 */
#define LV_PAGE_RIPPLE_RISE_DIV      2     /* 移动升起: 每帧 amp += (target-amp)/DIV, 越小升起越急 */
#define LV_PAGE_RIPPLE_MOVE_THRESHOLD 2    /* 移动检测阈值(越小越易触发波浪): 分界线每帧位移超过才视为移动 */
#define LV_PAGE_RIPPLE_SHARPNESS    3     /* 浪尖尖锐度: cos^N 幂次, 越大浪越尖, 1=圆润, 全屏覆盖 */
#define LV_PAGE_RIPPLE_BAND_H       16     /* 每带高度(px), 越大相邻带错开越明显、锯齿越强 */

/* ---- 定点缩放特效(POINT_ENLARGE / POINT_NARROW) ----
 * 两页共用同一个缩放中心(点击点)和同一个缩放幅度, 进度 p = 分界线/屏宽 ∈ [0,1]:
 *   POINT_ENLARGE: 旧页 1.0 -> 1+R 从点击点向外长, 同时淡出; 新页 1-R -> 1.0 同时淡入
 *   POINT_NARROW : 旧页 1.0 -> 1-R 向点击点收小, 同时淡出; 新页 1+R -> 1.0 同时淡入
 * 缩放中心取「上一次点击」的位置: 点击松手时记录, 滑动松手不记录(沿用上一次的点位)。 */
#define LV_PAGE_POINT_SCALE_DELTA  LV_PAGE_SCALE_DELTA /* 共用缩放幅度: 两页缩放比之差恒为此值 */
#define LV_PAGE_POINT_CENTER_DIV   2                   /* 从未点击过时的兜底中心 = 屏宽/2, 屏高/2 */

/* ---- sticky 特效松手惯性甩动 ----
 * 手指拖动期间已经跟手旋转, 松手后原本什么都不做(多面体停在任意角度)。这里补上:
 * 按松手速度继续转 -> 指数减速 -> 吸附到整面 -> 提交到该面所属页面。
 * 切页手感一行不改(跟手仍 1:1, a_old 仍 0->屏宽), 惯性只接管「松手之后」。
 * 落点页面靠「屏幕中心正对哪个面」反查, 不猜环的循环顺序, 因此天然兼容 cube。
 */
#define LV_PAGE_INERTIA_DBG            0      /* 打点日志, 标定完置 0 */
#define LV_PAGE_INERTIA_PERIOD_MS      LV_DEF_REFR_PERIOD /* 与 static_timer 同周期, 勿写死 33 */
#define LV_PAGE_INERTIA_V_SAMPLES      8      /* 速度采样环大小 */
#define LV_PAGE_INERTIA_V_WINDOW_MS    80     /* 取速窗口(≈5 帧): 窗口差分, 不用 EMA */
#define LV_PAGE_INERTIA_V_MIN          0.12f  /* px/ms, 收尾门槛: 低于此判停。末步 ≈ 0.12*16 ≈ 2px,
                                               * 再往下会被 (int32_t) 取整吃成 0 而提前僵住;
                                               * 调高它同时也抬高起甩门槛(两者共用这个值) */
#define LV_PAGE_INERTIA_V_MAX          4.00f  /* px/ms 上限(≈64px/帧)。这是"快甩甩得远"的旋钮: 慢甩不受影响,
                                               * 只有超过它的甩动会被削平。取 2.5 时快甩全被压成一样远 */
#define LV_PAGE_INERTIA_DECAY          0.94f  /* 主手感旋钮: 每 16ms 剩 94% 速度。
                                               * 到此停下所需时间 = TAU * ln(v0/V_MIN), TAU = -16/ln(DECAY)
                                               * 0.88 -> TAU 126ms(停得干脆) 0.92 -> 192ms 0.94 -> 258ms 0.96 -> 392ms(飘)
                                               * 滑行距离 ≈ v0 * TAU, 所以调大它是"又远又久", 一起变。
                                               * 必须 < 1.0: 取 1.0 等于全程匀速, 只能靠 FREE_MAX_MS 硬切停住 */
#define LV_PAGE_INERTIA_TAU_MS         192    /* = -16/ln(0.92); 仅吸附段算落点用(即 SNAP_ENABLE 打开时)。
                                               * 纯惯性(SNAP_ENABLE=0)下是死常量, 改 DECAY 不必同步它 */
#define LV_PAGE_INERTIA_SNAP_ENTER_PX  48     /* 距落点 < 0.4 面 -> 切入收尾段 */
#define LV_PAGE_INERTIA_FREE_MAX_MS    1200   /* 自由段最长时长。只做丢帧/时钟异常的兜底, 不要当刹车:
                                               * 取 160 时 v0>=0.7 的甩动全在仍以 >17px/帧 运动时被一刀切停。
                                               * 正常甩动应在 400~900ms 内由 DECAY 衰减到 V_MIN 自然停住。
                                               * 必须 >= TAU * ln(V_MAX/V_MIN), 否则快甩又会被切:
                                               * 258 * ln(4.0/0.12) ≈ 905ms */
#define LV_PAGE_INERTIA_SNAP_D_GAIN    3.0f   /* ease-out cubic 在 p=0 斜率=3, 取 T=3|d|/|v| 让松手瞬间不掉速 */
#define LV_PAGE_INERTIA_SNAP_MIN_MS    90
#define LV_PAGE_INERTIA_SNAP_MAX_MS    300
#define LV_PAGE_INERTIA_COMMIT_MS      32     /* a_old/a_new 统一时长; a_old 不得短于 a_new(见提交处注释) */
#define LV_PAGE_INERTIA_MAX_FACES      (POLYTOPE_CONFIG - 1) /* 灯笼: 6 面绕环, 留一面余量 */
#define LV_PAGE_INERTIA_CUBE_YAW_FACES 4      /* cube 水平拖动绕 Y 轴, 只有 4 个偏航面可达 */
#define LV_PAGE_INERTIA_CUBE_MAX_FACES (LV_PAGE_INERTIA_CUBE_YAW_FACES - 1) /* cube: 4 偏航面留一面 */
#define LV_PAGE_INERTIA_CUBE_ENABLE    1      /* cube 与灯笼同为 1px=0.5度(lv_cube.c: angle=|x|*PI*0.5/180) */

/* 甩动结束后要不要"吸附到整面 + 自动提交到该面所属页面"。
 * 0 = 纯惯性(当前产品要求): 按松手速度继续转 -> 指数减速到停, 停在哪算哪;
 *     不吸附、不切页, 只有点击某个面才进页面(走 timer_cb 里现成的点按分支)。
 * 1 = 吸附到最近的整面, 并自动提交到该面所属页面(需 1px=0.5度 的几何面网格)。 */
#define LV_PAGE_INERTIA_SNAP_ENABLE    0

/* ---- sticky 特效的背景 ctx 槽位 ----
 * 页面面占 0..POLYTOPE_CONFIG-1, 倒影/镜像面复用到 2*POLYTOPE_CONFIG-1, 之后都空着。
 * 背景取最后一个槽, 必须排在所有面槽之后, 否则会和倒影面抢槽。 */
#define LV_PAGE_EFFECT_BG_SLOT         15
#if (2 * POLYTOPE_CONFIG) > LV_PAGE_EFFECT_BG_SLOT
#error "jlgpu_ctx_list 槽位不够: 背景槽会被倒影面占用, 需扩 jlgpu_ctx_list 数组并调整 LV_PAGE_EFFECT_BG_SLOT"
#endif

#if LV_PAGE_INERTIA_DBG
#define INERTIA_LOG(fmt, ...)  printf("[IN] " fmt "\n", ##__VA_ARGS__)
#else
#define INERTIA_LOG(...)       do {} while (0)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/* 惯性阶段: IDLE(不甩) -> FREE(指数减速到停)。SNAP/ARMED 只在 SNAP_ENABLE 打开时用到 */
typedef enum {
    LV_PAGE_INERTIA_IDLE = 0,
    LV_PAGE_INERTIA_FREE,
    LV_PAGE_INERTIA_SNAP,
    LV_PAGE_INERTIA_ARMED,
} lv_page_inertia_phase_t;

typedef struct {
    lv_page_inertia_phase_t phase;
    int32_t  n;          /* 落点面序号(绝对), 仅 SNAP_ENABLE */
    int32_t  n_org;      /* 出发面序号 = 按下时的 touch_x / 每面像素, 仅 SNAP_ENABLE */
    int32_t  n_try;      /* 实际尝试的落点面序号: 撞到空槽/未注册页时逐面回退到 n_org */
    int32_t  target;     /* 落点绝对位置(px), 恒为"每面像素"的整数倍, 仅 SNAP_ENABLE */
    int32_t  x_phase0;   /* 收尾段起点位置, 仅 SNAP_ENABLE */
    int32_t  d0;         /* 收尾段总位移(带符号), 仅 SNAP_ENABLE */
    float    v;          /* 自由段速度(px/ms, 带符号, x 轴) */
    float    vy;         /* 自由段速度(px/ms, 带符号, y 轴)。cube 是双轴的:
                          * 库按 (xdir=y_offset, ydir=x_offset) 选旋转轴、按增量长度定角度,
                          * 只喂 x 会让松手瞬间的旋转方向被掰成纯水平轴, 所以要两个分量一起衰减。*/
    uint32_t t_last;     /* 自由段上次积分时刻 */
    uint32_t t_phase0;   /* 收尾段起点时刻 */
    uint32_t settle_ms;  /* 收尾段时长 */
    uint32_t t_arm;      /* ARMED 起点时刻 */
    int32_t  base_touch_x; /* 跟手绝对基准(替代按下时把 touch_x 直写 0) */
    struct {
        uint32_t t;
        int32_t x;
        int32_t y;
    } s[LV_PAGE_INERTIA_V_SAMPLES];
    uint8_t  n_s;        /* 有效样本数 */
    uint8_t  head;       /* 下一个写入位置 */
} lv_page_inertia_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern lv_obj_t *ui_get_setup_scr_obj(int32_t scr_id);
extern void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status);
extern void jlgpu_set_page_effect_x_anim(void *obj, int32_t v);
extern void jlgpu_page_effect_flatten_done(void);
extern int32_t lv_port_get_first_touch_x(void);
extern int32_t lv_port_get_first_touch_y(void);
extern void dbi_link_gpu(u8 en);
/**********************
 *  STATIC PROTOTYPES
 **********************/
static void jlgpu_page_effect_set_translate(int32_t tx);
static bool lv_page_effect_inertia_supported(void);
static void lv_page_effect_inertia_reset(void);
static void lv_page_effect_inertia_cancel(void);
static void lv_page_effect_inertia_push_sample(int32_t pos_x, int32_t pos_y);
static void lv_page_effect_inertia_begin(void);
static void lv_page_effect_inertia_tick(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_layer_t static_layer = {0};
static lv_timer_t *static_timer = NULL;
lv_jlgpu_ctx_t *jlgpu_ctx_list_ori = NULL;
static lv_page_inertia_t s_inertia = {0};
/* 背景对象(裸指针, 不持有)。非 NULL 时特效期间整屏 blit 它, 否则回落纯黑底 */
static lv_obj_t *s_page_effect_bg_obj = NULL;

/* 定点缩放特效的缩放中心 = 上一次「点击」的坐标。刻意放在 g_page_effect_state 之外:
 * 该结构体在特效退场时被整体清零, 而"上一次点击"必须跨特效会话存活(滑动手势要复用它)。 */
static int32_t s_point_center_x = 0;
static int32_t s_point_center_y = 0;
static uint8_t s_point_center_valid = 0;

_WEAK_ lv_obj_t *ui_get_setup_scr_obj(int32_t scr_id)
{
    return NULL;
}

int lv_page_effect_get_page_num(lv_screen_load_anim_t page_mode)
{
    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
    case LV_SCR_LOAD_ANIM_HEXAGON:
        return POLYTOPE_CONFIG;
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    case LV_SCR_LOAD_ANIM_LRB_MIRROR:
        /* 倒影类: 6 本体面 + 6 镜像面 = 12 个 quad */
        return POLYTOPE_CONFIG * 2;
    default:
        return 2;
    }
}

/* 反射/镜像类特效(本体面下挂镜像面): 页数 12, ctx 前半段与后半段共享本体指针 */
static bool lv_page_effect_is_reflection_mode(void)
{
    if (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION ||
        g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_LRB_MIRROR) {
        return true;
    }
    return false;
}

bool lv_page_effect_need_sticky(lv_screen_load_anim_t page_mode)
{
    if (page_mode == LV_SCR_LOAD_ANIM_CUBE ||
        page_mode == LV_SCR_LOAD_ANIM_HEXAGON ||
        page_mode == LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION ||
        page_mode == LV_SCR_LOAD_ANIM_LRB_MIRROR) {
        return true;
    } else {
        return false;
    }
}

/* 定点缩放特效: 两页都靠全局透明度渐变交接, 需要对整链做乘性调光, 因此必须留逐节点快照 */
static bool lv_page_effect_point_mode(lv_screen_load_anim_t page_mode)
{
    if (page_mode == LV_SCR_LOAD_ANIM_POINT_ENLARGE ||
        page_mode == LV_SCR_LOAD_ANIM_POINT_NARROW) {
        return true;
    } else {
        return false;
    }
}

/** Select the ops table for the current sticky 3D effect.
 *  Extend this switch when adding new effects. */
static const sticky_effect_ops_t *sticky_effect_get_ops(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return lv_cube_effect_get_ops();
    case LV_SCR_LOAD_ANIM_HEXAGON:
        return lv_hexagon_effect_get_ops();
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        return lv_hexagon_reflection_effect_get_ops();
    case LV_SCR_LOAD_ANIM_LRB_MIRROR:
        return lv_lrb_mirror_effect_get_ops();
    default:
        return NULL;
    }
}

void lv_page_effect_set_page_index(lv_screen_load_anim_t page_mode,
                                   uint32_t page_id, uint32_t page_index)
{
    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        lv_cube_effect_set_page_index(page_id, page_index);
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON:
        lv_hexagon_effect_set_page_index(page_id, page_index);
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        lv_hexagon_reflection_effect_set_page_index(page_id, page_index);
        break;
    case LV_SCR_LOAD_ANIM_LRB_MIRROR:
        lv_lrb_mirror_effect_set_page_index(page_id, page_index);
        break;
    default:
        break;
    }
}

void lv_page_effect_set_bg_obj(lv_obj_t *obj)
{
    s_page_effect_bg_obj = obj;
}

/* 返回初始正对相机的面索引，不同模型的正面不同 */
static int lv_page_effect_get_front_index(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return 3;  /* cube: 实测 face 3 初始朝向相机 */
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    default:
        return 0;  /* hexagon: 面0初始角度0°正对+Z */
    }
}

/* 将当前页面移到正面（面向用户），下次触发多页面特效时当前页始终面向用户。
 * 在 effect_enter 阶段、init() 之前调用。 */
static void lv_page_effect_make_front(void)
{
    const sticky_effect_ops_t *ops = sticky_effect_get_ops();
    if (!ops || !g_page_effect_state.act_scr) {
        return;
    }

    // 反查当前页 page_id
    gui_scr_t *scr = gui_scr_get_by_obj(g_page_effect_state.act_scr);
    int32_t target_id = gui_scr_get_id(scr);

    if (target_id < 0) {
        return;
    }

    uint32_t buf[16];
    ops->get_page_id(buf);

    // page_id 数组的实际长度：reflection 模式是 POLYTOPE_CONFIG（不含倒影面），
    // 其他模式与 page_num 一致
    int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
    int id_count = lv_page_effect_is_reflection_mode() ? POLYTOPE_CONFIG : page_num;
    int front_idx = lv_page_effect_get_front_index();

    if (front_idx < 0 || front_idx >= id_count) {
        front_idx = 0;
    }

    // 查找 target_id 是否已在数组中
    int found = -1;
    for (int i = 0; i < id_count; i++) {
        if ((int32_t)buf[i] == target_id) {
            found = i;
            break;
        }
    }

    if (found == front_idx) {
        // 已经在正面，不处理
    } else if (found >= 0) {
        // 已在数组中其他位置：swap(front_idx, found)
        uint32_t tmp = buf[front_idx];
        buf[front_idx] = buf[found];
        buf[found] = tmp;
    } else {
        // 不在数组中：右移一位，插入 front_idx（最后一位被挤出）
        for (int i = id_count - 1; i > front_idx; i--) {
            buf[i] = buf[i - 1];
        }
        buf[front_idx] = (uint32_t)target_id;
    }

    // 写回
    for (int i = 0; i < id_count; i++) {
        lv_page_effect_set_page_index(g_page_effect_state.page_mode, buf[i], i);
    }
}

uint8_t *lv_jlgpu_get_offset_buf(uint8_t *px_map, const lv_area_t *screen_window, lv_area_t *draw_area)
{
    uint32_t w = lv_area_get_width(screen_window);
    uint32_t y = draw_area->y1;//h-1
    uint32_t x = draw_area->x1;
    uint32_t bpp = 2;

    uint8_t *px_map_2 = px_map + (w * y + x) * bpp;
    return px_map_2;
}

/*
 * 缩放 + 水平平移, 与 lv_page_scale_get_matrix 同构(同样绕贴图中心缩放), 只在回程平移上
 * 多叠一个 dx, 等价于:
 *     fw = T(w/2 + dx, h/2) · S(ratio) · T(-w/2, -h/2)
 * 即先把整张贴图按 ratio 缩到中心, 再整体右移 dx。
 * 用于 SCALE_DOUBLE: 让页面"盒子"跟着分界线一起滑动, 而不是绕屏幕中心原地缩放。
 * ratio=1 且 dx=0 时与 lv_page_scale_get_matrix 完全等价。
 */
static void lv_page_scale_shift_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
        float ratio, float dx, int tex_w, int tex_h)
{
    jlgpu_matrix_t t0, t1, scale_m, tmp;

    /* Translate texture center to origin */
    jlgpu_matrix_set_identity(&t0);
    jlgpu_matrix_translate(&t0, -(float)tex_w / 2.0f, -(float)tex_h / 2.0f);

    /* Uniform scale */
    jlgpu_matrix_set_identity(&scale_m);
    scale_m.m[0][0] = ratio;
    scale_m.m[1][1] = ratio;

    /* Translate back (+ dx: 缩放后再整体平移, 盒子随之滑动) */
    jlgpu_matrix_set_identity(&t1);
    jlgpu_matrix_translate(&t1, (float)tex_w / 2.0f + dx, (float)tex_h / 2.0f);

    /* fw_mat = t1 * scale_m * t0 */
    jlgpu_matrix_multiply(&tmp, &t1, &scale_m);
    jlgpu_matrix_multiply(fw_mat, &tmp, &t0);

    /* bw_mat = invert(fw_mat) */
    jlgpu_matrix_invert(bw_mat, fw_mat);
}

/*
 * 绕任意支点缩放, 等价于:
 *     fw = T(px, py) · S(ratio) · T(-px, -py)
 * 与 lv_page_scale_shift_get_matrix 同构, 区别只在缩放中心由「贴图中心」换成「任意点 (px, py)」——
 * 定点缩放特效要绕点击点缩放, 而不是绕屏幕中心。ratio=1 时与恒等矩阵等价。
 */
static void lv_page_scale_pivot_get_matrix(jlgpu_matrix_t *fw_mat, jlgpu_matrix_t *bw_mat,
        float ratio, float px, float py)
{
    jlgpu_matrix_t t0, t1, scale_m, tmp;

    /* Translate pivot to origin */
    jlgpu_matrix_set_identity(&t0);
    jlgpu_matrix_translate(&t0, -px, -py);

    /* Uniform scale */
    jlgpu_matrix_set_identity(&scale_m);
    scale_m.m[0][0] = ratio;
    scale_m.m[1][1] = ratio;

    /* Translate pivot back */
    jlgpu_matrix_set_identity(&t1);
    jlgpu_matrix_translate(&t1, px, py);

    /* fw_mat = t1 * scale_m * t0 */
    jlgpu_matrix_multiply(&tmp, &t1, &scale_m);
    jlgpu_matrix_multiply(fw_mat, &tmp, &t0);

    /* bw_mat = invert(fw_mat) */
    jlgpu_matrix_invert(bw_mat, fw_mat);
}

/* sticky 特效的地板: 有背景 ctx 就恒等矩阵整屏 blit, 没有就回落原来的纯黑填充。
 * 矩阵必须显式复位: global trans param 是全局 SFR, 跨帧残留, 上一帧最后一个面的矩阵会带进来。 */
static void lv_page_effect_draw_bg(lv_draw_jlgpu_unit_t *unit, uint8_t *px_map,
                                   const lv_area_t *screen_window)
{
    lv_jlgpu_ctx_t *bg_ctx = g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT];
    if (bg_ctx == NULL) {
        lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
        return;
    }

    lv_area_t draw_area;
    draw_area.x1 = screen_window->x1;
    draw_area.x2 = screen_window->x2;
    draw_area.y1 = screen_window->y1;
    draw_area.y2 = screen_window->y2;

    unit->jlgpu_ctx = bg_ctx;
    lv_jlgpu_reset_global_trans_param();
    lv_jlgpu_sfr_set(unit, &draw_area, lv_area_get_width(screen_window),
                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area), JLGPU_FORMAT_RGB565);
    lv_jlgpu_task_list_run(unit);
}

//页面特效绘制
void lv_jlgpu_set_global_trans_param(lv_draw_jlgpu_unit_t *unit, const lv_area_t *screen_window, int32_t touch_x, lv_screen_load_anim_t page_mode, uint8_t *px_map)
{
    uint8_t gpu_link_dbi = lv_jl_lcd_get_need_gpu_link_dbi();
    if (gpu_link_dbi) {
        //关闭gpu联动dbi
        jlgpu_hw_set_sfr_top(NULL);
        lv_jlgpu_pnd_sem_clr();
        dbi_link_gpu(0);
    }

    uint8_t need_gxdma =  lv_jl_lcd_get_need_gxdma();
    if (need_gxdma) {
        /* 每帧重置 GPU 硬件状态, 保持 ctx 内部干净 */
        JLGXDMA_link_gpu(0);
        jlgpu_hw_set_sfr_top(NULL);
        lv_jlgpu_pnd_sem_clr();
        JLGXDMA_init();
    }

    //页面切换逻辑
    //2.特效绘制
    uint32_t fb_width = lv_area_get_width(screen_window);

    // 左右滑动方向适配：RTL 时镜像 touch_x 并交换新旧 ctx
    int32_t eff_touch_x = touch_x;
    lv_jlgpu_ctx_t *eff_ctx_old = g_page_effect_state.jlgpu_ctx_list[0];
    lv_jlgpu_ctx_t *eff_ctx_new = g_page_effect_state.jlgpu_ctx_list[1];
    if (g_page_effect_state.swipe_left) {
        eff_touch_x = (int32_t)fb_width - touch_x;
        eff_ctx_old = g_page_effect_state.jlgpu_ctx_list[1];
        eff_ctx_new = g_page_effect_state.jlgpu_ctx_list[0];
    }

    switch (page_mode) {
    case LV_SCR_LOAD_ANIM_OTHER_PAGE:
        // eff_touch_x 语义：0=全旧页, fb_width=全新页
        // 左区域 [0, eff_touch_x]=eff_ctx_new（新页面），右区域 [eff_touch_x, fb_width]=eff_ctx_old（旧页面）
        // RTL 时方向适配已在入口完成（eff_touch_x 镜像 + ctx 交换）
        if (!g_page_effect_state.swipe_ready) {
            //方向未确定：只画旧页面全屏，避免首帧闪烁
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        } else {
            int32_t div = eff_touch_x;
            if (div < 0) {
                div = 0;
            }
            if (div > (int32_t)fb_width) {
                div = fb_width;
            }

            /* 左右平移: 两页不再各自原地不动、只被分界线裁开, 而是整块跟着分界线滑动 ——
             *     新页盒 = [div - W, div]   旧页盒 = [div, div + W]
             * 新页从屏幕左侧滑入、旧页向右侧滑出, 接缝(= 分界线)随之扫过; 两盒恒首尾相接,
             * 全程铺满屏幕不露背景。复用 SCALE_DOUBLE 的 helper, 只是 ratio 固定 1 => 纯平移。
             * 采样源恒在 [0,W] 内(新: x-div+W, 旧: x-div), 不会取到贴图外。 */
            int fb_height = lv_area_get_height(screen_window);
            float dx_new = (float)div - (float)fb_width;
            float dx_old = (float)div;
            jlgpu_global_matrix_t gm = {0};
            jlgpu_matrix_t fw, bw;

            //绘制左边区域（新页面）
            unit->jlgpu_ctx = eff_ctx_new;
            {
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = div;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                if (draw_area.x1 <= draw_area.x2) {
                    lv_page_scale_shift_get_matrix(&fw, &bw, 1.0f, dx_new, fb_width, fb_height);
                    gm.trans_en = 1;
                    gm.fw_mat = fw;
                    gm.bw_mat = bw;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            }

            //绘制右边区域（旧页面）
            unit->jlgpu_ctx = eff_ctx_old;
            {
                lv_area_t draw_area;
                draw_area.x1 = div;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                if (draw_area.x1 <= draw_area.x2) {
                    lv_page_scale_shift_get_matrix(&fw, &bw, 1.0f, dx_old, fb_width, fb_height);
                    gm.trans_en = 1;
                    gm.fw_mat = fw;
                    gm.bw_mat = bw;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            }
        }
        break;
    case LV_SCR_LOAD_ANIM_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                //方向未确定：只画旧页面全屏
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                float degree = (float)eff_touch_x * 180.0f / (float)fb_width;

                /* Avoid exactly 90 degrees (degenerate case) */
                if (degree > 89.999f && degree < 90.001f) {
                    degree = 90.001f;
                }

                int fb_height = lv_area_get_height(screen_window);
                int affine_width = lv_page_flip_get_affine_width(fb_width, fb_height, degree);
                int half_w = fb_width / 2;
                int mid_left, mid_right;
                lv_area_t draw_left, draw_mid, draw_right;

                /* Compute regions based on degree */
                if (degree < 90.0f) {
                    /* Fold is on the right side of center */
                    mid_left  = half_w - (affine_width / 2);
                    mid_right = half_w;

                    draw_left.x1 = screen_window->x1;
                    draw_left.x2 = mid_left - 1;
                    draw_left.y1 = screen_window->y1;
                    draw_left.y2 = screen_window->y2;

                    draw_mid.x1 = mid_left;
                    draw_mid.x2 = mid_right;
                    draw_mid.y1 = screen_window->y1;
                    draw_mid.y2 = screen_window->y2;

                    draw_right.x1 = half_w;
                    draw_right.x2 = screen_window->x2;
                    draw_right.y1 = screen_window->y1;
                    draw_right.y2 = screen_window->y2;

                    /* Left: new page (static) */
                    if (draw_left.x1 <= draw_left.x2) {
                        unit->jlgpu_ctx = eff_ctx_new;
                        lv_jlgpu_sfr_set(unit, &draw_left, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_left),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    /* Middle: old page flipped (perspective warp) */
                    if (draw_mid.x1 <= draw_mid.x2) {
                        jlgpu_matrix_t flip_fw, flip_bw;
                        jlgpu_global_matrix_t gm = {0};
                        lv_page_flip_get_matrix(&flip_fw, &flip_bw, 180.0f - degree, fb_width, fb_height);
                        gm.trans_en = 1;
                        gm.fw_mat = flip_fw;
                        gm.bw_mat = flip_bw;

                        unit->jlgpu_ctx = eff_ctx_old;
                        jlgpu_set_global_trans_param(&gm);
                        lv_jlgpu_sfr_set(unit, &draw_mid, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_mid),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                        lv_jlgpu_reset_global_trans_param();
                    }

                    /* Right: old page (static) */
                    if (draw_right.x1 <= draw_right.x2) {
                        unit->jlgpu_ctx = eff_ctx_old;
                        lv_jlgpu_sfr_set(unit, &draw_right, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_right),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }
                } else {
                    /* degree > 90: fold passed center, back side of flip = new page */
                    mid_left  = half_w;
                    mid_right = half_w + (affine_width / 2);

                    draw_left.x1 = screen_window->x1;
                    draw_left.x2 = half_w - 1;
                    draw_left.y1 = screen_window->y1;
                    draw_left.y2 = screen_window->y2;

                    draw_mid.x1 = mid_left;
                    draw_mid.x2 = mid_right;
                    draw_mid.y1 = screen_window->y1;
                    draw_mid.y2 = screen_window->y2;

                    draw_right.x1 = mid_right + 1;
                    draw_right.x2 = screen_window->x2;
                    draw_right.y1 = screen_window->y1;
                    draw_right.y2 = screen_window->y2;

                    /* Left: new page (static) */
                    if (draw_left.x1 <= draw_left.x2) {
                        unit->jlgpu_ctx = eff_ctx_new;
                        lv_jlgpu_sfr_set(unit, &draw_left, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_left),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    /* Middle: new page's other half (back side of flip) */
                    if (draw_mid.x1 <= draw_mid.x2) {
                        jlgpu_matrix_t flip_fw, flip_bw;
                        jlgpu_global_matrix_t gm = {0};
                        lv_page_flip_get_matrix(&flip_fw, &flip_bw, 180.0f - degree, fb_width, fb_height);
                        gm.trans_en = 1;
                        gm.fw_mat = flip_fw;
                        gm.bw_mat = flip_bw;

                        unit->jlgpu_ctx = eff_ctx_new;
                        jlgpu_set_global_trans_param(&gm);
                        lv_jlgpu_sfr_set(unit, &draw_mid, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_mid),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                        lv_jlgpu_reset_global_trans_param();
                    }

                    /* Right: old page (static) */
                    if (draw_right.x1 <= draw_right.x2) {
                        unit->jlgpu_ctx = eff_ctx_old;
                        lv_jlgpu_sfr_set(unit, &draw_right, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_right),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            /* Draw target page full screen on exit */
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_SCALE_DOUBLE:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                int fb_height = lv_area_get_height(screen_window);
                float tmp_scale = LV_PAGE_SCALE_DELTA;
                float old_scale = tmp_scale * (float)(fb_width - eff_touch_x) / (float)fb_width + LV_PAGE_SCALE_MIN;
                float new_scale = tmp_scale * (float)eff_touch_x / (float)fb_width + LV_PAGE_SCALE_MIN;
                /* 平移量: 把每页缩放后的"盒子"贴到分界线上, 两页盒子首尾相接铺满全屏 ——
                 *     新页盒 = [d - W*new_scale, d]   旧页盒 = [d, d + W*old_scale]   (d = 分界线)
                 * 于是缩放的同时两页整体随分界线滑动(新页从左滑入并放大, 旧页向右滑出并缩小),
                 * 不再是各自绕屏幕中心原地缩放被硬切一刀。见 lv_page_scale_shift_get_matrix。 */
                float dx_old = (float)eff_touch_x - (float)fb_width * (1.0f - old_scale) * 0.5f;
                float dx_new = (float)eff_touch_x - (float)fb_width * (1.0f + new_scale) * 0.5f;
                jlgpu_global_matrix_t gm_old = {0};
                jlgpu_global_matrix_t gm_new = {0};
                jlgpu_matrix_t fw, bw;

                /* Old page: region [eff_touch_x, right], shrinks & slides right as it moves away */
                lv_area_t draw_old;
                draw_old.x1 = eff_touch_x;
                draw_old.x2 = screen_window->x2;
                draw_old.y1 = screen_window->y1;
                draw_old.y2 = screen_window->y2;
                if (draw_old.x1 <= draw_old.x2) {
                    lv_page_scale_shift_get_matrix(&fw, &bw, old_scale, dx_old, fb_width, fb_height);
                    gm_old.trans_en = 1;
                    gm_old.fw_mat = fw;
                    gm_old.bw_mat = bw;
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm_old);
                    lv_jlgpu_sfr_set(unit, &draw_old, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_old),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* New page: region [0, eff_touch_x], grows & slides right as it enters */
                lv_area_t draw_new;
                draw_new.x1 = screen_window->x1;
                draw_new.x2 = eff_touch_x < screen_window->x2 ? eff_touch_x : screen_window->x2;
                draw_new.y1 = screen_window->y1;
                draw_new.y2 = screen_window->y2;
                if (draw_new.x1 <= draw_new.x2) {
                    lv_page_scale_shift_get_matrix(&fw, &bw, new_scale, dx_new, fb_width, fb_height);
                    gm_new.trans_en = 1;
                    gm_new.fw_mat = fw;
                    gm_new.bw_mat = bw;
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm_new);
                    lv_jlgpu_sfr_set(unit, &draw_new, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_new),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_POINT_ENLARGE:
    case LV_SCR_LOAD_ANIM_POINT_NARROW:
        /* 定点缩放: 两页不切分界线、整屏绘制, 靠全局透明度交接。
         * 两页共用同一个缩放中心(上一次点击点)和同一个缩放幅度, 进度 p=0 时全是旧页, p=1 时全是新页:
         *     POINT_ENLARGE: 旧页 1.0 -> 1+R 从点击点向外长并淡出, 新页 1-R -> 1.0 长到满并渐实
         *     POINT_NARROW : 旧页 1.0 -> 1-R 向点击点收小并淡出, 新页 1+R -> 1.0 收到满并渐实 */
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                //方向未确定：只画旧页面全屏，避免首帧闪烁
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                int fb_height = lv_area_get_height(screen_window);
                /* 进度直接用未镜像的 touch_x: 它天然就是"0=旧页, 屏宽=新页"(滑动跟手位移 / 动画值
                 * 都是朝这个方向单调增的)。这里刻意不用入口那份镜像过的 eff_touch_x 和交换过的 ctx:
                 * 那是给"按左右分界线切屏"的特效做几何适配的, 定点缩放没有分界线, 用镜像值会把
                 * 进度反过来(RTL 时开场即新页全屏)。 */
                float p = (float)touch_x / (float)fb_width;
                lv_jlgpu_ctx_t *ctx_old = g_page_effect_state.jlgpu_ctx_list[0];
                lv_jlgpu_ctx_t *ctx_new = g_page_effect_state.jlgpu_ctx_list[1];
                if (p < 0.0f) {
                    p = 0.0f;
                }
                if (p > 1.0f) {
                    p = 1.0f;
                }
                float ratio_old;
                float ratio_new;
                if (page_mode == LV_SCR_LOAD_ANIM_POINT_ENLARGE) {
                    ratio_old = 1.0f + LV_PAGE_POINT_SCALE_DELTA * p;
                    ratio_new = 1.0f - LV_PAGE_POINT_SCALE_DELTA * (1.0f - p);
                } else {
                    ratio_old = 1.0f - LV_PAGE_POINT_SCALE_DELTA * p;
                    ratio_new = 1.0f + LV_PAGE_POINT_SCALE_DELTA * (1.0f - p);
                }

                /* 缩放中心: 上一次「点击」的位置; 从未点击过(纯滑动/按键触发)时兜底屏幕中心 */
                float pivot_x = (float)(fb_width / LV_PAGE_POINT_CENTER_DIV);
                float pivot_y = (float)(fb_height / LV_PAGE_POINT_CENTER_DIV);
                if (s_point_center_valid) {
                    pivot_x = (float)s_point_center_x;
                    pivot_y = (float)s_point_center_y;
                }

                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                uint8_t *draw_buf = lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area);

                jlgpu_global_matrix_t gm = {0};
                jlgpu_matrix_t fw, bw;

                /* 两页都参与透明度渐变, 各自绕点击点缩放。
                 * 绘制顺序按"覆盖范围"排: 比例恒 >= 1.0 的那页始终铺满全屏, 先画它垫底,
                 * 另一页(比例 < 1, 只占中间一块)带透明度盖上去。这样屏幕的每一处都至少有一页
                 * 在贡献颜色, 不会出现"清屏黑底直接露出来"的洞。
                 *   ENLARGE: 旧页比例恒 >= 1.0 -> 旧页垫底, 新页淡入
                 *   NARROW : 新页比例恒 >= 1.0 -> 新页垫底, 旧页淡出 */
                lv_jlgpu_ctx_t *ctx_base;
                lv_jlgpu_ctx_t *ctx_top;
                float ratio_base;
                float ratio_top;
                float f_base;   /* 垫底页的页级透明度系数 */
                float f_top;    /* 上层页的页级透明度系数 */
                if (page_mode == LV_SCR_LOAD_ANIM_POINT_ENLARGE) {
                    ctx_base = ctx_old;
                    ratio_base = ratio_old;
                    ctx_top = ctx_new;
                    ratio_top = ratio_new;
                    f_base = 1.0f - p;   /* 旧页向外长, 同时淡出 */
                    f_top = p;           /* 新页向外长到满, 同时渐实 */
                } else {
                    ctx_base = ctx_new;
                    ratio_base = ratio_new;
                    ctx_top = ctx_old;
                    ratio_top = ratio_old;
                    f_base = p;          /* 新页收小到满, 同时渐实 */
                    f_top = 1.0f - p;    /* 旧页向里收, 同时淡出 */
                }
                uint8_t alpha_base = (uint8_t)((float)JLGPU_MAX_GLOBAL_ALPHA * f_base);
                uint8_t alpha_top = (uint8_t)((float)JLGPU_MAX_GLOBAL_ALPHA * f_top);

                /* 两页都必须走 SRC_OVER: 填充类节点收集时写死 JLGPU_BLEND_SRC(直接覆盖),
                 * SRC 下源会把底图整块盖掉, 只改 global_alpha 看不到任何渐变。
                 * 这里只翻"收集时本就是 SRC"的节点, 文字(SRC_OVER)和按 dsc->blend_mode 走的
                 * 图片节点保持原样, 否则会把页内元素自身的渲染效果改坏。 */
                /* 页级透明度必须乘在"收集时的透明度"上, 不能整链覆盖:
                 * 各元素收集时写的是 dsc->opa/2, 覆盖式赋值会把元素自身的半透明一起冲掉。 */
                lv_page_scale_pivot_get_matrix(&fw, &bw, ratio_base, pivot_x, pivot_y);
                gm.trans_en = 1;
                gm.fw_mat = fw;
                gm.bw_mat = bw;
                unit->jlgpu_ctx = ctx_base;
                lv_jlgpu_task_list_set_blend_src_over(ctx_base);
                lv_jlgpu_task_list_scale_global_alpha(ctx_base, alpha_base);
                jlgpu_set_global_trans_param(&gm);
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width, draw_buf, JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);

                /* 上层: 绕同一点缩放 + 页级透明度渐变 */
                lv_page_scale_pivot_get_matrix(&fw, &bw, ratio_top, pivot_x, pivot_y);
                gm.trans_en = 1;
                gm.fw_mat = fw;
                gm.bw_mat = bw;
                unit->jlgpu_ctx = ctx_top;
                lv_jlgpu_task_list_set_blend_src_over(ctx_top);
                lv_jlgpu_task_list_scale_global_alpha(ctx_top, alpha_top);
                jlgpu_set_global_trans_param(&gm);
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width, draw_buf, JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
                lv_jlgpu_reset_global_trans_param();
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            /* 退出帧画的是本体页, 必须还原成收集时的样子: 上面把任务链的混合模式和全局透明度
             * 都改写过了, 而任务链是常驻复用、不会每帧重建的。按快照逐节点还原 —— 整链统一
             * 写死 128/SRC 是错的, 链上各节点的原值本来就不一样(文字恒 SRC_OVER/128, 图片按
             * dsc->blend_mode, 透明度取 dsc->opa/2), 硬写会把元素自身的半透明和混合模式改坏。 */
            lv_jlgpu_task_list_restore(unit->jlgpu_ctx);
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_RIPPLE:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                //方向未确定：只画旧页面全屏，避免首帧闪烁
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                //水波纹：浪尖锚定在手指处(完全跟手), 波谷朝滑动后方延伸 amp(起伏), 恢复慢不影响跟手
                int fb_height = lv_area_get_height(screen_window);
                int lambda = fb_height * 2;                                //覆盖全屏: 半宽=屏高(全屏行都有波纹)
                //最大振幅=屏宽/DIV, 实际振幅由下方按分界线速度(手指跟手/动画)驱动

                int32_t base_div = eff_touch_x;
                if (base_div < 0) {
                    base_div = 0;
                }
                if (base_div > (int32_t)fb_width) {
                    base_div = fb_width;
                }

                //振幅：由分界线速度统一驱动(手指跟手 + 按键/松手动画都适用)——分界线每帧位移
                //超过阈值则升起, 停下则衰减拉平
                if (g_page_effect_state.ripple_prev_div < 0) {
                    g_page_effect_state.ripple_prev_div = base_div;   //首帧初始化, 避免残留位移尖峰
                }
                int32_t div_dx = base_div - g_page_effect_state.ripple_prev_div;
                g_page_effect_state.ripple_prev_div = base_div;

                int32_t amp = g_page_effect_state.ripple_amp;
                int32_t adx = div_dx < 0 ? -div_dx : div_dx;
                if (adx > LV_PAGE_RIPPLE_MOVE_THRESHOLD) {
                    //分界线移动：更新凸出方向(朝移动方向)，目标振幅跟速度成正比
                    g_page_effect_state.ripple_dir = (div_dx > 0) ? 1 : -1;
                    int32_t max_amp = (int32_t)fb_width / LV_PAGE_RIPPLE_MAX_AMP_DIV;
                    int32_t target = adx * LV_PAGE_RIPPLE_SPEED_GAIN;
                    if (target > max_amp) {
                        target = max_amp;
                    }
                    if (target > amp) {
                        amp += (target - amp) / LV_PAGE_RIPPLE_RISE_DIV;
                    }
                } else {
                    //分界线停下：向上取整衰减, 保证回直到0(不会被整数除法卡在1)
                    amp -= (amp + LV_PAGE_RIPPLE_STOP_DECAY_DIV - 1) / LV_PAGE_RIPPLE_STOP_DECAY_DIV;
                    if (amp < 0) {
                        amp = 0;
                    }
                }
                g_page_effect_state.ripple_amp = amp;

                //浪尖y：实时跟随手指，未触摸时取屏幕中心
                int finger_y = g_page_effect_state.finger_y;
                if (finger_y < 0) {
                    finger_y = screen_window->y1 + fb_height / 2;
                }

                //凸出方向：跟随手指当前水平移动方向, 未移动时兜底朝右
                int sign = g_page_effect_state.ripple_dir;
                if (sign == 0) {
                    sign = 1;
                }

                //8行一条带，逐带重放新旧ctx，分界线随行交错形成波纹
                const int band_h = LV_PAGE_RIPPLE_BAND_H;
                int y = screen_window->y1;
                while (y <= screen_window->y2) {
                    int y_end = y + band_h - 1;
                    if (y_end > screen_window->y2) {
                        y_end = screen_window->y2;
                    }

                    int y_c = (y + y_end) / 2;                    //带中心行采样相位
                    int dy = y_c - finger_y;
                    int abs_dy = dy < 0 ? -dy : dy;
                    int32_t div_x = base_div;
                    if (abs_dy <= lambda / 2) {
                        float cosv = cosf((float)dy * 3.14159265f / (float)lambda);
                        //cos^N 幂次让峰顶更尖(N=1圆润, N大→尖峰), 同时保持全屏覆盖
                        float shape = 1.0f;
                        for (int i = 0; i < LV_PAGE_RIPPLE_SHARPNESS; i++) {
                            shape *= cosv;
                        }
                        //浪尖(手指处, shape=1)=base_div(跟手); 波谷(边缘, shape=0)=base_div-sign*amp(后方拖尾)
                        div_x = base_div + (int32_t)(sign * amp * (shape - 1.0f));
                    }
                    if (div_x < screen_window->x1) {
                        div_x = screen_window->x1;
                    }
                    if (div_x > screen_window->x2) {
                        div_x = screen_window->x2;
                    }

                    //左边 [x1, div_x] = 新页（首尾相接：新页右缘平移对齐到 div_x）
                    if (div_x > screen_window->x1) {
                        lv_area_t draw_left;
                        draw_left.x1 = screen_window->x1;
                        draw_left.x2 = div_x;
                        draw_left.y1 = y;
                        draw_left.y2 = y_end;
                        unit->jlgpu_ctx = eff_ctx_new;
                        jlgpu_page_effect_set_translate(div_x - (int32_t)fb_width);
                        lv_jlgpu_sfr_set(unit, &draw_left, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_left),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    //右边 [div_x, x2] = 旧页（旧页左缘平移对齐到 div_x）
                    if (div_x < screen_window->x2) {
                        lv_area_t draw_right;
                        draw_right.x1 = div_x;
                        draw_right.x2 = screen_window->x2;
                        draw_right.y1 = y;
                        draw_right.y2 = y_end;
                        unit->jlgpu_ctx = eff_ctx_old;
                        jlgpu_page_effect_set_translate(div_x);
                        lv_jlgpu_sfr_set(unit, &draw_right, fb_width,
                                         lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_right),
                                         JLGPU_FORMAT_RGB565);
                        lv_jlgpu_task_list_run(unit);
                    }

                    y = y_end + 1;
                }
                lv_jlgpu_reset_global_trans_param();
            }
        } else if (g_page_effect_state.effect_exit) {
            //出场：画目标页全屏
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_CENTER_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {

                int fb_height = lv_area_get_height(screen_window);
                float degree = 180.0f - (float)eff_touch_x * 180.0f / (float)fb_width;
                jlgpu_global_matrix_t gm = {0};

                /* Avoid exactly 90 degrees */
                if (degree > 89.999f && degree < 90.001f) {
                    degree = 90.001f;
                }

                lv_page_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height);

                /* degree > 90: showing old page front, flip reveals new page behind */
                /* degree < 90: showing new page back, old page visible behind */
                lv_jlgpu_ctx_t *flip_ctx;
                lv_jlgpu_ctx_t *under_ctx;
                if (degree > 90.0f) {
                    flip_ctx = eff_ctx_old;   /* old page on top, flipping */
                    under_ctx = eff_ctx_new;  /* new page behind */
                } else {
                    flip_ctx = eff_ctx_new;   /* new page on top (back side) */
                    under_ctx = eff_ctx_old;  /* old page behind */
                }

                /* Draw underlay (static, full screen) */
                {
                    //背景清0
                    lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                }

                /* Draw flipped layer on top with perspective matrix */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    unit->jlgpu_ctx = flip_ctx;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                }
                lv_jlgpu_reset_global_trans_param();
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_EDGE_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                int fb_height = lv_area_get_height(screen_window);
                float degree = (float)eff_touch_x * 45.0f / (float)fb_width;
                float angle = LV_EDGE_FLIP_ANGLE;
                jlgpu_global_matrix_t gm_old = {0};
                jlgpu_global_matrix_t gm_new = {0};

                /* Clean buffer (background) */
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);

                /* Both pages are drawn full screen with their own bottom-hinge flip matrix.
                 * face_flag=0: old page rotates from 0 to -angle (hinge at bottom)
                 * face_flag=1: new page rotates from +angle to 0 */

                /* Draw old page (current face, rotates away) */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_edge_flip_get_full_screen_matrix(&gm_old, degree, fb_width, fb_height,
                            angle, 0);
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm_old);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* Draw new page (next face, rotates into view) */
                {
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_edge_flip_get_full_screen_matrix(&gm_new, degree, fb_width, fb_height,
                            angle, 1);
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm_new);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_CUBE_FLIP:
        if (g_page_effect_state.effect_busy) {
            if (!g_page_effect_state.swipe_ready) {
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_area_t draw_area;
                draw_area.x1 = screen_window->x1;
                draw_area.x2 = screen_window->x2;
                draw_area.y1 = screen_window->y1;
                draw_area.y2 = screen_window->y2;
                lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                                 lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                                 JLGPU_FORMAT_RGB565);
                lv_jlgpu_task_list_run(unit);
            } else {
                int fb_height = lv_area_get_height(screen_window);
                float degree = (float)(fb_width - eff_touch_x) * 90.0f / (float)fb_width;

                /* Avoid exactly 90 degrees (degenerate) */
                if (degree > 89.999f) {
                    degree = 89.999f;
                }

                /* Clean buffer background */
                lv_jlgpu_clean_buf(unit, px_map, JLGPU_FORMAT_RGB565, screen_window);

                /* Draw old page (rotates away from viewer) */
                {
                    jlgpu_global_matrix_t gm = {0};
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_cube_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height, 1);
                    unit->jlgpu_ctx = eff_ctx_old;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }

                /* Draw new page on top (rotates into view) */
                {
                    jlgpu_global_matrix_t gm = {0};
                    lv_area_t full_area;
                    full_area.x1 = screen_window->x1;
                    full_area.x2 = screen_window->x2;
                    full_area.y1 = screen_window->y1;
                    full_area.y2 = screen_window->y2;
                    lv_page_cube_flip_get_full_screen_matrix(&gm, degree, fb_width, fb_height, 0);
                    unit->jlgpu_ctx = eff_ctx_new;
                    jlgpu_set_global_trans_param(&gm);
                    lv_jlgpu_sfr_set(unit, &full_area, fb_width,
                                     lv_jlgpu_get_offset_buf(px_map, screen_window, &full_area),
                                     JLGPU_FORMAT_RGB565);
                    lv_jlgpu_task_list_run(unit);
                    lv_jlgpu_reset_global_trans_param();
                }
            } // end else (swipe_ready)
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width,
                             lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area),
                             JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    case LV_SCR_LOAD_ANIM_CUBE:
    case LV_SCR_LOAD_ANIM_LRB_MIRROR:
        if (g_page_effect_state.effect_busy) {
            /* 地板: 有背景图就整屏 blit 它, 否则纯黑。
             * 这里是四个 sticky 模式唯一共用的那一行, 2 页特效走各自的 case, 不受影响。 */
            lv_page_effect_draw_bg(unit, px_map, screen_window);
            /* sticky 特效：x_offset/y_offset 取反后传入 move，
             * 因为 hexagon/cube 的旋转方向与动画值方向相反。 */
            {
                int32_t eff_x_offset = -g_page_effect_state.x_offset;
                int32_t eff_y_offset = -g_page_effect_state.y_offset;
                sticky_effect_get_ops()->move(unit, eff_x_offset, eff_y_offset);
                /* 消费即清零: move 只吃增量, 不清零的话按住不动时每帧都会被重复施加同一个
                 * offset(特效期间本来没人 invalidate 所以一直没暴露, 惯性逐帧 invalidate 后会)。
                 * 只对走跟手基准白名单的模式清零, LRB_MIRROR 保持原样。 */
                if (lv_page_effect_inertia_supported()) {
                    g_page_effect_state.x_offset = 0;
                    g_page_effect_state.y_offset = 0;
                }
            }
            int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
            for (int i = 0; i < page_num; i++) {
                uint8_t draw_order = g_page_effect_state.draw_order[i];
                if (draw_order >= page_num) {
                    continue; /* 背面：跳过不可见面 */
                }
                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[draw_order];
                // clamp draw_area to screen bounds to avoid out-of-bounds framebuffer access
                lv_area_t da = *g_page_effect_state.draw_area[draw_order];
                if (da.x1 < screen_window->x1) {
                    da.x1 = screen_window->x1;
                }
                if (da.y1 < screen_window->y1) {
                    da.y1 = screen_window->y1;
                }
                if (da.x2 > screen_window->x2) {
                    da.x2 = screen_window->x2;
                }
                if (da.y2 > screen_window->y2) {
                    da.y2 = screen_window->y2;
                }
                if (da.x1 > da.x2 || da.y1 > da.y2) {
                    continue;
                }
                lv_jlgpu_sfr_set(unit, &da, fb_width, lv_jlgpu_get_offset_buf(px_map, screen_window, &da), JLGPU_FORMAT_RGB565);
                /* 按槽位设置 GPU 全局透明度: 倒影类特效把镜像槽压暗成阴影。
                 * 与 app 层 global_alpha_tab({32...}, {128...}) 的效果一致, 但由 GPU
                 * 混合完成, 无需逐像素 CPU 计算。镜像槽与本体槽复用同一条任务链,
                 * 所以必须在每次重放前重新设置。 */
                {
                    const sticky_effect_ops_t *slot_ops = sticky_effect_get_ops();
                    if (slot_ops && slot_ops->slot_alpha) {
                        int slot_a = slot_ops->slot_alpha(draw_order, page_num);
                        if (slot_a >= 0) {
                            lv_jlgpu_task_list_set_global_alpha(unit->jlgpu_ctx, (uint8_t)slot_a);
                        }
                    }
                }
                jlgpu_set_global_trans_param(g_page_effect_state.global_matrix[draw_order]);
                lv_jlgpu_task_list_run(unit);
            }
        } else if (g_page_effect_state.effect_exit) {
            unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[g_page_effect_state.end_page_index_id];
            /* 退出帧画的是本体页, 必须恢复不透明: 倒影类特效在槽循环里改写过任务链的
             * 全局透明度(镜像槽压暗), 而任务链是常驻复用、不会每帧重建的。 */
            {
                const sticky_effect_ops_t *exit_ops = sticky_effect_get_ops();
                if (exit_ops && exit_ops->slot_alpha) {
                    lv_jlgpu_task_list_set_global_alpha(unit->jlgpu_ctx, 128);
                }
            }
            lv_area_t draw_area;
            draw_area.x1 = screen_window->x1;
            draw_area.x2 = screen_window->x2;
            draw_area.y1 = screen_window->y1;
            draw_area.y2 = screen_window->y2;
            lv_jlgpu_sfr_set(unit, &draw_area, fb_width, lv_jlgpu_get_offset_buf(px_map, screen_window, &draw_area), JLGPU_FORMAT_RGB565);
            lv_jlgpu_task_list_run(unit);
        }
        break;
    default:
        break;
    }

    if (gpu_link_dbi) {
        //打开gpu联动dbi
        dbi_link_gpu(1);
    }
}

//关闭特效
void lv_jlgpu_reset_global_trans_param(void)
{
    jlgpu_matrix_t matrix;
    jlgpu_matrix_set_identity(&matrix);

    jlgpu_global_matrix_t global_matrix = {0};
    global_matrix.trans_en = 0;
    global_matrix.fw_mat = matrix;//正向矩阵
    global_matrix.bw_mat = matrix;//逆向矩阵
    jlgpu_set_global_trans_param(&global_matrix);
}

//设置全局平移变换：tx>0 向右平移，tx<0 向左平移（fw: 源→屏幕, bw: 逆）
static void jlgpu_page_effect_set_translate(int32_t tx)
{
    jlgpu_matrix_t fw, bw;
    jlgpu_matrix_set_identity(&fw);
    fw.m[0][2] = (float)tx;
    jlgpu_matrix_set_identity(&bw);
    bw.m[0][2] = -(float)tx;

    jlgpu_global_matrix_t gm = {0};
    gm.trans_en = 1;
    gm.fw_mat = fw;
    gm.bw_mat = bw;
    jlgpu_set_global_trans_param(&gm);
}


static void layer_initialize(lv_display_t *disp_drv, lv_layer_t *temp_layer, const lv_area_t *area)
{
    lv_layer_init(temp_layer);
    temp_layer->parent = NULL;
    temp_layer->_clip_area = *area;
    temp_layer->buf_area = *area;
    temp_layer->phy_clip_area = *area;
    temp_layer->color_format = LV_COLOR_FORMAT_RGB565;

    //必须链到链表里
    if (disp_drv->layer_head) {
        lv_layer_t *tail = disp_drv->layer_head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = temp_layer;
    } else {
        disp_drv->layer_head = temp_layer;
    }

    //借取显存的layerbuf,不额外申请buf;
    lv_layer_t *layer = disp_drv->layer_head;
    temp_layer->draw_buf = layer->draw_buf;
}

static void layer_remove_from_disp(lv_display_t *disp_drv, lv_layer_t *new_layer)
{
    if (!disp_drv || !new_layer || !disp_drv->layer_head) {
        return;
    }

    lv_layer_t *current = disp_drv->layer_head;
    lv_layer_t *previous = NULL;
    bool found = false;

    while (current != NULL) {
        if (current == new_layer) {
            found = true;
            break;
        }
        previous = current;
        current = current->next;
    }

    if (!found) {
        LV_LOG_INFO("Layer not found in list, skipping removal.");
        return;
    }

    if (previous == NULL) {
        disp_drv->layer_head = current->next;
    } else {
        previous->next = current->next;
    }

    LV_LOG_INFO("Layer removed and freed successfully.");
}

static void layer_set_empty_area(lv_layer_t *layer)
{
    //把new_layer的area清空，避免影响下一轮刷新
    lv_area_t empty_area = {0};
    layer->_clip_area = empty_area;
    layer->buf_area = empty_area;
    layer->phy_clip_area = empty_area;
}


static void lv_anim_set_current_value(lv_anim_t *a, int32_t current_value)
{
    uint32_t t = lv_map(current_value, a->start_value, a->end_value, 0, a->duration);
    a->act_time = t;
}
static void lv_anim_set_act_time(lv_anim_t *a, int32_t act_time)
{
    a->act_time = act_time;
}

/* ==================== 惯性甩动 ==================== */

#if LV_PAGE_INERTIA_SNAP_ENABLE
/* 每面多少像素: 库几何是 1px = 0.5 度(hexagon: angle = x_angle_val * PI * 0.5/180),
 * 6 面体一面 = 360/6 = 60 度 = 120 单位。屏宽(320)不是 120 的整数倍, 所以不能用屏宽当
 * "一页": 那样落点会停在两个面正中间, 正面反查就会失准、可能落到不相邻的页。吸附网格
 * 必须用几何面。甩动手感偏灵敏/迟钝就改这里。 */
static int32_t lv_page_effect_inertia_px_per_face(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return 720 / LV_PAGE_INERTIA_CUBE_YAW_FACES;      /* 4 面 -> 180 */
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    default:
        return 720 / POLYTOPE_CONFIG;                     /* 6 面 -> 120, 8 面 -> 90 */
    }
}

/* 单次甩动最多跨几个面(绕环留一面余量, 免得转满一圈又落回原页) */
static int32_t lv_page_effect_inertia_max_faces(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_CUBE:
        return LV_PAGE_INERTIA_CUBE_MAX_FACES;
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
    default:
        return LV_PAGE_INERTIA_MAX_FACES;
    }
}
#endif /* LV_PAGE_INERTIA_SNAP_ENABLE */

/* 哪些模式接惯性。LRB_MIRROR 有自己的 settle 机制, 且依赖"按下时把 touch_x 直写 0"
 * 做失配检测, 一律不接, 保持现状。 */
static bool lv_page_effect_inertia_supported(void)
{
    switch (g_page_effect_state.page_mode) {
    case LV_SCR_LOAD_ANIM_HEXAGON:
    case LV_SCR_LOAD_ANIM_HEXAGON_REFLECTION:
        return true;
    case LV_SCR_LOAD_ANIM_CUBE:
        return LV_PAGE_INERTIA_CUBE_ENABLE ? true : false;
    default:
        return false;
    }
}

static void lv_page_effect_inertia_reset(void)
{
    s_inertia.phase = LV_PAGE_INERTIA_IDLE;
    s_inertia.n = 0;
    s_inertia.n_org = 0;
    s_inertia.n_try = 0;
    s_inertia.target = 0;
    s_inertia.v = 0.0f;
    s_inertia.vy = 0.0f;
    s_inertia.n_s = 0;
    s_inertia.head = 0;
    s_inertia.base_touch_x = 0;
}

/* 只取消"正在甩"这件事, 保留跟手基准(同一根手指还要继续接管) */
static void lv_page_effect_inertia_cancel(void)
{
    s_inertia.phase = LV_PAGE_INERTIA_IDLE;
    s_inertia.v = 0.0f;
    s_inertia.vy = 0.0f;
}

static void lv_page_effect_inertia_push_sample(int32_t pos_x, int32_t pos_y)
{
    s_inertia.s[s_inertia.head].t = lv_tick_get();
    s_inertia.s[s_inertia.head].x = pos_x;
    s_inertia.s[s_inertia.head].y = pos_y;
    s_inertia.head = (uint8_t)((s_inertia.head + 1) % LV_PAGE_INERTIA_V_SAMPLES);
    if (s_inertia.n_s < LV_PAGE_INERTIA_V_SAMPLES) {
        s_inertia.n_s++;
    }
}

/* 松手速度: 取最近 WINDOW_MS 内最老的那个样本做差分, 单位 px/ms, x/y 各一份。
 * 按住不动再抬手时窗口内位移≈0 -> v≈0 -> 不甩, 这正是需要的行为(EMA 做不到)。 */
static void lv_page_effect_inertia_velocity(float *out_vx, float *out_vy)
{
    uint8_t idx_new, idx_old, i, k;
    uint32_t dt;

    *out_vx = 0.0f;
    *out_vy = 0.0f;

    if (s_inertia.n_s < 2) {
        return;
    }

    idx_new = (uint8_t)((s_inertia.head + LV_PAGE_INERTIA_V_SAMPLES - 1) % LV_PAGE_INERTIA_V_SAMPLES);
    idx_old = idx_new;
    for (i = 1; i < s_inertia.n_s; i++) {
        k = (uint8_t)((s_inertia.head + LV_PAGE_INERTIA_V_SAMPLES - 1 - i) % LV_PAGE_INERTIA_V_SAMPLES);
        if ((s_inertia.s[idx_new].t - s_inertia.s[k].t) > LV_PAGE_INERTIA_V_WINDOW_MS) {
            break;
        }
        idx_old = k;
    }

    dt = s_inertia.s[idx_new].t - s_inertia.s[idx_old].t;
    if (dt == 0) {
        return;
    }
    *out_vx = (float)(s_inertia.s[idx_new].x - s_inertia.s[idx_old].x) / (float)dt;
    *out_vy = (float)(s_inertia.s[idx_new].y - s_inertia.s[idx_old].y) / (float)dt;
}

/* 甩动收尾: 停在这个姿态, 不吸附也不切页。
 * 必须把 a_old 的值同步成 touch_x, 恢复「touch_x == a_old 当前值」这条不变量;
 * 否则下一次点按 resume 时 a_old 会从陈旧的旧值重跑, 多面体瞬间倒退整个甩动位移。 */
static void lv_page_effect_inertia_stop(void)
{
    if (g_page_effect_state.a_old) {
        lv_anim_set_values(g_page_effect_state.a_old, g_page_effect_state.touch_x,
                           g_page_effect_state.touch_x);
        lv_anim_set_current_value(g_page_effect_state.a_old, g_page_effect_state.touch_x);
    }
    s_inertia.phase = LV_PAGE_INERTIA_IDLE;
    s_inertia.v = 0.0f;
    s_inertia.vy = 0.0f;
}

#if LV_PAGE_INERTIA_SNAP_ENABLE
/* 进入收尾段: ease-out cubic 从当前速度平滑收到精确落点。
 * 取 settle = GAIN*|d|/|v|, 因为 ease-out cubic 在 p=0 处斜率恰为 3, 收尾段初速就等于
 * 松手速度 -> 两段之间没有速度跳变。落点永远由这一段决定, 自由段只负责走完大部分路程,
 * 所以调参风险被隔离在自由段。 */
static void lv_page_effect_inertia_enter_snap(int32_t from_x)
{
    int32_t d, ms;
    float vv;

    s_inertia.x_phase0 = from_x;
    s_inertia.d0 = s_inertia.target - from_x;
    s_inertia.t_phase0 = lv_tick_get();

    if (s_inertia.d0 == 0) {
        s_inertia.phase = LV_PAGE_INERTIA_ARMED;
        s_inertia.t_arm = s_inertia.t_phase0;
        return;
    }

    vv = s_inertia.v;
    if (vv < 0.0f) {
        vv = -vv;
    }
    if (vv < 0.05f) {
        vv = 0.05f;
    }
    d = s_inertia.d0;
    if (d < 0) {
        d = -d;
    }

    ms = (int32_t)(LV_PAGE_INERTIA_SNAP_D_GAIN * (float)d / vv);
    if (ms < LV_PAGE_INERTIA_SNAP_MIN_MS) {
        ms = LV_PAGE_INERTIA_SNAP_MIN_MS;
    } else if (ms > LV_PAGE_INERTIA_SNAP_MAX_MS) {
        ms = LV_PAGE_INERTIA_SNAP_MAX_MS;
    }
    s_inertia.settle_ms = (uint32_t)ms;
    s_inertia.phase = LV_PAGE_INERTIA_SNAP;
}

/* 对齐帧已经画出去之后: 反查"现在正对镜头的是哪一页"并提交。
 * 反查而不是按槽位算术, 是为了不依赖环的循环节/方向, 也天然兼容 cube 的
 * "6 槽环 / 4 个水平可达面"不同构。撞到空槽/未注册页就逐面回退; 退到出发面
 * 就只吸附归位、不提交 —— 绝不能带 NULL 走到 scr_anim_completed。 */
static void lv_page_effect_inertia_commit(const sticky_effect_ops_t *ops)
{
    lv_display_t *disp;
    lv_obj_t *new_page;
    int32_t cx, cy, P;
    int scr_id = -1;
    int face = -1;

    disp = lv_obj_get_display(g_page_effect_state.act_scr);
    cx = lv_display_get_horizontal_resolution(disp) / 2;
    cy = lv_display_get_vertical_resolution(disp) / 2;
    if (ops->get_face_scr_id) {
        scr_id = ops->get_face_scr_id((int)cx, (int)cy);
    }
    if (ops->get_face_index) {
        face = ops->get_face_index((int)cx, (int)cy);
    }
    new_page = (scr_id >= 0) ? ui_get_setup_scr_obj(scr_id) : NULL;

    INERTIA_LOG("land face=%d scr=%d obj=%p n=%d/%d tx=%d",
                face, scr_id, (void *)new_page, (int)s_inertia.n_try, (int)s_inertia.n_org,
                (int)g_page_effect_state.touch_x);

    /* 反查失败(空槽 / 页未注册 / 没有落点)或已经退回出发面: 逐面回退再收尾。
     * 任何情况下都不让 new_scr == NULL 走到 scr_anim_completed。 */
    if (new_page == NULL || face < 0 || s_inertia.n_try == s_inertia.n_org) {
        if (s_inertia.n_try != s_inertia.n_org) {
            P = lv_page_effect_inertia_px_per_face();
            s_inertia.n_try += (s_inertia.n_try > s_inertia.n_org) ? -1 : 1;
            s_inertia.target = s_inertia.n_try * P;
            s_inertia.v = 0.0f;
            INERTIA_LOG("retry n=%d target=%d", (int)s_inertia.n_try, (int)s_inertia.target);
            lv_page_effect_inertia_enter_snap(g_page_effect_state.touch_x);
            return;
        }
        /* 已回到出发面: 只归位、不提交, 保持跟手会话等下一次拖动/点按。
         * 顺手把 a_old 的值同步成 touch_x, 恢复「touch_x == a_old 当前值」这条不变量;
         * 否则下一次点按 resume 时 a_old 会从陈旧的旧值重跑, 多面体瞬间倒退整个甩动位移。
         * 起点==终点, resume 后首帧 x_offset 为 0, 零跳变。 */
        if (g_page_effect_state.a_old) {
            lv_anim_set_values(g_page_effect_state.a_old, g_page_effect_state.touch_x,
                               g_page_effect_state.touch_x);
            lv_anim_set_current_value(g_page_effect_state.a_old, g_page_effect_state.touch_x);
        }
        s_inertia.phase = LV_PAGE_INERTIA_IDLE;
        return;
    }

    if (g_page_effect_state.a_old == NULL || g_page_effect_state.a_new == NULL) {
        s_inertia.phase = LV_PAGE_INERTIA_IDLE;
        return;
    }

    /* 提交, 与"点击某个面"分支同构。
     * 1) 必须让 a_new 自然走完 -> scr_anim_completed 才会把 d->act_scr 改成目标页、
     *    发 SCREEN_LOADED/UNLOADED 并回收 prev_scr。不要走 flatten_done 那条自驱退出路。
     * 2) a_old 时长必须 >= a_new: a_old 没有 completed_cb, 完成即被动画系统 free,
     *    只有 a_new 的 completed_cb 会把 a_old 置 NULL, 否则中间几帧是野指针。
     * 3) 先把 a_old 的值域塌缩到当前位置: 按下期间它被 pause、act_time 停在 0 附近,
     *    直接 resume 会从 0 重跑, 瞬间倒退整个跟手位移。start==end 时首帧 x_offset 为 0。 */
    lv_anim_set_values(g_page_effect_state.a_old, g_page_effect_state.touch_x, g_page_effect_state.touch_x);
    lv_anim_set_current_value(g_page_effect_state.a_old, g_page_effect_state.touch_x);
    lv_anim_set_duration(g_page_effect_state.a_old, LV_PAGE_INERTIA_COMMIT_MS);
    lv_anim_set_duration(g_page_effect_state.a_new, LV_PAGE_INERTIA_COMMIT_MS);
    lv_anim_set_act_time(g_page_effect_state.a_new, 0);

    lv_anim_resume(g_page_effect_state.a_old);
    lv_anim_resume(g_page_effect_state.a_new);

    g_page_effect_state.new_scr = new_page;
    g_page_effect_state.end_page_index_id = (uint8_t)face;

    if (static_timer) {
        lv_timer_pause(static_timer);
    }
    s_inertia.phase = LV_PAGE_INERTIA_IDLE;
}
#endif /* LV_PAGE_INERTIA_SNAP_ENABLE */

/* 松手: 估速 -> 自由段(指数减速到停) */
static void lv_page_effect_inertia_begin(void)
{
    float vx, vy;
#if LV_PAGE_INERTIA_SNAP_ENABLE
    int32_t P, tx, n, n_org, fmax;
    float pred, f;
#endif

    if (!lv_page_effect_inertia_supported()) {
        INERTIA_LOG("skip: mode %d not in whitelist", (int)g_page_effect_state.page_mode);
        return;
    }
    /* 必须是"按下把动画暂停后跟手拖动"的场景; 纯动画(按键触发)不接管 */
    if (g_page_effect_state.a_old == NULL) {
        INERTIA_LOG("skip: a_old NULL");
        return;
    }
    if (!lv_anim_is_paused(g_page_effect_state.a_old)) {
        INERTIA_LOG("skip: a_old not paused (纯动画, 未经跟手)");
        return;
    }

    lv_page_effect_inertia_velocity(&vx, &vy);
    INERTIA_LOG("vel n_s=%d vx_milli=%d vy_milli=%d tx=%d ty=%d base=%d",
                (int)s_inertia.n_s, (int)(vx * 1000.0f), (int)(vy * 1000.0f),
                (int)g_page_effect_state.touch_x, (int)g_page_effect_state.touch_y,
                (int)s_inertia.base_touch_x);

    /* 分量各自夹上限: 保证单帧位移不会大到"瞬移"(x/y 同时触顶时向量约 1.4 倍, 仍小于半面) */
    if (vx > LV_PAGE_INERTIA_V_MAX) {
        vx = LV_PAGE_INERTIA_V_MAX;
    } else if (vx < -LV_PAGE_INERTIA_V_MAX) {
        vx = -LV_PAGE_INERTIA_V_MAX;
    }
    if (vy > LV_PAGE_INERTIA_V_MAX) {
        vy = LV_PAGE_INERTIA_V_MAX;
    } else if (vy < -LV_PAGE_INERTIA_V_MAX) {
        vy = -LV_PAGE_INERTIA_V_MAX;
    }
    /* 拖得太慢/按住没动: 不甩, 停在原地(不吸附) */
    if (vx > -LV_PAGE_INERTIA_V_MIN && vx < LV_PAGE_INERTIA_V_MIN &&
        vy > -LV_PAGE_INERTIA_V_MIN && vy < LV_PAGE_INERTIA_V_MIN) {
        s_inertia.phase = LV_PAGE_INERTIA_IDLE;
        return;
    }

    s_inertia.v = vx;
    s_inertia.vy = vy;
    s_inertia.t_last = lv_tick_get();
    s_inertia.t_phase0 = s_inertia.t_last;

#if LV_PAGE_INERTIA_SNAP_ENABLE
    P = lv_page_effect_inertia_px_per_face();
    tx = g_page_effect_state.touch_x;

    /* 自由段是 v *= DECAY 的指数衰减, 从 v 积到 0 的总位移正好是 v * TAU */
    pred = vx * (float)LV_PAGE_INERTIA_TAU_MS;
    f = ((float)tx + pred) / (float)P;
    n = (int32_t)(f + (f >= 0.0f ? 0.5f : -0.5f));

    /* 出发面序号: 按下那一刻 touch_x 必然落在整面上(每次甩动都以整面收尾) */
    n_org = s_inertia.base_touch_x / P;
    /* 上限夹在"跨过的面数"而不是绝对面序号: 绕环留一面余量, 避免转满一圈回到原页 */
    fmax = lv_page_effect_inertia_max_faces();
    if (n - n_org > fmax) {
        n = n_org + fmax;
    } else if (n_org - n > fmax) {
        n = n_org - fmax;
    }

    s_inertia.n = n;
    s_inertia.n_org = n_org;
    s_inertia.n_try = n;
    s_inertia.target = n * P;
    INERTIA_LOG("release tx=%d vx_milli=%d n=%d/%d target=%d P=%d",
                (int)tx, (int)(vx * 1000.0f), (int)n, (int)n_org, (int)s_inertia.target, (int)P);

    if (vx == 0.0f) {
        lv_page_effect_inertia_enter_snap(tx);
        return;
    }
#endif

    s_inertia.phase = LV_PAGE_INERTIA_FREE;
}

static void lv_page_effect_inertia_tick(void)
{
    uint32_t now;
    int32_t x, y;

    if (!g_page_effect_state.effect_busy || g_page_effect_state.act_scr == NULL) {
        lv_page_effect_inertia_reset();
        return;
    }

    now = lv_tick_get();

    switch (s_inertia.phase) {
    case LV_PAGE_INERTIA_FREE: {
        int32_t steps, i;
        bool stop = false;

        /* 按真实经过时间推进若干步, 不假设每次正好是一个周期(丢帧时不至于走短) */
        steps = (int32_t)((now - s_inertia.t_last) / (uint32_t)LV_PAGE_INERTIA_PERIOD_MS);
        if (steps < 1) {
            steps = 1;
        } else if (steps > 4) {
            steps = 4;
        }
        s_inertia.t_last = now;

        x = g_page_effect_state.touch_x;
        y = g_page_effect_state.touch_y;
        for (i = 0; i < steps; i++) {
            s_inertia.v *= LV_PAGE_INERTIA_DECAY;
            s_inertia.vy *= LV_PAGE_INERTIA_DECAY;
            x += (int32_t)(s_inertia.v * (float)LV_PAGE_INERTIA_PERIOD_MS);
            y += (int32_t)(s_inertia.vy * (float)LV_PAGE_INERTIA_PERIOD_MS);
        }

#if LV_PAGE_INERTIA_SNAP_ENABLE
        {
            int32_t d = s_inertia.target - x;
            if (d < 0) {
                d = -d;
            }
            if (d <= LV_PAGE_INERTIA_SNAP_ENTER_PX) {
                lv_page_effect_inertia_enter_snap(x);
                break;
            }
        }
#endif
        /* 速度衰减到门槛以下就停; 时间上限只是丢帧/时钟异常的兜底 */
        if ((s_inertia.v > -LV_PAGE_INERTIA_V_MIN && s_inertia.v < LV_PAGE_INERTIA_V_MIN &&
             s_inertia.vy > -LV_PAGE_INERTIA_V_MIN && s_inertia.vy < LV_PAGE_INERTIA_V_MIN) ||
            (now - s_inertia.t_phase0) >= (uint32_t)LV_PAGE_INERTIA_FREE_MAX_MS) {
            stop = true;
        }

        /* 位置先喂出去, 再收尾: 最后一帧停在速度归零的那个姿态上 */
        jlgpu_set_page_effect_x_anim(g_page_effect_state.act_scr, x);
        g_page_effect_state.y_offset = y - g_page_effect_state.touch_y;
        g_page_effect_state.touch_y = y;
        lv_obj_invalidate(g_page_effect_state.act_scr);

        if (stop) {
            INERTIA_LOG("stop tx=%d ty=%d vx_milli=%d vy_milli=%d ms=%d",
                        (int)x, (int)y, (int)(s_inertia.v * 1000.0f), (int)(s_inertia.vy * 1000.0f),
                        (int)(now - s_inertia.t_phase0));
            lv_page_effect_inertia_stop();
        }
        break;
    }

#if LV_PAGE_INERTIA_SNAP_ENABLE
    case LV_PAGE_INERTIA_SNAP: {
        uint32_t el = now - s_inertia.t_phase0;
        float p = (float)el / (float)s_inertia.settle_ms;
        float e;

        if (p > 1.0f) {
            p = 1.0f;
        }
        e = 1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p);
        /* 末帧直接喂精确落点, 把浮点/取整误差清零 -> 落点精确对齐整面,
         * 与退出帧的 identity 整屏 blit 是同一姿态, 提交瞬间不跳 */
        x = (p >= 1.0f) ? s_inertia.target
            : (s_inertia.x_phase0 + (int32_t)((float)s_inertia.d0 * e));

        jlgpu_set_page_effect_x_anim(g_page_effect_state.act_scr, x);
        lv_obj_invalidate(g_page_effect_state.act_scr);

        if (p >= 1.0f) {
            /* 已对齐; 等下一帧(让对齐帧真正画出去)再反查正面并提交 */
            s_inertia.phase = LV_PAGE_INERTIA_ARMED;
            s_inertia.t_arm = now;
        }
        break;
    }

    case LV_PAGE_INERTIA_ARMED:
        if ((now - s_inertia.t_arm) >= (uint32_t)LV_PAGE_INERTIA_PERIOD_MS) {
            const sticky_effect_ops_t *ops = sticky_effect_get_ops();
            if (ops) {
                lv_page_effect_inertia_commit(ops);
            } else {
                lv_page_effect_inertia_reset();
            }
        }
        break;
#endif

    default:
        break;
    }
}

//跟手定时器
static void timer_cb(lv_timer_t *timer)
{
    int32_t x;
    int32_t y;
    uint8_t status;
    static uint8_t first_down = 0;
    static uint8_t g_prev_touch_status = 0; //上一轮触摸状态
    static int32_t g_first_touch_x = 0; //记录定时器首次按下坐标(x)
    static int32_t g_first_touch_y = 0; //记录定时器首次按下坐标(y)

    get_touch_x_y_status(&x, &y, &status);
    //printf("-----x,y,status= %d,%d,%d\n",x,y,status);

    //针对sticky特效
    //检测定时器期间首次触摸按下，记录坐标
    if (status && !g_prev_touch_status) {
        g_first_touch_x = x;
        g_first_touch_y = y;
    }
    g_prev_touch_status = status;

    //水波纹：振幅/方向由绘制阶段按分界线速度统一驱动(手指跟手 + 按键/松手动画都适用)。
    //此处负责两件事:
    // 1) 手指按住停住/回直阶段持续重绘, 让波纹逐帧衰减(分界线不动时绘制阶段会衰减振幅);
    // 2) 动画结束(分界线到边界)进入 ripple_flatten 后, 波纹回直到0再收尾, 避免瞬间消失太突兀。
    if (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_RIPPLE) {
        if (g_page_effect_state.act_scr) {
            if ((status || g_page_effect_state.ripple_flatten) && g_page_effect_state.ripple_amp > 0) {
                lv_obj_invalidate(g_page_effect_state.act_scr);
            }
            if (g_page_effect_state.ripple_flatten && g_page_effect_state.ripple_amp == 0) {
                jlgpu_page_effect_flatten_done();
            }
        }
    }

    //松手惯性甩动:跟手松手后由这里按周期推进(自由减速 -> 吸附 -> 提交)。
    //此期间手指已抬起, 正常分支不会做任何事; 一旦重新按住就立刻取消惯性、交回跟手。
    if (s_inertia.phase != LV_PAGE_INERTIA_IDLE) {
        if (status) {
            lv_page_effect_inertia_cancel();
        } else {
            lv_page_effect_inertia_tick();
            return;
        }
    }

    //特效期间触摸按下
    if (g_page_effect_state.effect_busy && status && g_page_effect_state.a_old) {
        //暂停动画
        if (!lv_anim_is_paused(g_page_effect_state.a_old)) {
            lv_anim_pause(g_page_effect_state.a_old);
            lv_anim_pause(g_page_effect_state.a_new);
        }

        //保留第一下按下的坐标，检测滑动方向
        if (!first_down) {
            g_page_effect_state.start_x = x;
            g_page_effect_state.start_y = y;
            first_down = 1;

            //根据首次触摸位置判断滑动方向
            lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
            int32_t fb_w = lv_display_get_horizontal_resolution(disp);
            int32_t first_touch = lv_port_get_first_touch_x();
            if (first_touch > fb_w / 2) {
                //触摸点在右半部分：从右往左滑（旧页在左，新页从右进入）
                g_page_effect_state.swipe_left = 1;
            } else {
                //触摸点在左半部分：从左往右滑（旧页在右，新页从左进入）
                g_page_effect_state.swipe_left = 0;
            }
            g_page_effect_state.swipe_ready = 1;

            /* sticky 特效：重置 touch_x/touch_y 避免动画→触控接管时 offset 跳变。
             * 接惯性的模式改用「绝对基准」: touch_x 必须始终等于 wrapper 内部累计角度
             * (它也是松手提交时 resume a_old 的起点), 所以按下时不把它清零, 只记下基准,
             * 跟手位移在下面叠加到基准上。LRB_MIRROR 依赖"直写 0"做复位检测, 保持原样。 */
            if (lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
                if (lv_page_effect_inertia_supported()) {
                    lv_page_effect_inertia_cancel();
                    s_inertia.base_touch_x = g_page_effect_state.touch_x;
                } else {
                    g_page_effect_state.touch_x = 0;
                }
                g_page_effect_state.touch_y = 0;
            }
        }

        //水波纹浪尖实时跟随手指y
        g_page_effect_state.finger_y = y;

        //触摸接管：使用手指移动距离（delta）作为分界位置
        {
            int32_t first_touch;
            int32_t delta_x;
            bool is_sticky = lv_page_effect_need_sticky(g_page_effect_state.page_mode);
            bool moved = false;

            if (!is_sticky) {
                first_touch = lv_port_get_first_touch_x();
            } else {
                first_touch = g_first_touch_x;
            }

            if (is_sticky) {
                /* sticky 特效：使用原始位移（允许负值支持反向滑动）。
                 * g_first_touch_x - x：手指左移为正、右移为负，方向自带。 */
                delta_x = g_first_touch_x - x;
                /* 接惯性的模式: 位移叠加在按下时刻的绝对基准上, touch_x 仍是绝对位置,
                 * 这样松手提交时 a_old 才能直接从当前位置接上(见 inertia_commit)。 */
                if (lv_page_effect_inertia_supported()) {
                    delta_x += s_inertia.base_touch_x;
                }
            } else if (g_page_effect_state.swipe_left) {
                //从右往左滑：手指左移时 delta 增大
                delta_x = first_touch - x;
            } else {
                //从左往右滑：手指右移时 delta 增大
                delta_x = x - first_touch;
            }

            if (!is_sticky && delta_x < 0) {
                delta_x = 0;
            }
            if (!is_sticky) {
                //获取屏幕宽度用于 clamp（仅非 sticky 需要）
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_w = lv_display_get_horizontal_resolution(disp);
                if (delta_x > fb_w) {
                    delta_x = fb_w;
                }
            }

            if ((g_page_effect_state.touch_x != delta_x)) {
                jlgpu_set_page_effect_x_anim(g_page_effect_state.act_scr, delta_x);
                if (is_sticky && lv_page_effect_inertia_supported()) {
                    moved = true;
                }
            }

            /* sticky 特效：同步计算 y_offset（与 x_offset 逻辑一致） */
            if (is_sticky) {
                int32_t delta_y = g_first_touch_y - y;
                g_page_effect_state.y_offset = delta_y - g_page_effect_state.touch_y;
                if (g_page_effect_state.touch_y != delta_y) {
                    g_page_effect_state.touch_y = delta_y;
                    moved = true;
                }
                /* 只有位置真的变了才采样(含时间戳): 松手速度用 80ms 窗口差分算, 单位 px/ms。
                 * 按住不动不采样, 这样"按住再抬手"窗口内位移≈0 -> 不甩。 */
                if (moved) {
                    lv_page_effect_inertia_push_sample(g_page_effect_state.touch_x,
                                                       g_page_effect_state.touch_y);
                }
            }
        }

        //特效期间触摸抬起
    } else if (g_page_effect_state.effect_busy && !status && g_page_effect_state.a_old) {

        //两个页面的情况,交回给动画接管
        if (!lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
            /* 定点缩放特效的缩放中心取"上一次点击点": 这里按累计位移区分点击与滑动,
             * 只有点击(位移 <= 阈值)才记录, 滑动时不记录 —— 沿用上一次的点位。 */
            if (first_down) {
                int32_t first_touch = g_first_touch_x;
                int32_t moved_x = g_page_effect_state.swipe_left ? (first_touch - x) : (x - first_touch);
                if (moved_x < 0) {
                    moved_x = -moved_x;
                }
                if (moved_x <= LV_PAGE_CLICK_SWIPE_THRESHOLD) {
                    /* 记录按下点而非抬起点: 点击位置语义上是手指落下的地方,
                     * 且按下点不受"抬手瞬间的小抖动"影响。 */
                    s_point_center_x = g_first_touch_x;
                    s_point_center_y = g_first_touch_y;
                    s_point_center_valid = 1;
                }
            }

            //退出后需要把动画的current值更新上去
            if (lv_anim_is_paused(g_page_effect_state.a_old)) {

                int32_t first_touch = lv_port_get_first_touch_x();
                int32_t delta_x;
                if (g_page_effect_state.swipe_left) {
                    delta_x = first_touch - x;
                } else {
                    delta_x = x - first_touch;
                }
                if (delta_x < 0) {
                    delta_x = 0;
                }

                //获取屏幕宽度，判断分界线位置决定动画方向
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_width = lv_display_get_horizontal_resolution(disp);
                if (delta_x > fb_width) {
                    delta_x = fb_width;
                }

                if (delta_x <= fb_width / 2) {
                    //分界线在左半部分：取消切换，回弹到0（旧页面全屏）
                    lv_anim_set_values(g_page_effect_state.a_old, delta_x, 0);
                    lv_anim_set_current_value(g_page_effect_state.a_old, delta_x);

                    //目标页面改回旧页面
                    g_page_effect_state.end_page_index_id = 0;
                    g_page_effect_state.new_scr = g_page_effect_state.act_scr;
                } else {
                    //分界线在右半部分：继续切换，滑动到fb_width（新页面全屏）
                    lv_anim_set_values(g_page_effect_state.a_old, delta_x, fb_width);
                    lv_anim_set_current_value(g_page_effect_state.a_old, delta_x);

                    //目标页面保持为新页面
                    g_page_effect_state.end_page_index_id = 1;
                }
                lv_anim_set_act_time(g_page_effect_state.a_new, g_page_effect_state.a_old->act_time);
                lv_anim_resume(g_page_effect_state.a_old);
                lv_anim_resume(g_page_effect_state.a_new);

                //松手后不再进入。水波纹例外：动画结束后还要靠 timer 驱动"回直再收尾", 不能停
                if (g_page_effect_state.page_mode != LV_SCR_LOAD_ANIM_RIPPLE) {
                    lv_timer_pause(timer);
                }
            } else {
                /* 快速滑动松手 / 纯动画（按键触发等）：
                 * a_old 未暂停，动画已在运行。用首次触摸位置判断方向。
                 * 纯动画场景下 lv_port_get_first_touch_x() 可能是旧值，
                 * 但不影响功能——动画回调已设默认方向作为兜底。 */
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                int32_t fb_w = lv_display_get_horizontal_resolution(disp);
                int32_t first_touch = lv_port_get_first_touch_x();

                if (first_touch > fb_w / 2) {
                    g_page_effect_state.swipe_left = 1; // 右→左
                } else {
                    g_page_effect_state.swipe_left = 0; // 左→右
                }
                g_page_effect_state.swipe_ready = 1;
                lv_obj_invalidate(g_page_effect_state.act_scr);
            }
            //多页面情况
        } else {
            //抬起
            if (first_down) {
                // 计算累计滑动距离，用于区分滑动和点击
                int32_t first_touch = g_first_touch_x;//lv_port_get_first_touch_x();
                int32_t delta_x;
                if (g_page_effect_state.swipe_left) {
                    delta_x = first_touch - x;
                } else {
                    delta_x = x - first_touch;
                }
                if (delta_x < 0) {
                    delta_x = -delta_x;
                }

                INERTIA_LOG("up sticky dx=%d thr=%d first_down=%d",
                            (int)delta_x, LV_PAGE_CLICK_SWIPE_THRESHOLD, (int)first_down);

                if (delta_x <= LV_PAGE_CLICK_SWIPE_THRESHOLD) {
                    // 点击：根据触摸坐标确定目标页面
                    int scr_id = -1;
                    int face_index = -1;
                    const sticky_effect_ops_t *ops = sticky_effect_get_ops();
                    if (ops) {
                        scr_id = ops->get_face_scr_id(x, y);
                        face_index = ops->get_face_index(x, y);
                    }

                    if (lv_anim_is_paused(g_page_effect_state.a_old) && scr_id != -1) {
                        if (lv_page_effect_inertia_supported()) {
                            /* 点选进页面 = 直接跳页, 退出过程不做任何旋转。
                             * 不改的话有一条"随机乱转"的路径: a_old 的值是建特效时设的 [0, 屏宽],
                             * 而 jlgpu_set_page_effect_x_anim 的语义是 x_offset = v - touch_x; touch_x = v,
                             * 所以 resume 后第一帧 v = path(act_time=0) = 0, 算出
                             *      x_offset = 0 - touch_x = -touch_x
                             * 一次性灌给 wrapper(1px = 0.5度)。跟手拖动/甩动攒下的 touch_x 有多大,
                             * 这一帧就硬转多少度 —— 转多少取决于抬手前拖了多远, 所以看着是"随机"的。
                             * 1) lv_page_effect_inertia_stop() 把 a_old 的值钉成 [touch_x, touch_x]
                             *    => 之后每帧 new_value == touch_x => x_offset 恒 0, 一帧都不转;
                             * 2) 两个动画时长一起归零 -> 下一轮 anim_timer 就完成 -> scr_anim_completed
                             *    -> effect_exit 由 jlgpu_ctx_list[end_page_index_id] 整屏 blit 目标页。
                             * 必须一起归零: a_new 在链表里排在 a_old 前面会先完成, 只归零 a_new 的话
                             * a_old 会在特效已经拆掉之后再空跑整个时长, 每帧 invalidate 一块已经不在台上的
                             * 旧 screen(prev_scr 甚至可能已被 lv_obj_delete)。 */
                            lv_page_effect_inertia_stop();
                            lv_anim_set_duration(g_page_effect_state.a_old, 0);
                            lv_anim_set_duration(g_page_effect_state.a_new, 0);
                            INERTIA_LOG("tap: direct jump face=%d scr=%d tx=%d ty=%d",
                                        face_index, scr_id,
                                        (int)g_page_effect_state.touch_x, (int)g_page_effect_state.touch_y);
                        }

                        lv_anim_resume(g_page_effect_state.a_old);
                        lv_anim_resume(g_page_effect_state.a_new);

                        //松手后不再进入
                        lv_timer_pause(timer);

                        //修改目标页面
                        lv_obj_t *new_page_obj = ui_get_setup_scr_obj(scr_id);
                        g_page_effect_state.new_scr = new_page_obj;
                        g_page_effect_state.end_page_index_id = (uint8_t)face_index;
                    }
                } else {
                    // 滑动（delta_x > 阈值）：按松手速度继续转, 自然减速后吸附到整面并提交。
                    // 不支持的场景里 inertia_begin 自己会 return, 等价于原来的"保持跟手状态"。
                    lv_page_effect_inertia_begin();
                }
            }
        }

        //清空标志
        first_down = 0;
        g_page_effect_state.start_x = x;
    }

}

void lv_page_effect_clean(void)
{
    int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
    int ctx_count = page_num;
    bool shared_ctx = lv_page_effect_is_reflection_mode();

    // reflection 模式 ctx 后半段和前半段共享指针，只释放前半段
    if (shared_ctx) {
        ctx_count = POLYTOPE_CONFIG;
    }

    for (int i = 0; i < ctx_count; i++) {
        if (g_page_effect_state.jlgpu_ctx_list[i]) {
            /* 与 enter 阶段 lv_jlgpu_ctx_init 之后补的那次 gate_leave 配平:
             * uninit 内部会无条件 gate_leave,这里必须先占门,否则计数被多减一次
             * (特效 ctx 常驻期间门是放开的,clean 时要把门重新拿回来) */
            jlgpu_gpu_gate_enter();
            lv_jlgpu_ctx_uninit(g_page_effect_state.jlgpu_ctx_list[i]);
            lv_free(g_page_effect_state.jlgpu_ctx_list[i]);
            g_page_effect_state.jlgpu_ctx_list[i] = NULL;
        }
    }
    // shared ctx 的后半段指针置空，避免悬空
    if (shared_ctx) {
        for (int i = ctx_count; i < page_num; i++) {
            g_page_effect_state.jlgpu_ctx_list[i] = NULL;
        }
    }

    /* 背景 ctx 在页面槽之外(槽 15), 上面的循环覆盖不到, 必须单独释放 */
    if (g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT]) {
        lv_jlgpu_ctx_t *bg_ctx = g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT];
        jlgpu_gpu_gate_enter();
        lv_jlgpu_ctx_uninit(bg_ctx);
        lv_free(bg_ctx);
        g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT] = NULL;
    }

    for (int i = 0; i < page_num; i++) {
        if (g_page_effect_state.global_matrix[i]) {
            lv_free(g_page_effect_state.global_matrix[i]);
            g_page_effect_state.global_matrix[i] = NULL;
        }
        if (g_page_effect_state.draw_area[i]) {
            lv_free(g_page_effect_state.draw_area[i]);
            g_page_effect_state.draw_area[i] = NULL;
        }
    }
}

int lv_page_effect_manager(lv_display_t *disp_drv, lv_draw_jlgpu_unit_t *unit, const lv_area_t *screen_area, uint8_t *px_map)
{
    if (g_page_effect_state.page_mode >= LV_SCR_LOAD_ANIM_OTHER_PAGE && g_page_effect_state.page_mode <= LV_SCR_LOAD_ANIM_USER) {
        //入场cb
        if (g_page_effect_state.effect_enter) {
            //固定环节
            g_page_effect_state.effect_enter = 0;
            g_page_effect_state.finger_y = -1;//水波纹浪尖默认屏幕中心
            /* 惯性状态必须每次特效从头开始: 上一次的落点/速度/采样环不能跨会话残留 */
            lv_page_effect_inertia_reset();
            INERTIA_LOG("enter mode=%d inertia=%d snap=%d",
                        (int)g_page_effect_state.page_mode,
                        (int)lv_page_effect_inertia_supported(),
                        (int)LV_PAGE_INERTIA_SNAP_ENABLE);
            g_page_effect_state.ripple_dir = 0;
            g_page_effect_state.ripple_amp = 0;
            g_page_effect_state.ripple_prev_div = -1;//未初始化, 首帧绘制时用当前分界线位置初始化
            g_page_effect_state.ripple_flatten = 0;

            /* 滑动方向必须在第一帧绘制前就定下来。
             * 管理器的 timer_cb 要到本次入场末尾才创建, 首帧时 swipe_ready 必然还是 0,
             * 于是首帧会走"方向未确定: 只画旧页面全屏"的退化分支(定点缩放还额外清一次屏),
             * 第二帧起才画真正的特效 —— 表现出来就是首帧正常、第二帧整屏跳一下。
             * 这里提前用与 timer_cb 相同的判据(首次按下点落在屏幕哪一半)锁存方向和 swipe_ready,
             * 让首帧直接画真正的特效。 */
            {
                lv_display_t *disp = lv_obj_get_display(g_page_effect_state.act_scr);
                if (disp) {
                    int32_t fb_w = lv_display_get_horizontal_resolution(disp);
                    int32_t first_touch = lv_port_get_first_touch_x();
                    if (first_touch > fb_w / 2) {
                        g_page_effect_state.swipe_left = 1; // 右→左
                    } else {
                        g_page_effect_state.swipe_left = 0; // 左→右
                    }
                    g_page_effect_state.swipe_ready = 1;
                }
            }

            /* 定点缩放特效的缩放中心 = 本次「点击」的按下点, 必须在这里就记下来。
             * 点击是抬手时才派发 LV_EVENT_CLICKED 的, 特效入场时手指已经松开; 而管理器的
             * 定时器(timer_cb)正是在本次入场末尾才创建, 整个手势期间一次都没跑过 ——
             * timer_cb 里那套 first_down 记录逻辑对点击路径根本不生效, s_point_center_valid
             * 一直是 0, 缩放中心就永远回落到屏幕中心。
             * 判据与 timer_cb 里保持一致: 手指已抬起 + 累计位移不超过阈值 = 点击。
             * 按下途中就进特效的(滑动触发)以及位移超阈值的一律不记, 沿用上一次点击点。 */
            if (g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_POINT_ENLARGE
                || g_page_effect_state.page_mode == LV_SCR_LOAD_ANIM_POINT_NARROW) {
                int32_t now_x = 0;
                int32_t now_y = 0;
                uint8_t now_status = 0;
                get_touch_x_y_status(&now_x, &now_y, &now_status);
                if (!now_status) {
                    int32_t first_touch = lv_port_get_first_touch_x();
                    int32_t moved_x = now_x - first_touch;
                    if (moved_x < 0) {
                        moved_x = -moved_x;
                    }
                    if (moved_x <= LV_PAGE_CLICK_SWIPE_THRESHOLD) {
                        /* 取按下点而非抬起点: 点击位置语义上是手指落下的地方,
                         * 也避开抬手瞬间的抖动。 */
                        s_point_center_x = first_touch;
                        s_point_center_y = lv_port_get_first_touch_y();
                        s_point_center_valid = 1;
                    }
                }
            }

            lv_jlgpu_task_list_free(unit);
            jlgpu_ctx_list_ori = unit->jlgpu_ctx;

            //初始化layer,区域为全屏,配置为显存buf,用来收集链表
            layer_initialize(disp_drv, &static_layer, screen_area);

            int page_num = lv_page_effect_get_page_num(g_page_effect_state.page_mode);
            if (page_num == 2) { //双页面特效
                g_page_effect_state.end_page_index_id = 1; //默认切换到新页面
                g_page_effect_state.jlgpu_ctx_list[0] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                g_page_effect_state.global_matrix[0] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                g_page_effect_state.draw_area[0] = lv_malloc_zeroed(sizeof(lv_area_t));
                lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[0]);
                jlgpu_gpu_gate_leave();   /* 特效 ctx 是常驻到特效结束的,不能一直占着推屏独占门 */

                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[0];
                lv_obj_redraw(&static_layer, g_page_effect_state.act_scr);//收集
                /* 定点缩放特效要对整链调光, 必须在收集刚完成、链还是"原样"时留一份逐节点快照。
                 * 链上各节点的混合模式和透明度本来就互不相同(文字恒 SRC_OVER/128, 图片按
                 * dsc->blend_mode, 透明度取 dsc->opa/2), 被页级改写后就再也还原不回来了。 */
                if (lv_page_effect_point_mode(g_page_effect_state.page_mode)) {
                    lv_jlgpu_task_list_snapshot(g_page_effect_state.jlgpu_ctx_list[0]);
                }

                g_page_effect_state.jlgpu_ctx_list[1] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                g_page_effect_state.global_matrix[1] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                g_page_effect_state.draw_area[1] = lv_malloc_zeroed(sizeof(lv_area_t));
                lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[1]);
                jlgpu_gpu_gate_leave();   /* 同上:特效 ctx 常驻 */

                unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[1];
                lv_obj_redraw(&static_layer, g_page_effect_state.new_scr);//收集
                if (lv_page_effect_point_mode(g_page_effect_state.page_mode)) {
                    lv_jlgpu_task_list_snapshot(g_page_effect_state.jlgpu_ctx_list[1]);
                }
            } else { //多页面特效
                lv_obj_t *page_obj;

                // 当前页面移到正面
                lv_page_effect_make_front();

                uint32_t page_id_arr[POLYTOPE_CONFIG];
                sticky_effect_get_ops()->get_page_id(page_id_arr);

                for (int i = 0; i < POLYTOPE_CONFIG; i++) {
                    page_obj = ui_get_setup_scr_obj(page_id_arr[i]);
                    g_page_effect_state.jlgpu_ctx_list[i] = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                    g_page_effect_state.global_matrix[i] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                    g_page_effect_state.draw_area[i] = lv_malloc_zeroed(sizeof(lv_area_t));
                    lv_jlgpu_ctx_init(g_page_effect_state.jlgpu_ctx_list[i]);
                    jlgpu_gpu_gate_leave();   /* 同上:特效 ctx 常驻 */

                    unit->jlgpu_ctx = g_page_effect_state.jlgpu_ctx_list[i];
                    if (page_obj) {
                        lv_obj_redraw(&static_layer, page_obj);//收集任务
                    }
                    // reflection 模式：倒影面可以复用本体 ctx（仅矩阵和 draw_area 独立），
                    // 但需要单独分配 global_matrix 和 draw_area（每个面的投影矩阵不同）
                    if (lv_page_effect_is_reflection_mode()) {
                        int refl_idx = i + POLYTOPE_CONFIG;
                        g_page_effect_state.jlgpu_ctx_list[refl_idx] = g_page_effect_state.jlgpu_ctx_list[i]; // 指针复用
                        g_page_effect_state.global_matrix[refl_idx] = lv_malloc_zeroed(sizeof(jlgpu_global_matrix_t));
                        g_page_effect_state.draw_area[refl_idx] = lv_malloc_zeroed(sizeof(lv_area_t));
                    }
                }

                sticky_effect_get_ops()->init(unit, true);

                /* sticky 特效进入时，a_old 动画在 scr_load_anim_start 被 pause 前可能已执行过
                 * 1 帧，残留的 x_offset/y_offset 会让第一帧多面体带一个小角度偏转、页面不正对屏幕。
                 * 这里在第一次 busy 绘制前清零，保证第一帧正面正对屏幕。
                 * 注意不能动 touch_x：保持与 a_old 动画值一致，松手点击 resume 时才不会产生位移跳变。 */
                g_page_effect_state.x_offset = 0;
                g_page_effect_state.y_offset = 0;

                /* 背景: 和页面完全一样地收进一个常驻 ctx, 每帧绘制时用恒等矩阵整屏 blit 在最底层。
                 * 挂在槽 15(页面/倒影面之外), lv_page_effect_clean() 里单独释放。 */
                if (s_page_effect_bg_obj) {
                    lv_jlgpu_ctx_t *bg_ctx = lv_malloc_zeroed(sizeof(lv_jlgpu_ctx_t));
                    if (bg_ctx) {
                        g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT] = bg_ctx;
                        lv_jlgpu_ctx_init(bg_ctx);
                        jlgpu_gpu_gate_leave();   /* 同上:特效 ctx 常驻 */
                        /* 显式写死不透明(128 = 不透明): 任务链节点默认就是这个值, 这里只是防呆,
                         * 避免将来有人把背景槽和镜像槽共用一条链而被 slot_alpha 压暗。 */
                        lv_jlgpu_task_list_set_global_alpha(bg_ctx, 128);
                        unit->jlgpu_ctx = bg_ctx;
                        lv_obj_update_layout(s_page_effect_bg_obj); /* 防 coords 还没刷新过 */
                        lv_obj_redraw(&static_layer, s_page_effect_bg_obj);

                        /* 没收到任务(对象不可见 / 图源为空)时不能留个空 ctx: 空链 run 什么都不写,
                         * 地板会残留上一帧像素。直接拆掉, 回落纯黑。 */
                        if (bg_ctx->jlgpu_task_num == 0) {
                            jlgpu_gpu_gate_enter();
                            lv_jlgpu_ctx_uninit(bg_ctx);
                            lv_free(bg_ctx);
                            g_page_effect_state.jlgpu_ctx_list[LV_PAGE_EFFECT_BG_SLOT] = NULL;
                        }
                    }
                }
            }

            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            //把new_layer的area清空，避免影响下一轮刷新
            layer_set_empty_area(&static_layer);
            g_page_effect_state.effect_busy = 1;
            //创建定时器
            if (static_timer == NULL) {
                static_timer = lv_timer_create(timer_cb, LV_DEF_REFR_PERIOD, NULL);
            }
        }//effect_enter

        if (g_page_effect_state.effect_busy) {
            lv_jlgpu_set_global_trans_param(unit, screen_area, g_page_effect_state.touch_x, g_page_effect_state.page_mode, px_map);
            lv_jlgpu_reset_global_trans_param();
            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            return 0;
        } //effect_busy

        //出场cb
        if (g_page_effect_state.effect_exit) {
            lv_jlgpu_set_global_trans_param(unit, screen_area, g_page_effect_state.touch_x, g_page_effect_state.page_mode, px_map);
            g_page_effect_state.effect_exit = 0;

            //清理内存
            lv_page_effect_clean();

            //temp_layer在链表中删除,无则忽略
            layer_remove_from_disp(disp_drv, &static_layer);

            //3d特效清理
            if (lv_page_effect_need_sticky(g_page_effect_state.page_mode)) {
                sticky_effect_get_ops()->uninit();
            }

            unit->jlgpu_ctx = jlgpu_ctx_list_ori;
            lv_jlgpu_task_list_free(unit);//所有链表的遗留内存都清理掉

            //标准矩阵
            lv_jlgpu_reset_global_trans_param();

            if (static_timer) {
                lv_timer_delete(static_timer);
                static_timer = NULL;
            }

            //清空所有标志
            jlgpu_ctx_list_ori = NULL;
            lv_page_effect_inertia_reset();
            g_page_effect_state.touch_x = 0;
            g_page_effect_state.touch_y = 0;
            g_page_effect_state.x_offset = 0;
            g_page_effect_state.y_offset = 0;
            g_page_effect_state.start_x = 0;
            g_page_effect_state.start_y = 0;
            g_page_effect_state.finger_y = -1;
            g_page_effect_state.ripple_dir = 0;
            g_page_effect_state.ripple_amp = 0;
            g_page_effect_state.ripple_prev_div = -1;
            g_page_effect_state.ripple_flatten = 0;
            g_page_effect_state.swipe_left = 0;
            g_page_effect_state.swipe_ready = 0;
            g_page_effect_state.act_scr = NULL;
            g_page_effect_state.new_scr = NULL;
            g_page_effect_state.page_mode = LV_SCR_LOAD_ANIM_NONE;
            g_page_effect_state.end_page_index_id = 0;

            return 0;
        }//effect_exit

    }

    return 1;
}

#endif
#endif
#endif /* #if CONFIG_LVGL_UI_ENABLE */
