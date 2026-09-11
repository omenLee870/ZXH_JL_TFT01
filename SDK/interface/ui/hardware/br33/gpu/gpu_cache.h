/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
#ifndef __GPU_CACHE_H__
#define __GPU_CACHE_H__

#include "typedef.h"

/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_cache.h
 *
 * @brief GPU 输入流缓存模块 — 基于 cache_pool 实现
 *
 * 模块职责：
 * 1. 缓存 GPU 输入流数据（纹理、tile、图片等），按 check 特征值检索
 * 2. 底层节点管理复用 cache_pool 模块（句柄 + 节点 + 生命值 + 锁定）
 * 3. 通过 cache_pool 的 weak 内存接口覆盖，转发到本模块注册的 malloc/free 回调
 *
 * 核心概念：
 * - check：应用层指定的数据特征值（crc/hash），作为 cache_pool 节点的唯一 ID
 * - cache_addr：缓存数据地址（cache_pool 节点的数据区指针）
 * - valid：有效值，0xff 表示常驻（锁定），其他值表示生命次数（TTL）
 *
 * 使用流程：
 * @code
 * gpu_cache_set_alloc_cb(malloc, free);
 * gpu_cache_init(read_cb, free_cache);
 * void *buf = gpu_cache_add(priv, data, len, check, mmu_tb, mmu_size);
 * ...
 * void *hit = gpu_cache_check(check);
 * gpu_cache_reset();      // 页面切换时清空缓存
 * gpu_cache_free();       // 模块销毁
 * @endcode
 *
 * @author zhuhaifang
 *
 * @version V1.0.0
 *
 * @date 2026-08-21
 */
/* ------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 注册缓存内存申请/释放回调
 *
 * 覆盖 cache_pool 的 weak 内存接口，转发到注册的回调。未注册时默认使用 PSRAM。
 *
 * @Params malloc 内存申请回调（size, ram_type, module_type）
 * @Params free   内存释放回调（buf, ram_type, module_type）
 */
/* ------------------------------------------------------------------------------------*/
void gpu_cache_set_alloc_cb(void *(*malloc)(int size, u32 ram_type, u32 module_type),
                            void (*free)(void *buf, u32 ram_type, u32 module_type));

#include "ui_math/ui_prime.h"
#define gpu_cache_hash_id(v, c) ui_prime_hash_int(v, c)

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 初始化 gpu cache 模块
 *
 * @Params read_cb    读数据回调，NULL 时使用默认 memcpy + psram flush
 * @Params free_cache 释放缓存回调（cache_pool 方案下暂未使用，预留）
 */
/* ------------------------------------------------------------------------------------*/
void gpu_cache_init(void (*read_cb)(void *priv, void *dst, void *src, int len),
                    int (*free_cache)(void *cache));

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 释放 cache 模块句柄（需先清空数据再释放）
 */
/* ------------------------------------------------------------------------------------*/
void gpu_cache_free(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 加载数据到缓存
 *
 * @Params priv     私有句柄（文件句柄、资源地址等）
 * @Params data     需要拷贝的数据地址（mmu 模式下为页内偏移）
 * @Params len      拷贝数据长度
 * @Params check    数据特征值（唯一 ID）
 * @Params mmu_tb   flash mmu 页表，NULL 表示非 mmu 模式
 * @Params mmu_size mmu 页表项数
 *
 * @return 缓存数据地址，失败返回 NULL
 */
/* ------------------------------------------------------------------------------------*/
void *gpu_cache_add(void *priv, void *data, int len, u32 check, void *mmu_tb, int mmu_size);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 加载数据到缓存（支持私有读回调）
 *
 * 参数同 gpu_cache_add，read_cb 为本次加载使用的读回调。
 */
/* ------------------------------------------------------------------------------------*/
void *gpu_cache_add_with_callback(void *priv, void *data, int len, u32 check,
                                  void *mmu_tb, int mmu_size,
                                  void (*read_cb)(void *priv, void *dst, void *src, int len));

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 删除指定地址的缓存数据
 *
 * @Params cache_addr 缓存数据地址
 *
 * @return 0 成功，-1 失败（cache_pool 无对应节点时返回 -1）
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_del(void *cache_addr);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过特征值 check 查找缓存资源地址
 *
 * @Params check 数据特征值
 *
 * @return 缓存资源地址，未找到返回 NULL
 */
/* ------------------------------------------------------------------------------------*/
void *gpu_cache_check(u32 check);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 更新缓存 buf 大小
 *
 * @Params size  新 buf 需要的大小
 * @Params check 数据特征值
 *
 * @return 缓存数据地址，失败返回 NULL
 */
/* ------------------------------------------------------------------------------------*/
void *gpu_cache_realloc(int size, u32 check);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 清空全部缓存数据
 *
 * @return 0 成功，-1 失败
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_reset(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 清除所有有效值 <= 0 的数据
 *
 * @return 0 成功，-1 失败
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_clr_all_invalid(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 设置节点有效值
 *
 * @Params cache_addr 缓存数据地址
 * @Params value      有效值，0xff 表示常驻（锁定），其他值表示生命次数
 *
 * @return 0 成功，-1 失败
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_valid_value_set(u32 cache_addr, u8 value);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 所有节点有效值自减
 *
 * @return 0 成功，-1 失败
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_valid_sub(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 指定节点有效值自增
 *
 * @Params cache_addr 缓存数据地址
 *
 * @return 0 成功，-1 失败
 */
/* ------------------------------------------------------------------------------------*/
int gpu_cache_valid_add(u32 cache_addr);

void gpu_cache_dump_all();


#endif
