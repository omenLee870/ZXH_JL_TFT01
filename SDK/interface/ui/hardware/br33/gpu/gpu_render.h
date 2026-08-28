#ifndef __GPU_RENDER_H__
#define __GPU_RENDER_H__

#include "drive/jlgpu_driver.h"

/**
 * @brief GPU硬件状态枚举
 */
typedef enum {
    GPU_STATUS_IDLE = 0,     /**< 空闲状态 */
    GPU_STATUS_VERTEX = 1,   /**< 顶点处理状态 */
    GPU_STATUS_FRAGMENT = 2, /**< 片段处理状态 */
} gpu_hw_status_t;
typedef gpu_hw_status_t gpu_status_t;

/**
 * @brief GPU中断类型枚举
 */
typedef enum {
    GPU_PND_LAY_DONE,  /**< 图层渲染完成 */
    GPU_PND_WIN_DONE,  /**< 窗口渲染完成 */
    GPU_PND_BREAK,     /**< 断点中断 */
    GPU_PND_PAUSE,     /**< 暂停中断 */
    GPU_PND_RLE_ERR,   /**< RLE错误 */
    GPU_PND_WCNT_OV,   /**< 窗口计数溢出 */
    GPU_PND_ECNT_OV,   /**< 边计数溢出 */
    GPU_PND_SPH_DONE,  /**< 球形变换完成 */
    GPU_PND_CFG_ERR,   /**< 配置错误 */
    GPU_PND_TOP_DONE,  /**< 顶层渲染完成 */
} gpu_hw_pnd_type_t;
typedef gpu_hw_pnd_type_t gpu_pnd_type_t;

/**
 * @brief 渲染模式枚举
 * @details 定义不同的渲染方式
 * - 帧buf：链表、队列
 * - 块buf：链表
 */
typedef enum {
    GPU_RENDER_UNKNOWN,         /**< 未配置 */
    GPU_RENDER_BY_CHAIN_BLOCK,  /**< 链表方式分块渲染 */
    GPU_RENDER_BY_CHAIN_FRAME,  /**< 链表方式整帧渲染 */
    GPU_RENDER_BY_QUEUE_FRAME,  /**< 队列方式整帧渲染 */
} gpu_render_mode_t;


/**
 * @brief 渲染参数配置结构体
 * @details 包含GPU渲染的所有配置参数
 */
typedef struct {
    int32_t stride;       /**< 输出缓冲区步长 */
    uint8_t out_fmt;      /**< 输出格式 */
    uint8_t rbs;          /**< RB交换标志 */
    uint8_t endian;       /**< 大小端配置 */
    uint8_t alpha_end;    /**< 透明度位置 */
    uint8_t *data;        /**< 输出缓冲区地址 */

    int16_t x_min;        /**< 窗口最小X坐标 */
    int16_t x_max;        /**< 窗口最大X坐标 */
    int16_t y_min;        /**< 窗口最小Y坐标 */
    int16_t y_max;        /**< 窗口最大Y坐标 */

    uint8_t r;            /**< ColorKey R分量 */
    uint8_t g;            /**< ColorKey G分量 */
    uint8_t b;            /**< ColorKey B分量 */
    uint8_t g_trans_en;   /**< 全局变换使能 */
    float *fw_matrix;     /**< 正向变换矩阵 */
    float *bw_matrix;     /**< 反向变换矩阵 */

    uint8_t vector_en;    /**< 矢量使能 */
    uint8_t bez_div;      /**< 贝塞尔细分数 */
    uint8_t quality;      /**< 渲染质量 */
    uint8_t cbuf_split;   /**< 坐标缓冲区分割 */
    uint8_t cbuf_stride;  /**< 坐标缓冲区步长 */
    uint8_t edge_en;      /**< 边缘使能 */
    uint16_t max_edge;    /**< 最大边数 */

    uint8_t *cbuf;        /**< 坐标缓冲区 */
    uint8_t *ebuf;        /**< 边缓冲区 */

    uint8_t *uv_data;     /**< UV数据地址 */
    uint16_t top_w;       /**< 顶层宽度 */
    uint16_t top_h;       /**< 顶层高度 */
    uint16_t offset_x;    /**< X偏移 */
    uint16_t offset_y;    /**< Y偏移 */

    uint16_t buf_w;       /**< 缓冲区宽度 */
    uint16_t buf_h;       /**< 缓冲区高度 */
    uint8_t *buf0;        /**< 缓冲区0 */
    uint8_t *buf1;        /**< 缓冲区1 */

    uint8_t sph_mode;     /**< 球形变换模式 */
    int16_t sph_src_len;  /**< 球形源长度 */
    int16_t sph_dst_len;  /**< 球形目标长度 */
    float *sph_src;       /**< 球形源数据 */
    float *sph_dst;       /**< 球形目标数据 */
    float *sph_matrix;    /**< 球形变换矩阵 */

    uint8_t render_mode;  /**< 渲染模式 */

} JLGPURenderCfg_t, *pJLGPURenderCfg_t;

#define jlgpu_render_stride(render)			((render)->stride)  /**< 获取输出缓冲区步长 */
#define jlgpu_render_out_fmt(render)		((render)->out_fmt)  /**< 获取输出格式 */
#define jlgpu_render_rb_swap(render)		((render)->rbs)  /**< 获取RB交换标志 */
#define jlgpu_render_endian(render)			((render)->endian)  /**< 获取大小端配置 */
#define jlgpu_render_alpha_end(render)		((render)->alpha_end)  /**< 获取透明度位置 */
#define jlgpu_render_out_addr(render)		((render)->data)  /**< 获取输出缓冲区地址 */
#define jlgpu_render_win_minx(render)		((render)->x_min)  /**< 获取窗口最小X坐标 */
#define jlgpu_render_win_maxx(render)		((render)->x_max)  /**< 获取窗口最大X坐标 */
#define jlgpu_render_win_miny(render)		((render)->y_min)  /**< 获取窗口最小Y坐标 */
#define jlgpu_render_win_maxy(render)		((render)->y_max)  /**< 获取窗口最大Y坐标 */
#define jlgpu_render_colorkey_r(render)		((render)->r)  /**< 获取ColorKey R分量 */
#define jlgpu_render_colorkey_g(render)		((render)->g)  /**< 获取ColorKey G分量 */
#define jlgpu_render_colorkey_b(render)		((render)->b)  /**< 获取ColorKey B分量 */
#define jlgpu_render_global_trans(render)	((render)->g_trans_en)  /**< 获取全局变换使能 */
#define jlgpu_render_fw_matrix(render)		((render)->fw_matrix)  /**< 获取正向变换矩阵 */
#define jlgpu_render_bw_matrix(render)		((render)->bw_matrix)  /**< 获取反向变换矩阵 */
#define jlgpu_render_vtx_enable(render)		((render)->vector_en)  /**< 获取矢量使能 */
#define jlgpu_render_vtx_bez_div(render)	((render)->bez_div)  /**< 获取贝塞尔细分数 */
#define jlgpu_render_vtx_quality(render)	((render)->quality)  /**< 获取渲染质量 */
#define jlgpu_render_vtx_cbuf_split(render)	((render)->cbuf_split)  /**< 获取坐标缓冲区分割 */
#define jlgpu_render_vtx_cbuf_stride(render) ((render)->cbuf_stride)  /**< 获取坐标缓冲区步长 */
#define jlgpu_render_vtx_cbuf(render)		((render)->cbuf)  /**< 获取坐标缓冲区 */
#define jlgpu_render_vtx_ebuf(render)		((render)->ebuf)  /**< 获取边缓冲区 */
#define jlgpu_render_vtx_max_edge(render)	((render)->max_edge)  /**< 获取最大边数 */
#define jlgpu_render_vtx_clear_edge(render)	((render)->edge_en)  /**< 获取清空边缘使能 */
#define jlgpu_render_uv_data(render)		((render)->uv_data)  /**< 获取UV数据地址 */
#define jlgpu_render_top_w(render)			((render)->top_w)  /**< 获取顶层宽度 */
#define jlgpu_render_top_h(render)			((render)->top_h)  /**< 获取顶层高度 */
#define jlgpu_render_offset_x(render)		((render)->offset_x)  /**< 获取X偏移 */
#define jlgpu_render_offset_y(render)		((render)->offset_y)  /**< 获取Y偏移 */
#define jlgpu_render_buf_w(render)			((render)->buf_w)  /**< 获取缓冲区宽度 */
#define jlgpu_render_buf_h(render)			((render)->buf_h)  /**< 获取缓冲区高度 */
#define jlgpu_render_buf0(render)			((render)->buf0)  /**< 获取缓冲区0 */
#define jlgpu_render_buf1(render)			((render)->buf1)  /**< 获取缓冲区1 */
#define jlgpu_render_sph_mode(render)		((render)->sph_mode)  /**< 获取球形变换模式 */
#define jlgpu_render_sph_src_len(render)	((render)->sph_src_len)  /**< 获取球形源长度 */
#define jlgpu_render_sph_dst_len(render)	((render)->sph_dst_len)  /**< 获取球形目标长度 */
#define jlgpu_render_sph_src(render)		((render)->sph_src)  /**< 获取球形源数据 */
#define jlgpu_render_sph_dst(render)		((render)->sph_dst)  /**< 获取球形目标数据 */
#define jlgpu_render_sph_matrix(render)		((render)->sph_matrix)  /**< 获取球形变换矩阵 */


#define jlgpu_render_set_stride(render, s)			jlgpu_render_stride(render) = (s)  /**< 设置输出缓冲区步长 */
#define jlgpu_render_set_out_fmt(render, f)			jlgpu_render_out_fmt(render) = (f)  /**< 设置输出格式 */
#define jlgpu_render_set_rb_swap(render, r)			jlgpu_render_rb_swap(render) = (r)  /**< 设置RB交换标志 */
#define jlgpu_render_set_endian(render, e)			jlgpu_render_endian(render) = (e)  /**< 设置大小端配置 */
#define jlgpu_render_set_alpha_end(render, a)		jlgpu_render_alpha_end(render) = (a)  /**< 设置透明度位置 */
#define jlgpu_render_set_out_addr(render, d)		jlgpu_render_out_addr(render) = (d)  /**< 设置输出缓冲区地址 */
/**
 * @brief 设置输出区域（窗口范围）
 */
#define jlgpu_render_set_out_area(render, xmin, xmax, ymin, ymax) \
{ \
	jlgpu_render_win_minx(render) = (xmin); \
	jlgpu_render_win_maxx(render) = (xmax); \
	jlgpu_render_win_miny(render) = (ymin); \
	jlgpu_render_win_maxy(render) = (ymax); \
}
/**
 * @brief 设置ColorKey颜色
 */
#define jlgpu_render_set_colorkey(render, R, G, B) \
{ \
	jlgpu_render_colorkey_r(render) = (R); \
	jlgpu_render_colorkey_g(render) = (G); \
	jlgpu_render_colorkey_b(render) = (B); \
}
#define jlgpu_render_set_global_trans(render, t)	jlgpu_render_global_trans(render) = (t)  /**< 设置全局变换使能 */
#define jlgpu_render_set_fw_matrix(render, m)		jlgpu_render_fw_matrix(render) = (m)  /**< 设置正向变换矩阵 */
#define jlgpu_render_set_bw_matrix(render, m)		jlgpu_render_bw_matrix(render) = (m)  /**< 设置反向变换矩阵 */
#define jlgpu_render_set_vtx_enable(render, e)		jlgpu_render_vtx_enable(render) = (e)  /**< 设置矢量使能 */
#define jlgpu_render_set_vtx_bez_div(render, b)		jlgpu_render_vtx_bez_div(render) = (b)  /**< 设置贝塞尔细分数 */
#define jlgpu_render_set_vtx_quality(render, q)		jlgpu_render_vtx_quality(render) = (q)  /**< 设置渲染质量 */
#define jlgpu_render_set_vtx_cbuf_split(render, s)	jlgpu_render_vtx_cbuf_split(render) = (s)  /**< 设置坐标缓冲区分割 */
#define jlgpu_render_set_vtx_cbuf_stride(render, s)	jlgpu_render_vtx_cbuf_stride(render) = (s)  /**< 设置坐标缓冲区步长 */
#define jlgpu_render_set_vtx_cbuf(render, c)		jlgpu_render_vtx_cbuf(render) = (c)  /**< 设置坐标缓冲区 */
#define jlgpu_render_set_vtx_ebuf(render, c)		jlgpu_render_vtx_ebuf(render) = (c)  /**< 设置边缓冲区 */
#define jlgpu_render_set_vtx_max_edge(render, e)	jlgpu_render_vtx_max_edge(render) = (e)  /**< 设置最大边数 */
#define jlgpu_render_set_vtx_clear_edge(render, c)	jlgpu_render_vtx_clear_edge(render) = (c)  /**< 设置清空边缘使能 */
#define jlgpu_render_set_uv_data(render, d)			jlgpu_render_uv_data(render) = (d)  /**< 设置UV数据地址 */
#define jlgpu_render_set_top_w(render, w)			jlgpu_render_top_w(render) = (w)  /**< 设置顶层宽度 */
#define jlgpu_render_set_top_h(render, h)			jlgpu_render_top_h(render) = (h)  /**< 设置顶层高度 */
#define jlgpu_render_set_offset_x(render, x)		jlgpu_render_offset_x(render) = (x)  /**< 设置X偏移 */
#define jlgpu_render_set_offset_y(render, y)		jlgpu_render_offset_y(render) = (y)  /**< 设置Y偏移 */
#define jlgpu_render_set_buf_w(render, w)			jlgpu_render_buf_w(render) = (w)  /**< 设置缓冲区宽度 */
#define jlgpu_render_set_buf_h(render, h)			jlgpu_render_buf_h(render) = (h)  /**< 设置缓冲区高度 */
#define jlgpu_render_set_buf0(render, a)			jlgpu_render_buf0(render) = (a)  /**< 设置缓冲区0 */
#define jlgpu_render_set_buf1(render, a)			jlgpu_render_buf1(render) = (a)  /**< 设置缓冲区1 */
#define jlgpu_render_set_sph_mode(render, m)		jlgpu_render_sph_mode(render) = (m)  /**< 设置球形变换模式 */
#define jlgpu_render_set_sph_matrix(render, m)		jlgpu_render_sph_matrix(render) = (m)  /**< 设置球形变换矩阵 */
/**
 * @brief 设置球形变换源数据和长度
 */
#define jlgpu_render_set_sph_src(render, addr, len) \
{ \
	jlgpu_render_sph_src(render) = (addr); \
	jlgpu_render_sph_src_len(render) = (len); \
}
/**
 * @brief 设置球形变换目标数据和长度
 */
#define jlgpu_render_set_sph_dst(render, addr, len) \
{ \
	jlgpu_render_sph_dst(render) = (addr); \
	jlgpu_render_sph_dst_len(render) = (len); \
}

/* 获取渲染方式 */
#define jlgpu_render_mode(render)	((render)->render_mode)

/* 设置渲染方式 */
#define jlgpu_render_set_mode(render, mode)	jlgpu_render_mode(render) = (mode)

/**
 * @brief 更新最大边数（取较大值）
 */
#define jlgpu_render_update_vtx_max_edge(render, e) \
	jlgpu_render_set_vtx_max_edge(render, MAX(jlgpu_render_vtx_max_edge(render), e))

/**
 * @brief 更新最大贝塞尔细分数（取较大值）
 */
#define jlgpu_render_update_vtx_max_bez_div(render, b) \
	jlgpu_render_set_vtx_bez_div(render, MAX(jlgpu_render_vtx_bez_div(render), b))

/**
 * @brief 更新最大渲染质量（取较大值）
 */
#define jlgpu_render_update_vtx_max_quality(render, q) \
	jlgpu_render_set_vtx_quality(render, MAX(jlgpu_render_vtx_quality(render), q))



/**
 * @brief GPU硬件初始化
 * @note 初始化GPU硬件寄存器
 */
void gpu_hw_sfr_init(void);

/**
 * @brief 设置GPU硬件第一个任务
 * @param base_task 第一个任务地址
 */
void gpu_hw_base_task(void *base_task);

/**
 * @brief 启动GPU硬件渲染
 */
void gpu_hw_kick_start(void);

/**
 * @brief 暂停GPU硬件
 */
void gpu_hw_set_pause(void);

/**
 * @brief 清空GPU硬件中断
 */
void gpu_hw_pnd_clear(void);

/**
 * @brief 获取GPU硬件状态
 * @return GPU硬件状态
 */
gpu_status_t gpu_hw_get_status(void);

/**
 * @brief 获取GPU硬件当前的指令
 * @return 当前正在执行的指令
 */
void *gpu_hw_get_curr_inst();

/**
 * @brief 获取GPU硬件下一个指令
 * @return 下一个待执行的质量
 */
void *gpu_hw_get_next_inst();

/**
 * @brief 打印GPU硬件计数
 */
void gpu_hw_tcnt_print();

/**
 * @brief 复位GPU硬件计数
 * @param VTX0 顶点计数器0复位标志
 * @param VTX1 顶点计数器1复位标志
 * @param FRAG 片段计数器复位标志
 * @param TESS 细分计数器复位标志
 * @param RAST 光栅化计数器复位标志
 */
void gpu_hw_tcnt_clear(int VTX0, int VTX1, int FRAG, int TESS, int RAST);

/**
 * @brief 创建渲染配置句柄
 * @return 创建成功返回渲染配置指针，失败返回NULL
 */
pJLGPURenderCfg_t jlgpu_render_create_handler();

/**
 * @brief 删除渲染配置句柄
 * @param cfg 渲染配置指针
 */
void jlgpu_render_delete_handler(pJLGPURenderCfg_t cfg);

/**
 * @brief 复位渲染配置句柄
 * @param cfg 渲染配置指针
 */
void jlgpu_render_reset_handler(pJLGPURenderCfg_t cfg);

/**
 * @brief 复制渲染配置句柄
 * @param dst 目标渲染配置
 * @param src 源渲染配置
 */
void jlgpu_render_copy_handler(pJLGPURenderCfg_t dst, pJLGPURenderCfg_t src);

/**
 * @brief 设置输出缓冲区
 * @param render 渲染配置指针
 * @param out_buf 输出缓冲区地址
 * @param rect 输出区域
 * @param left 左边界
 * @param stride 行步长
 * @return 设置成功返回0，失败返回错误码
 */
int jlgpu_render_set_out_buf(pJLGPURenderCfg_t render, u8 *out_buf, struct rect *rect, int left, int stride);

/**
 * @brief 将配置数据装载到GPU寄存器
 * @param render 渲染配置指针
 * @return 装载成功返回0，失败返回错误码
 */
int jlgpu_render_load_config(pJLGPURenderCfg_t render);

/**
 * @brief 打印渲染配置信息
 * @param render 渲染配置指针
 */
void jlgpu_render_dump_config(pJLGPURenderCfg_t render);

/**
 * @brief 获取GPU硬件中断状态
 * @return GPU中断寄存器值
 */
int jlgpu_get_isr_state(void);

/**
 * @brief 清理GPU硬件中断状态
 */
void jlgpu_clr_isr_state(void);

/**
 * @brief 检查指定中断是否存在
 * @param pnd 待检查的中断号
 * @return 1指定中断已产生，0指定中断未产生
 */
int jlgpu_check_isr_state(gpu_pnd_type_t pnd);

/**
 * @brief 清理指定中断
 * @param pnd 待清理的中断号
 */
void jlgpu_clear_isr_state(gpu_pnd_type_t pnd);



#endif


