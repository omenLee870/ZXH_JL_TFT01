/* Copyright(C)
 * not free
 * All right reserved
 *
 * @file ui_page_switch.h
 * @brief 卡片页面滑动管理模块API头文件
 * @author
 * @version V201
 * @date 2022-12-15
 */

#ifndef __UI_PAGE_SWITCH_H__
#define __UI_PAGE_SWITCH_H__

#include "ui_core.h"
#include "ui_animation.h"
#include "cube.h"
/* ------------------------------------------------------------------------------------*/
/**
 * @brief 滑动方向定义
 */
/* ------------------------------------------------------------------------------------*/
enum {
    DIRECTION_NOT_INIT,	// 未初始化
    DIRECTION_NONE,		// 滑动方向未知
    DIRECTION_UP,		// 向上滑动
    DIRECTION_DOWN,		// 向下滑动
    DIRECTION_LEFT,		// 向左滑动
    DIRECTION_RIGHT,	// 向右滑动
};

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 转场特效
 */
/* ------------------------------------------------------------------------------------*/
enum {
    PAGE_MOVE_MODE_DOUBLE_PAGE,		//双页面滑动
    PAGE_MOVE_MODE_CURR_PAGE,		//当前页面滑动，其他页面不动
    PAGE_MOVE_MODE_OTHER_PAGE,		//当前页面不懂，其他页面滑动
    PAGE_MOVE_MODE_SCALE_DOUBLE,	//双页面缩放
    PAGE_MOVE_MODE_FLIP,	        //翻页
    PAGE_MOVE_MODE_CENTER_FLIP,     //中心轴翻转
    PAGE_MOVE_MODE_EDGE_FLIP,	    //边沿翻转
    PAGE_MOVE_MODE_CUBE_FLIP,       //立方体翻页(+ 倒影)
    PAGE_MOVE_MODE_DRIFT_FLIP,      //漂移翻页(+ 倒影)
    PAGE_MOVE_MODE_BOARD_FLIP,          // 翻板翻转特效
    PAGE_MOVE_MODE_BOARD_SLICING_FLIP,  // 切片翻板翻转特效(TODO)

    PAGE_MOVE_MODE_CUBE,	        //3D 立方体
    PAGE_MOVE_MODE_HEXAGON,         //3D 灯笼
    PAGE_MOVE_MODE_REFLECTION,      //3D 灯笼 + 倒影
    PAGE_MOVE_MODE_CUBE_REFLECTION, //立方体(灯笼) + 倒影
    PAGE_MOVE_MODE_USER,			//用户自定义
};

struct ui_page_anim_info {
    u32 mode : 8;
    u32 dir : 8;
    int curr_win;
    struct element_touch_event *e;	// 惯性信息
    void (*anim_start)(int curr_win);
    void (*anim_stop)(int curr_win);
};

struct ui_page_anim {
    u16 pos_x;
    int next_win;
    struct ui_page_anim_info info;
    ui_anim_t anim;
};

struct ui_page_priv {
    u32 threshold: 14;			/*回弹阈值*/
    u32 step: 14;				/*居中步进*/
    u32 delay: 4;				/*刷新延时*/

    /*转场动画效果*/
    int transition_anim_busy;

    /*过渡动画*/
    struct ui_page_anim *p_anim;
    u16 anim_time_max;		// 动画最大运行时间，ms
    u16 anim_time_min;		// 动画最小运行时间，ms
    u8  anim_energy_coeff;	// 惯性补充系数

    /*缩放系数*/
    float scale_min;
    float scale_max;
    /*任务相关的*/
    u16 left_flip: 1;		/*滑动方向 1：left to right*/
    u16 mode: 7;				/*模式*/
    u8 curr_page_index;		/*页索引*/

    /*翻转系数*/
    float degree;				/*翻转角度*/
    float last_degree;			/*翻转角度 last*/
    s16 flip_left;			/*flip_left*/
    s16 flip_width;			/*flip_width*/

    float edge_flip_angle; /* PAGE_MOVE_MODE_EDGE_FLIP两个页面之间的夹角 */

    /*用户自定义回调*/
    void (*user_mode_cb)(struct element *curr_elm, struct element *prev_elm, struct element *nex_elm, int cur_left);
};
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_get_param 获取卡片滑动参数
 *
 * @return 卡片滑动参数
 */
/* ------------------------------------------------------------------------------------*/
struct ui_page_priv *ui_page_get_param();
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_set_param 配置卡片页面滑动参数
 *
 * @param threshold 滑动阈值（沿某方向滑动距离超过阈值时，move_auto会划向下一页，否则回弹）
 * @param step 滑动步距（滑动时每次移动的步距）
 * @param delay 自动滑动定时器时间（delay * 10 ms）
 */
/* ------------------------------------------------------------------------------------*/
void ui_page_set_param(int threshold, int step, int delay);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_move 界面滑动处理
 *
 * @param curr_win 当前页面ID
 * @param xoffset X方向的移动量
 * @param yoffset Y方向的移动量
 * @param mode 移动模式（暂未开发相关功能）
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_move(int curr_win, int xoffset, int yoffset, int mode);
int ui_page_down(int curr_win, int mode);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_switch 界面手动切换
 *
 * @param curr_win 当前界面
 * @param next_win 下一个界面
 * @param xoffset X方向移动量
 * @param mode 模式（暂未开发相关功能）
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_switch(int curr_win, int next_win, int xoffset, int mode);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_move_auto 卡片界面自动滑动（根据阈值判断自动滑动方向）
 *
 * @param curr_win 当前界面ID
 * @param mode 模式
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_move_auto(int curr_win, int mode);
int ui_page_move_stop(int curr_win, int mode);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_move_callback_run 运行卡片是否使能回调（在卡片滑动时，先获取该回调结果判断是否响应卡片滑动）
 *
 * @return true 响应卡片滑动，false 不响应卡片滑动
 */
/* ------------------------------------------------------------------------------------*/
u8 ui_page_move_callback_run(void);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_direction 通过tp两次报点计算滑动方向
 *
 * @param startx 起始X坐标
 * @param starty 起始Y坐标
 * @param endx 结束X坐标
 * @param endy 结束Y坐标
 *
 * @return 滑动方向
 */
/* ------------------------------------------------------------------------------------*/
int get_direction(int startx, int starty, int endx, int endy);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_page_scale_set_delay 设置页面进入、退出缩放过程延时
 *
 * @Params delay 延时时间 us
 */
/* ------------------------------------------------------------------------------------*/
void ui_page_scale_set_delay(int delay);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 实现页面进入退出缩放效果
 *
 * @param curr_win 当前页面ID
 * @param next_win 跳转页面ID
 * @param dir 1:进入 0:退出
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_scale(int curr_win, int next_win, int dir);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 动画功能停止
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void ui_page_anim_stop(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 动画开始
 *
 * @param *info 动画参数
 *
 * @return 0:ok
 */
/* ------------------------------------------------------------------------------------*/
int ui_page_anim_auto(struct ui_page_anim_info *info);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 设置动画最大最小时间
 *
 * @param max_ms 最大时间，单位ms
 * @param min_ms 最小时间，单位ms
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void ui_page_set_anim_run_time(u16 max_ms, u16 min_ms);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 设置边沿翻转特效两页面间的夹角
 *
 * @param angle 夹角角度
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void ui_page_set_edge_flip_anble(float angle);

void ui_page_set_busy(int busy);

int ui_page_get_busy();

#endif
