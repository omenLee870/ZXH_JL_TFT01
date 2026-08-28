#ifndef __UI_CURVE_GRAD_H__
#define __UI_CURVE_GRAD_H__


/* cg 为 curve gradient 缩写，避免 point 结构体命名与其它冲突 */
struct cg_point {
    int x;
    int y;
};


typedef struct {
    int x;
    int y;
    u8 alpha;
} Pixel;

/* 绘制信息结构体 */
struct curve_grad_info {
    int x;
    int y;
    int width;
    int height;
    struct cg_point *points;
    int points_num;
    int line_width;
    u32 color;
};


/* ------------------------------------------------------------------------------------*/
/**
 * @brief create_a8_gradient 创建alpha渐变数组，渐变色宽度为start --> end
 *
 * @Params start 起始点索引（包含）
 * @Params end 结束点索引（包含）
 * @Params grid_max alpha的最大值
 *
 * @return 堆内存分配的渐变数组指针，需调用者释放
 */
/* ------------------------------------------------------------------------------------*/
u8 *create_a8_gradient(int start, int end, int grid_max);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief compute_smooth_curve 通过离散点计算平滑曲线，曲线经过所有离散点
 *
 * 注意：
 * 1. 离散点必须时x坐标递增的；
 * 2. 平滑曲线点只计算y坐标，默认x坐标从0递增到points[points_num-1].x；
 * 3. 返回堆内存，需调用者释放内存。
 *
 * @Params points 离散点坐标列表
 * @Params points_num 离散点坐标数量
 * @Params x_max x坐标最大值，等于points[points_num - 1].x
 * @Params y_max y坐标最大值，所有点都在[0, y_max]区间
 *
 * @return 堆内存分配的平滑曲线y坐标数组指针，需调用者释放
 */
/* ------------------------------------------------------------------------------------*/
int *compute_smooth_curve(struct cg_point *points, int points_num, int x_max, int y_max);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief get_curve_grad_check 计算平滑曲线渐变图的校验值
 * 对折线的宽、高、坐标、线宽、颜色进行校验计算，如果校验值变化，则重新生成，否则只调整坐标。
 *
 * @Params points 离散点坐标列表
 * @Params points_num 离散点坐标数量
 * @Params draw_width 绘制宽度
 * @Params draw_height 绘制高度
 * @Params line_width 线宽
 * @Params color 颜色
 *
 * @return crc校验值
 */
/* ------------------------------------------------------------------------------------*/
u32 get_curve_grad_check(struct cg_point *points, int points_num, int draw_width, int draw_height, int line_width, u32 color);



#endif


