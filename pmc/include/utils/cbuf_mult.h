#ifndef __P11_CBUF__H__
#define __P11_CBUF__H__

/* --------------------------------------------------------------------------*/
/**
 * @brief cbuffer 私有成员结构体
 */
/* ----------------------------------------------------------------------------*/
typedef struct _cbuffer_child {
    u16 en    : 1;
    u16 offset: 15;
} cbuffer_child_t;



/* --------------------------------------------------------------------------*/
/**
 * @brief cbuffer结构体
 */
/* ----------------------------------------------------------------------------*/
typedef struct _p11_cbuffer {
    u8  *begin;
    u8  *end;
    u8  *read_ptr;
    u8  *write_ptr;
    u8  *tmp_ptr ;
    u16 tmp_len;
    u16 data_len;
    u16 total_len;
    u8  child_count;
    cbuffer_child_t *child;
} p11_cbuffer_t;

/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief cbuffer初始化
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] buf 缓存空间
 * @param [in] size 缓存总大小
 * @param [in] count 需要读数的对象总数
 * @param [in] child 需要读数的结构体私有属性
 */
/* ----------------------------------------------------------------------------*/
void cbuf_mult_read_init(p11_cbuffer_t *cbuffer, void *buf, u32 size, u32 count, cbuffer_child_t *child);


/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief cbuffer 通道开关
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 通道索引
 * @param [in] enable 通道开关
 */
/* ----------------------------------------------------------------------------*/
void cbuf_mult_entry_enable(p11_cbuffer_t *cbuffer, int index, int enable);

/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief 获取存储数据的字节长度
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @return 存储数据的字节长度
 */
/* ----------------------------------------------------------------------------*/
u32 cbuf_mult_read_get_data_len(p11_cbuffer_t *cbuffer, int index);


/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:memcpy管理+cbuf_read_alloc系列管理函数
 * @brief 预分配待读取数据的空间,并把读取到的数据存入buf数组
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @param [out] buf 存储读取的数据的目标buf数组
 * @param [in] len 要读取的数据的字节长度
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
u32 cbuf_mult_read_alloc_len(p11_cbuffer_t *cbuffer, int index, void *buf, u32 len);


/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:memcpy管理+cbuf_read_alloc系列管理函数
 * @brief 更新cbuf管理handle的读指针位置和数据长度
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] index 当前通道的索引
 * @param [in] len 要更新的数据的字节长度
 */
/* ----------------------------------------------------------------------------*/
void cbuf_mult_read_alloc_len_updata(p11_cbuffer_t *cbuffer, int index, u32 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:cb_memcpy管理
 * @brief 把buf数组数据拷贝p11_cbuffer_t结构体管理的内存空间
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] buf 指向用于存储写入内容的目标数组
 * @param [in] len 要写入的字节长度
 *
 * @return 成功写入的字节长度
 */
/* ----------------------------------------------------------------------------*/
u32 cbuf_mult_write(p11_cbuffer_t *cbuffer, void *buf, u32 len);
/* --------------------------------------------------------------------------*/
/**
 * @brief 适用范围:全局
 * @brief 判断是否可写入len字节长度的数据
 *
 * @param [in] cbuffer cbuffer 句柄
 * @param [in] len len字节长度的数据
 *
 * @return  返回可以写入的len字节长度
 */
/* ----------------------------------------------------------------------------*/
u32 cbuf_mult_is_write_able(p11_cbuffer_t *cbuffer, u32 len);
#endif




