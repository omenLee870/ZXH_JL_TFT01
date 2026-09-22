/*****************************************************************************
 * GPU 圆球光斑特效: 一群随机弹跳的半透明小球, 用径向渐变 CLUT 做成光斑, 直接
 * 挂到 GPU 任务链表上绘制(不占 LVGL 的 draw buffer, 也不占 CPU 软件光栅化时间)。
 *
 * 开销就两项, 都随 DEFINES 段的宏线性变化, 换配置按公式自己算:
 *
 *   1) 常驻(LVGL 堆, 开启期间一直占着):
 *        LV_BALL_EFFECT_NUM * sizeof(lv_ball_t)     <- 球状态数组
 *      + lv_timer_t + 分配器开销
 *      当前配置(30 个球)约 1.4KB。
 *
 *   2) 每帧(GPU 任务链表的内存池, 非常驻):
 *        LV_BALL_EFFECT_NUM * (CLUT 字节数 + 渐变任务字节数)
 *        CLUT = (2^BALL_GLOW_LUT_LEVEL + 1) * 4
 *        渐变任务长度由 jlgpu_draw_grad_task_len() 算出, 跟变换模式有关
 *      当前配置 30 * (68 + 80) = 4440B, 约占 JLGPU_LIST_MEM_POOL_SIZE
 *      (lv_conf.h, 默认 30KB)的 15%; layer == 0 时另加一块底板任务。
 *
 *   第 2 项是"借"的: 池在同一帧里被所有 UI 任务共用, 链表释放时 pool_pos 归零
 *   (lv_jlgpu_mem_pool_clean), 生命周期跟 GPU 任务链一致。池满时会自动退化成
 *   attach buf, 不会失败, 只是每帧多几次 malloc/free。
 *   双链表(LV_USE_DRAW_JLGPU_LIST == 2)下只有当帧真正提交的那个 unit 会挂到,
 *   另一个 unit 的池不受影响。
 *
 *   栈: 每次 lv_ball_insert_grad() 峰值几百字节(含栈上的临时 CLUT), 球是一个一个
 *   顺序挂的, 不累加。
 *
 * 极致省内存: 第 1 项对不开特效的产品是白占的, 确定用不上时调
 * lv_ball_effect_unload() 可以把球数组和定时器整个还给 LVGL 堆; 之后想再开,
 * lv_ball_effect_enable(true) 会自动重新分配。
 *****************************************************************************/

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_jlgpu.h"
#include "lv_draw_jlgpu_ball.h"
#include "../../misc/lv_area_private.h"
#include "../../misc/lv_color.h"
#include "../../misc/lv_math.h"
#include "../../misc/lv_timer.h"
#include "../../display/lv_display.h"
#include "../../indev/lv_indev.h"
#include "../../core/lv_obj.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE

#if LV_USE_DRAW_JLGPU
#if LV_USE_DRAW_JLGPU_LIST

/*********************
 *      DEFINES
 *********************/

/* 驱动定时器周期, 约 50fps */
#define BALL_PERIOD_MS          20//LV_DEF_REFR_PERIOD

/* 自由运动限速(px/帧) 与 阻尼 */
#define BALL_FREE_V_MAX         4.0f
#define BALL_FREE_V_MIN         0.35f
#define BALL_DAMP_FREE          0.995f

/* 跟手时的阻尼, 越大跟得越紧 */
#define BALL_DAMP_TOUCH         0.88f
/* 跟手限速(px/帧) */
#define BALL_TOUCH_V_MAX        9.0f

/* ===== 松手迸发 =====
 * 手指抬起时, 从松手点朝外给球一个冲量, "炸"开; burst 从 1 衰减到 0 的这
 * 段时间里速度上限抬高、额外加阻尼, 所以是短促的一下, 不是一直高速乱窜。
 *
 * 初速 = SPD_BASE + SPD_HEAT * heat: 松手点附近(被指尖喂亮的)那批球炸得最狠,
 * 远处的球也被冲击波推出去一把, 所以是整片一起散、近处特别猛。
 * SPD_BASE 要跟常态散开的初速比 (lv_ball_scatter() 给的那一档) ——
 * 比它小的话远处的球反而比原来还慢, 就不叫"更剧烈"了。
 * 想让迸发只发生在指尖附近那几个球上, 把 SPD_BASE 设成 0。 */
#define BALL_BURST_SPD_BASE     4.5f
#define BALL_BURST_SPD_HEAT     9.5f
/* 迸发的速度上限(常态是 BALL_FREE_V_MAX)。burst 在这两者之间插值。
 * 注意别让 SPD_BASE + SPD_HEAT 超过这个值, 超了第一帧就被削平, 白给 */
#define BALL_BURST_V_MAX        10.0f
/* burst = 1 时额外乘的阻尼, 随 burst 衰减自动收回常态阻尼 */
#define BALL_BURST_DAMP         0.90f
/* burst 每帧衰减系数: 0.9 约 37 帧(0.74s)从 1 落到 0.02 */
#define BALL_BURST_DECAY        0.90f
/* 径向方向上的随机偏角(±度), 免得同一方向的球炸出去排成一条射线 */
#define BALL_BURST_SPREAD_DEG   22

/* 每个定时周期里推进的物理子步数。
 * 整套运动学是按帧迭代的, 多迭代几遍等价于把仿真时间轴压缩: 轨迹形状、
 * 随机扰动的统计都不变, 只是整体运动变快。调这一个数就能整体提速。
 * 1 = 原速, 2 = 2 倍速, 3 = 3 倍速。 */
#define BALL_SUB_STEPS          1

/* ===== 光斑(失焦光点)效果 ===== */

/* CLUT 级数: 条目数 = 2^level + 1。越大径向衰减越细腻、越不容易出色带,
 * 代价是每个球每帧多占 (2^level+1)*4 字节内存池和一点 CPU。
 *   0 -> 2 条(没有衰减, 硬边圆盘)   3 -> 9 条   4 -> 17 条   5 -> 33 条
 * 想知道每帧多占多少内存池: LV_BALL_EFFECT_NUM * (2^level+1) * 4, 拿去和
 * lv_conf.h 的 JLGPU_LIST_MEM_POOL_SIZE 比。
 * 嫌费 CPU 就降一级; 注意别超过 6, 栈上的临时 CLUT 会到 1KB 以上。 */
#define BALL_GLOW_LUT_LEVEL     4

/* 中心混白程度(0~1): 0 = 纯球色, 1 = 全白。让核心看起来"过曝成一片白",
 * 而不是一块纯色 —— 这是光斑和色块最大的区别。 */
#define BALL_GLOW_CORE_WHITE    0.2f

/* 边缘亮环(散景圆盘的亮边), 在球色之上叠的亮度增量。
 * 0 = 只有柔和光晕, 没有亮边; 0.12 ~ 0.25 会浮出一圈镜头散景那样的亮边, 风格更强。 */
#define BALL_GLOW_RIM           0.12f

/* 亮度呼吸: 每个球的相位推进速度不同, 整片球各亮各的, 不会同步闪。
 * 数值是每帧相位增量(一圈 = 65536), 换成秒要乘 BALL_PERIOD_MS。 */
#define BALL_GLOW_PHASE_SPD_MIN 260
#define BALL_GLOW_PHASE_SPD_MAX 660

/* 呼吸幅度: 一个周期内系数能达到的最小值 / 最大值(1.0 = 不呼吸的基准)。
 *
 *   OPA_*  不透明度呼吸(alpha)
 *   LUM_*  明暗呼吸(整体缩放 RGB, 不改色相, 所以不是"色彩呼吸")
 *
 * 呼吸就是这两个轴一起做的。色相角在 lv_ball_randomize() 里随机定一次, 之后
 * 不参与呼吸 —— 所以小球的呼吸是"明暗 + 透明度", 没有色彩变化。
 *
 * 想让呼吸更明显, 关键不是把数值堆大, 而是让两个轴同相(见
 * BALL_GLOW_LUM_INVERT)。球画在背景上的实际贡献正比于 OPA系数 x LUM系数:
 *   同相  -> 两者相乘, 一个周期里 贡献从 MIN*MIN 冲到 MAX*MAX, 幅度直接叠起来;
 *   反相  -> 两者互相抵消, 整球亮度几乎不变, 只剩"通透/厚实"的质感在变。
 * 反相不是没用(有光在流转的感觉), 但整体明暗变化很弱。
 *
 * 削顶: 呼吸峰值一旦超过表达能力的上限就会被削平, 幅度白给。两个已知位置:
 *   - alpha: 基准 opa x OPA_MAX > 255 时上半程一直是满 alpha。基准 opa 的取值
 *     范围在 lv_ball_randomize() 里, 改 OPA_MAX 时两边要一起看;
 *   - 通道: LUM_MAX 超过 1.0 的溢出量在已经混白的核心会顶到 255, 只影响核心色,
 *     不影响外圈。 */
#define BALL_GLOW_OPA_MIN       0.45f
#define BALL_GLOW_OPA_MAX       1.60f
#define BALL_GLOW_LUM_MIN       0.65f
#define BALL_GLOW_LUM_MAX       1.35f

/* 明暗轴相对不透明度轴的相位:
 *   0 = 同相: 最实的同时最亮, 一呼一吸一目了然, 整体明暗变化最大
 *   1 = 反相: 最实的时候最暗, 通透但有光在流转, 整体明暗变化弱 */
#define BALL_GLOW_LUM_INVERT    1

/* ===== 手指按下时指尖附近小球的特殊化 ===== */

/* 指尖影响半径(px), 超出这个距离完全不受影响。
 * 按距离平方衰减 1-(d/R)^2, 指尖附近有一小片平顶, 球聚过来时那片会一起亮。 */
#define BALL_TOUCH_RADIUS       110.0f
/* heat 向目标值平滑逼近的比例, 越小越"慢热慢凉", 手指扫过不会硬切变 */
#define BALL_TOUCH_HEAT_SPEED   0.18f
/* 受激时的不透明度增益(1.0 = 不增亮) */
#define BALL_TOUCH_OPA_GAIN     1.35f
/* 受激时向白色的额外偏移量(0~1), 在 BALL_GLOW_CORE_WHITE 之上叠加 */
#define BALL_TOUCH_WHITE        0.55f
/* 受激时跟手增益的放大倍数: 指尖附近那几个球明显更"黏"、更快贴上去 */
#define BALL_TOUCH_K_GAIN       1.5f

#define BALL_GLOW_LUT_N         ((1 << BALL_GLOW_LUT_LEVEL) + 1)

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    float x;            /* 圆心(屏幕坐标 px) */
    float y;
    float vx;           /* 速度(px/帧) */
    float vy;
    float k;            /* 跟手增益, 每个球不同 -> 跟随速度不同 */
    float swirl;        /* 切向系数, 每个球不同 -> 轨迹弧线不同 */
    float r;            /* 半径 */
    lv_opa_t opa;       /* 半透明度 */
    lv_color_t color;

    /* --- 光斑 --- */
    uint16_t phase;     /* 亮度呼吸相位, 0~65535 一圈 */
    uint16_t phase_spd; /* 呼吸速度, 每个球不同 -> 不会同步闪 */
    float    heat;      /* 跟手受激程度 0~1, 平滑逼近指尖影响值 */
    float    burst;     /* 松手迸发程度 1~0, 每帧衰减; 0 = 常态物理 */
} lv_ball_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ball_effect_timer_cb(lv_timer_t *timer);
static lv_indev_t *lv_ball_get_pointer_indev(void);
static void lv_ball_randomize(lv_ball_t *b, int32_t w, int32_t h);
static void lv_ball_scatter(lv_ball_t *b);
static void lv_ball_burst(lv_ball_t *b, float fx, float fy);
static void lv_ball_step(lv_ball_t *b, int32_t w, int32_t h, bool touched, int32_t tx, int32_t ty);
static void lv_ball_build_clut(const lv_ball_t *b, uint32_t *clut, int32_t n);
static void lv_ball_apply_scr_transparent(lv_obj_t *scr);
static void lv_ball_restore_scr(void);
static void lv_ball_restore_bottom(void);
static bool lv_ball_need_base(void);
static void lv_ball_sync_scr_bg(void);
static void lv_ball_insert_grad(lv_draw_jlgpu_unit_t *unit, const lv_ball_t *b, uint16_t layer);
static void lv_ball_insert_base(lv_draw_jlgpu_unit_t *unit, int32_t w, int32_t h, uint16_t layer);

/**********************
 *  STATIC VARIABLES
 **********************/

/* 球状态数组。故意用堆指针而不是静态数组: 静态数组是 BSS, 一旦定义就常驻,
 * 卸载也收不回来; 指针改成惰性分配(见 lv_ball_effect_enable)后, 不开特效时
 * 这 1.3KB 根本不占, lv_ball_effect_unload() 也能完整还回去。
 * 安全不变式: ball_active == true 蕴含 balls != NULL —— 分配发生在置
 * ball_active 之前, 释放发生在 ball_active 清零之后。 */
static lv_ball_t *balls = NULL;
static bool ball_active = false;    /* 实际已经在跑 */
static bool ball_want_active = false;/* 应用层想要的状态, 可能因为屏幕还没建好而暂时没生效 */
static lv_timer_t *ball_timer = NULL;
static bool ball_touch_down = false;
/* 最后一次按下的指尖位置。松手那一帧 indev 已经是 RELEASED, 取不到坐标, 而迸发
 * 方向要从松手点算起, 所以按下的每一帧都记一份 */
static int32_t ball_touch_x = 0;
static int32_t ball_touch_y = 0;

/* 特效群插到 GPU 任务链的哪一层。0 = 链表头(最底层, 画在 LVGL 的 UI 之下, 默认);
 * 正数 = 插到该层之前; 负数 = 追加到链表尾(画在所有 UI 之上)。 */
static int32_t ball_layer = 0;

/* 被改成透明背景的屏幕, 以及它的原始 bg_opa / bg_color */
static lv_obj_t *ball_scr = NULL;
static lv_opa_t ball_scr_opa_saved = LV_OPA_COVER;
static lv_color_t ball_bg_color = {0};

/* 显示器的 bottom_layer, 同样需要打透明, 原因见 lv_ball_apply_scr_transparent() */
static lv_obj_t *ball_bottom = NULL;
static lv_opa_t ball_bottom_opa_saved = LV_OPA_COVER;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_ball_effect_init(void)
{
    if (ball_timer) {
        return;
    }

    lv_rand_set_seed(0x5A5A1234);

    ball_timer = lv_timer_create(lv_ball_effect_timer_cb, BALL_PERIOD_MS, NULL);
    if (ball_timer == NULL) {
        LV_LOG_ERROR("create ball timer fail");
        return;
    }

    /* 建出来就是暂停态: 光有定时器还开不起来(这会儿 display 可能还没建好), 也没人
     * 知道应用层想不想开。先停住, 等应用层调 lv_ball_effect_enable(true) 再跑 ——
     * 那个调用会 lv_timer_resume()。停着的时候定时器不占 CPU, 也不影响系统睡眠深度。 */
    lv_timer_pause(ball_timer);
}

bool lv_ball_effect_is_active(void)
{
    return ball_active;
}

void lv_ball_effect_set_layer(int32_t layer)
{
    if (ball_layer == layer) {
        return;
    }
    ball_layer = layer;

    /* 层号一变, "要不要自己兜底板"的结论可能也变了(见 lv_ball_need_base()),
     * 立刻同步一次屏幕背景状态。本函数在 LVGL 线程上下文调用, 改样式是安全的;
     * 但不要在渲染过程中(比如 disp_flush / 各 draw_xxx 回调里)调。 */
    if (ball_active) {
        lv_ball_sync_scr_bg();

        lv_obj_t *scr = lv_screen_active();
        if (scr) {
            lv_obj_invalidate(scr);
        }
    }
}

void lv_ball_effect_enable(bool en)
{
    ball_want_active = en;

    if (en == ball_active) {
        return;
    }

    if (en) {
        /* 兜底: 应用层没主动调过 lv_ball_effect_init() 就来了(常见情况, 定时器
         * 平时是没人建的)。少了这句, 定时器不存在 -> 球画出来之后没人推进物理、
         * 也没人再 invalidate, 表现为球定死不动且不报错。
         * init() 是幂等的, 建完是暂停态, 真正跑起来靠下面那句 resume。 */
        if (ball_timer == NULL) {
            lv_ball_effect_init();
        }

        lv_display_t *disp = lv_display_get_default();
        int32_t w = disp ? lv_display_get_horizontal_resolution(disp) : 0;
        int32_t h = disp ? lv_display_get_vertical_resolution(disp) : 0;
        if (w <= 0 || h <= 0) {
            /* 屏幕还没建好(应用层在 lv_init 之后、display 就绪之前就调了)。
             * 意图已经记在 ball_want_active 里, 把定时器叫起来让它重试:
             * timer_cb 里的补开分支会再调一次本函数, 那时屏幕已就绪。
             * 不能在这里自己重试 —— 拿不到屏幕就没法在正确的屏幕上做后续处理。 */
            if (ball_timer) {
                lv_timer_resume(ball_timer);
            }
            return;
        }

        /* 球状态数组按需分配(见 balls 的定义)。放在这里而不是 lv_ball_effect_init():
         * init 是 lv_draw_jlgpu_init() 在开机时无条件调的, 在那儿分配等于把 BSS
         * 换了个地方, 省不下来。放在"屏幕确实就绪、真要开"的这一步之前, 才能保证
         * 不开特效的系统一个字节都不占。分配必须先于 ball_active = true。 */
        if (balls == NULL) {
            balls = lv_malloc_zeroed(sizeof(lv_ball_t) * LV_BALL_EFFECT_NUM);
            if (balls == NULL) {
                LV_LOG_ERROR("ball: alloc %d bytes fail", (int)sizeof(lv_ball_t) * LV_BALL_EFFECT_NUM);
                return;     /* ball_active 仍是 false, 再调一次 enable(true) 会重试 */
            }
        }

        /* 屏幕自己的主题底色会被 GPU 链表上我们插的底板任务取代,
         * 所以把屏幕背景打透明, 否则 LVGL 会在球上面再糊一层背景色把球盖掉。
         * 插在上层时不需要动, 由 lv_ball_sync_scr_bg() 内部判断。 */
        lv_ball_sync_scr_bg();

        for (int i = 0; i < LV_BALL_EFFECT_NUM; i++) {
            lv_ball_randomize(&balls[i], w, h);
        }
        ball_touch_down = false;

        ball_active = true;
        if (ball_timer) {
            lv_timer_resume(ball_timer);
        }
        if (ball_scr) {
            lv_obj_invalidate(ball_scr);
        }
    } else {
        ball_active = false;
        if (ball_timer) {
            lv_timer_pause(ball_timer);
        }

        lv_ball_restore_bottom();
        lv_ball_restore_scr();

        lv_obj_t *scr = lv_screen_active();
        if (scr) {
            lv_obj_invalidate(scr);
        }
    }
}

void lv_ball_effect_unload(void)
{
    /* 极致省内存用: 把特效彻底拆掉, 连球数组和定时器都还给 LVGL 堆。不开这个
     * 特效的产品, 这 1.3KB 常驻内存本来也该是 0。
     *
     * 和 enable()/set_layer() 一样, 必须在 LVGL 任务上下文里调用; 正在跑
     * disp_flush 时不要调。
     *
     * 先把特效关掉(恢复被改透明的屏幕背景 + 暂停定时器), 再拆自己 ——
     * enable(false) 在已经关着时是空操作, 所以重复 unload 无害。 */
    lv_ball_effect_enable(false);

    if (ball_timer) {
        lv_timer_delete(ball_timer);
        ball_timer = NULL;
    }

    if (balls) {
        lv_free(balls);
        balls = NULL;
    }

    /* 清掉跨帧残留, 免得重新开启时还带着上次的手指位置 / 迸发状态 */
    ball_want_active = false;
    ball_touch_down = false;
    ball_touch_x = 0;
    ball_touch_y = 0;

    /* ball_layer 是应用层配置, 故意不复位: 重新开启时沿用原层号, 免得球被悄悄
     * 挪回最底层(那会连带把屏幕背景再打透明)。 */

    /* 卸载后想再用, 直接 lv_ball_effect_enable(true) 即可: 它会重建定时器
     * (init 幂等)并按需重新分配 balls。 */
}

void lv_ball_effect_build_tasks(lv_draw_jlgpu_unit_t *unit)
{
    /* 注意: 本函数跑在 disp_flush 里, 处于 lv_refr 的渲染过程中。
     * 这里只能往 GPU 链表挂任务, 绝对不能改样式或 invalidate。
     * balls == NULL 按不变式不该出现(ball_active 蕴含 balls != NULL), 但这条是
     * 渲染热路径, 一次野指针就是花屏/死机, 用一个判断兜住, 开销可忽略。 */
    if (!ball_active || balls == NULL || unit == NULL || unit->jlgpu_ctx == NULL) {
        return;
    }

    lv_display_t *disp = lv_display_get_default();
    if (disp == NULL) {
        return;
    }
    int32_t w = lv_display_get_horizontal_resolution(disp);
    int32_t h = lv_display_get_vertical_resolution(disp);
    if (w <= 0 || h <= 0) {
        return;
    }

    /* ball_layer 是应用层指定的层号(0 = 最底层, 负数 = 最上层)。
     * lv_jlgpu_insert_task_at() 的层号超过节点数时会自己夹到链表尾, 所以这里用
     * 0x7FFF 当"最上层"的哨兵值就行, 不会溢出到别的层去。
     * 按层号逐个插入, 而不是前插/后插: 代价只是一点指针遍历, 好处是 ball_layer
     * 怎么改都不用跟着调整插入顺序。 */
    uint16_t layer = (uint16_t)((ball_layer < 0) ? 0x7FFF : ball_layer);

    /* 底板在最下, 球依次压在它上面: [底板][球0][球1]...[LVGL 的 UI...] */
    if (lv_ball_need_base()) {
        lv_ball_insert_base(unit, w, h, layer);
        layer++;
    }
    for (int i = 0; i < LV_BALL_EFFECT_NUM; i++) {
        lv_ball_insert_grad(unit, &balls[i], (uint16_t)(layer + i));
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ball_effect_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (!ball_active) {
        if (!ball_want_active) {
            /* 应用层不想要特效了, 定时器自己停掉, 别空转 */
            lv_timer_pause(ball_timer);
            return;
        }
        /* 开机自动开启时, init 阶段屏幕还没建好, 在这里补开。
         * 定时器上下文不是渲染过程, 改样式/invalidate 是安全的。 */
        lv_ball_effect_enable(true);
        if (!ball_active) {
            return;     /* 屏幕仍不可用, 下个周期再试 */
        }
    }

    lv_display_t *disp = lv_display_get_default();
    if (disp == NULL) {
        return;
    }
    int32_t w = lv_display_get_horizontal_resolution(disp);
    int32_t h = lv_display_get_vertical_resolution(disp);
    if (w <= 0 || h <= 0) {
        return;
    }

    /* 切页后活动屏幕会变, 重新把背景调成当前层号该有的状态(顺带重取底板色) */
    lv_obj_t *scr = lv_screen_active();
    lv_ball_sync_scr_bg();

    /* 只读触摸状态, 不注册任何 indev 回调, 因此不影响 LVGL 原有的触摸处理 */
    bool touched = false;
    int32_t tx = 0;
    int32_t ty = 0;
    lv_indev_t *indev = lv_ball_get_pointer_indev();
    if (indev) {
        if (lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED) {
            lv_point_t p;
            lv_indev_get_point(indev, &p);
            if (p.x >= 0 && p.y >= 0) {
                touched = true;
                tx = p.x;
                ty = p.y;
            }
        }
    }

    if (touched) {
        ball_touch_x = tx;
        ball_touch_y = ty;
    }

    /* 松手边沿: 球从松手点朝外炸开, 各自带上新的初速和新的轨迹参数 */
    if (ball_touch_down && !touched) {
        for (int i = 0; i < LV_BALL_EFFECT_NUM; i++) {
            /* 刚被指尖喂亮的球顺着余温再闪一下再慢慢凉; 0.2 这个门限是为了让
             * 离指尖远的球不参与, 免得整屏一起亮 */
            if (balls[i].heat > 0.2f) {
                balls[i].heat = 1.0f;
            }
            /* 先 scatter 重掷 swirl(散开之后不再走同一条弧线), 再 burst 覆盖初速。
             * 顺序不能反: burst 要盖掉 scatter 给的随机方向 */
            lv_ball_scatter(&balls[i]);
            /* heat 已经在上一步刷新过, 所以闪白的球正好就是炸得最狠的那几个 */
            lv_ball_burst(&balls[i], (float)ball_touch_x, (float)ball_touch_y);
        }
    }
    ball_touch_down = touched;

    /* 子步推进: 每个子步的位移还是原来的量级(边界夹取、限速都不用改),
     * 一帧里多推几步就让球整体跑得更快, 轨迹形状保持不变 */
    for (int s = 0; s < BALL_SUB_STEPS; s++) {
        for (int i = 0; i < LV_BALL_EFFECT_NUM; i++) {
            lv_ball_step(&balls[i], w, h, touched, tx, ty);
        }
    }

    if (scr) {
        lv_obj_invalidate(scr);
    }
}

/* 找当前注册的指针类输入设备(触摸屏)。
 * 不能用 lv_indev_active(): 它返回的 indev_act 只在 lv_indev_read() 的执行期间
 * 有值(lv_indev.c:241 置位 / :297 清空), 本 SDK 的触摸又是按需调
 * lv_indev_timer_read_touch() 去喂的(lv_port_indev.c, 触摸的 read_timer 被删掉),
 * 跟我们的驱动定时器是两个独立的时刻, 在定时器里取永远是 NULL。
 * state / pointer.act_point 是 indev 结构体里的字段, 读完一直保留, 可以放心读。 */
static lv_indev_t *lv_ball_get_pointer_indev(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            return indev;
        }
        indev = lv_indev_get_next(indev);
    }
    return NULL;
}

static void lv_ball_randomize(lv_ball_t *b, int32_t w, int32_t h)
{
    b->r = (float)(int32_t)lv_rand(LV_BALL_EFFECT_R_MIN, LV_BALL_EFFECT_R_MAX);
    b->x = (float)(int32_t)lv_rand((uint32_t)b->r, (uint32_t)((float)w - b->r));
    b->y = (float)(int32_t)lv_rand((uint32_t)b->r, (uint32_t)((float)h - b->r));

    /* 基准不透明度。上限超过 255 / BALL_GLOW_OPA_MAX 就会把呼吸上半程削平
     * (见 BALL_GLOW_OPA_MAX 的注释), 所以这两个值是一对, 改一个要跟着改另一个 */
    b->opa = (lv_opa_t)lv_rand(10, 159);
    b->color = lv_color_hsv_to_rgb((uint16_t)lv_rand(0, 359), 90, 100);

    /* 跟手增益取小值: 球是"被指尖拖着走"而不是"钉在指尖上", 每个球跟的速度不一样 */
    b->k = 0.008f + (float)lv_rand(0, 60) * 0.0006f;    /* 0.008 ~ 0.044 */

    /* 呼吸相位起点和速度都随机: 整片球各亮各的, 不会一起闪 */
    b->phase = (uint16_t)lv_rand(0, 65535);
    b->phase_spd = (uint16_t)lv_rand(BALL_GLOW_PHASE_SPD_MIN, BALL_GLOW_PHASE_SPD_MAX);
    b->heat = 0.0f;
    b->burst = 0.0f;

    lv_ball_scatter(b);
}

static void lv_ball_scatter(lv_ball_t *b)
{
    float ang = (float)lv_rand(0, 359) * 0.0174533f;
    float sp  = 1.5f + (float)lv_rand(0, 300) * 0.012f;  /* 1.5 ~ 5.1 px/帧 */

    b->vx = MATH_COSF(ang) * sp;
    b->vy = MATH_SINF(ang) * sp;

    /* 轨迹参数重掷: 散开之后不再走同一条弧线 */
    b->swirl = ((float)(int32_t)lv_rand(0, 1000) - 500.0f) * 0.00007f;  /* ±0.035 */
}

/* 松手迸发: 从松手点 (fx, fy) 朝外给球一个冲量, 直接改写速度(不叠在原来的惯性
 * 上 —— 冲击波本来就该盖过惯性), 并点亮 burst 让 lv_ball_step() 抬高速度上限。
 *
 * 方向取"松手点 -> 球"的径向。再叠一个随机偏角: 不然靠近同一条射线的球会排成
 * 队飞出去, 一眼就能看出是算出来的。 */
static void lv_ball_burst(lv_ball_t *b, float fx, float fy)
{
    float dx = b->x - fx;
    float dy = b->y - fy;
    float d2 = dx * dx + dy * dy;

    if (d2 > 1.0f) {
        /* 归一化。用距离平方判断, 免得为"球正好压在松手点上"这种小概率单独开方 */
        float inv = 1.0f / MATH_SQRTF(d2);
        dx *= inv;
        dy *= inv;
    } else {
        float ang = (float)lv_rand(0, 359) * 0.0174533f;
        dx = MATH_COSF(ang);
        dy = MATH_SINF(ang);
    }

    /* 单位向量转 ±SPREAD_DEG。sin(jit+90) 就是 cos(jit), 省一次查表 */
    int32_t jit = (int32_t)lv_rand(0, BALL_BURST_SPREAD_DEG * 2) - BALL_BURST_SPREAD_DEG;
    float c = (float)lv_trigo_sin((int16_t)(jit + 90)) / 32768.0f;
    float s = (float)lv_trigo_sin((int16_t)jit) / 32768.0f;
    float nx = dx * c - dy * s;
    float ny = dx * s + dy * c;

    /* 刚被指尖喂亮的球炸得最狠, 远处的球只是被冲击波推一把 */
    float sp = BALL_BURST_SPD_BASE + BALL_BURST_SPD_HEAT * b->heat;

    b->vx = nx * sp;
    b->vy = ny * sp;
    b->burst = 1.0f;
}

static void lv_ball_step(lv_ball_t *b, int32_t w, int32_t h, bool touched, int32_t tx, int32_t ty)
{
    float v2;
    float heat_target = 0.0f;

    /* 呼吸相位推进, uint16_t 自己回绕(一圈 = 65536) */
    b->phase += b->phase_spd;

    if (touched) {
        float dx = (float)tx - b->x;
        float dy = (float)ty - b->y;

        /* 指尖影响程度: 用距离平方算, 不开方。1-(d/R)^2 在指尖附近有一小片平顶,
         * 球聚过来时那一片会一起被点亮 */
        float d2 = dx * dx + dy * dy;
        float rr = BALL_TOUCH_RADIUS * BALL_TOUCH_RADIUS;
        if (d2 < rr) {
            heat_target = 1.0f - d2 / rr;
        }

        /* 已经被喂热的球跟得更紧(受激越深、增速越多), 冷球还是原来的手感 */
        float k = b->k * (1.0f + BALL_TOUCH_K_GAIN * b->heat);

        /* 向心 + 切向: 切向项让球绕着指尖走弧线, swirl 各不相同 -> 轨迹各不相同 */
        float ax = dx * k - dy * b->swirl;
        float ay = dy * k + dx * b->swirl;

        b->vx = (b->vx + ax) * BALL_DAMP_TOUCH;
        b->vy = (b->vy + ay) * BALL_DAMP_TOUCH;

        v2 = b->vx * b->vx + b->vy * b->vy;
        if (v2 > (BALL_TOUCH_V_MAX * BALL_TOUCH_V_MAX)) {
            float s = BALL_TOUCH_V_MAX / MATH_SQRTF(v2);
            b->vx *= s;
            b->vy *= s;
        }
    } else {
        /* 自由运动: 保留惯性, 叠一点随机扰动, 弱阻尼 */
        b->vx += ((float)(int32_t)lv_rand(0, 200) - 100.0f) * 0.006f;
        b->vy += ((float)(int32_t)lv_rand(0, 200) - 100.0f) * 0.006f;
        b->vx *= BALL_DAMP_FREE;
        b->vy *= BALL_DAMP_FREE;

        /* 迸发期间: 速度上限抬高 + 额外一份阻尼。阻尼按 burst 插值而不是取常数,
         * 这样 burst 衰减的同时阻尼也自动收回常态, 不会在迸发尾段把球按住不动 */
        float v_max = BALL_FREE_V_MAX;
        if (b->burst > 0.0f) {
            float bd = 1.0f - (1.0f - BALL_BURST_DAMP) * b->burst;
            b->vx *= bd;
            b->vy *= bd;
            v_max = BALL_FREE_V_MAX + (BALL_BURST_V_MAX - BALL_FREE_V_MAX) * b->burst;
        }

        v2 = b->vx * b->vx + b->vy * b->vy;
        if (v2 > (v_max * v_max)) {
            float s = v_max / MATH_SQRTF(v2);
            b->vx *= s;
            b->vy *= s;
        } else if (v2 < (BALL_FREE_V_MIN * BALL_FREE_V_MIN) && b->burst <= 0.0f) {
            /* 别停死, 随机补一脚。迸发还没结束时先不补 —— 刚炸出去就被改方向,
             * 看起来会像半路卡住 */
            lv_ball_scatter(b);
        }

        /* burst 衰减。小于 0.02 直接归零, 免得留一个永远退不下去的小尾巴 */
        if (b->burst > 0.0f) {
            b->burst *= BALL_BURST_DECAY;
            if (b->burst < 0.02f) {
                b->burst = 0.0f;
            }
        }
    }

    /* heat 平滑逼近目标值: 手指扫过是渐亮渐凉, 不做硬切变; 松手后自然衰减回 0 */
    b->heat += (heat_target - b->heat) * BALL_TOUCH_HEAT_SPEED;

    b->x += b->vx;
    b->y += b->vy;

    /* 边界反弹。同时保证整球始终完整在屏内: 球的 render_area 是按
     * 直径算的绝对区域, 越界会被 GPU 裁掉或画出坏数据, 这里直接夹住。 */
    if (b->x < b->r) {
        b->x = b->r;
        b->vx = MATH_FABSF(b->vx);
    } else if (b->x > (float)w - b->r) {
        b->x = (float)w - b->r;
        b->vx = -MATH_FABSF(b->vx);
    }

    if (b->y < b->r) {
        b->y = b->r;
        b->vy = MATH_FABSF(b->vy);
    } else if (b->y > (float)h - b->r) {
        b->y = (float)h - b->r;
        b->vy = -MATH_FABSF(b->vy);
    }
}

/* 要不要在链表最底层自己补一块底板?
 * 只有球插在 LVGL 的 UI 之下(层号 0)时才需要: 那种情况下 LVGL 的屏幕背景填充是排在
 * 我们后面的任务, 会把球盖掉, 所以只能把屏幕背景打透明、改由 GPU 补一块纯色底板。
 * 插在上层时屏幕背景本来就在球下面, 底板不但是多余的, 而且它是块不透明全屏填充,
 * 真插上反而会把整个 UI 盖掉。 */
static bool lv_ball_need_base(void)
{
    return ball_layer == 0;
}

/* 把屏幕背景调成当前层号该有的状态: 底层模式打透明, 上层模式恢复原样。
 * 两个分支都带幂等保护(ball_scr / ball_bottom 为空时直接返回), 可以每帧调。 */
static void lv_ball_sync_scr_bg(void)
{
    if (lv_ball_need_base()) {
        lv_ball_apply_scr_transparent(lv_screen_active());
    } else {
        lv_ball_restore_bottom();
        lv_ball_restore_scr();
    }
}

static void lv_ball_restore_scr(void)
{
    if (ball_scr == NULL) {
        return;
    }

    /* 屏幕可能已经被删掉了(切页), 那种情况不动它 */
    if (lv_obj_is_valid(ball_scr)) {
        lv_obj_set_style_bg_opa(ball_scr, ball_scr_opa_saved, LV_PART_MAIN);
    }
    ball_scr = NULL;
}

static void lv_ball_restore_bottom(void)
{
    if (ball_bottom == NULL) {
        return;
    }

    if (lv_obj_is_valid(ball_bottom)) {
        lv_obj_set_style_bg_opa(ball_bottom, ball_bottom_opa_saved, LV_PART_MAIN);
    }
    ball_bottom = NULL;
}

static void lv_ball_apply_scr_transparent(lv_obj_t *scr)
{
    /* 除了活动屏幕, 显示器的 bottom_layer 也必须一起打透明。
     * lv_display_create() 对无 alpha 的格式(RGB565)会给 bottom_layer 强制
     * bg_opa=255(lv_display.c:175), 它是屏幕之下的一层全屏纯色。
     * 平时屏幕自己不透明, lv_refr 走 "Draw a bottom layer background if there is
     * no top object" 的分支时会跳过它(lv_refr.c:1187); 而屏幕一旦被我们打透明,
     * lv_refr_get_top_obj() 就找不到不透明对象(COVER_CHECK 里 bg_opa < LV_OPA_MAX
     * 直接判 NOT_COVER, lv_obj.c:717), bottom_layer 的全屏填充就会被画出来。
     * 它的任务排在 GPU 链表里我们前插的任务之后, 于是把底板和球全盖掉 —— 表现为纯白屏。 */
    lv_display_t *disp = lv_display_get_default();
    lv_obj_t *bottom = disp ? lv_display_get_layer_bottom(disp) : NULL;
    if (bottom != NULL && bottom != ball_bottom) {
        ball_bottom_opa_saved = lv_obj_get_style_bg_opa(bottom, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bottom, LV_OPA_TRANSP, LV_PART_MAIN);
        ball_bottom = bottom;
    }

    if (scr == NULL || scr == ball_scr) {
        return;
    }

    lv_ball_restore_scr();

    ball_bg_color = lv_obj_get_style_bg_color(scr, LV_PART_MAIN);
    ball_scr_opa_saved = lv_obj_get_style_bg_opa(scr, LV_PART_MAIN);

    /* 本地球不参与 opaque 面积统计, 所以这里直接改样式 */
    lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, LV_PART_MAIN);

    ball_scr = scr;
}

/* 生成球的光斑 CLUT: n+1 条预乘 ARGB8888, 索引 i 对应径向归一化位置 t = i/n
 * (t=0 圆心, t=1 球边缘)。
 *
 * 为什么整套剖面要烘在颜色表里: GPU 只能表达"从 in_radius 到 out_radius 线性
 * 推进的 t", jlgpu_computeRoundParameters() 也只拿得到半径参数, 衰减曲线没有
 * 别的表达途径。
 *
 * 一条 CLUT 里同时叠了三件事:
 *   - 径向衰减剖面 fall(t)  —— 决定"光斑"的软硬;
 *   - 呼吸调制的不透明度 opa_peak(整条一起缩放);
 *   - 呼吸调制的亮度 lum_k 与中心混白 white(只改颜色, 不改 alpha)。
 * 注意最后一条在 t=1 处 fall=0, 必须是全透明: render_area 是 2r x 2r 的方框,
 * 四个角超出 out_radius, 按 PAD 钳到 clut[last], 不透明的话会画出一个方块。 */
static void lv_ball_build_clut(const lv_ball_t *b, uint32_t *clut, int32_t n)
{
    /* 相位 0~65535 映射成 0~359 度。lv_trigo_sin 是角度制的, 返回 [-32768, 32768] */
    int16_t ang = (int16_t)((b->phase * 360u) >> 16);
    float breath = (float)lv_trigo_sin(ang) / 32768.0f;

    /* 把 sin 的 -1~+1 映射成 0~1 的呼吸进度 x: 0 = 周期谷, 1 = 周期峰 */
    float x = (breath + 1.0f) * 0.5f;

    /* 两个轴各自在 MIN~MAX 之间推进, 相位关系由 BALL_GLOW_LUM_INVERT 决定:
     * 同相时最实的同时最亮, 两者相乘, 整球明暗幅度直接叠起来; 反相时互相抵消,
     * 只剩通透/厚实的质感变化 */
    float opa_k = BALL_GLOW_OPA_MIN + (BALL_GLOW_OPA_MAX - BALL_GLOW_OPA_MIN) * x;
#if BALL_GLOW_LUM_INVERT
    float lum_k = BALL_GLOW_LUM_MAX - (BALL_GLOW_LUM_MAX - BALL_GLOW_LUM_MIN) * x;
#else
    float lum_k = BALL_GLOW_LUM_MIN + (BALL_GLOW_LUM_MAX - BALL_GLOW_LUM_MIN) * x;
#endif

    /* 指尖附近的球被激发: 更实(更不透明)、更白 */
    float opa_peak = (float)b->opa * opa_k * (1.0f + (BALL_TOUCH_OPA_GAIN - 1.0f) * b->heat);
    if (opa_peak > 255.0f) {
        opa_peak = 255.0f;
    } else if (opa_peak < 0.0f) {
        opa_peak = 0.0f;
    }
    float white = BALL_GLOW_CORE_WHITE + (BALL_TOUCH_WHITE - BALL_GLOW_CORE_WHITE) * b->heat;

    float base_r = (float)b->color.red;
    float base_g = (float)b->color.green;
    float base_b = (float)b->color.blue;

    for (int32_t i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        float t2 = t * t;
        float t3 = t2 * t;

        /* 衰减剖面: heat=0 时是 1-t^3(核心平亮、外三分之一柔和滚降); 受激时向
         * 1-t^2 过渡 —— 边缘更实, 看起来更"聚焦" */
        float fall = (1.0f - t3) + (t3 - t2) * b->heat;
        /* 散景亮环: BALL_GLOW_RIM 为 0 时这一项不生效。开启时 t^3*(1-t)*4 在
         * t=0.75 处起峰, 像镜头散景的一圈亮边 */
        fall += BALL_GLOW_RIM * t3 * (1.0f - t) * 4.0f;
        if (fall < 0.0f) {
            fall = 0.0f;
        } else if (fall > 1.0f) {
            fall = 1.0f;
        }

        float a_f = opa_peak * fall;
        uint32_t a = (uint32_t)(a_f + 0.5f);
        if (a > 255u) {
            a = 255u;
        }

        /* 中心混白: (1-t)^2 让白色只集中在核心, 制造"过曝成一片白"的光点感 */
        float w = white * (1.0f - t) * (1.0f - t);

        /* 亮度调制叠在混白之后, 逐通道夹回 8bit */
        float cr = ((base_r * (1.0f - w)) + 255.0f * w) * lum_k;
        float cg = ((base_g * (1.0f - w)) + 255.0f * w) * lum_k;
        float cb = ((base_b * (1.0f - w)) + 255.0f * w) * lum_k;
        if (cr > 255.0f) {
            cr = 255.0f;
        } else if (cr < 0.0f) {
            cr = 0.0f;
        }
        if (cg > 255.0f) {
            cg = 255.0f;
        } else if (cg < 0.0f) {
            cg = 0.0f;
        }
        if (cb > 255.0f) {
            cb = 255.0f;
        } else if (cb < 0.0f) {
            cb = 0.0f;
        }

        uint32_t r  = (uint32_t)cr;
        uint32_t g  = (uint32_t)cg;
        uint32_t bl = (uint32_t)cb;

        /* 预乘 alpha, 打包成 ARGB8888(和 lv_draw_jlgpu_premult_and_swizzle 同款)。
         * 直接按 8bit 算而不是调那个函数: 它一次只吃一个颜色和一个 opa, 表达不了
         * "逐条混白 + 亮度调制" */
        clut[i] = (a << 24)
                  | ((r  * a / 255u) << 16)
                  | ((g  * a / 255u) << 8)
                  | (bl * a / 255u);
    }
}

/* 一个球 = 一个圆角渐变任务, 渐变几何只负责"圆形 + 径向参数 t",
 * 光斑长什么样全在 CLUT 里(见 lv_ball_build_clut), 不需要 path。 */
static void lv_ball_insert_grad(lv_draw_jlgpu_unit_t *unit, const lv_ball_t *b, uint16_t layer)
{
    int32_t r = (int32_t)b->r;
    if (r <= 0) {
        return;
    }

    jlgpu_draw_grad_t draw_grad = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};
    jlgpu_matrix_t matrix;

    uint32_t clut_tmp[BALL_GLOW_LUT_N];
    lv_ball_build_clut(b, clut_tmp, BALL_GLOW_LUT_N - 1);

    /* CLUT 走内存池, 不走 lv_jlgpu_attach_buf_list_add: 后者每个 buf 都要
     * malloc 一个节点 + 一份数据, 球数一多就是每帧几十次 malloc/free。
     * 内存池是同帧累加、任务链表释放时 pool_pos 归零(lv_jlgpu_mem_pool_clean),
     * 生命周期跟 GPU 任务链完全一致; 池满时它会自己退化成 attach buf, 不会失败。
     * 代价: 每个球 (2^BALL_GLOW_LUT_LEVEL + 1) * 4 字节 CLUT, 再加上 HAL 算出的
     * 渐变任务长度(由 jlgpu_draw_grad_task_len() 决定), 乘上 LV_BALL_EFFECT_NUM
     * 就是每帧的池占用; 拿去和 lv_conf.h 的 JLGPU_LIST_MEM_POOL_SIZE 比即可。 */
    uint32_t *clut_data = (uint32_t *)lv_jlgpu_mem_pool_get_buf(unit, (uint8_t *)clut_tmp, sizeof(clut_tmp));
    if (clut_data == NULL) {
        return;
    }

    jlgpu_matrix_set_identity(&matrix);

    draw_grad.clut_level = BALL_GLOW_LUT_LEVEL;

    jlgpu_roundgrad_param_t roundgrad_param;
    roundgrad_param.point_center.x = (int)(b->x);      /* 圆心: 屏幕绝对坐标 */
    roundgrad_param.point_center.y = (int)(b->y);
    roundgrad_param.in_width  = 0;      /* 内环尺寸为 0 -> 纯圆形渐变 */
    roundgrad_param.in_height = 0;
    /* 渐变铺满整个半径: 归一化参数 t 从 0(圆心)线性走到 1(球边缘), 衰减剖面
     * 全部由 CLUT 表达(见 lv_ball_build_clut)。
     * 必须保证 out_radius > in_radius, 否则会撞上 jlgpu_computeRoundParameters()
     * 里的除零。 */
    roundgrad_param.in_radius  = 0;
    roundgrad_param.out_radius = r;

    jlgpu_computeRoundParameters(draw_grad.clut_level, &roundgrad_param, &matrix);
    draw_grad.grad_trans_mode = JLGPU_TRANS_MODE_AFFINE;

    draw_grad.paint_info.premult = 1;
    draw_grad.paint_info.pt_clip.x_min = -4096;
    draw_grad.paint_info.pt_clip.x_max = 4095;
    draw_grad.paint_info.pt_clip.y_min = -4096;
    draw_grad.paint_info.pt_clip.y_max = 4095;
    draw_grad.lineargrad_ptr = &roundgrad_param;
    draw_grad.grad_mode = JLGPU_GRAD_MODE_ROUND_GRADIENT;
    draw_grad.surface2paint = &matrix;
    draw_grad.clut_format = JLGPU_CLUT_FORMAT_ARGB8888;
    draw_grad.clut_adr = (uint8_t *)clut_data;

    jlgpu_paint_inter_params_t grad_inter_params;
    grad_inter_params.coeff0 = 0;
    grad_inter_params.coeff1 = 0;
    grad_inter_params.inter_mode = JLGPU_SAMPLE_MODE_X1;
    draw_grad.grad_inter_params = &grad_inter_params;

    render_params.layer_en = 1;
    render_params.gb_trans_en = 1;
    render_params.render_area.x_min = (int)(b->x) - r;
    render_params.render_area.x_max = (int)(b->x) + r;
    render_params.render_area.y_min = (int)(b->y) - r;
    render_params.render_area.y_max = (int)(b->y) + r;

    render_params.blend_mode = JLGPU_BLEND_SRC_OVER;
    /* 固定 1.0(128 = 不透明): 透明度已经完全烘在 8bit 的 CLUT 里了,
     * 这里再乘一遍等于把有效精度砍到 7bit, 羽化带上会出台阶。 */
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDER_QUALITY;
    render_params.dither_en = 1;    /* 抖动, 降低羽化边缘的色带 */

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    uint8_t task_len = jlgpu_draw_grad_task_len(unit->jlgpu_ctx->ctx, &draw_grad, &mask_params);
    if (task_len == 0) {
        return;
    }
    uint8_t *task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
    if (task == NULL) {
        LV_LOG_ERROR("ball grad: pool alloc fail");
        return;
    }
    lv_memset(task, 0, task_len);

    jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
    jlgpu_draw_grad(unit->jlgpu_ctx->ctx, &render_params, &draw_grad, &mask_params);
    lv_jlgpu_insert_task_at(unit, layer, task);
}

/* 底板: 一个盖满屏幕的纯色填充任务。
 * 屏幕 bg_opa 已经被打成透明, 这一层就是原来的屏幕背景填充, 成本是净平移,
 * 换来的是球可以被压在 LVGL 的 UI 之下、又不会继承显存里的旧内容。 */
static void lv_ball_insert_base(lv_draw_jlgpu_unit_t *unit, int32_t w, int32_t h, uint16_t layer)
{
    jlgpu_draw_fill_t draw_fill_params = {0};
    jlgpu_mask_param_t mask_params = {0};
    jlgpu_render_param_t render_params = {0};

    draw_fill_params.color.r = ball_bg_color.red;
    draw_fill_params.color.g = ball_bg_color.green;
    draw_fill_params.color.b = ball_bg_color.blue;
    draw_fill_params.color.a = 255;
    draw_fill_params.fill_trans_mode = JLGPU_TRANS_MODE_NONE;
    draw_fill_params.surface2paint = 0;
    draw_fill_params.paint_info.premult = 1;

    draw_fill_params.paint_info.pt_clip.x_min = 0;      /* 相对区域 */
    draw_fill_params.paint_info.pt_clip.x_max = w;
    draw_fill_params.paint_info.pt_clip.y_min = 0;
    draw_fill_params.paint_info.pt_clip.y_max = h;

    render_params.layer_en = 1;
    render_params.render_area.x_min = 0;                /* 绝对区域 */
    render_params.render_area.x_max = w;
    render_params.render_area.y_min = 0;
    render_params.render_area.y_max = h;
    render_params.blend_mode = JLGPU_BLEND_SRC;         /* 直接盖掉旧内容 */
    render_params.global_alpha = 128;
    render_params.render_quality = JLGPU_RENDERING_QUALITY_NONANTIALIASED;

    mask_params.mask_mode = JLGPU_MASK_MODE_NONE;
    mask_params.mask_ptr = 0;

    uint8_t task_len = jlgpu_draw_fill_task_len(unit->jlgpu_ctx->ctx, &draw_fill_params, &mask_params);
    if (task_len == 0) {
        return;
    }
    uint8_t *task = lv_jlgpu_mem_pool_get_buf(unit, NULL, task_len);
    if (task == NULL) {
        LV_LOG_ERROR("ball base: pool alloc fail");
        return;
    }
    lv_memset(task, 0, task_len);

    jlgpu_set_task(unit->jlgpu_ctx->ctx, task, task_len);
    jlgpu_draw_fill(unit->jlgpu_ctx->ctx, &render_params, &draw_fill_params, &mask_params);
    lv_jlgpu_insert_task_at(unit, layer, task);
}

/* 链表没开(纯 CPU 或单次提交)时特效无处可挂, 这里给出空实现:
 * 应用层调 lv_ball_effect_enable()/set_layer() 时不会跟着链表宏走,
 * 少了这几个符号会链接失败。 */
#else

void lv_ball_effect_init(void) {}
void lv_ball_effect_enable(bool en)
{
    LV_UNUSED(en);
}
bool lv_ball_effect_is_active(void)
{
    return false;
}
void lv_ball_effect_set_layer(int32_t layer)
{
    LV_UNUSED(layer);
}
void lv_ball_effect_unload(void) {}
void lv_ball_effect_build_tasks(lv_draw_jlgpu_unit_t *unit)
{
    LV_UNUSED(unit);
}

#endif /*LV_USE_DRAW_JLGPU_LIST*/
#endif /*LV_USE_DRAW_JLGPU*/
#endif /*CONFIG_LVGL_UI_ENABLE*/
