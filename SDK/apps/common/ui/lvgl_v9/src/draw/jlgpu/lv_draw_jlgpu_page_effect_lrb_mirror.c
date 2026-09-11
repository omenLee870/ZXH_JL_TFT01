/*********************
 *      INCLUDES
 *********************/
#include "src/core/lv_refr_private.h"
#include "lv_draw_jlgpu.h"
#include "display/lv_display.h"
#include "math.h"
#include "lv_draw_jlgpu_page_effect_lrb_mirror.h"

/* 需要 lv_anim_is_paused / lv_anim_pause / lv_anim_resume 判断与恢复 manager 的 0->W
 * 动画, 以及自有"松手回正"逐帧 lv_timer —— lv_anim.h 会带入 lv_timer.h/lv_tick.h
 * (其内再带 lv_conf_internal.h, 提供 LV_DEF_REFR_PERIOD; lv_obj.h 链则提供 lv_obj_invalidate)。 */
#include "src/misc/lv_anim.h"

/* overlay 渐变要用 lv_malloc/lv_free/lv_memset(倒影渐隐的 ov_clut/ov_task 逐帧懒分配)。
 * 与 lv_anim.h 同属纯 lvgl 内部头(只引 lv_conf_internal.h/lv_types.h), 需在
 * app_config.h 之前引入, 避免与 app 层的 bool 重定义冲突。 */
#include "src/stdlib/lv_mem.h"

/* 临时点按诊断开关: 1 = get_face 打一行 printf(仅 manager 点按松手触发, 量小), 验证后置 0 */
#define LRB_TAP_DBG_EN 1

#if LRB_TAP_DBG_EN
/* 注意: 不能 #include <stdio.h>/"printf.h" —— 前者与 fs.h 的 FILE 冲突, 后者不在本
 * 模块 include 路径。直接按 SDK printf.h 的签名前向声明, 链接到平台自带 printf。 */
int printf(const char *format, ...);
#endif

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

/*********************
 *      DEFINES
 *********************/

/* ============================================================================
 * LRB_MIRROR(左右下镜像) 自研 wrap-coverflow + 无缝倒影 引擎
 *
 * 背景: 闭源 hexagon 反射库的刚性 6 棱柱模型无法同时满足
 *   "中页大而正 + 邻页可读地拉近 + 邻镜静止出画", 用户已拍板自研。
 * 模型: 页面 = 立在"地板"世界 y=0 上的卡(底边贴地), 可绕自身竖直中轴
 *   yaw / 侧移 x / 退深 z; 弱透视投影(与 hexagon get_cam fov60 同源):
 *     sx = W/2 + FX*x/(vd+z)
 *     sy = seam + FY*y/(vd+z)     seam = 地板线屏幕行
 *   所有卡底边(world y=0)都投影到 seam 行 -> 倒影与本体的接缝天然 0 间隙。
 * 倒影 = 卡关于地板 y→-y 镜像再投影: 本体顶边(y=-ch)翻到倒影底(y=+ch), 高度 = 本体高
 *   (满高真镜像, 不随 |a| 压缩 -> 无"折叠"); 上屏策略见 LRB_MIR_ALWAYS_ON:
 *   默认 1 = 所有可见页(|a|<=A_MAX)的倒影一直保留显示。
 * 环向: 6 页固定槽位, 第 p 页环向坐标 = p(mod 6); 中心偏移 C(页单位);
 *   该页相对中心 a = wrap(p - C) ∈ (-3,3]; 只画 |a|<=A_MAX。
 *   拖动 C: 0→1 时右邻滑向正中(左滑前进), 可连续跨多页环绕。
 * 画序: 远(|a|大) -> 近, 每页先镜后体, 中心最后在最上层。
 *
 * 常量表与仿真器 /tmp/cf.py 同名同值(default_cfg), 改参数先跑仿真器收敛。
 * ==========================================================================*/

#define LRB_PI       3.14159265358979f
#define LRB_POLYTOPE_SIX   6            /* 6 本体页(+6 镜像槽 = 12 槽) */
#define LRB_FOV_DEG  60.0f              /* 相机垂直/水平 fov(同 hexagon) */

/* ---- 世界/相机(与 cf.py default_cfg 一一对应) ---- */
#define LRB_VD       3.0f               /* vd: 视距; 越大全部越小 */
#define LRB_CARD_W   1.137f             /* card_w: 卡世界宽 */
#define LRB_CARD_H   1.0f               /* card_h: 卡世界高 */
#define LRB_NEI_YAW  30.0f              /* nei_g1: |a|=1 邻页 yaw(度), 朝观众转 */
#define LRB_NEI_Z1   0.62f              /* nei_z1: |a|=1 卡中心退深 */
#define LRB_NEI_GAP  9.0f               /* nei_gap: 邻页内边与中页边缘的屏幕间隙(px) */
#define LRB_FAR_Z    1.7f               /* far_z:  |a|>1 每多 1 额外退深 */
#define LRB_FAR_X    2.3f               /* far_x:  |a|>1 每多 1 额外外移(world x) */
#define LRB_A_MAX    2.1f               /* a_max: |a| 超过则整卡不画 */

/* ---- 倒影开关(无折叠): 满高真镜像 + 上屏门限 ----
 * 倒影一律"满高(w=1)": 卡关于地板(y->-y)的真镜像, 高度 = 本体高, 不随 |a| 压缩。
 * 旧版按 |a| 线性缩短倒影高度, 滑动中倒影被压扁 -> 观感"折叠"; 现高度恒 1(消除折叠)。
 * LRB_MIR_ALWAYS_ON 选上屏策略(编译期切换, 便于真机 A/B):
 *  1 = 所有页的倒影一直保留显示(只看 |a| <= A_MAX 的可见性, 不额外加门限);
 *  0 = 二值门限: |a| < LRB_MIR_SLOT_U - eps 才画 —— 中心页恒有; 静止邻页(|a|=1)不画
 *      => "停稳后只剩中间页倒影"; 拖动中邻页 |a|<1 立即有满高倒影。 */
#define LRB_MIR_ALWAYS_ON 1
#define LRB_MIR_SLOT_U   1.0f           /* 门限模式: 邻页静止槽 |a|(=1): 达到即无倒影 */
#define LRB_MIR_GATE_EPS 0.02f          /* 门限模式容差(约 6px 拖动量, 防浮点) */

/* ---- 真倒影: 倒影纵向渐隐(后置 overlay 钩子里画) ----
 * 本体实心镜像画完后, 在每个倒影四边形 bbox 上再叠一层竖直渐变, 让倒影像玻璃反射一样
 * 离接缝越远越淡, 而不是硬邦邦的同一份内容倒着画。
 * LRB_MIR_FADE_MODE 选渐隐实现, 编译期切换便于真机 A/B:
 *  0 = SRC_OVER 黑色 alpha 渐变覆盖(接缝 alpha0 -> 远边 alpha255 压黑): 实现最早, 但
 *      alpha 走低精度混合插值, 在 RGB565 上横纹明显, 开 dither/MSAA 改善有限;
 *  1 = MULTIPLY 灰度逐像素变暗(接缝白 255 -> 远边黑 0, 源恒不透明): 在色域内乘性变暗,
 *      不依赖 alpha 混合精度, 横纹更少, 还能保留倒影里的暗部细节。推荐。
 * 曲线由 LRB_MIR_FADE_BASE/HOLD/GAMMA 整形: BASE = 接缝起点的基础变暗量(0 = 接缝原样,
 * 越大整条倒影越深), HOLD = 顶部不渐隐占比, GAMMA = 淡出曲线指数(1 = 线性)。
 * RGB565 直写渐变要平滑, 需 fill.c 同款三件套: MSAA_8X(超采样查 clut, 摊平逐级跳变) +
 * dither(抖动量化) + 完整 257 级 clut。MSAA=0 时按最邻近取 clut -> 每行一级、呈可见横纹;
 * 若 MSAA 后仍有红/绿残留, 把 DITHER 拨 0 对比(彩色残留多来自抖动通道解耦)。 */
#define LRB_MIR_FADE_MODE   1           /* 0 = SRC_OVER alpha 压黑; 1 = MULTIPLY 灰度变暗(推荐) */
#define LRB_MIR_FADE_MSAA   1           /* 渐变渲染质量: 0 = NONANTIALIASED(最邻近, 会出横纹); 1 = MSAA_8X */
#define LRB_MIR_FADE_DITHER 1           /* 渐变抖动: 1 = 开(摊平 565 量化); 0 = 关(出纯台阶, A/B 用) */
#define LRB_MIR_FADE_LUT    8           /* 渐变 clut 分级: 2^8+1 = 257 级, 越高分层越不可见 */
#define LRB_MIR_FADE_HOLD   0.0f        /* 顶部不渐隐占比(0..1): 0 = 从接缝就开始淡出 */
#define LRB_MIR_FADE_BASE   0.35f       /* 接缝起点基础变暗量(0..1): 0 = 接缝处原样不变暗;
                                         * >0 = 从接缝起就压深(整条倒影更暗), 远边仍到最黑。
                                         * 观感"倒影起点就深一些"就调这个。 */
#define LRB_MIR_FADE_GAMMA  1.0f        /* 淡出曲线指数: 1 = 线性; >1 顶部更亮、越到远边越急 */
#define LRB_MIR_FADE_ALPHA  128         /* 全局 alpha: 与 lv_jlgpu 不透明约定一致(128 = 1.0) */

/* ---- 驱动 ---- */
#define LRB_PX_PER_PAGE_PX (__this->width)   /* 拖动/动画每页对应的像素(默认一屏一页) */
#define LRB_DIR       1.0f              /* 方向: +1=左滑/动画0->W 时右邻(slot+1)滑向正中; 真机方向相反改 -1 */
/* 中心偏移 C 不限幅: C 是"环相位"(周期 = 6 页环), 姿态全由 wrap(p-C) 决定, 对任意 C 有
 * 意义。去掉 ±6 硬限幅后同方向可连续环绕多圈, 不会转满一圈被顶死只能回滑。
 * 每次拖动/点按后的 settle/导航都会把中性点 base 重新锚到最近整数页, C 不会失控漂移。 */

/* ---- 点按导航 ---- */
#define LRB_TAP_CTR_U  0.5f             /* |u| 低于此的页=中央当前页: 点了=放大进入该页(无左右翻); 之上=邻页方向感知翻页 */

/* ---- 松手自动居中(拖动松手后吸附最近整页) ---- */
#define LRB_SETTLE_MS      240            /* 回正缓动时长(ms) */
#define LRB_SETTLE_EPS     0.005f         /* |C - round(C)| 小于此 = 已停整页, 不触发(~1.6px) */
#define LRB_SETTLE_PERIOD LV_DEF_REFR_PERIOD /* 回正逐帧定时器周期 = 屏幕刷新周期 */

/**********************
 *      TYPEDEFS
 **********************/

/* 屏幕四边形: 8 个 float = x0,y0 .. x3,y3 */
typedef struct {
    float x[4];
    float y[4];
} lrb_quad_t;

struct lrb_mirror_priv {
    lv_draw_jlgpu_unit_t *unit;
    u8 init;
    int width;                       /* 屏分辨率 */
    int height;
    float fx;                        /* 相机焦距(px/world): W/tan(fov/2) */
    float fy;
    float vd;
    float seam;                      /* 地板线屏幕行 = 倒影/本体共用边 */
    float hw;                        /* 卡半宽 world */
    float xm0;                       /* |a|=1 邻页中轴 world x(cf.py solve_xm) */
    u32 page_id[LRB_POLYTOPE_SIX];

    /* 帧驱动(绝对 touch_x + 进入时的中性基准):
     *   C = LRB_DIR * (touch_x - base_x) / 页像素
     * 进入特效时 manager 把 a_old 动画中途 pause, touch_x 会残留非零值;
     * 以 base_x 归零 -> 首帧/静止(未触摸)前页必在正中(与 hexagon 用 x_offset 归零同理)。
     * manager 按下把 touch_x 直写 0(不走 x_offset)时, 通过 x_offset!=Δtouch_x 判为复位,
     * 平移 base_x 使 C 连续(不跳页)。 */
    int base_x;
    int t_last;
    u8 frame_init;
    int seq;                         /* render_frame 已跑次数(诊断: 命中缓存是否曾被刷新) */
    int gen;                         /* init() 被调用次数(诊断: 是否经历 uninit->init 重入) */

    /* 上一帧命中缓存(manager 点按回调与 move 不同步, 用最近一帧姿态判点) */
    u8 drawn[LRB_POLYTOPE_SIX];      /* 本体本帧是否上屏 */
    u8 mir_drawn[LRB_POLYTOPE_SIX];  /* 倒影本帧是否上屏 */
    float page_u[LRB_POLYTOPE_SIX];  /* 该页 |a|, 用于就近命中 */
    lrb_quad_t body[LRB_POLYTOPE_SIX];
    lrb_quad_t mir[LRB_POLYTOPE_SIX];

    /* 点按导航接管: manager 点按 resume 的是固定 0->W 动画(a_old exec 把 touch_x
     * 单调推进到 W)。两种接管方式, 都由动画进度 p 驱动:
     *  - 邻页(nav_enter=0): C = nav_c0 + nav_delta*p, delta 带符号(右邻+1/左邻-1),
     *    目标页从它所在侧滑向正中 —— 翻页方向随点按侧;
     *  - 中央页(nav_enter=1): C 不用, 直接把目标页卡从封面位放大到全屏
     *    (lrb_render_enter), 无左右翻页感; 铺满后 manager commit 切真实页, 无跳变。
     * 两者 p>=1 前把驱动中性点搬到终点, 交还正常驱动不跳页。 */
    u8 nav_active;
    u8 nav_enter;                  /* 1 = 中央页"放大进入"; 0 = 邻页方向翻页 */
    int nav_page;                  /* 目标页(槽位) */
    float nav_c0;                  /* 点按瞬间的 C(翻页起点姿态) */
    float nav_delta;               /* 目标页相对 nav_c0 的带符号距离(页单位) */
    int nav_v0;                    /* 首个动画帧 touch_x; -1 = 未捕获 */
    int nav_v1;                    /* 动画终点 touch_x(= 屏宽, 与 a_old 0->W 一致) */
    float enter_card_bottom;       /* 放大进入: 本帧目标卡底边屏幕 y(水平)。倒影随卡一起
                                    * 放大并贴在该边下方; overlay 据此把渐隐起点夹到卡底以外,
                                    * 避免把卡底内容一起压暗(见 lrb_mirror_fade_overlay)。 */

    /* 松手自动居中: manager 滑动松手(>10px)不做任何事也不重绘, coverflow 会停在拖动
     * 半途。LRB 用自有 lv_timer 逐帧把 C 缓动吸附到最近整页 —— 不动 touch_x, 每帧只
     * 改写 base_x(令 drive_C() 恰返回目标 C), 再 invalidate 让正常 move() 出帧。 */
    u8 settle_active;              /* 是否正在缓动回正 */
    float settle_c0;               /* 松手起点 C */
    float settle_c1;               /* 吸附目标 = roundf(起点 C) */
    u32 settle_t0;                 /* 缓动起点 tick(lv_tick_get) */
    lv_timer_t *settle_timer;      /* 回正逐帧定时器(init 建 / uninit 删) */

    float last_C;                  /* 最近一帧实际渲染的中心 C(命中/导航用真实姿态;
                                    * manager 按下会把 touch_x 直写 0, drive_C() 会失真) */

    /* 真倒影纵向渐隐(后置 overlay): 复用管理器的 ctx 句柄编码"单节点渐变"画在每个倒影
     * bbox 上, 不改动页 ctx 的任务链(不污染), 也不建自己的 ctx(避免重复初始化硬件)。
     * 资源懒分配: ov_clut/ov_task 首帧分配, uninit 释放。 */
    u8 ov_ready;                   /* 渐隐资源是否已就绪 */
    uint32_t *ov_clut;             /* alpha 渐隐查找表: (1<<LUT)+1 项 ARGB8888(黑, alpha 0->255) */
    uint8_t *ov_task;              /* 渐变任务节点缓冲(逐镜复用) */
    u32 ov_task_cap;               /* 已分配缓冲字节数 */
};

/**********************
 *  STATIC VARIABLES
 **********************/
static struct lrb_mirror_priv lrb_mirror_priv_t = {0};
#define __this (&lrb_mirror_priv_t)

#if LV_USE_DRAW_JLGPU_LIST

/* 读触摸按下/抬起状态(回正定时器判断"松手"; 与 manager 同款 extern 方式声明) */
extern void get_touch_x_y_status(int32_t *x, int32_t *y, uint8_t *status);

/* ========================== 矩阵小工具(本地实现, 避免跨 lib 依赖) ========================== */

/* 3x3 逆(row-major)。退化返回 0。 */
static int lrb_mat3_invert(const float *m, float *o)
{
    float d = m[0] * (m[4] * m[8] - m[5] * m[7])
              - m[1] * (m[3] * m[8] - m[5] * m[6])
              + m[2] * (m[3] * m[7] - m[4] * m[6]);
    if (d > -1e-9f && d < 1e-9f) {
        return 0;
    }
    float inv = 1.0f / d;
    o[0] =  (m[4] * m[8] - m[5] * m[7]) * inv;
    o[1] = -(m[1] * m[8] - m[2] * m[7]) * inv;
    o[2] =  (m[1] * m[5] - m[2] * m[4]) * inv;
    o[3] = -(m[3] * m[8] - m[5] * m[6]) * inv;
    o[4] =  (m[0] * m[8] - m[2] * m[6]) * inv;
    o[5] = -(m[0] * m[5] - m[2] * m[3]) * inv;
    o[6] =  (m[3] * m[7] - m[4] * m[6]) * inv;
    o[7] = -(m[0] * m[7] - m[1] * m[6]) * inv;
    o[8] =  (m[0] * m[4] - m[1] * m[3]) * inv;
    return 1;
}

/* 闭式解 源内容矩形(0,0)-(iw,ih) -> 屏幕四边形 q(顺序 TL,TR,BR,BL)。
 * H 为 source->screen 正向, row-major, H22=1。
 * (与 Python 逐行同源, 已对任意单应验证到 ~1e-14) */
static void lrb_hom_rect_to_quad(const lrb_quad_t *q, float iw, float ih, float *H)
{
    float x0 = q->x[0], y0 = q->y[0];
    float x1 = q->x[1], y1 = q->y[1];
    float x2 = q->x[2], y2 = q->y[2];
    float x3 = q->x[3], y3 = q->y[3];

    float dx1 = x1 - x2, dy1 = y1 - y2;
    float dx2 = x3 - x2, dy2 = y3 - y2;
    float dx3 = x0 - x1 + x2 - x3, dy3 = y0 - y1 + y2 - y3;
    float den = dx1 * dy2 - dy1 * dx2;

    float g = 0.0f, hh = 0.0f;
    if (den > 1e-9f || den < -1e-9f) {
        g  = (dx3 * dy2 - dy3 * dx2) / den;
        hh = (dx1 * dy3 - dy1 * dx3) / den;
    }

    float a = x1 - x0 + g * x1, b = x3 - x0 + hh * x3, c = x0;
    float d = y1 - y0 + g * y1, e = y3 - y0 + hh * y3, f = y0;

    H[0] = a / iw;
    H[1] = b / ih;
    H[2] = c;
    H[3] = d / iw;
    H[4] = e / ih;
    H[5] = f;
    H[6] = g / iw;
    H[7] = hh / ih;
    H[8] = 1.0f;
}

/* ========================== 页面姿态(与 cf.py 同源) ========================== */

/* 把环向坐标折入 (-3, 3]: a = wrap(p - C) */
static float lrb_wrap(float x)
{
    x = x - 6.0f * floorf((x + 3.0f) / 6.0f);
    if (x <= -3.0f) {
        x += 6.0f;
    }
    return x;
}

static float lrb_minf(float a, float b)
{
    return a < b ? a : b;
}
static float lrb_maxf(float a, float b)
{
    return a > b ? a : b;
}

/* 倒影是否上屏。高度恒满高(见 lrb_card_quad 传 w=1), 不随 |a| 缩短 —— 消除滑动中的"折叠"。
 * LRB_MIR_ALWAYS_ON=1: 恒返回 1(所有页倒影一直显示, 只看外部 |a|<=A_MAX 可见性);
 * =0: 二值门限 |a| < 静止槽(1) - eps 才画(静止邻页无倒影)。 */
static int lrb_mirror_on(float u)
{
#if LRB_MIR_ALWAYS_ON
    (void)u;
    return 1;
#else
    return (u < LRB_MIR_SLOT_U - LRB_MIR_GATE_EPS) ? 1 : 0;
#endif
}

/* 求 a(页单位)对应卡的世界四角并投影到屏幕。
 * body != 0: 本体(顶边 world y=-ch); body == 0: 倒影(顶边 world y=+w*ch, 底边共用 y=0)。
 * 输出 q[0..3] 顺序即源内容角点 TL,TR,BR,BL 的落点:
 *   - 本体: 内容顶行落卡上边(正立);
 *   - 倒影: 内容顶行落到接缝下方(垂直翻转镜像, 内容底行贴 seam)。
 * 横截面段端点: 近边(靠中心/靠前)与远边(外侧/退后)。关键: 内容"左列"该放哪一边
 * 取决于页面在中心哪侧 —— a>0(右卡)近边是它的左缘, 内容左列在近边(正常读);
 * a<0(左卡)近边是它的右缘, 内容左列必须在远边, 否则左卡整体被水平镜像(内容左右颠倒)。
 * (与 /tmp/cf.py _card_columns 同源) */
static void lrb_card_quad(float a, int body, float w, lrb_quad_t *q)
{
    float s = (a < 0.0f) ? -1.0f : 1.0f;
    float u = (a < 0.0f) ? -a : a;
    float ch = LRB_CARD_H;
    float hw = __this->hw;

    float ytop = body ? -ch : (w * ch); /* 卡顶/镜顶 world y */

    float f1 = lrb_minf(u, 1.0f);
    float f2 = lrb_maxf(u - 1.0f, 0.0f);
    float yaw = LRB_NEI_YAW * LRB_PI / 180.0f * lrb_minf(u, 1.0f);
    float zc = LRB_NEI_Z1 * f1 + LRB_FAR_Z * f2;
    float xm = s * (__this->xm0 * f1 + LRB_FAR_X * f2);
    float cosy = cosf(yaw);
    float siny = sinf(yaw);

    float xl = xm - hw * cosy;                 /* 内容左列 */
    float xr = xm + hw * cosy;                 /* 内容右列 */
    float zl = zc - s * hw * siny;             /* 左列纵深(近边侧 -; 远边侧 +) */
    float zr = zc + s * hw * siny;

    float xw[4] = { xl, xr, xr, xl };
    float zw[4] = { zl, zr, zr, zl };
    float yw[4] = { ytop, ytop, 0.0f, 0.0f };

    for (int i = 0; i < 4; i++) {
        float d = __this->vd + zw[i];
        q->x[i] = (float)__this->width / 2.0f + __this->fx * xw[i] / d;
        q->y[i] = __this->seam + __this->fy * yw[i] / d;
    }
}

/* 四边形是否与屏相交 */
static int lrb_quad_visible(const lrb_quad_t *q)
{
    float minx = q->x[0], maxx = q->x[0], miny = q->y[0], maxy = q->y[0];
    for (int i = 1; i < 4; i++) {
        if (q->x[i] < minx) {
            minx = q->x[i];
        }
        if (q->x[i] > maxx) {
            maxx = q->x[i];
        }
        if (q->y[i] < miny) {
            miny = q->y[i];
        }
        if (q->y[i] > maxy) {
            maxy = q->y[i];
        }
    }
    return !(maxx < 0 || minx >= __this->width || maxy < 0 || miny >= __this->height);
}

/* 顶点带权重四边形 bbox -> 屏幕矩形 draw_area(夹到屏内) */
static void lrb_quad_bbox_area(const lrb_quad_t *q, lv_area_t *area)
{
    float minx = q->x[0], maxx = q->x[0], miny = q->y[0], maxy = q->y[0];
    for (int i = 1; i < 4; i++) {
        if (q->x[i] < minx) {
            minx = q->x[i];
        }
        if (q->x[i] > maxx) {
            maxx = q->x[i];
        }
        if (q->y[i] < miny) {
            miny = q->y[i];
        }
        if (q->y[i] > maxy) {
            maxy = q->y[i];
        }
    }
    int x1 = (int)floorf(minx);
    int x2 = (int)ceilf(maxx) - 1;
    int y1 = (int)floorf(miny);
    int y2 = (int)ceilf(maxy) - 1;
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 > __this->width - 1) {
        x2 = __this->width - 1;
    }
    if (y2 > __this->height - 1) {
        y2 = __this->height - 1;
    }
    area->x1 = x1;
    area->y1 = y1;
    area->x2 = x2;
    area->y2 = y2;
}

/* 把"源内容 -> 屏幕四边形"的单应写入槽位 slot 的 fw/bw。
 * 约定(与 page_*_get_matrix 一致): fw = source->screen, bw = screen->source(GPU 采样用) */
static void lrb_slot_apply_matrix(int slot, const lrb_quad_t *q)
{
    jlgpu_global_matrix_t *gm = g_page_effect_state.global_matrix[slot];
    float H[9];

    lrb_hom_rect_to_quad(q, (float)__this->width, (float)__this->height, H);

    gm->trans_en = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            gm->fw_mat.m[i][j] = H[i * 3 + j];
        }
    }
    lrb_mat3_invert(H, (float *)&gm->bw_mat.m[0][0]);

    lrb_quad_bbox_area(q, g_page_effect_state.draw_area[slot]);
}

/* 点是否在凸四边形内(跨符号判向) */
static int lrb_quad_contains(const lrb_quad_t *q, float px, float py)
{
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) & 3;
        float ax = q->x[j] - q->x[i], ay = q->y[j] - q->y[i];
        float bx = px - q->x[i],   by = py - q->y[i];
        float cr = ax * by - ay * bx; /* 叉积; 本引擎顶点均为逆时针布序 */
        if (cr < 0.0f) {
            return 0;
        }
    }
    return 1;
}

/* ========================== 每帧几何(被 move 调用) ========================== */

/* 当前驱动量 C: 绝对 touch_x - 中性基准 -> 页单位中心偏移(与命中测试共用)。
 * C 不限幅(见 LRB_DIR 下注释): 连续同向拖动跨过整数环后仍单调增长, 环相位由
 * lrb_wrap 折回 (-3,3], 页面姿态周期 6 页 —— 松开后的 settle roundf(C) 吸附到最近
 * 整数页, 整数 C 与 C±6 是同一环姿态, 吸附目标跨环一致, 无跳页。 */
static float lrb_drive_C(void)
{
    return LRB_DIR * (float)(g_page_effect_state.touch_x - __this->base_x)
           / (float)LRB_PX_PER_PAGE_PX;
}

/* 按给定中心偏移 C(页单位)解算一帧的姿态/矩阵/画序并填进 g_page_effect_state。
 * C 的来源: 手指拖动= lrb_drive_C(); 点按导航= nav_c0+nav_delta*p(见 move)。 */
static void lrb_render_frame(float C)
{
    __this->last_C = C;   /* 记录本帧实际渲染的中心: 命中/导航取真实姿态而非 live drive_C */
    uint8_t *do_ = g_page_effect_state.draw_order;
    int ord[12];
    int n = 0;

    /* pass1: 逐页算姿态/矩阵/面积, 记录可见页 */
    for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
        __this->drawn[p] = 0;
        __this->mir_drawn[p] = 0;

        float a = lrb_wrap((float)p - C);
        float u = (a < 0.0f) ? -a : a;
        __this->page_u[p] = u;
        if (u > LRB_A_MAX) {
            continue;
        }

        /* 本体 */
        lrb_quad_t *bq = &__this->body[p];
        lrb_card_quad(a, 1, 1.0f, bq);
        if (!lrb_quad_visible(bq)) {
            continue; /* 本体完全出画则整页不画(与仿真一致) */
        }
        __this->drawn[p] = 1;
        lrb_slot_apply_matrix(p, bq);          /* 本体槽 = p */

        /* 倒影(同一内容, 槽 = p+6): 满高真镜像(不压缩), 上屏门限见 lrb_mirror_on */
        if (lrb_mirror_on(u)) {
            lrb_quad_t *mq = &__this->mir[p];
            lrb_card_quad(a, 0, 1.0f, mq);
            if (lrb_quad_visible(mq)) {
                __this->mir_drawn[p] = 1;
                lrb_slot_apply_matrix(p + LRB_POLYTOPE_SIX, mq);
            }
        }
    }

    /* pass2: 画家序 远(|a|大) -> 近; 每页先镜后体。
     * 注意: 用本地 pending 快照做"未排入"标记, 不动 __this->drawn[]
     * (drawn[] 是命中测试的缓存, 须保留本帧真实可见状态)。 */
    u8 pending[LRB_POLYTOPE_SIX];
    for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
        pending[p] = __this->drawn[p];
    }
    for (int far = 0; far < LRB_POLYTOPE_SIX; far++) {
        int pick = -1;
        float best = -1.0f;
        for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
            if (pending[p] && __this->page_u[p] > best) {
                best = __this->page_u[p];
                pick = p;
            }
        }
        if (pick < 0) {
            break;
        }
        pending[pick] = 0; /* 已取走 */
        if (__this->mir_drawn[pick]) {
            ord[n++] = pick + LRB_POLYTOPE_SIX;
        }
        ord[n++] = pick;
    }

    /* 填 draw_order, 余位 0xFF(>= page_num 即跳过) */
    for (int i = 0; i < LRB_POLYTOPE_SIX * 2; i++) {
        do_[i] = (i < n) ? (uint8_t)ord[i] : 0xFF;
    }

    __this->seq++;
}

/* 点中中央页的"进入"渲染: 背景保持静止封面(邻页竖条/倒影不动, 无跳变), 前景目标卡
 * 从封面位放大到全屏。p∈[0,1]: p=0 即封面卡原大小; p=1 卡恰好铺满整屏
 * = 真实页全屏渲染 -> a_new 完成时 manager commit 切到真实页, 无可见跳变。
 * 邻页被卡两侧盖过、镜像被卡下沿压过, 自然消失; 无左右翻页感 = "点中央页 -> 进入该页"。
 * (p 已在调用处 ease; C = 点按瞬间的中心偏移, 决定背景邻页/倒影姿态) */
static void lrb_render_enter(int page, float p, float C)
{
    /* 1) 静止封面整帧(含邻页与目标页小卡与倒影), 目标页 a≈0 必为最末绘制 */
    lrb_render_frame(C);

    /* 2) 目标卡: 封面位(a=0 本体, 与上一步小卡同位置同内容) -> 全屏矩形 逐角插值 */
    lrb_quad_t r;                          /* TL,TR,BR,BL */
    lrb_card_quad(0.0f, 1, 1.0f, &r);

    float fw = (float)__this->width;
    float fh = (float)__this->height;
    const float gx[4] = { 0.0f, fw, fw, 0.0f };
    const float gy[4] = { 0.0f, 0.0f, fh, fh };

    lrb_quad_t q;
    for (int i = 0; i < 4; i++) {
        q.x[i] = r.x[i] + (gx[i] - r.x[i]) * p;
        q.y[i] = r.y[i] + (gy[i] - r.y[i]) * p;
    }

    /* 覆盖回目标槽(内容不变, 只换姿态): 缓存与矩阵都指到放大后的卡 */
    __this->body[page] = q;
    __this->page_u[page] = 0.0f;
    lrb_slot_apply_matrix(page, &q);       /* draw_order 不变: 目标卡本就最末, 盖在最上层 */

    /* 3) 倒影与本体一起放大: 关于卡"当前底边"做镜像(卡端正时底边=地板线 seam)。
     * p=0 时底边恰在 seam, 镜像与静止倒影完全重合 -> 起手无跳变, 不再出现"倒影瞬间消失";
     * p->1 卡底边落到屏底(385), 倒影整条随之滑出屏外 -> "本体铺满后倒影自然消失"。
     * 底边水平(两底角等高), 故只需按 y 翻转; 顶点布序与 lrb_card_quad 倒影一致
     * (q[0]/q[1] 落倒影远端, q[2]/q[3] 贴接缝), 内容仍是垂直镜像。
     * 倒影滑出屏后 bbox 退化(draw_area 反向), manager 重放循环自带跳过, 安全。 */
    float yb = q.y[3];
    lrb_quad_t mq;
    for (int i = 0; i < 4; i++) {
        mq.x[i] = q.x[i];
        mq.y[i] = 2.0f * yb - q.y[i];
    }
    __this->mir[page] = mq;
    __this->enter_card_bottom = yb;
    lrb_slot_apply_matrix(page + LRB_POLYTOPE_SIX, &mq);
}

/* ========================== 真倒影纵向渐隐(后置 overlay) ========================== */

/* 懒分配/填充渐隐查找表。索引 0 = 接缝(da.y1, 不淡), 大 index = 远边(da.y2, 最淡);
 * LRB_MIR_FADE_MODE=1(MULTIPLY): 不透明 白255 -> 黑0, 逐像素乘性变暗(推荐);
 * LRB_MIR_FADE_MODE=0(SRC_OVER): RGB 恒 0(黑), alpha 0 -> 255 压黑。 */
static void lrb_overlay_ensure(void)
{
    uint32_t n = (1u << LRB_MIR_FADE_LUT);
    uint32_t *c = lv_malloc(sizeof(uint32_t) * (n + 1u));
    if (c == NULL) {
        return;
    }
    for (uint32_t i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        float x = (t - LRB_MIR_FADE_HOLD) / (1.0f - LRB_MIR_FADE_HOLD);
        if (x < 0.0f) {
            x = 0.0f;
        }
        if (x > 1.0f) {
            x = 1.0f;
        }
        float z = LRB_MIR_FADE_BASE
                  + (1.0f - LRB_MIR_FADE_BASE) * powf(x, LRB_MIR_FADE_GAMMA); /* 0 = 不淡, 1 = 远边最淡 */
        if (z < 0.0f) {
            z = 0.0f;
        }
        if (z > 1.0f) {
            z = 1.0f;
        }
#if LRB_MIR_FADE_MODE
        uint32_t g = (uint32_t)((1.0f - z) * 255.0f + 0.5f);
        c[i] = (255u << 24) | (g << 16) | (g << 8) | g;   /* ARGB8888 不透明灰: 白->黑 */
#else
        uint32_t ai = (uint32_t)(z * 255.0f + 0.5f);
        if (ai > 255u) {
            ai = 255u;
        }
        c[i] = (ai << 24);   /* ARGB8888: RGB 恒 0(黑), A 从接缝 0 渐增到远边 255 */
#endif
    }
    __this->ov_clut = c;
    __this->ov_ready = 1;
}

/* 在每个本帧已上屏的倒影四边形 bbox 上叠加竖直渐隐 -> 倒影像玻璃反射一样越远离接缝越
 * 淡, 到自身远边恰好消失(下方本就是黑地板)。与普通镜像本质区别: 普通镜像是"同一份内容
 * 再画一遍只是倒过来", 此层让远端内容被逐渐减淡, 形成"反射衰减"观感。逐镜 bbox 独立叠:
 * 矮倒影在自身远边就淡完, 不把下面的其它页一起压暗。
 * 实现见 LRB_MIR_FADE_MODE: 0 = SRC_OVER 黑色 alpha 压黑; 1 = MULTIPLY 不透明灰度乘性
 * 变暗(色域内操作, alpha 混合精度不再参与, 横纹更少)。
 *
 * 实现上复用 manager 的 ctx 句柄(不新建, 避免重复初始化硬件): 每镜把渐变编成"独立单节
 * 点"(不复用页 ctx 任务链, 不污染 12 槽共享 ctx), 窗口 sfr_set 到该镜 bbox、全局矩阵
 * 复位恒等后 jlgpu_run 立即出。
 */
static void lrb_mirror_fade_overlay(lv_draw_jlgpu_unit_t *unit, uint8_t *px_map,
                                    const lv_area_t *screen_window)
{
    if (!__this->init || unit == NULL || unit->jlgpu_ctx == NULL) {
        return;
    }
    /* 中央页"放大进入"接管中: 目标卡逐帧铺满全屏。倒影已随本体一起放大并滑向屏底
     * (见 lrb_render_enter), 渐隐照常叠 —— 否则倒影一进动画就丢掉暗部("阴影消失")。
     * 但放大中的卡会盖过邻近页的倒影区, 若这些倒影仍从各自 seam(304) 起叠渐隐,
     * 就会把卡底内容一起压暗。故进入期间把每个倒影的渐隐起始行夹到"当前卡底边"以外。
     * p=0 时卡底边 = seam, 夹取等价于不夹 -> 起手与进入前完全一致, 不跳变。 */
    int enter_clip = (__this->nav_active && __this->nav_enter);
    int clip_top = enter_clip ? (int)(__this->enter_card_bottom + 0.999f) : 0;
    if (!__this->ov_ready) {
        lrb_overlay_ensure();
        if (!__this->ov_ready) {
            return;   /* 分配失败: 退化为原实心镜像(不淡出) */
        }
    }

    int fbw = lv_area_get_width(screen_window);   /* 行跨距 = out_buf 偏移基准 */

    for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
        if (!__this->mir_drawn[p]) {
            continue;
        }
        lv_area_t da = *g_page_effect_state.draw_area[p + LRB_POLYTOPE_SIX];
        /* 与 manager 重放循环同一套屏幕内夹取(存储的 draw_area 可能带出屏部分):
         * 保证 out_buf 指针与 sfr 窗口都不越界。 */
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
        /* 放大进入: 渐隐只从"卡底边以下"起, 不压到卡内容(见上 clip_top 说明) */
        if (enter_clip && da.y1 < clip_top) {
            da.y1 = clip_top;
        }
        if (da.x1 > da.x2 || da.y1 > da.y2) {
            continue;
        }

        jlgpu_lineargrad_param_t lineargrad_param;
        lineargrad_param.point_start.x = da.x1;
        lineargrad_param.point_start.y = da.y1;   /* 接缝: clut index 0, 不淡(alpha0 / 白 255) */
        lineargrad_param.point_end.x = da.x1;
        lineargrad_param.point_end.y = da.y2;     /* 远边: clut 末项, 最淡(alpha255 黑 / 黑 0) */
        lineargrad_param.spread_mode = JLGPU_COLOR_RAMP_SPREAD_PAD;

        jlgpu_matrix_t matrix;
        jlgpu_computeLinearParameters(LRB_MIR_FADE_LUT, &lineargrad_param, &matrix);

        jlgpu_draw_grad_t draw_grad = {0};
        jlgpu_mask_param_t mask_params = {0};
        jlgpu_render_param_t render_params = {0};
        jlgpu_paint_inter_params_t inter_params;

        draw_grad.clut_level = LRB_MIR_FADE_LUT;
        draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;
        draw_grad.paint_info.premult = 1;
        draw_grad.paint_info.pt_clip.x_min = -4096;
        draw_grad.paint_info.pt_clip.x_max = 4095;
        draw_grad.paint_info.pt_clip.y_min = -4096;
        draw_grad.paint_info.pt_clip.y_max = 4095;
        draw_grad.lineargrad_ptr = &lineargrad_param;
        draw_grad.grad_mode = JLGPU_GRAD_MODE_LINEAR_GRADIENT;
        draw_grad.surface2paint = &matrix;
        draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;
        draw_grad.clut_adr = (uint8_t *)__this->ov_clut;

        inter_params.coeff0 = 0;
        inter_params.coeff1 = 0;
        inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;
        draw_grad.grad_inter_params = &inter_params;

        render_params.layer_en = 1;
        render_params.gb_trans_en = 1;
        /* 渐变实现按 LRB_MIR_FADE_MODE: 0 = SRC_OVER 黑色压黑; 1 = MULTIPLY 灰度乘性变暗。 */
        render_params.blend_mode = LRB_MIR_FADE_MODE ? JLGPU_BLEND_MULTIPLY : JLGPU_BLEND_SRC_OVER;
        render_params.global_alpha = LRB_MIR_FADE_ALPHA;
        /* 渲染质量可 A/B(横纹与插值路径相关): 0 = NONANTIALIASED(普通纹理合成同款);
         * 1 = MSAA_8X(fill.c 渐变配方, 实测对横纹改善有限且更耗). dither 抖动量化。 */
        render_params.render_quality = LRB_MIR_FADE_MSAA ? JLGPU_RENDERING_QUALITY_MSAA_8X
                                       : JLGPU_RENDERING_QUALITY_NONANTIALIASED;
        render_params.dither_en = LRB_MIR_FADE_DITHER;   /* 抖动量化, 摊平 565 渐变分层 */
        render_params.render_area.x_min = da.x1;
        render_params.render_area.x_max = da.x2 + 1;
        render_params.render_area.y_min = da.y1;
        render_params.render_area.y_max = da.y2 + 1;

        mask_params.mask_mode = JLGPU_MASK_MODE_NONE;

        int task_len = jlgpu_draw_grad_task_len(unit->jlgpu_ctx->ctx, &draw_grad, &mask_params);
        if (task_len <= 0) {
            continue;
        }
        if ((u32)task_len > __this->ov_task_cap) {
            uint8_t *nb = lv_malloc(task_len);
            if (nb == NULL) {
                continue;
            }
            if (__this->ov_task) {
                lv_free(__this->ov_task);
            }
            __this->ov_task = nb;
            __this->ov_task_cap = (u32)task_len;
        }
        lv_memset(__this->ov_task, 0, task_len);

        jlgpu_set_task(unit->jlgpu_ctx->ctx, __this->ov_task, task_len);
        jlgpu_draw_grad(unit->jlgpu_ctx->ctx, &render_params, &draw_grad, &mask_params);

        /* 单节点不含几何变换: 窗口夹到本镜像 bbox, 全局矩阵复位为恒等再跑(与 manager
         * 每槽先 sfr_set 再 set_global_trans 的语义一致)。 */
        uint8_t *out_buf = px_map + ((uint32_t)fbw * (uint32_t)da.y1 + (uint32_t)da.x1) * 2u;
        lv_jlgpu_sfr_set(unit, &da, (uint32_t)fbw, out_buf, JLGPU_FORMAT_RGB565);
        lv_jlgpu_reset_global_trans_param();
        jlgpu_run(unit->jlgpu_ctx->ctx, __this->ov_task);
    }
}

/* ========================== 命中测试 ========================== */

/* 点是否命中页面并把命中页/是否倒影写回。
 * 1) 先查最近一帧缓存(drawn[]/body[]/mir[] = 屏幕正在显示的那帧姿态);
 * 2) 缓存为空(可能被 uninit 清过)或未命中时, 现场按当前驱动量 C 重算各页屏幕四边形判点
 *    (与 render_frame 同一公式/限幅, 无副作用, 与屏幕同源)。
 * 命中集合里取最前页(|u| 最小)。返回 page 0..5, 无命中 -1。
 * via_live 输出: 0 = 命中最近一帧缓存; 1 = 缓存未命中, 现场重算命中。 */
static int lrb_pick_any(int x, int y, int *out_is_mir, int *via_live)
{
    float px = (float)x;
    float py = (float)y;
    int best = -1;
    int best_mir = 0;
    float best_u = 1e9f;
    *via_live = 0;

    /* 1) 缓存路径 */
    for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
        if (__this->drawn[p] && lrb_quad_contains(&__this->body[p], px, py)) {
            if (__this->page_u[p] < best_u) {
                best_u = __this->page_u[p];
                best = p;
                best_mir = 0;
            }
        }
        if (__this->mir_drawn[p] && lrb_quad_contains(&__this->mir[p], px, py)) {
            if (__this->page_u[p] < best_u) {
                best_u = __this->page_u[p];
                best = p;
                best_mir = 1;
            }
        }
    }
    if (best >= 0) {
        *out_is_mir = best_mir;
        return best;
    }

    /* 2) 现场重算路径 */
    *via_live = 1;
    float C = lrb_drive_C();
    for (int p = 0; p < LRB_POLYTOPE_SIX; p++) {
        float a = lrb_wrap((float)p - C);
        float u = (a < 0.0f) ? -a : a;
        if (u > LRB_A_MAX) {
            continue;
        }
        lrb_quad_t bq;
        lrb_card_quad(a, 1, 1.0f, &bq);
        if (!lrb_quad_visible(&bq)) {
            continue;
        }
        if (lrb_quad_contains(&bq, px, py)) {
            if (u < best_u) {
                best_u = u;
                best = p;
                best_mir = 0;
            }
        }
        if (lrb_mirror_on(u)) {
            lrb_quad_t mq;
            lrb_card_quad(a, 0, 1.0f, &mq);
            if (lrb_quad_visible(&mq) && lrb_quad_contains(&mq, px, py)) {
                if (u < best_u) {
                    best_u = u;
                    best = p;
                    best_mir = 1;
                }
            }
        }
    }
    *out_is_mir = best_mir;
    return best;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_lrb_mirror_effect_set_page_index 配置 LRB 页面(环向: 槽 p 的环向坐标 = p)
 */
/* ------------------------------------------------------------------------------------*/
void lv_lrb_mirror_effect_set_page_index(uint32_t page_id, uint32_t page_index)
{
    if (page_index >= LRB_POLYTOPE_SIX) {
        __this->page_id[LRB_POLYTOPE_SIX - 1] = page_id;
    } else {
        __this->page_id[page_index] = page_id;
    }
}

void lv_lrb_mirror_effect_get_page_id(uint32_t *page_id_arr)
{
    for (int i = 0; i < LRB_POLYTOPE_SIX ; i++) {
        page_id_arr[i] = __this->page_id[i];
    }
}

int lv_lrb_mirror_effect_get_face_scr_id(int x, int y)
{
    int is_mir;
    int via_live;
    int page = lrb_pick_any(x, y, &is_mir, &via_live);

    /* 命中页相对"屏幕当前姿态中心"的 |a| 与中心 C。
     * 优先用最近一帧缓存(page_u/last_C = 屏幕正显示的姿态): manager 按下会把 touch_x
     * 直写 0(不走 x_offset), 松手前若又没产出新帧, live drive_C() 会因 base_x 未跟随而
     * 失真 —— 此时必须按屏幕上真实姿态判 |a|/记 C, 否则把已居中的前页误判成邻页。
     * 缓存被 uninit 清空(罕见)才落到现场重算(live 用 drive_C, 与屏幕同源)。 */
    float u = -1.0f;
    float Cpose = 0.0f;
    if (page >= 0) {
        if (!via_live) {
            u = __this->page_u[page];
            Cpose = __this->last_C;
        } else {
            Cpose = lrb_drive_C();
            float a = lrb_wrap((float)page - Cpose);
            u = (a < 0.0f) ? -a : a;
        }
    }
    int scr_id = (page < 0) ? -1 : (int)__this->page_id[page];

    /* 点按接管 manager 即将 resume 的 0->W 动画(门条件与 manager 点按分支一致:
     * 须 a_old 暂停态, 否则 manager 不会 resume, nav 挂了没人驱动)。
     *   |u| < 中央阈值: 返回的 scr_id = 当前中央页 -> manager commit 后即"进入该页",
     *     nav 走"放大进入"(nav_enter=1), 不翻页;
     *   否则: 邻页, nav 走方向翻页(右邻 +1 从右滑入 / 左邻 -1 从左滑入)。 */
    if (page >= 0 && !__this->nav_active
        && g_page_effect_state.a_old
        && lv_anim_is_paused(g_page_effect_state.a_old)) {
        __this->nav_c0 = Cpose;   /* 翻页/进入都从屏幕真实姿态起, 不跳姿态 */
        __this->nav_v0 = -1;      /* 首个动画帧在 move() 里捕获起点 */
        if (u < LRB_TAP_CTR_U) {
            __this->nav_enter = 1;          /* 中央页: 放大进入 */
            __this->nav_page = page;
            __this->nav_delta = 0.0f;
        } else {
            __this->nav_enter = 0;          /* 邻页: 方向翻页 */
            __this->nav_page = page;
            __this->nav_delta = lrb_wrap((float)page - Cpose);
        }
        __this->nav_active = 1;
    }

#if LRB_TAP_DBG_EN
    /* 仅 manager 点按分支(松手≤10px)会调本函数 —— 单行自答: 命中页/进入 or 翻页接管 */
    printf("[LRB]tap(%d,%d) busy=%d ap=%d enter=%d touch=%d hit=%d via=%d mir=%d "
           "u100=%d scr=%d gen=%d seq=%d C=%d nv=%d d100=%d\n",
           x, y,
           g_page_effect_state.effect_busy,
           (g_page_effect_state.a_old) ? lv_anim_is_paused(g_page_effect_state.a_old) : -1,
           g_page_effect_state.effect_enter,
           g_page_effect_state.touch_x,
           page, (page < 0) ? -1 : via_live, is_mir,
           (int)(u * 100.0f),
           scr_id, __this->gen, __this->seq,
           (int)(Cpose * 100.0f),
           __this->nav_active ? (__this->nav_enter ? 2 : 1) : 0,
           (int)(__this->nav_delta * 100.0f));
#endif
    return scr_id;
}

/* 兼容 ops->get_face_index: 返回命中的槽(0..11)。
 * 为保持旧 hexagon 语义(本体面索引 >= 本体数), 本体返回 p+6, 倒影返回 p。 */
static int lv_lrb_mirror_effect_get_face_index(int x, int y)
{
    int is_mir;
    int via_live;
    int page = lrb_pick_any(x, y, &is_mir, &via_live);
    if (page < 0) {
        return -1;
    }
    return is_mir ? page : page + LRB_POLYTOPE_SIX;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 松手自动居中逐帧驱动(自有 lv_timer, init 建 / uninit 删)
 *
 * manager 滑动松手(>10px)分支"不做任何操作, 保持跟手状态 / timer 继续运行" —— 既不 resume
 * a_old 也不重绘, coverflow 就停在拖动半途。LRB 不能改 manager, 只能自备节奏源: 每个
 * 刷新周期读一次触摸状态自驱回正:
 *   - 手指按住: 跟手, 取消进行中的回正(下个 move 帧 rebase 会自动衔接 C 不跳);
 *   - 手指松开且 C 偏离整页超 LRB_SETTLE_EPS: 起 240ms 缓动 C -> roundf(C)(吸附最近页),
 *     每帧把目标 C 编码进 base_x(drive_C() 恰返回它), 再 invalidate 由正常 move() 出帧。
 * 门条件: 特效在用 + a_old 暂停态 —— 动画/翻页/进入接管中绝不吸附(那是交回 a_old 走 commit)。
 * ------------------------------------------------------------------------------------*/
static void lrb_settle_tick(lv_timer_t *timer)
{
    (void)timer;
    if (!__this->init || !g_page_effect_state.effect_busy) {
        return;
    }
    if (__this->nav_active) {
        __this->settle_active = 0;      /* 翻页/进入接管中: 交还 a_old, 不吸附 */
        return;
    }
    if (!g_page_effect_state.a_old || !lv_anim_is_paused(g_page_effect_state.a_old)) {
        __this->settle_active = 0;      /* a_old 在跑(入场/导航/外部接管): 停用并丢弃旧缓动 */
        return;
    }

    int32_t tx;
    int32_t ty;
    uint8_t st;
    get_touch_x_y_status(&tx, &ty, &st);
    (void)tx;
    (void)ty;
    if (st) {
        __this->settle_active = 0;      /* 手指按住 = 跟手, 取消回正 */
        return;
    }

    float C = lrb_drive_C();
    float c1 = roundf(C);

    if (!__this->settle_active) {
        if ((C - c1) < LRB_SETTLE_EPS && (c1 - C) < LRB_SETTLE_EPS) {
            return;                      /* 已停整页(含未拖动的静止态), 无需吸附 */
        }
        __this->settle_c0 = C;
        __this->settle_c1 = c1;
        __this->settle_t0 = lv_tick_get();
        __this->settle_active = 1;
#if LRB_TAP_DBG_EN
        printf("[LRB]st(%d -> %d) touch=%d base=%d\n",
               (int)(C * 100.0f), (int)(c1 * 100.0f),
               g_page_effect_state.touch_x, __this->base_x);
#endif
    }

    uint32_t el = lv_tick_get() - __this->settle_t0;
    float p = (float)el / (float)LRB_SETTLE_MS;
    if (p > 1.0f) {
        p = 1.0f;
    }
    float e = p * p * (3.0f - 2.0f * p);                /* smoothstep */
    float Ca = __this->settle_c0
               + (__this->settle_c1 - __this->settle_c0) * e;

    /* 把 Ca 编码进 base_x: C = LRB_DIR*(touch-base)/px  =>  base = touch - C*px/LRB_DIR。
     * touch_x 松手后不再变, 每帧 drive_C() 恰返回 Ca, move() 正常出姿态帧。 */
    __this->base_x = g_page_effect_state.touch_x
                     - (int)(Ca * (float)LRB_PX_PER_PAGE_PX / LRB_DIR);

    if (p >= 1.0f) {
        __this->settle_active = 0;       /* 已到位: base_x 落整页, 之后静止即整页 */
#if LRB_TAP_DBG_EN
        printf("[LRB]st_done C=%d touch=%d base=%d\n",
               (int)(Ca * 100.0f), g_page_effect_state.touch_x, __this->base_x);
#endif
    }
    if (g_page_effect_state.act_scr) {
        lv_obj_invalidate(g_page_effect_state.act_scr);
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_lrb_mirror_effect_init 初始化(自研几何参数仅依赖屏分辨率)
 */
/* ------------------------------------------------------------------------------------*/
void lv_lrb_mirror_effect_init(lv_draw_jlgpu_unit_t *unit, bool list_flag)
{
    if (__this->init) {
        return;
    }
    (void)list_flag;

    lv_display_t *disp = lv_refr_get_disp_refreshing();
    __this->width  = lv_display_get_horizontal_resolution(disp);
    __this->height = lv_display_get_vertical_resolution(disp);
    __this->unit = unit;

    float half_fov = LRB_FOV_DEG * LRB_PI / 360.0f; /* fov/2 in rad */
    __this->fx = (float)__this->width  / tanf(half_fov);
    __this->fy = (float)__this->height / tanf(half_fov);
    __this->vd = LRB_VD;
    __this->hw = LRB_CARD_W / 2.0f;

    /* 地板线屏幕行: 使本体垂直中心恰在屏中线(H/2), 镜像向下出画被屏底截断 */
    __this->seam = (float)__this->height / 2.0f
                   + __this->fy * LRB_CARD_H / (2.0f * __this->vd);

    /* |a|=1 邻页中轴 world x(cf.py solve_xm): 令邻页内边 = 中页右缘 + 屏间隙 */
    {
        float yaw = LRB_NEI_YAW * LRB_PI / 180.0f;
        float cosy = cosf(yaw);
        float siny = sinf(yaw);
        float zn = LRB_NEI_Z1 - __this->hw * siny;
        float x_in = (float)__this->width / 2.0f
                     + __this->fx * __this->hw / __this->vd + LRB_NEI_GAP;
        __this->xm0 = (x_in - (float)__this->width / 2.0f)
                      * (__this->vd + zn) / __this->fx + __this->hw * cosy;
    }

    __this->base_x = g_page_effect_state.touch_x; /* 静止中性点: 首帧前页正对屏幕 */
    __this->t_last = __this->base_x;
    __this->frame_init = 1;
    __this->nav_active = 0;                       /* 新实例不带残留导航 */
    __this->nav_enter = 0;
    __this->seq = 0;                              /* 诊断: 本次实例已渲染帧数 */
    __this->gen++;                                /* 诊断: 实例代数(uninit->init 重入+1) */
    __this->last_C = 0.0f;                        /* 首帧前: 命中用 drive_C 兜底 */
    __this->settle_active = 0;

    /* 松手自动居中定时器: 特效实例生存期内常驻(manager 滑动松手不重绘, 需自有节奏源) */
    if (__this->settle_timer == NULL) {
        __this->settle_timer = lv_timer_create(lrb_settle_tick, LRB_SETTLE_PERIOD, NULL);
    }

    __this->init = 1;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_lrb_mirror_effect_uninit 反初始化(只清私有状态 + 释放倒影渐隐懒分配资源)
 */
/* ------------------------------------------------------------------------------------*/
void lv_lrb_mirror_effect_uninit(void)
{
    __this->init = 0;
    __this->nav_active = 0;
    __this->settle_active = 0;
    if (__this->settle_timer) {
        lv_timer_delete(__this->settle_timer);
        __this->settle_timer = NULL;
    }
    /* 倒影渐隐资源: 本次实例一次性分配, 退出释放(ov_ready 清 0, 下次 init 懒重建) */
    if (__this->ov_clut) {
        lv_free(__this->ov_clut);
        __this->ov_clut = NULL;
    }
    if (__this->ov_task) {
        lv_free(__this->ov_task);
        __this->ov_task = NULL;
    }
    __this->ov_task_cap = 0;
    __this->ov_ready = 0;
    for (int i = 0; i < LRB_POLYTOPE_SIX; i++) {
        __this->drawn[i] = 0;
        __this->mir_drawn[i] = 0;
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief lv_ui_lrb_mirror_move 每帧几何入口(manager busy 帧调用)
 *
 * 说明: 不依赖 xoffset 累加, 每帧直接用累计量 g_page_effect_state.touch_x
 * 解算全帧姿态, 避免拖动/动画/点击 resume 交错时的累积漂移。
 *
 * @param unit
 * @param xoffset  (未用)
 * @param yoffset  (未用)
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int lv_ui_lrb_mirror_move(lv_draw_jlgpu_unit_t *unit, int xoffset, int yoffset)
{
    (void)yoffset;
    if (!__this->init) {
        lv_lrb_mirror_effect_init(unit, false);
        if (!__this->init) {
            return 0;
        }
    }

    /* 点按导航接管帧: manager 已 resume a_old(0->W)并由 jlgpu_set_page_effect_x_anim
     * 把 touch_x 单调推进到 W。用该进度 p 驱动:
     *   邻页 = C 带方向行程(nav_delta 符号随点按侧);
     *   中央页 = 目标卡放大进入(lrb_render_enter), 铺满后 manager commit 切真实页。
     * 帧末把驱动中性点搬到终点, 交还正常驱动不跳页。 */
    if (__this->nav_active) {
        int t = g_page_effect_state.touch_x;
        if (__this->nav_v0 < 0) {
            __this->nav_v0 = t;             /* 首帧: 捕获动画起点 */
            __this->nav_v1 = __this->width; /* 动画终点 = 屏宽(a_old 的 0->W 终点) */
        }
        __this->t_last = t;                 /* 维持基准连续, 交还驱动时不跳 */
        int span = __this->nav_v1 - __this->nav_v0;
        float p = (span > 0) ? (float)(t - __this->nav_v0) / (float)span : 1.0f;
        if (p < 0.0f) {
            p = 0.0f;
        }
        if (p > 1.0f) {
            p = 1.0f;
        }
        if (__this->nav_enter) {
            /* 放大进入: 背景静止在点按前姿态, 目标卡铺向全屏, 无左右翻页。
             * p>=1 后保持铺满帧, 直到 a_new 完成 -> manager commit(effect_busy=0)结束特效。 */
            float pe = p * p * (3.0f - 2.0f * p);   /* smoothstep */
            lrb_render_enter(__this->nav_page, pe, __this->nav_c0);
            return 0;
        }
        float C = __this->nav_c0 + __this->nav_delta * p;
        lrb_render_frame(C);
        if (p >= 1.0f) {
            /* 翻页走完: 目标页居中; 令正常驱动在 C 处连续(下帧若还 busy 也不跳页) */
            __this->base_x = t - (int)(C * (float)LRB_PX_PER_PAGE_PX / LRB_DIR);
            __this->nav_active = 0;
        }
        return 0;
    }

    /* 帧驱动基准校正:
     * 真实滑动/动画帧, manager 通过 jlgpu_set_page_effect_x_anim 设
     *   x_offset = Δtouch_x, 即 xoffset(-x_offset) == -Δtouch_x;
     * 按下复位等对 touch_x 的直写不经 x_offset, 会失配 —— 此时平移 base_x
     * 保持 C 连续, 不因绝对驱动而在换手势/动画接管的瞬间跳页。 */
    int t = g_page_effect_state.touch_x;
    int dt = t - __this->t_last;
    __this->t_last = t;
    if (__this->frame_init) {
        __this->frame_init = 0;   /* 首帧 dt 无意义(基线即中性点) */
    } else if (xoffset != -dt) {
        __this->base_x += dt;
    }

    lrb_render_frame(lrb_drive_C());
    return 0;
}

static const sticky_effect_ops_t lrb_mirror_effect_ops = {
    .init            = lv_lrb_mirror_effect_init,
    .uninit          = lv_lrb_mirror_effect_uninit,
    .move            = lv_ui_lrb_mirror_move,
    .get_face_scr_id = lv_lrb_mirror_effect_get_face_scr_id,
    .get_face_index  = lv_lrb_mirror_effect_get_face_index,
    .get_page_id     = lv_lrb_mirror_effect_get_page_id,
    .overlay         = lrb_mirror_fade_overlay,
};

const sticky_effect_ops_t *lv_lrb_mirror_effect_get_ops(void)
{
    return &lrb_mirror_effect_ops;
}

#endif /* LV_USE_DRAW_JLGPU_LIST */
#endif /* #if CONFIG_LVGL_UI_ENABLE */
