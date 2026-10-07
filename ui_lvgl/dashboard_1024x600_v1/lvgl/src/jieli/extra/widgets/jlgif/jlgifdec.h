/**
 * @file jlgifdec.h
 */

#ifndef JLGIFDEC_H
#define JLGIFDEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#if LV_USE_JLGIF

/*********************
 *      DEFINES
 *********************/

#define JLGIF_COLOR_TABLE_SIZE 256
#define JLGIF_COLOR_TABLE_LENGTH JLGIF_COLOR_TABLE_SIZE * 4

/**********************
 *      TYPEDEFS
 **********************/

// GIF 文件头结构体
typedef struct {
    char version[4];    // 版本号
    uint16_t width;          // 宽度
    uint16_t height;         // 高度
    uint16_t frame_num;      // 总帧数
    uint16_t delay;          // 帧间隔
} jl_gif_file_header_t;

// GIF 帧结构体
typedef struct {
    uint16_t x;              // 当前帧x偏移
    uint16_t y;              // 当前帧y偏移
    uint16_t width;          // 当前帧宽度
    uint16_t height;         // 当前帧高度
    uint16_t curr_index;     // 当前帧号
    uint16_t base_index;     // 参考帧号
    uint16_t delay;          // 当前帧到下一帧的延时
    uint16_t flag;           // 标志位
    uint32_t addr;           // 帧数据起始地址
    uint32_t size;           // 帧数据大小
    uint32_t decsize;        // 帧数据原始大小
} jl_gif_file_frame_t;

// 颜色表结构体
typedef struct {
    int size;
    uint8_t colors[JLGIF_COLOR_TABLE_LENGTH];
} jl_color_table_t;

// JLGIF 解码器结构体
typedef struct {
    lv_fs_file_t fd;               // 文件描述符
    const uint8_t * data;               // 内存数据指针
    uint8_t is_file;                    // 是否为文件模式
    uint32_t f_rw_p;                    // 文件读写指针

    jl_gif_file_header_t header;   // GIF文件头
    jl_gif_file_frame_t * frames;  // 帧信息数组

    int32_t curr_frame;            // 当前帧索引
    int32_t loop_count;            // 循环次数
    jl_color_table_t gct;          // 全局颜色表
    jl_color_table_t * palette;    // 当前使用的颜色表

    uint16_t base_index;                // 参考帧索引
    uint8_t trans_index;                // 透明色索引
    uint8_t has_trans;                  // 是否有透明色

    uint8_t * canvas;                   // 画布 (包含调色板 + 索引数据)
    uint8_t * index_data;               // 索引数据的起始位置
    uint8_t * frame;                    // 当前帧数据
    uint8_t * base_frame;               // 参考帧数据
} jl_gif_t;

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/**
 * @brief 从文件打开JLGIF
 * @param fname 文件路径
 * @return JLGIF解码器句柄
 */
jl_gif_t * jl_open_gif_file(const char * fname);

/**
 * @brief 从内存数据打开JLGIF
 * @param data 内存数据指针
 * @param size 数据大小
 * @return JLGIF解码器句柄
 */
jl_gif_t * jl_open_gif_data(const void * data, uint32_t size);

/**
 * @brief 渲染特定帧到缓冲区
 * @param gif JLGIF解码器句柄
 * @param buffer 目标缓冲区
 */
void jl_render_frame(jl_gif_t * gif, uint8_t * buffer);

/**
 * @brief 获取下一帧
 * @param gif JLGIF解码器句柄
 * @return 成功返回1，到达结尾返回0，失败返回-1
 */
int jl_get_frame(jl_gif_t * gif);

/**
 * @brief 获取指定帧
 * @param gif JLGIF解码器句柄
 * @param frame_index 帧索引
 * @return 成功返回1，失败返回-1
 */
int jl_get_specific_frame(jl_gif_t * gif, uint16_t frame_index);

/**
 * @brief 重置GIF到初始状态
 * @param gif JLGIF解码器句柄
 */
void jl_rewind(jl_gif_t * gif);

/**
 * @brief 关闭GIF并释放资源
 * @param gif JLGIF解码器句柄
 */
void jl_close_gif(jl_gif_t * gif);

#endif /*LV_USE_JLGIF*/

#ifdef __cplusplus
}
#endif

#endif /* JLGIFDEC_H */
