#ifndef LV_DRAW_JLGPU_BALL_H
#define LV_DRAW_JLGPU_BALL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_conf_internal.h"

#if LV_USE_DRAW_JLGPU

#include "lv_draw_jlgpu.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 初始化圆球特效(创建驱动定时器, 此时定时器是暂停的)。
 * 一般不用主动调: lv_ball_effect_enable() 里有兜底, 没初始化过也能直接用。
 * 函数幂等, 重复调用无害。
 */
void lv_ball_effect_init(void);

/**
 * 打开/关闭圆球特效。应用层唯一需要调的接口。
 *
 * 打开时, 若当前层号是 0(球画在 UI 之下), 会把活动屏幕的 bg_opa 改成 LV_OPA_TRANSP,
 * 改由 GPU 链表自己绘制纯色底板(底板色取自屏幕原本的主题背景色); 关闭时恢复屏幕
 * 原本的 bg_opa。层号不为 0 时不改屏幕背景, 详见 lv_ball_effect_set_layer()。
 * 屏幕还没就绪时调用也安全: 意图会被记下, 等屏幕可用后自动生效。
 *
 * 必须在 LVGL 任务上下文里调用(和 lv_obj_* / lv_timer_* 同一线程), 不要在其他
 * 线程直接调。应用层如果有别的线程, 走 UI 消息队列递过来。
 *
 * 注意: 特效开启期间屏幕底色由 GPU 底板任务兜底, 页面切换/足球等自带特效的
 * 推屏通路会绕开这个兜底, 所以切换页面前应先 enable(false)。
 */
void lv_ball_effect_enable(bool en);

/** 圆球特效当前是否处于激活状态 */
bool lv_ball_effect_is_active(void);

/**
 * 设置圆球特效群插到 GPU 任务链的哪一层(链表顺序 = 绘制顺序)。
 *   layer == 0 : 插到链表头, 画在所有 LVGL UI 之下 —— 默认值。
 *   layer  > 0 : 插到第 layer 个节点之前, 由应用层自己数层。
 *   layer  < 0 : 追加到链表尾, 画在所有 UI 之上。
 *
 * 注意层号是"当帧"的: 每帧的节点数取决于当帧有多少内容被重绘, 跨帧不保证指向
 * 同一个 UI 元素。要稳定地压在某个具体控件之下/之上, 用 lv_jlgpu_task_list_count()
 * + lv_jlgpu_task_list_get() 拿到锚点节点, 再用 lv_jlgpu_insert_task_before/after()。
 *
 * 层号还会影响背景处理, 这个不是可选项而是必须的:
 *   - layer == 0 时 LVGL 的屏幕背景填充排在我们后面, 会把球盖掉, 所以特效会把
 *     活动屏幕(以及 display 的 bottom_layer)的 bg_opa 打透明, 改由自己插一块
 *     纯色底板兜底, 底板色取自屏幕原本的主题背景色; 关闭时恢复原样。
 *   - layer != 0 时屏幕背景本来就在球下面, 什么都不动, 也不插底板 —— 因为底板是
 *     一块不透明全屏填充, 插在上层会把整个 UI 盖掉。
 * 所以切换层号会连带切换屏幕背景的透明状态, 这是设计如此。
 *
 * 在 LVGL 任务上下文里调用。不要在渲染过程中(disp_flush / draw_xxx 回调)调用。
 */
void lv_ball_effect_set_layer(int32_t layer);

/**
 * 把圆球特效的 GPU 任务插入到 unit 任务链(默认插在头部, 即画在 LVGL 的 UI 之下,
 * 位置可以用 lv_ball_effect_set_layer() 改)。
 * 由 disp_flush 在任务链跑起来之前调用, 两种链表模式都适用:
 *   - 双链表(LIST==2): 拿到 jlgpu_run_disp_sem 之后、os_taskq_post 之前调,
 *     否则静态资源可能在上一帧还没被 GPU 消费完时就被覆盖;
 *   - 单链表(LIST==1): 在 jlgpu_task_list_disp_run() 之前调即可, 它是同步的,
 *     上一帧的链表/内存池已在 lv_jlgpu_task_list_free() 里释放。
 */
void lv_ball_effect_build_tasks(lv_draw_jlgpu_unit_t *unit);

/**
 * 卸载圆球特效: 关掉特效(恢复屏幕背景), 删掉驱动定时器, 释放球状态数组。
 *
 * 给"极致省内存"用: 特效开启期间常驻约 1.4KB(30 个球的数组 1320B + 定时器约 48B
 * + 标量状态), 确定用不上的产品调一次就能全部还给 LVGL 堆。没开启过也可以调,
 * 是空操作。
 *
 * 卸载之后想再开, 直接 lv_ball_effect_enable(true) —— 定时器和球数组会自动重建。
 * lv_ball_effect_set_layer() 设过的层号不会被复位, 重新开启时沿用。
 *
 * 和 lv_ball_effect_enable() 一样, 必须在 LVGL 任务上下文里调用, 不要在
 * disp_flush / draw_xxx 回调等渲染过程中调用。
 */
void lv_ball_effect_unload(void);

/**********************
 *      MACROS
 **********************/

/* 球数。改这一个数就行: 球数组、初始化、每帧步进、每帧挂任务都是按它循环的。
 * 需求上限 30。 */
#define LV_BALL_EFFECT_NUM          30

/* 半径范围(px), 需求上限 25 */
#define LV_BALL_EFFECT_R_MIN        10
#define LV_BALL_EFFECT_R_MAX        25

#endif /*LV_USE_DRAW_JLGPU*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_JLGPU_BALL_H*/
