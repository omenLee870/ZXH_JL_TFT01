#pragma once
#if 0

typedef enum {
    ALGO_NOTHING,           //没有相关动作
    ALGO_WRIST_UP,          //抬腕动作
    ALGO_WRIST_DOWN,        //落腕动作
    ALGO_DOUBLE_CLICK,      //双击屏幕
    ALGO_HITTING,           //击球动作
    ALGO_ON_DESK,           //放置桌面
} jl_gesture_event_t;



/* --------------------------------------------------------------------------*/
/*
 * @brief 杰理手势算法 获取版本号
 *
 * @param [out] 10~255
 *
 *--------------------------------------------------------------------------*/
unsigned char sensor_jl_gesture_ver(void);

/* --------------------------------------------------------------------------*/
/*
 * @brief 杰理手势算法 开启debug模式
 *
 * @param [in] enable  0：关闭log打印 1：开启log打印
 *
 *--------------------------------------------------------------------------*/
void sensor_jl_gesture_debug(char enable);


/* --------------------------------------------------------------------------*/
/*
 * @brief 杰理手势算法 设置抬腕算法的角度
 *
 * @param [in] angle  [0]:翻腕角度，默认5   [1]:手腕与水平面的角度,默认<45
 *
 *--------------------------------------------------------------------------*/
void sensor_jl_gesture_set_wrist_angle(char angle[2]);

/* --------------------------------------------------------------------------*/
/*
 * @brief 杰理手势算法 运行接口
 *
 * @param [in] x    accelerate x轴数据
 * @param [in] y    accelerate y轴数据
 * @param [in] z    accelerate z轴数据
 * @param [out]     算法输出，见 jl_gesture_event_t 定义
 *
 *--------------------------------------------------------------------------*/
jl_gesture_event_t sensor_jl_gesture_run(short x, short y, short z);

#endif
