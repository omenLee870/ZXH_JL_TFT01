/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/**
 * @file gpu.h
 * @brief GPU硬件驱动模块统一对外接口头文件
 * @details 本文件作为GPU模块的统一入口，外部使用时只需include此文件即可调用所有GPU相关API。
 *          包含了GPU任务操作、渲染控制、任务链管理、格式转换、缓存管理、矢量路径等完整功能。
 *
 * @note 使用示例：
 * @code
 * #include "gpu.h"
 * // 即可使用所有GPU API
 * @endcode
 *
 * @author GPU Driver Team
 * @version 1.0
 * @date 2026
 */

#ifndef __PUBLIC_GPU_DRIVER_H__
#define __PUBLIC_GPU_DRIVER_H__


/*============================ 头文件包含 ============================*/

/* GPU硬件底层驱动接口 - 外部不可直接调用 */
#include "drive/jlgpu_driver.h"

/* GPU任务单元配置模块 - 提供单个GPU任务的创建和配置接口 */
#include "gpu_port.h"

/* GPU渲染控制模块 - 提供渲染启动、暂停、参数配置等接口 */
#include "gpu_render.h"

/* GPU任务链管理模块 */
#include "gpu_chain.h"          /**< 单任务链操作接口 */
#include "gpu_chain_mult.h"     /**< 多任务链管理接口 */
#include "gpu_chain_tran.h"     /**< 任务链矩阵变换接口 */

/* GPU帧缓冲渲染队列模块 - 使用帧buf缓存时才使用(待完善) */
#include "gpu_queue.h"

/* GPU格式转换模块 - 实现GPU硬件格式与杰理UI框架格式互转 */
#include "gpu_format.h"

/* GPU数据缓存模块 - 继承自AC707N项目，提供数据缓存功能 */
#include "gpu_cache.h"

/* 区域转换模块 - 提供区域坐标转换功能，如UI框架的rect转换为GPU的bbox */
#include "box_convert.h"

/* GPU矢量路径模块 - 提供软件生成矢量path用于GPU渲染 */
#include "gpu_path.h"

/* GPU API兼容映射模块 - 将本项目API映射到AC707N，方便项目迁移 */
#include "gpu_map_707n.h"


#endif /**< __PUBLIC_GPU_DRIVER_H__ */

