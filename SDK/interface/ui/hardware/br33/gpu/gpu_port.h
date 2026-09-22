/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_port.h
 *
 * @brief GPU任务接口模块
 * 注意：
 * 1. 应用层引用时建议直接 #include "gpu.h"；
 * 2. 配置时：req 必须参数，opt 可选参数，def 默认参数，ref 参考参数；
 * 3. 标注 <<弃用参数>> 的参数后续会删除，若有使用须及时替换成宏API设置；
 * 4. union 的参数，表示多个功能只能N选1使用；
 * 5. 配置参数采用宏API，禁止直接配置结构体参数，否则后续升级适配困难！
 * 6. 宏API建议调用“独有”部分，即填充、纹理、渐变、MASK独有API，“公共”部分会被公共部分调用。
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2025-12-12
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __GPU_PORT_H__
#define __GPU_PORT_H__


#include "rect.h"
#include "res/resfile.h"

#include "drive/jlgpu_driver.h"

#include "gpu_format.h"
#include "gpu_cache.h"
#include "gpu_path.h"


#define GPU_TASK_TRANS(d, t)		JLGPU_TASK_TRANS(d, t)  /**< 任务类型编码组合 */

/* GPU 任务类型定义 */
#define GPU_TASK_FILL				GPU_TASK_TRANS(JLGPU_DRAW_FILL, JLGPU_TRANS_MODE_NONE) /**< 填充任务 */
#define GPU_TASK_FILL_TRANS(t)		GPU_TASK_TRANS(JLGPU_DRAW_FILL, t)                     /**< 填充变换任务 */
#define GPU_TASK_IMAGE				GPU_TASK_TRANS(JLGPU_DRAW_IMG, JLGPU_TRANS_MODE_NONE)  /**< 纹理任务 */
#define GPU_TASK_IMAGE_TRANS(t)		GPU_TASK_TRANS(JLGPU_DRAW_IMG, t)                      /**< 纹理变换任务 */
#define GPU_TASK_TEXT				GPU_TASK_IMAGE                                         /**< 文本任务 */
#define GPU_TASK_DRAW				GPU_TASK_FILL                                          /**< 自定义绘图任务 */
#define GPU_TASK_LINEAR				GPU_TASK_TRANS(JLGPU_DRAW_GRAD, JLGPU_TRANS_MODE_NONE) /**< 线性渐变 */
#define GPU_TASK_LINEAR_TRANS(t)	GPU_TASK_TRANS(JLGPU_DRAW_GRAD, t)                     /**< 线性渐变变换 */
#define GPU_TASK_GRAD(g)			GPU_TASK_TRANS(g, JLGPU_TRANS_MODE_NONE)               /**< 自由渐变任务 */
#define GPU_TASK_GRAD_TRANS(g, t)	GPU_TASK_TRANS(g, t)                                   /**< 自由渐变变换 */
#define GPU_TASK_JPEG				JLGPU_TASK_EXTEND(JLGPU_EXTEND_COMMAND_CONFIG_GXDEC)   /**< JPEG任务 */
#define GPU_TASK_UVADR				JLGPU_TASK_EXTEND(JLGPU_EXTEND_COMMAND_MODIFY_UVADR)   /**< JPEG任务 */
/**
 * @brief 颜色查找表(CLUT)数据来源枚举
 */
enum {
    CLUT_TAB_NULL,        /**< 无CLUT表 */
    CLUT_TAB_FROM_FLASH,  /**< CLUT表来自Flash存储 */
    CLUT_TAB_FROM_RAM,    /**< CLUT表来自RAM内存 */
};

/**
 * @brief GPU合成与绘制模式枚举
 */
enum {
    GPU_SYNTHESIS_AND_DRAW,   /**< 合成并绘制 */
    GPU_SYNTHESIS,            /**< 仅合成 */
    GPU_DRAW,                 /**< 仅绘制 */
    GPU_DRAW_EACH_GROUP,      /**< 逐组绘制 */
};

/**
 * @brief GPU模块调试标志枚举
 * @note 配置到 lib_jlui_config.c 的 GPU_DEBUG_ENABLE 中
 */
enum {
    GPU_DB_NONE       = 0x00000000, /**< 调试打印关闭 */

    GPU_DB_HAL        = 0x00000001, /**< 调试硬件函数 */
    GPU_DB_ARG        = 0x00000002, /**< 调试函数参数 */
    GPU_DB_FUNC       = 0x00000004, /**< 打印函数流程 */
    GPU_DB_RECT       = 0x00000008, /**< 打印矩形区域 */
    GPU_DB_RETS       = 0x00000010, /**< 打印上层地址 */

    GPU_DB_FILL       = 0x00000020, /**< 打印填充任务 */
    GPU_DB_IMG        = 0x00000040, /**< 打印图片任务 */
    GPU_DB_GRAD       = 0x00000080, /**< 打印渐变任务 */
    GPU_DB_MASK       = 0x00000100, /**< 打印MASK任务 */
    GPU_DB_PATH       = 0x00000200, /**< 打印PATH任务 */

    GPU_DB_TASK       = 0x00000400, /**< 任务单元模块 */
    GPU_DB_CHAIN      = 0x00000800, /**< 任务链表模块 */
    GPU_DB_CHAIN_MULT = 0x00001000, /**< 多任务链模块 */
    GPU_DB_CHAIN_TRAN = 0x00002000, /**< 链表变换模块 */
    GPU_DB_RENDER     = 0x00004000, /**< 渲染配置模块 */

    GPU_DB_ALL        = 0xffffffff, /**< 开启所有打印 */
};

/* gpu_task_param结构中的task_type变量操作 */
#define jlgpu_get_draw_type(var)        (GET_BYTE(var, BYTE_DRAW))       /**< 获取变量中的任务类型 */
#define jlgpu_set_draw_type(var, m)     ((var) |= DRAW_TYPE(m))          /**< 设置变量中的任务类型 */
#define jlgpu_get_paint_trans(var)      (GET_BYTE(var, BYTE_PNT_TRANS))  /**< 获取变量中的画笔变换类型 */
#define jlgpu_set_paint_trans(var, m)   ((var) |= PNT_TRANS(m))          /**< 设置变量中的画笔变换类型 */
#define jlgpu_get_image_trans(var)      (GET_BYTE(var, BYTE_IMG_TRANS))  /**< 获取变量中的纹理变换类型 */
#define jlgpu_set_image_trans(var, m)   ((var) |= IMG_TRANS(m))          /**< 设置变量中的纹理变换类型 */
#define jlgpu_get_render_mode(var)      (GET_BYTE(var, BYTE_R_OR_E))     /**< 获取变量中的渲染模式 */
#define jlgpu_set_render_mode(var, m)   ((var) |= RENDER_MODE(m))        /**< 设置变量中的渲染模式 */
#define jlgpu_get_extend_mode(var)      (GET_BYTE(var, BYTE_R_OR_E))     /**< 获取变量中的拓展任务类型 */
#define jlgpu_set_extend_mode(var, m)   ((var) |= EXTEND_MODE(m))        /**< 设置变量中的拓展任务类型 */

/* gpu_task_param结构中的mask_type变量操作 */
#define jlgpu_get_mask_type(var)        (GET_BYTE(var, BYTE_MASK))       /**< 获取变量中的MASK任务类型 */
#define jlgpu_set_mask_type(var, m)     ((var) |= MASK_MODE(m))          /**< 设置变量中的MASK任务类型 */
#define jlgpu_get_mask_trans(var)       (GET_BYTE(var, BYTE_MASK_TRANS)) /**< 获取变量中的MASK任务变换类型 */
#define jlgpu_set_mask_trans(var, m)    ((var) |= MASK_TRANS(m))         /**< 设置变量中的MASK任务变换类型 */


#define GPU_UNUSED                      (0xff) /**< 无效字节 */
#define GPU_MAGIC						(0x5a) /**< 映射旧版 */
#define BYTE_DRAW                       (0)  /**< 绘制类型字节位置 */
#define BYTE_PNT_TRANS                  (1)  /**< 画笔变换字节位置 */
#define BYTE_IMG_TRANS                  (2)  /**< 纹理变换字节位置 */
#define BYTE_R_OR_E                     (3)  /**< 混合模式/拓展指令字节位置 */
#define BYTE_MASK                       (0)  /**< MASK类型字节位置 */
#define BYTE_MASK_TRANS                 (1)  /**< MASK变换字节位置 */

#define GET_BYTE(val, idx)				(((val) >> ((idx) * 8)) & 0xff)  /**< 获取指定位字节的值 */
#define SET_BYTE(val, idx)				(((val) & 0xff) << ((idx) * 8))  /**< 将值移到指定字节位置 */
#define GET_BITS(x, start, n)			(((x) >> (start)) & ((1U << (n)) - 1))  /**< 获取从start位开始的n位值 */

#define RENDER_MODE(cmd)				SET_BYTE(cmd, BYTE_R_OR_E)     /**< 混合模式 */
#define IMG_TRANS(cmd)					SET_BYTE(cmd, BYTE_IMG_TRANS)  /**< 纹理变换 */
#define PNT_TRANS(cmd)					SET_BYTE(cmd, BYTE_PNT_TRANS)  /**< 画笔变换 */
#define DRAW_TYPE(cmd)					SET_BYTE(cmd, BYTE_DRAW)       /**< 绘制类型 */

#define EXTEND_MODE(cmd)				SET_BYTE(cmd, BYTE_R_OR_E)     /**< 拓展指令 */
#define MASK_TRANS(cmd)					SET_BYTE(cmd, BYTE_MASK_TRANS) /**< 矢量变换 */
#define MASK_MODE(cmd)					SET_BYTE(cmd, BYTE_MASK)       /**< 遮罩类型 */

#define NOT_EXTEND						(GPU_UNUSED)   /**< 无拓展任务，使用混合任务 */
#define NOT_TASK						(GPU_UNUSED)   /**< 无混合任务，使用拓展任务 */


/* task_type = 混合模式/拓展指令 | 纹理变换 | 画笔变换 | 混合任务 */
/* 注意：draw_type 等于 GPU_UNUSED 时才使能拓展任务，仅blend有效  */
#define TASK_TYPE(blend, img_trans, pnt_trans, draw_type) \
	(RENDER_MODE(blend) | IMG_TRANS(img_trans) | PNT_TRANS(pnt_trans) | DRAW_TYPE(draw_type))

/**
 * @brief 组合绘制类型与变换类型生成task_type
 */
#define JLGPU_TASK_TRANS(draw_type, tran_type) \
	TASK_TYPE(NOT_EXTEND, tran_type, tran_type, draw_type)

/**
 * @brief 组合拓展指令生成task_type
 */
#define JLGPU_TASK_EXTEND(c) \
	TASK_TYPE(c, JLGPU_TRANS_MODE_NONE, JLGPU_TRANS_MODE_NONE, NOT_TASK)


/* mask_type = 矢量变换 | mask_type */
/* 注意：矢量变换 仅mask_type == JLGPU_MASK_MODE_VECTOR 时有效 */
#define MASK_TYPE(path_trans, mask_type) \
	(MASK_TRANS(path_trans) | MASK_MODE(mask_type))

#define GPU_MASK_NONE               /**< MASK 不使能 */ \
	MASK_TYPE(JLGPU_TRANS_MODE_NONE, JLGPU_MASK_MODE_NONE)


/**
 * @brief 表数据参数结构体
 * @note 用于MMU、CLUT等表数据配置
 */
typedef struct tab_param {
    uint8_t  en;   /**< 数据使能：MMU=img|clut, CLUT=enable&from */
    uint8_t  flag; /**< 数据标志：MMU=page_size, CLUT=clut_format */
    uint16_t size; /**< 数据大小：MMU=data_size, CLUT=clut_size */
    uint8_t *addr; /**< 数据地址：MMU=data_addr, CLUT=clut_addr */
} tab_param_t;

/**
 * @brief 颜色参数结构体
 */
typedef struct color_param {
    uint8_t a; /**< 透明度(Alpha)分量 */
    uint8_t r; /**< 红色(Red)分量 */
    uint8_t g; /**< 绿色(Green)分量 */
    uint8_t b; /**< 蓝色(Blue)分量 */
} color_param_t;

/**
 * @brief 仿射变换参数结构体
 */
typedef struct affine_param {
    float tx, ty;     /**< 平移距离(X, Y方向) */
    float sx, sy;     /**< 缩放倍率(X, Y方向) */
    float cx, cy;     /**< 旋转中心坐标(X, Y) */
    float angle;      /**< 旋转角度(弧度) */
} affine_param_t;

/**
 * @brief 透视变换参数结构体
 */
typedef struct perspective_param {
    float sx, sy, sz;     /**< 缩放参数(X, Y, Z方向) */
    float rx, ry, rz;     /**< 旋转参数(X, Y, Z轴) */
    float distance;       /**< 摄像机视角距离 */
    float fovx, fovy;     /**< 摄像机视场角(X, Y方向) */
} perspective_param_t;

/**
 * @brief 变换功能参数结构体
 */
typedef struct tran_param {
    uint8_t reserved;     /**< 保留位 */
    uint8_t scale_en;     /**< 缩放使能标志 */
    uint8_t rotate_en;    /**< 旋转使能标志 */
    uint8_t used_matrix;  /**< 自定义矩阵使能标志 */

    union {
        struct affine_param aff;       /**< 仿射变换参数 */
        struct perspective_param per;  /**< 透视变换参数 */
        jlgpu_matrix_t *matrix;        /**< 自定义变换矩阵指针 */
    };

    uint8_t sample_mode;  /**< 插值方式，参考 jlgpu_tex_samplemode_t */
    uint8_t shift_sel;    /**< 移动参数：sample_mode=0/1时为6，其他根据插值系数配置 */
    uint8_t coeff0;       /**< 高斯模糊参数0(gaussian_en使能时有效) */
    uint8_t coeff1;       /**< 高斯模糊参数1(gaussian_en使能时有效) */
    uint8_t coeff2;       /**< 高斯模糊参数2(gaussian_en使能时有效) */
    uint8_t coeff3;       /**< 高斯模糊参数3(gaussian_en使能时有效) */
    uint8_t coeff4;       /**< 高斯模糊参数4(gaussian_en使能时有效) */
    uint8_t coeff5;       /**< 高斯模糊参数5(gaussian_en使能时有效) */

    float ratio_w;        /**< @deprecated 弃用参数 */
    float ratio_h;        /**< @deprecated 弃用参数 */
    s16 rotate_cx;        /**< @deprecated 弃用参数 */
    s16 rotate_cy;        /**< @deprecated 弃用参数 */
    s16 rotate_dx;        /**< @deprecated 弃用参数 */
    s16 rotate_dy;        /**< @deprecated 弃用参数 */
    float rotate_angle;   /**< @deprecated 弃用参数 */
} tran_param_t;

/**
 * @brief MASK功能参数结构体
 */
typedef struct mask_param {
    uint8_t reserve;      /**< 保留位 */
    uint8_t big_end;      /**< MASK字节序(可选) */
    uint8_t mask_inv;     /**< MASK反转标志(可选) */
    uint8_t mask_format;  /**< MASK颜色格式(必选) */
    uint16_t width;       /**< MASK宽度(必选) */
    uint16_t height;      /**< MASK高度(必选) */
    uint8_t *mask_data;   /**< MASK数据地址(必选) */
    struct rect rect;     /**< MASK渲染区域(必选) */

    u32 mask_stride;      /**< @deprecated 弃用参数 */
} mask_param_t;

/**
 * @brief PATH功能参数结构体
 */
typedef struct path_param {
    struct jl_path *path;   /**< 矢量路径指针(必选)，参考 jl_path_t */
    struct tran_param tran; /**< 变换参数 */
} path_param_t;

/**
 * @brief 填充功能参数结构体
 */
typedef struct fill_param {
    uint32_t reserve : 24;     /**< 保留位 */
    uint32_t premult :  8;     /**< 预乘使能(可选)，参考 jlgpu_premult_mode_t，默认 JLGPU_PREMULT_TRUE */
    struct color_param color;  /**< 填充颜色(必选) */
    struct tran_param tran;    /**< 填充变换参数(可选) */
} fill_param_t;

/**
 * @brief 渐变功能参数结构体
 */
typedef struct grad_param {
    uint8_t premult;      /**< 预乘使能(可选)，参考 jlgpu_premult_mode_t，默认 JLGPU_PREMULT_TRUE */
    uint8_t grad_mode;    /**< 渐变模式(必选)，参考 jlgpu_paint_type_t */
    uint8_t spread_mode;  /**< 扩展模式(必选)，参考 jlgpu_spread_mode_t */
    uint8_t sample_mode;  /**< 采样模式(可选)，参考 jlgpu_pt_samplemode_t，默认 JLGPU_SAMPLE_MODE_X1 */

    jlgpu_lineargrad_param_t linear; /**< 线性渐变参数 */
    jlgpu_anglegrad_param_t  angle;  /**< 角度渐变参数 */
    union {
        jlgpu_radialgrad_param_t radial; /**< 径向渐变参数 */
        jlgpu_roundgrad_param_t  round;  /**< 圆角渐变参数 */
        jlgpu_ringgrad_param_t   ring;   /**< 圆环渐变参数 */
    };

    struct tab_param clut;   /**< 颜色表(必选) */
    struct tab_param clut1;  /**< 复合渐变任务时的径向渐变颜色表(可选) */
    struct tran_param tran;  /**< 渐变变换参数(可选) */
} grad_param_t;

/**
 * @brief 纹理功能参数结构体
 */
typedef struct texture_param {
    uint16_t width;       /**< 纹理实际宽度(必选) */
    uint16_t height;      /**< 纹理实际高度(必选) */
    uint8_t *data;        /**< 纹理数据地址(必选) */

    uint8_t format;       /**< 纹理格式(必选)，参考 jlgpu_tex_format_t */
    uint8_t premult;      /**< 透明度预乘(可选)，参考 jlgpu_premult_mode_t，默认 JLGPU_PREMULT_FALSE */
    uint8_t color_ext;    /**< 低位扩展模式(可选)，参考 jlgpu_color_extend_t，默认 JLGPU_COLOR_EXTEND_HIGH */
    uint8_t boundary_ext; /**< 边界扩展模式(可选)，参考 jlgpu_boundary_extend_t，默认 JLGPU_BOUNDARY_EXTEND_ZERO */

    uint8_t sample_mode;  /**< 纹理采样模式(可选)，参考 jlgpu_tex_samplemode_t，默认 JLGPU_SAMPLE_MODE_NEAR */
    uint8_t endian;       /**< 大小端配置(可选)，参考 jlgpu_endian_t，默认 JLGPU_ENDIAN_LITTLE */
    uint8_t alpha_end;    /**< 透明度位置(可选)，参考 jlgpu_alpha_end_t，默认 JLGPU_ALPHA_END_ARGB */
    uint8_t rbs;          /**< RB位置交换(可选)，参考 jlgpu_rbs_t，默认 JLGPU_RBS_ARGB */

    uint8_t adr_mode;     /**< 排列方式(可选)，参考 jlgpu_adr_mode_t，默认 JLGPU_ADRMODE_LINE */
    uint8_t colorkey_en;  /**< ColorKey使能(可选)，默认 DISABLE */
    uint8_t compress;     /**< 压缩标志 */
    uint8_t shift;        /**< 位移参数 */
    uint32_t compress_size; /**< 压缩数据大小 */

    uint32_t stride;      /**< 行地址偏移量(可选) */
    uint8_t *coeff;       /**< 系数数据指针 */

    struct tab_param mmu;     /**< MMU配置(可选) */
    struct tab_param clut;    /**< CLUT配置(可选) */
    struct color_param color; /**< 纹理颜色(可选) */
    struct rect crop;         /**< 裁剪区域(可选) */
    struct tran_param tran;   /**< 变换参数(可选) */

    u8 color_ext_mode;   /**< @deprecated 弃用参数 */
    u8 not_compress;     /**< @deprecated 弃用参数 */
    u8 mmu_page_size;    /**< @deprecated 弃用参数 */
    u8 *mmu_tab_base;    /**< @deprecated 弃用参数 */
} texture_param_t;

/**
 * @brief GPU任务配置参数结构体
 */
typedef struct gpu_task_param {
    uint32_t task_id;    /**< 任务ID(必选) */
    uint32_t element_id; /**< 控件ID(必选) */
    uint32_t task_type;  /**< 任务类型(必选)，参考 TASK_TYPE */
    uint32_t mask_type;  /**< 遮罩类型(可选)，参考 MASK_TYPE */

    int16_t priority;    /**< 优先级(可选)，值越大越上层，默认 -1 */
    uint8_t invisible;   /**< 默认隐藏标志(可选)，默认 false */
    uint8_t global_alpha; /**< 全局透明度(可选)，范围0~128，默认 128 */

    uint8_t blend_mode;  /**< 混合模式(可选)，参考 jlgpu_blendmode_t，默认 JLGPU_BLEND_SRC_OVER */
    uint8_t quality;     /**< 渲染质量(可选)，参考 jlgpu_render_quality_t，默认 JLGPU_RENDERING_QUALITY_NONANTIALIASED */
    uint8_t dither_en;   /**< 抖动使能(可选)，默认 true */

    uint8_t gb_trans_en;  /**< 全局变换标志(可选)，默认 false */
    uint8_t rgb_limit_en; /**< RGB限制使能(可选)，默认 false */
    uint8_t breakpoint_en; /**< 断点使能(可选)，默认 false */

    struct rect draw;    /**< 绘制区域(必选)，当前任务要绘制到的区域，相对于0坐标计算 */
    struct rect area;    /**< 限制区域(必选)，相当于父控件区域，绘图最终必须落在area范围内 */

    jlgpu_matrix_t *matrix; /**< 父控件变换矩阵(可选) */
    void *custom_data;

    struct fill_param fill;     /**< 纯色填充参数 */
    struct grad_param grad;     /**< 渐变填充参数 */
    struct texture_param texture; /**< 纹理填充参数 */
    union {
        struct mask_param mask; /**< 纹理MASK参数 */
        struct path_param path; /**< 矢量绘图参数 */
    };

    u8 premult;         /**< @deprecated 弃用参数 */
    u8 mask_en;         /**< @deprecated 弃用参数 */
    u8 perspective_en;  /**< @deprecated 弃用参数 */
    u8 scale_en;        /**< @deprecated 弃用参数 */
    u8 rotate_en;       /**< @deprecated 弃用参数 */
    u8 crop_en;         /**< @deprecated 弃用参数 */
    u8 draw_en;         /**< @deprecated 弃用参数 */
    u8 grad_type;       /**< @deprecated 弃用参数 */
    u8 color_key_en;    /**< @deprecated 弃用参数 */
    u8 ext_mode;        /**< @deprecated 弃用参数 */
    u8 gaussian_en;     /**< @deprecated 弃用参数 */
    u8 custom_color;    /**< @deprecated 弃用参数 */
    u8 has_clut;        /**< @deprecated 弃用参数 */
    u8 clut_format;     /**< @deprecated 弃用参数 */
    u8 gpu_free_data;   /**< @deprecated 弃用参数 */
    int clut_size;      /**< @deprecated 弃用参数 */
    u8 *clut_tab;       /**< @deprecated 弃用参数 */
    u8 alpha;           /**< @deprecated 弃用参数 */
    u8 red;             /**< @deprecated 弃用参数 */
    u8 green;           /**< @deprecated 弃用参数 */
    u8 blue;            /**< @deprecated 弃用参数 */
    GPU_format_t format; /**< @deprecated 弃用参数 */
    float offset_x;     /**< @deprecated 弃用参数 */
    float offset_y;     /**< @deprecated 弃用参数 */
    struct image_file image; /**< @deprecated 弃用参数 */
    struct flash_file_info info; /**< @deprecated 弃用参数 */

    void (*cb_func);   /**< 自定义绘图回调函数 */
    void (*cb_exit);   /**< 自定义绘图退出回调 */
    void *priv;        /**< 私有数据指针 */
    u32 priv_len;      /**< 私有数据长度 */
    int draw_id;       /**< 绘图ID */
} JLGPUTaskParam_t, *pJLGPUTaskParam_t;



/***************** 公共 基础参数配置 *****************/
/**
 * @brief 设置矩形区域值（内部使用）
 */
#define __jlgpu_set_rect_val(r, l, t, w, h) \
{ \
	rect_left(r)   = (l); \
	rect_top(r)    = (t); \
	rect_width(r)  = (w); \
	rect_height(r) = (h); \
}

/**
 * @brief 设置颜色值（内部使用）
 */
#define __jlgpu_set_color_val(obj, A, R, G, B) \
{ \
	task_param.obj.color.a = (uint8_t)(A); \
	task_param.obj.color.r = (uint8_t)(R); \
	task_param.obj.color.g = (uint8_t)(G); \
	task_param.obj.color.b = (uint8_t)(B); \
}

/**
 * @brief 初始化任务参数并设置任务ID和控件ID
 */
#define jlgpu_task_param_init(tsk_id, elm_id) \
	JLGPUTaskParam_t task_param = {0}; \
	task_param.task_id    = (tsk_id); \
	task_param.element_id = (elm_id); \

#define jlgpu_task_param_set_task_type(task_t)		task_param.task_type = (task_t)  /**< 设置任务类型 */
#define jlgpu_task_param_set_mask_type(mask_t)		task_param.mask_type = (mask_t)  /**< 设置遮罩类型 */
#define jlgpu_task_param_set_priority(prio)			task_param.priority = (prio)  /**< 设置优先级 */
#define jlgpu_task_param_set_invisible(inv)			task_param.invisible = (inv)  /**< 设置隐藏标志 */
#define jlgpu_task_param_set_global_alpha(g_a)		task_param.global_alpha = (g_a)  /**< 设置全局透明度 */
#define jlgpu_task_param_set_blend_mode(blend_m) 	task_param.blend_mode = (blend_m)  /**< 设置混合模式 */
#define jlgpu_task_param_set_quality(qua)			task_param.quality = (qua)  /**< 设置渲染质量 */
#define jlgpu_task_param_set_dither_en(en)			task_param.dither_en = (en)  /**< 设置抖动使能 */
#define jlgpu_task_param_set_gb_trans(swap)			task_param.gb_trans_en = (swap)  /**< 设置全局变换标志 */
#define jlgpu_task_param_set_rgb_limit(en)			task_param.rgb_limit_en = (en)  /**< 设置RGB限制使能 */
#define jlgpu_task_param_set_breakpoint(en)			task_param.breakpoint_en = (en)  /**< 设置断点使能 */
#define jlgpu_task_param_set_draw(l, t, w, h) 		__jlgpu_set_rect_val(&task_param.draw, l, t, w, h)  /**< 设置绘制区域 */
#define jlgpu_task_param_set_area(l, t, w, h)		__jlgpu_set_rect_val(&task_param.area, l, t, w, h)  /**< 设置限制区域 */
#define jlgpu_task_param_set_external_matrix(m)		task_param.matrix = (m)  /**< 设置外部矩阵 */

/**
 * @brief GPU 任务公共的基础参数初始化，设置默认值
 */
#define jlgpu_task_basic_param_init(task_t, tsk_id, elm_id) \
	jlgpu_task_param_init(tsk_id, elm_id); \
	jlgpu_task_param_set_task_type(task_t); \
	jlgpu_task_param_set_mask_type(GPU_MASK_NONE); \
	jlgpu_task_param_set_priority(-1); \
	jlgpu_task_param_set_global_alpha(JLGPU_MAX_GLOBAL_ALPHA); \
	jlgpu_task_param_set_blend_mode(JLGPU_BLEND_SRC_OVER); \
	jlgpu_task_param_set_dither_en(1); \
	task_param.premult = JLGPU_PREMULT_TRUE; \
	task_param.alpha = GPU_MAGIC; \
	task_param.red   = GPU_MAGIC; \
	task_param.green = GPU_MAGIC; \
	task_param.blue  = GPU_MAGIC; \


/**
 * @defgroup GPU_TASK_PUBLIC_TRANS 公共变换参数配置宏
 * @{
 */

/**
 * @brief 设置自定义变换矩阵
 */
#define jlgpu_task_trans_param_matrix(obj, m) \
{ \
	task_param.obj.tran.used_matrix = true; \
	task_param.obj.tran.matrix = (jlgpu_matrix_t *)(m); \
}

/**
 * @brief 设置仿射变换平移参数
 */
#define jlgpu_task_trans_param_affine_translate(obj, x, y) \
{ \
	task_param.obj.tran.aff.tx = (x); \
	task_param.obj.tran.aff.ty = (y); \
}

/**
 * @brief 设置仿射变换缩放参数
 */
#define jlgpu_task_trans_param_affine_scale(obj, w, h) \
{ \
	task_param.obj.tran.scale_en = true; \
	task_param.obj.tran.aff.sx = (w); \
	task_param.obj.tran.aff.sy = (h); \
}

/**
 * @brief 设置仿射变换旋转参数
 */
#define jlgpu_task_trans_param_affine_rotate(obj, x, y, a) \
{ \
	task_param.obj.tran.rotate_en = true; \
	task_param.obj.tran.aff.cx = (x); \
	task_param.obj.tran.aff.cy = (y); \
	task_param.obj.tran.aff.angle = (a); \
}

/**
 * @brief 设置透视变换缩放参数
 */
#define jlgpu_task_trans_param_perspective_scale(obj, x, y, z) \
{ \
	task_param.obj.tran.scale_en = true; \
	task_param.obj.tran.per.sx = (x); \
	task_param.obj.tran.per.sy = (y); \
	task_param.obj.tran.per.sz = (z); \
}

/**
 * @brief 设置透视变换旋转参数
 */
#define jlgpu_task_trans_param_perspective_rotate(obj, x, y, z) \
{ \
	task_param.obj.tran.rotate_en = true; \
	task_param.obj.tran.per.rx = (x); \
	task_param.obj.tran.per.ry = (y); \
	task_param.obj.tran.per.rz = (z); \
}

/**
 * @brief 设置透视变换摄像机参数
 */
#define jlgpu_task_trans_param_perspective_camera(obj, d, x, y) \
{ \
	task_param.obj.tran.per.distance = (d); \
	task_param.obj.tran.per.fovx = (x); \
	task_param.obj.tran.per.fovy = (y); \
}

/** @} */


/**
 * @defgroup GPU_TASK_FILL 填充任务参数配置宏
 * @{
 */
#define jlgpu_task_fill_set_premult(en) 				task_param.fill.premult = (en)              /**< 设置预乘使能 */
#define jlgpu_task_fill_set_color(A, R, G, B)			__jlgpu_set_color_val(fill, A, R, G, B)      /**< 设置填充颜色 */
#define jlgpu_task_fill_set_draw(x, y, w, h)			jlgpu_task_param_set_draw(x, y, w, h)        /**< 设置绘制区域 */
#define jlgpu_task_fill_set_area(x, y, w, h)			jlgpu_task_param_set_area(x, y, w, h)        /**< 设置限制区域 */
#define jlgpu_task_fill_set_quality(q)					jlgpu_task_param_set_quality(q)              /**< 设置渲染质量 */
#define jlgpu_task_fill_set_invisible(i)				jlgpu_task_param_set_invisible(i)            /**< 设置隐藏标志 */
#define jlgpu_task_fill_set_blend_mode(b)               jlgpu_task_param_set_blend_mode(b)            /**< 设置混合模式 */
#define jlgpu_task_fill_set_global_alpha(ga)            jlgpu_task_param_set_global_alpha(ga)         /**< 设置全局透明度 */
#define jlgpu_task_fill_set_external_matrix(m)			jlgpu_task_param_set_external_matrix(m)      /**< 设置外部矩阵 */
#define jlgpu_task_fill_set_trans_mode(m)				jlgpu_set_paint_trans(task_param.task_type, m) /**< 设置变换模式 */
#define jlgpu_task_fill_set_trans_matrix(m)				jlgpu_task_trans_param_matrix(fill, m)       /**< 设置变换矩阵 */
#define jlgpu_task_fill_set_affine_translate(x, y)		jlgpu_task_trans_param_affine_translate(fill, x, y)   /**< 设置仿射平移 */
#define jlgpu_task_fill_set_affine_scale(w, h)			jlgpu_task_trans_param_affine_scale(fill, w, h)       /**< 设置仿射缩放 */
#define jlgpu_task_fill_set_affine_rotate(x, y, a)		jlgpu_task_trans_param_affine_rotate(fill, x, y, a)   /**< 设置仿射旋转 */
#define jlgpu_task_fill_set_perspective_scale(x, y, z)	jlgpu_task_trans_param_perspective_scale(fill, x, y, z)   /**< 设置透视缩放 */
#define jlgpu_task_fill_set_perspective_rotate(x, y, z)	jlgpu_task_trans_param_perspective_rotate(fill, x, y, z)  /**< 设置透视旋转 */
#define jlgpu_task_fill_set_perspective_camera(d, x, y)	jlgpu_task_trans_param_perspective_camera(fill, d, x, y)  /**< 设置透视摄像机 */

#define jlgpu_task_fill_param_init(task_id, elm_id, a, r, g, b) /**< 填充任务参数初始化和配置 */ \
	jlgpu_task_basic_param_init(GPU_TASK_FILL, task_id, elm_id); \
	jlgpu_task_fill_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_fill_set_color(a, r, g, b); \


/* @独有 =================================<< 纹理参数配置 >>================================= */
/**
 * @brief 设置纹理源图宽高（未裁剪时自动设crop为全图）
 */
#define jlgpu_task_img_set_size(w, h) \
{ \
	task_param.texture.width  = (w); \
	task_param.texture.height = (h); \
	if (!task_param.crop_en) {       \
		jlgpu_task_img_set_clip(0, 0, w, h); \
	} \
}

#define jlgpu_task_img_set_format(fmt)					task_param.texture.format = (fmt)  /**< 设置纹理格式 */
#define jlgpu_task_img_set_data(addr)       			task_param.texture.data = (uint8_t*)(addr)  /**< 设置纹理数据地址 */
#define jlgpu_task_img_set_premult(en) 					task_param.texture.premult = (en)  /**< 设置纹理预乘 */
#define jlgpu_task_img_set_color_ext(en) 				task_param.texture.color_ext = (en)  /**< 设置低位扩展模式 */
#define jlgpu_task_img_set_boundary_ext(en) 			task_param.texture.boundary_ext = (en)  /**< 设置边界扩展模式 */
#define jlgpu_task_img_set_colorkey_en(en) 				task_param.texture.colorkey_en = (en)  /**< 设置ColorKey使能 */
#define jlgpu_task_img_set_sample_mode(m) 				task_param.texture.sample_mode = (m)  /**< 设置采样模式 */
#define jlgpu_task_img_set_shift(s)	 					task_param.texture.shift = (s)  /**< 设置位移参数 */
#define jlgpu_task_img_set_compress(cmp)				task_param.texture.compress = (cmp)  /**< 设置压缩模式 */
#define jlgpu_task_img_set_compress_size(s)				task_param.texture.compress_size = (s)  /**< 设置压缩数据大小 */
#define jlgpu_task_img_set_rb_swap(s)					task_param.texture.rbs = (s)  /**< 设置RB交换 */
#define jlgpu_task_img_set_endian(e)					task_param.texture.endian = (e)  /**< 设置大小端 */
#define jlgpu_task_img_set_alpha_end(e)					task_param.texture.alpha_end = (e)  /**< 设置透明度位置 */
#define jlgpu_task_img_set_adr_mode(m)					task_param.texture.adr_mode = (m)  /**< 设置寻址模式 */
#define jlgpu_task_img_set_stride(s)					task_param.texture.stride = (s)  /**< 设置纹理步长 */
#define jlgpu_task_img_set_color(r, g, b)				__jlgpu_set_color_val(texture, 0xff, r, g, b)  /**< 设置纹理渲染色（A格式纹理） */
#define jlgpu_task_img_set_clip(l, t, w, h) 			__jlgpu_set_rect_val(&task_param.texture.crop, l, t, w, h)  /**< 设置纹理裁剪区域 */
#define jlgpu_task_img_set_quality(q)					jlgpu_task_param_set_quality(q)  /**< 设置渲染质量 */
#define jlgpu_task_img_set_invisible(i)					jlgpu_task_param_set_invisible(i)  /**< 设置隐藏标志 */
#define jlgpu_task_img_set_blend_mode(b)                jlgpu_task_param_set_blend_mode(b)  /**< 设置混合模式 */
#define jlgpu_task_img_set_draw(l, t, w, h) 			jlgpu_task_param_set_draw(l, t, w, h)  /**< 设置绘制区域 */
#define jlgpu_task_img_set_area(l, t, w, h)				jlgpu_task_param_set_area(l, t, w, h)  /**< 设置限制区域 */
#define jlgpu_task_img_set_external_matrix(m)			jlgpu_task_param_set_external_matrix(m)  /**< 设置外部矩阵 */
#define jlgpu_task_img_set_trans_mode(m)				jlgpu_set_image_trans(task_param.task_type, m)  /**< 设置变换模式 */
#define jlgpu_task_img_set_trans_matrix(m) 				jlgpu_task_trans_param_matrix(texture, m)  /**< 设置变换矩阵 */
#define jlgpu_task_img_set_affine_translate(x, y)		jlgpu_task_trans_param_affine_translate(texture, x, y)  /**< 设置仿射平移 */
#define jlgpu_task_img_set_affine_scale(w, h)			jlgpu_task_trans_param_affine_scale(texture, w, h)  /**< 设置仿射缩放 */
#define jlgpu_task_img_set_affine_rotate(x, y, a)		jlgpu_task_trans_param_affine_rotate(texture, x, y, a)  /**< 设置仿射旋转 */
#define jlgpu_task_img_set_perspective_scale(x, y, z)	jlgpu_task_trans_param_perspective_scale(texture, x, y, z)  /**< 设置透视缩放 */
#define jlgpu_task_img_set_perspective_rotate(x, y, z)	jlgpu_task_trans_param_perspective_rotate(texture, x, y, z)  /**< 设置透视旋转 */
#define jlgpu_task_img_set_perspective_camera(d, x, y)	jlgpu_task_trans_param_perspective_camera(texture, d, x, y)  /**< 设置透视摄像机 */
#define jlgpu_task_img_set_clut_en(s)					task_param.texture.clut.en = (s)  /**< 设置CLUT使能 */
#define jlgpu_task_img_set_clut_format(f)				task_param.texture.clut.flag = (f)  /**< 设置CLUT格式 */
#define jlgpu_task_img_set_clut(s, t)					task_param.texture.clut.size = (s); task_param.texture.clut.addr = (t);  /**< 设置CLUT大小和地址 */
#define jlgpu_task_img_set_mmu_page_size(s)				task_param.texture.mmu.flag = (s)  /**< 设置MMU页大小 */

/**
 * @brief 设置MMU中的CLUT使能标志
 */
#define jlgpu_task_img_set_mmu_clut(e) \
	task_param.texture.mmu.en = (task_param.texture.mmu.en & ~0xFU) | ((e) ? 0xFU : 0x0U)

/**
 * @brief 设置MMU页表
 */
#define jlgpu_task_img_set_mmu(s, d) \
	task_param.texture.mmu.en |= (0xf0); \
	task_param.texture.mmu.size = (s); \
	task_param.texture.mmu.addr = (d); \

#define jlgpu_task_texture_param_init(task_id, elm_id)  /**< 纹理任务参数初始化和配置 */ \
	jlgpu_task_basic_param_init(GPU_TASK_IMAGE, task_id, elm_id); \
	jlgpu_task_img_set_format(GPU_UNUSED); \
	jlgpu_task_img_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_img_set_adr_mode(JLGPU_ADRMODE_LINE); \
	jlgpu_task_img_set_mmu_page_size(JLGPU_PAGE_SIZE_4); \

#define jlgpu_task_img_param_init	jlgpu_task_texture_param_init /**< 纹理参数初始化映射为img_param */

/**
 * @brief 设置YUV420纹理的UV数据地址任务
 */
#define jlgpu_task_img_set_uv_adr(head,tid,eid,prior,adr) \
	do{\
		jlgpu_uv_adr_t jlgpu_uv_adr = {0};\
		jlgpu_uv_adr.uv_data = adr;\
		jlgpu_task_basic_param_init(GPU_TASK_UVADR,tid,eid);\
		task_param.priv = &jlgpu_uv_adr;\
		task_param.priv_len = sizeof(jlgpu_uv_adr_t);\
		task_param.priv = &jlgpu_uv_adr;\
    	task_param.priority = prior;\
		jlgpu_update_task_by_id(head,tid, eid,&task_param);\
	}while(0)\

/* @独有 =================================<< 渐变参数配置 >>================================= */
#define jlgpu_task_gradient_set_premult(p)				task_param.grad.premult = (p)  /**< 设置渐变预乘 */
#define jlgpu_task_gradient_set_quality(q)				jlgpu_task_param_set_quality(q)  /**< 设置渲染质量 */
#define jlgpu_task_gradient_set_invisible(i)			jlgpu_task_param_set_invisible(i)  /**< 设置隐藏标志 */
#define jlgpu_task_gradient_set_draw(x, y, w, h)		jlgpu_task_param_set_draw(x, y, w, h)  /**< 设置绘制区域 */
#define jlgpu_task_gradient_set_area(x, y, w, h)		jlgpu_task_param_set_area(x, y, w, h)  /**< 设置限制区域 */
#define jlgpu_task_gradient_set_grad_mode(m)			task_param.grad.grad_mode = (m)  /**< 设置渐变模式 */
#define jlgpu_task_gradient_set_spread_mode(m)			task_param.grad.spread_mode = (m)  /**< 设置扩展模式 */
#define jlgpu_task_gradient_set_sample_mode(m)			task_param.grad.sample_mode = (m)  /**< 设置采样模式 */
#define jlgpu_task_gradient_set_grad_type(t)			jlgpu_task_param_set_task_type(t)  /**< 设置渐变任务类型 */
#define jlgpu_task_gradient_set_external_matrix(m)		jlgpu_task_param_set_external_matrix(m)  /**< 设置外部矩阵 */
#define jlgpu_task_gradient_set_trans_mode(m)			jlgpu_set_paint_trans(task_param.task_type, m)  /**< 设置变换模式 */
#define jlgpu_task_gradient_set_trans_matrix(m)			jlgpu_task_trans_param_matrix(grad, m)  /**< 设置变换矩阵 */
#define jlgpu_task_gradient_set_affine_translate(x, y)	jlgpu_task_trans_param_affine_translate(grad, x, y)  /**< 设置仿射平移 */
#define jlgpu_task_gradient_set_affine_scale(w, h)		jlgpu_task_trans_param_affine_scale(grad, w, h)  /**< 设置仿射缩放 */
#define jlgpu_task_gradient_set_affine_rotate(x, y, a)	jlgpu_task_trans_param_affine_rotate(grad, x, y, a)  /**< 设置仿射旋转 */
#define jlgpu_task_gradient_set_perspective_scale(x, y, z)	jlgpu_task_trans_param_perspective_scale(grad, x, y, z)  /**< 设置透视缩放 */
#define jlgpu_task_gradient_set_perspective_rotate(x, y, z)	jlgpu_task_trans_param_perspective_rotate(grad, x, y, z)  /**< 设置透视旋转 */
#define jlgpu_task_gradient_set_perspective_camera(d, x, y)	jlgpu_task_trans_param_perspective_camera(grad, d, x, y)  /**< 设置透视摄像机 */

/**
 * @brief 设置渐变颜色查找表
 */
#define jlgpu_task_gradient_set_clut(clut_data, clut_lvl, fmt) \
{ \
	task_param.grad.clut.en = true; \
	task_param.grad.clut.flag = (fmt); \
	task_param.grad.clut.size = (clut_lvl); \
	task_param.grad.clut.addr = (uint8_t*)(clut_data); \
}

/* 复合渐变任务时，允许单独设置径向渐变的颜色表，单独渐变时无需设置 */
#define jlgpu_task_gradient_set_radial_clut(clut_data, clut_lvl, fmt) \
{ \
	task_param.grad.clut1.en = true; \
	task_param.grad.clut1.flag = (fmt); \
	task_param.grad.clut1.size = (clut_lvl); \
	task_param.grad.clut1.addr = (uint8_t*)(clut_data); \
}

/**
 * @brief 设置线性渐变起点和终点
 */
#define jlgpu_task_gradient_set_linear(sx, sy, ex, ey) \
{ \
	task_param.grad.linear.point_start.x = (sx); \
	task_param.grad.linear.point_start.y = (sy); \
	task_param.grad.linear.point_end.x = (ex); \
	task_param.grad.linear.point_end.y = (ey); \
}

/**
 * @brief 设置径向渐变圆心和内外径
 */
#define jlgpu_task_gradient_set_radial(cx, cy, ri, ro) \
{ \
	task_param.grad.radial.point_center.x = (cx); \
	task_param.grad.radial.point_center.y = (cy); \
	task_param.grad.radial.in_radius = (ri); \
	task_param.grad.radial.out_radius = (ro); \
}

/**
 * @brief 设置圆角渐变参数
 */
#define jlgpu_task_gradient_set_round(cx, cy, ri, ro, iw, ih) \
{ \
	task_param.grad.round.point_center.x = (cx); \
	task_param.grad.round.point_center.y = (cy); \
	task_param.grad.round.in_radius = (ri); \
	task_param.grad.round.out_radius = (ro); \
	task_param.grad.round.in_width = (iw); \
	task_param.grad.round.in_height = (ih); \
}

/**
 * @brief 设置圆环渐变参数
 */
#define jlgpu_task_gradient_set_ring(cx, cy, ri, ro) \
{ \
	task_param.grad.ring.point_center.x = (cx); \
	task_param.grad.ring.point_center.y = (cy); \
	task_param.grad.ring.in_radius = (ri); \
	task_param.grad.ring.out_radius = (ro); \
}

/**
 * @brief 设置角度渐变范围
 */
#define jlgpu_task_gradient_set_angle(cx, cy, sa, ea) \
{ \
	task_param.grad.angle.point_center.x = (cx); \
	task_param.grad.angle.point_center.y = (cy); \
	task_param.grad.angle.start_angle = (sa); \
	task_param.grad.angle.end_angle = (ea); \
}

/**
 * @brief 渐变任务参数初始化和配置
 */
#define jlgpu_task_gradient_param_init(task_id, elm_id) \
	jlgpu_task_basic_param_init(GPU_TASK_LINEAR, task_id, elm_id); \
	jlgpu_task_gradient_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_gradient_set_sample_mode(JLGPU_SAMPLE_MODE_X1); \


/* @独有 =================================<< IMG_FILL配置 >>================================= */
/**
 * @brief 纹理+填充复合任务参数初始化
 */
#define jlgpu_task_img_fill_param_init(task_id, elm_id, a, r, g, b) \
	jlgpu_task_basic_param_init(JLGPU_DRAW_IMG_FILL, task_id, elm_id); \
	jlgpu_task_fill_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_fill_set_color(a, r, g, b); \
	jlgpu_task_img_set_format(GPU_UNUSED); \
	jlgpu_task_img_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_img_set_adr_mode(JLGPU_ADRMODE_LINE); \
	jlgpu_task_img_set_mmu_page_size(JLGPU_PAGE_SIZE_4); \

#define jlgpu_task_img_fill_set_quality(q)			jlgpu_task_param_set_quality(q)  /**< 设置渲染质量 */
#define jlgpu_task_img_fill_set_invisible(i)		jlgpu_task_param_set_invisible(i)  /**< 设置隐藏标志 */
#define jlgpu_task_img_fill_set_draw(l, t, w, h)	jlgpu_task_param_set_draw(l, t, w, h)  /**< 设置绘制区域 */
#define jlgpu_task_img_fill_set_area(l, t, w, h)	jlgpu_task_param_set_area(l, t, w, h)  /**< 设置限制区域 */



/* @独有 =================================<< IMG_GRAD配置 >>================================= */
/**
 * @brief 纹理+渐变复合任务参数初始化
 */
#define jlgpu_task_img_grad_param_init(task_id, elm_id) \
	jlgpu_task_basic_param_init(JLGPU_DRAW_IMG_GRAD, task_id, elm_id); \
	jlgpu_task_gradient_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_gradient_set_sample_mode(JLGPU_SAMPLE_MODE_X1); \
	jlgpu_task_img_set_format(GPU_UNUSED); \
	jlgpu_task_img_set_premult(JLGPU_PREMULT_TRUE); \
	jlgpu_task_img_set_adr_mode(JLGPU_ADRMODE_LINE); \
	jlgpu_task_img_set_mmu_page_size(JLGPU_PAGE_SIZE_4); \

#define jlgpu_task_img_grad_set_quality(q)			jlgpu_task_param_set_quality(q)  /**< 设置渲染质量 */
#define jlgpu_task_img_grad_set_invisible(i)		jlgpu_task_param_set_invisible(i)  /**< 设置隐藏标志 */
#define jlgpu_task_img_grad_set_draw(l, t, w, h)	jlgpu_task_param_set_draw(l, t, w, h)  /**< 设置绘制区域 */
#define jlgpu_task_img_grad_set_area(l, t, w, h)	jlgpu_task_param_set_area(l, t, w, h)  /**< 设置限制区域 */


/* @独有 =================================<< MASK参数配置 >>================================= */
#define jlgpu_task_mask_set_enable()				jlgpu_set_mask_type(task_param.mask_type, JLGPU_MASK_MODE_EXT)  /**< 使能MASK任务 */
#define jlgpu_task_mask_set_size(w, h)				task_param.mask.width = (w); task_param.mask.height = (h);  /**< 设置MASK大小 */
#define jlgpu_task_mask_set_format(f)				task_param.mask.mask_format = (f)  /**< 设置MASK格式 */
#define jlgpu_task_mask_set_endian(e)				task_param.mask.big_end = (e)  /**< 设置MASK字节序 */
#define jlgpu_task_mask_set_invert(i)				task_param.mask.mask_inv = (i)  /**< 设置MASK反转标志 */
#define jlgpu_task_mask_set_data(d)					task_param.mask.mask_data = (d)  /**< 设置MASK数据地址 */
#define jlgpu_task_mask_set_rect(l, t, w, h)		__jlgpu_set_rect_val(&task_param.mask.rect, l, t, w, h)  /**< 设置MASK渲染区域 */


/* @独有 =================================<< PATH参数配置 >>================================= */
#define jlgpu_task_path_set_enable()				jlgpu_set_mask_type(task_param.mask_type, JLGPU_MASK_MODE_VECTOR)  /**< 使能PATH任务 */
#define jlgpu_task_path_set_path(p)					task_param.path.path = (p)  /**< 设置矢量路径指针 */
#define jlgpu_task_path_set_bez_div(b)				jl_path_set_bez_div(task_param.path.path, b)  /**< 设置贝塞尔细分数 */
#define jlgpu_task_path_set_quality(q)				jl_path_set_quality(task_param.path.path, q)  /**< 设置渲染质量 */
#define jlgpu_task_path_set_fill_rule(f)			jl_path_set_fill_rule(task_param.path.path, f)  /**< 设置填充规则 */

#define jlgpu_task_path_set_trans_mode(m)				jlgpu_set_mask_trans(task_param.mask_type, m)  /**< 设置变换模式 */
#define jlgpu_task_path_set_trans_matrix(m)				jlgpu_task_trans_param_matrix(path, m)  /**< 设置变换矩阵 */
#define jlgpu_task_path_set_affine_translate(x, y)		jlgpu_task_trans_param_affine_translate(path, x, y)  /**< 设置仿射平移 */
#define jlgpu_task_path_set_affine_scale(w, h)			jlgpu_task_trans_param_affine_scale(path, w, h)  /**< 设置仿射缩放 */
#define jlgpu_task_path_set_affine_rotate(x, y, a)		jlgpu_task_trans_param_affine_rotate(path, x, y, a)  /**< 设置仿射旋转 */
#define jlgpu_task_path_set_perspective_scale(x, y, z)	jlgpu_task_trans_param_perspective_scale(path, x, y, z)  /**< 设置透视缩放 */
#define jlgpu_task_path_set_perspective_rotate(x, y, z)	jlgpu_task_trans_param_perspective_rotate(path, x, y, z)  /**< 设置透视旋转 */
#define jlgpu_task_path_set_perspective_camera(d, x, y)	jlgpu_task_trans_param_perspective_camera(path, d, x, y)  /**< 设置透视摄像机 */


#define jlgpu_task_breakpoint_enable()                  jlgpu_task_param_set_breakpoint(true)  /**< 使能任务断点 */
#define jlgpu_task_set_custom_data(d)                   task_param.custom_data = (void *)(d)  /**< 设置任务自定义数据 */



/**
 * @brief 配置3x3高斯模糊低层级参数
 */
#define jlgpu_task_param_3x3_gaussian_low_level(param, tran) \
	(param)->gaussian_en = 1; \
	(tran)->sample_mode = 3; \
	(tran)->shift_sel = 6; \
	(tran)->coeff0 = 1; \
	(tran)->coeff1 = 10; \
	(tran)->coeff2 = 20; \
	(tran)->coeff3 = 0; \
	(tran)->coeff4 = 0; \
	(tran)->coeff5 = 0;


/**
 * @defgroup GPU_REGION_API 区域操作API
 * @{
 */

/**
 * @brief 获取GPU默认窗口位置
 * @param rect 存储窗口位置的矩形结构体指针
 */
void jlgpu_get_win_rect(struct rect *rect);

/**
 * @brief 设置GPU默认窗口位置
 * @param rect 窗口位置的矩形结构体指针
 */
void jlgpu_set_win_rect(struct rect *rect);

/**
 * @brief 移动区域到GPU窗口内
 * @param rect 待移动的矩形区域指针
 */
void jlgpu_shift_rect(struct rect *rect);

/** @} */



/**
 * @defgroup GPU_TASK_API GPU任务指令API
 * @{
 */

/**
 * @brief 通过param获取GPU指令所需buf长度
 * @param param gpu_task_param参数指针
 * @return GPU指令所需buf长度
 */
int jlgpu_task_length(pJLGPUTaskParam_t param);

/**
 * @brief 设置GPU指令buf作为渲染指令配置
 * @param ctx GPU硬件上下文
 * @param task GPU指令buf
 */
void jlgpu_task_render(jlgpu_hal_context_t *ctx, void *task);

/**
 * @brief 设置GPU指令buf作为拓展指令配置
 * @param ctx GPU硬件上下文
 * @param task GPU指令buf
 */
void jlgpu_task_extend(jlgpu_hal_context_t *ctx, void *task);

/**
 * @brief 通过gpu_task_param配置GPU指令
 * @param task GPU指令buf
 * @param param gpu_task_param配置参数指针
 */
void jlgpu_task_create(void *task, pJLGPUTaskParam_t param);

/**
 * @brief 释放GPU指令，注意这里不会释放指令buf本身
 * @param task GPU指令buf
 */
void jlgpu_task_delete(void *task);

/**
 * @brief 设置GPU指令使能
 * @param task GPU指令buf
 * @param enable 使能标志（0 不使能，1 使能）
 */
void jlgpu_task_enable(void *task, int enable);

/**
 * @brief 在prev后面插入task指令，原prev后面的指令都往后移一位
 * @param prev 前一个指令buf
 * @param task 待插入指令buf
 */
void jlgpu_task_insert(void *prev, void *task);

/**
 * @brief 替换GPU指令，把task替换为new指令
 * @param prev 前一个指令buf，如果要替换链表第一个指令，prev可以为NULL
 * @param task 待替换指令buf，会将它从链表中剔除
 * @param new 新替换指令buf，会替换task在原来链表中的位置
 */
void jlgpu_task_replace(void *prev, void *task, void *new);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlgpu_task_matrix 计算变换矩阵
 *
 * @Params dst 输出矩阵指针
 * @Params base 基础矩阵指针，如果特效叠加时，里传原来的矩阵
 * @Params param GPU任务参数
 */
/* ------------------------------------------------------------------------------------*/
// void jlgpu_task_matrix(jlgpu_matrix_t *dst, jlgpu_matrix_t *base, pJLGPUTaskParam_t param);

/**
 * @brief 获取GPU指令buf缓存的大小，用于指令拷贝
 * @param task GPU指令buf
 * @return 0表示获取失败，其他表示task指令buf的缓存大小
 */
int jlgpu_task_size(void *task);

/**
 * @brief GPU指令拷贝
 * @param src 原指令buf
 * @param dst 待拷贝指令buf
 * @param size GPU指令buf大小
 * @param shared_cache 是否共享ram内存
 */
void jlgpu_task_copy(void *src, void *dst, uint8_t size, uint8_t shared_cache);

/** @} */



/**
 * @defgroup GPU_TASK_UTIL_API GPU任务工具API
 * @{
 */

/**
 * @brief 设置任务矩阵乘法
 * @param task GPU指令buf
 * @param matrix 变换矩阵指针
 */
void jlgpu_task_mul_matrix(void *task, jlgpu_matrix_t *matrix);

/**
 * @brief 设置任务矩阵
 * @param task GPU指令buf
 * @param matrix 矩阵指针
 * @param matrix1 矩阵1指针
 * @param matrix2 矩阵2指针
 */
void jlgpu_task_set_matrix(void *task, jlgpu_matrix_t *matrix, jlgpu_matrix_t *matrix1, jlgpu_matrix_t *matrix2);

/**
 * @brief 获取任务矩阵
 * @param task GPU指令buf
 * @param matrix 矩阵指针
 * @param matrix1 矩阵1指针
 * @param matrix2 矩阵2指针
 */
void jlgpu_task_get_matrix(void *task, jlgpu_matrix_t *matrix, jlgpu_matrix_t *matrix1, jlgpu_matrix_t *matrix2);

/**
 * @brief 设置任务包围盒
 * @param task GPU指令buf
 * @param bbox 包围盒指针
 */
void jlgpu_task_set_bbox(void *task, jlgpu_boundbox_t *bbox);

/**
 * @brief 获取任务包围盒
 * @param task GPU指令buf
 * @param bbox 包围盒指针
 */
void jlgpu_task_get_bbox(void *task, jlgpu_boundbox_t *bbox);

/**
 * @brief 获取纹理地址信息
 * @param task GPU指令buf
 * @param tex_addr 纹理地址指针
 * @param rle_adr RLE地址指针
 * @param tex_stride 纹理步长指针
 */
void jlgpu_task_get_texture_addr(void *task, u32 *tex_addr, u32 *rle_adr, u32 *tex_stride);

/**
 * @brief 获取纹理MMU配置
 * @param task GPU指令buf
 * @param mmu_en MMU使能指针
 * @param mmu_tab MMU表指针
 */
void jlgpu_task_get_texture_mmu(void *task, u32 *mmu_en, u32 *mmu_tab);

/**
 * @brief GPU内存分配
 * @param size 分配大小
 * @return 分配的内存指针
 */
void *jlgpu_malloc(int size);

/**
 * @brief GPU内存释放
 * @param p 待释放的内存指针
 */
void jlgpu_free(void *p);

/**
 * @brief GPU模块初始化
 * @param screen_width 屏幕宽度
 * @param screen_height 屏幕高度
 */
void jlgpu_module_init(u16 screen_width, u16 screen_height);

/**
 * @brief GPU模块释放
 */
void jlgpu_module_free();

/**
 * @brief 等待GPU操作完成
 * @return 操作结果
 */
int jlgpu_wait_done(void);

/**
 * @brief 清除GPU信号量
 * @return 操作结果
 */
int jlgpu_pnd_sem_clr(void);

/**
 * @brief 初始化GPU信号量
 */
void jlgpu_pnd_sem_init(void);

/** @} */



#include "gpu.h"


#endif

