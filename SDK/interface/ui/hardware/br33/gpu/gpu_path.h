/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_path.h
 * @brief 矢量路径生成模块
 * 注意：
 * 1. 文件lib_jlui_config.c中GPU_PATH_MEMORY配置最小内存和递增内存，一般使用默认即可；
 * 2. 应用层一般直接使用jl_path_create创建一个路径句柄，内部自动分配内存；
 * 3. 通过jl_path_append_rect等API往path中追加形状、点等；
 * 4. 路径设置完毕，必须通过jl_path_close, jl_path_end追加结束符；
 * 5. 追加形状、路径等超过默认内存时会自动调整内存大小，应用层无需手动调整内存；
 * 6. jl_path_destroy会销毁矢量数据并回收path内存。
 * @author JIELI TECHNOLOGY ZHUHAI
 * @version V1.0.0
 * @date 2026-03-27
 */
/* ------------------------------------------------------------------------------------*/
#ifndef __GPU_PATH_H__
#define __GPU_PATH_H__

#include "drive/jlgpu_driver.h"



/**
 * @brief 矢量路径句柄结构体
 * @details 存储软件生成的矢量路径数据，用于GPU矢量渲染
 */
typedef struct jl_path {
    uint8_t  unused;      /**< 保留 */
    uint8_t  fill_rule;   /**< 填充规则，参考 jlgpu_fillrule_t */
    uint8_t  quality;     /**< 渲染质量，参考 jlgpu_render_quality_t */
    uint8_t  bez_div;     /**< 贝塞尔细分数，值越大圆弧越细腻 */
    uint16_t nsegments;   /**< 路径段数 */
    uint16_t edge_num;    /**< 边数 */
    uint16_t mem_size;    /**< 路径内存大小 */
    uint16_t path_len;    /**< 路径数据长度 */
    uint8_t  *base;       /**< 路径数据基地址 */
    uint8_t  *cur_seg;    /**< 当前路径段指针 */
    uint16_t *cur_coord;  /**< 当前坐标指针 */
    jlgpu_boundbox_t bbox; /**< 路径包围盒 */
} jl_path_t;

#define jl_path_fill_rule(path)	((path)->fill_rule)  /**< 获取路径填充规则 */
#define jl_path_quality(path)	((path)->quality)  /**< 获取路径渲染质量 */
#define jl_path_bez_div(path)	((path)->bez_div)  /**< 获取路径贝塞尔细分数 */
#define jl_path_seg_num(path)	((path)->nsegments)  /**< 获取路径段数 */
#define jl_path_edge_num(path)	((path)->edge_num)  /**< 获取路径边数 */
#define jl_path_mem_size(path)	((path)->mem_size)  /**< 获取路径内存大小 */
#define jl_path_mem_addr(path)	((path)->base)  /**< 获取路径数据基地址 */
#define jl_path_data_len(path)	((path)->path_len)  /**< 获取路径数据长度 */
#define jl_path_cur_seg(path)	((path)->cur_seg)  /**< 获取当前路径段指针 */
#define jl_path_cur_coord(path)	((path)->cur_coord)  /**< 获取当前坐标指针 */
#define jl_path_bbox(path)		(&((path)->bbox))  /**< 获取路径包围盒 */

#define jl_path_set_fill_rule(path, r)	jl_path_fill_rule(path) = (r)  /**< 设置路径填充规则 */
#define jl_path_set_quality(path, q)	jl_path_quality(path) = (q)  /**< 设置路径渲染质量 */
#define jl_path_set_bez_div(path, b)	jl_path_bez_div(path) = (b)  /**< 设置路径贝塞尔细分数 */
#define jl_path_set_seg_num(path, n)	jl_path_seg_num(path) = (n)  /**< 设置路径段数 */
#define jl_path_set_data_len(path, l)	jl_path_data_len(path) = (l)  /**< 设置路径数据长度 */
#define jl_path_set_edge_num(path, e)	jl_path_edge_num(path) = (e)  /**< 设置路径边数 */
/**
 * @brief 设置路径内存基地址和大小
 */
#define jl_path_set_memory(path, m, s)	{jl_path_mem_addr(path) = (m); jl_path_mem_size(path) = (s);}
#define jl_path_set_cur_seg(path, a)	jl_path_cur_seg(path) = (a)  /**< 设置当前路径段指针 */
#define jl_path_set_cur_coord(path, c)	jl_path_cur_coord(path) = (c)  /**< 设置当前坐标指针 */
/**
 * @brief 设置路径包围盒
 */
#define jl_path_set_bbox(path, minx, maxx, miny, maxy) \
{ \
	bbox_min_x(jl_path_bbox(path)) = minx; \
	bbox_max_x(jl_path_bbox(path)) = maxx; \
	bbox_min_y(jl_path_bbox(path)) = miny; \
	bbox_max_y(jl_path_bbox(path)) = maxy; \
}



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_create 创建path句柄，内部会分配内存，并初始化句柄各参数
 *
 * @return NULL 创建失败，其他 path句柄
 */
/* ------------------------------------------------------------------------------------*/
jl_path_t *jl_path_create(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_destroy 销毁path句柄，内部会释放内存
 *
 * @Params path 路径句柄
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_destroy(jl_path_t *path);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_init 初始化path句柄
 *
 * @Params path 待初始化的path句柄
 * @Params base 用于存放path路径数据的buf地址
 * @Params size 用于存放path路径数据的buf长度
 * @Params bez_div 贝塞尔细分数（值越大圆弧越细腻，消耗性能越多）
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_init(jl_path_t *path, uint8_t *base, uint16_t size, uint8_t bez_div);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_deinit 反初始化path句柄
 *
 * @Params path 路径句柄
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_deinit(jl_path_t *path);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_reset 复位path句柄，复位并清空所有参数，但不会释放内存
 *
 * @Params path 待复位的path句柄
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_reset(jl_path_t *path);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_copy_create 复制创建path句柄
 *
 * @Params src 源path句柄
 *
 * @return NULL 创建失败，其他 新path句柄
 */
/* ------------------------------------------------------------------------------------*/
jl_path_t *jl_path_copy_create(jl_path_t *src);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_segment 向句柄追加segment
 *
 * @Params path 待追加句柄
 * @Params seg 需追加的segment
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_segment(jl_path_t *path, jlgpu_path_segment_t seg);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_point 向句柄追加坐标点
 *
 * @Params path 待追加句柄
 * @Params x 需追加点的X坐标
 * @Params y 需追加点的Y坐标
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_point(jl_path_t *path, float x, float y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_data 向句柄追加一段path
 *
 * @Params path 待追加句柄
 * @Params data path数据起始地址
 * @Params len path数据长度
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_data(jl_path_t *path, const void *data, uint16_t len);



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_move_to 追加MOVE_TO数据
 *
 * @Params path 待追加句柄
 * @Params x 待MOVE_TO的X坐标
 * @Params y 待MOVE_TO的Y坐标
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_move_to(jl_path_t *path, float x, float y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_line_to 追加LINE_TO数据
 *
 * @Params path 待追加句柄
 * @Params x 待LINE_TO的X坐标
 * @Params y 待LINE_TO的Y坐标
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_line_to(jl_path_t *path, float x, float y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_quad_to 追加QUAD_TO数据
 *
 * @Params path 待追加句柄
 * @Params cx 待QUAD_TO的圆心X坐标
 * @Params cy 待QUAD_TO的圆心Y坐标
 * @Params x 待QUAD_TO的X坐标
 * @Params y 待QUAD_TO的Y坐标
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_quad_to(jl_path_t *path, float cx, float cy, float x, float y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_cubic_to 追加CUBIC_TO数据
 *
 * @Params path 待追加句柄
 * @Params cx1
 * @Params cy1
 * @Params cx2
 * @Params cy2
 * @Params x
 * @Params y
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_cubic_to(jl_path_t *path, float cx1, float cy1, float cx2, float cy2, float x, float y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_close 闭合PATH数据
 *
 * @Params path 待闭合句柄
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_close(jl_path_t *path);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_end 追加PATH结束符，矢量路径必须结束才能正常渲染
 *
 * @Params path 待结束句柄
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_end(jl_path_t *path);



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_rect 追加矩形数据
 *
 * @Params path
 * @Params x
 * @Params y
 * @Params w
 * @Params h
 * @Params r 圆角半径
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_rect(jl_path_t *path, float x, float y, float w, float h, float r);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_circle 追加圆圈数据
 *
 * @Params path
 * @Params cx
 * @Params cy
 * @Params rx
 * @Params ry
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_circle(jl_path_t *path, float cx, float cy, float rx, float ry);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_arc_right_angle 追加扇形数据
 *
 * @Params path
 * @Params start_x
 * @Params start_y
 * @Params center_x
 * @Params center_y
 * @Params end_x
 * @Params end_y
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_arc_right_angle(jl_path_t *path, float start_x, float start_y, float center_x, float center_y, float end_x, float end_y);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_arc 追加圆弧数据
 *
 * @Params path
 * @Params cx
 * @Params cy
 * @Params radius
 * @Params start_angle
 * @Params sweep
 * @Params pie
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_arc(jl_path_t *path, float cx, float cy, float radius, float start_angle, float sweep, uint8_t pie);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_polygon 追加多边形数据
 *
 * @Params path
 * @Params coord
 * @Params nedges
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_polygon(jl_path_t *path, float *coord, int nedges);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_append_path 追加一个path数据
 *
 * @Params path
 * @Params src
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_append_path(jl_path_t *path, jl_path_t *src);



/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_move_offset 将path内所有坐标点移动指定偏移量
 *
 * @Params path     路径句柄
 * @Params xoffset  X方向偏移量（允许正/负数）
 * @Params yoffset  Y方向偏移量（允许正/负数）
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_move_offset(jl_path_t *path, float xoffset, float yoffset);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief jl_path_dump 打印矢量句柄及path数据
 *
 * @Params path
 */
/* ------------------------------------------------------------------------------------*/
void jl_path_dump(jl_path_t *path);


#endif


