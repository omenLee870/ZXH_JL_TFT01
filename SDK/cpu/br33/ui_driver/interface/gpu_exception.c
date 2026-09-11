/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_exception.c
 *
 * @brief GPU异常信息打印模块
 * 开启 GPU_EXCEPTION_ANALYZE_ENABLE 后，如果CPU或外设发生异常，且异常ID与GPU相关时，
 * gpu_exception_analyze() 会被调用，并打印出GPU相关的配置信息用于协助异常debug。
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-05-30
 */
/* ------------------------------------------------------------------------------------*/


#include "gpu.h"


extern const int GPU_EXCEPTION_ANALYZE_ENABLE; /* GPU 异常分析标志 */
extern int debug_puts(const char *s);  /* 异常时打印字符串 */
extern void debug_put_u32hex(u32 dat);  /* 异常时打印 HEX值 */


/* ***************************************************************************/
/**
 * \Brief :        判断异常ID是否与GPU有关
 *
 * \Param :        id 异常设备ID
 *
 * \Return :        true 异常与GPU有关，gpu_exception_analyze 被调用打印信息；false 异常与GPU无关，gpu_exception_analyze 不被调用
 */
/* ***************************************************************************/
int gpu_exception_analyze_check_id(u32 id)
{
    if (GPU_EXCEPTION_ANALYZE_ENABLE) {
        //GPU2D 0xa7
        //GXDEC 0xa8
        if ((id == 0xa7) || (id == 0xa8)) {
            return true;
        }
    }
    return false;
}


/* ***************************************************************************/
/**
 * \Brief :        打印当前GPU相关的配置
 */
/* ***************************************************************************/
void gpu_exception_analyze()
{
    if (GPU_EXCEPTION_ANALYZE_ENABLE) {
        int i = 0;
        void *task = gpu_hw_get_curr_inst();
        void *unit = jlgpu_chain_task_get_unit(task);
        int   size = jlgpu_task_size(task) / 4;

        debug_puts("\n========== [GPU INSTRUCTION INFO] ==========\n");
        for (i = 0; i < jlgpu_chain_task_info_len() / 4; i ++) {
            debug_put_u32hex(((u32*)unit)[i]);
            debug_puts("\n");
        }

        debug_puts("\n========== [GPU INSTRUCTION INST] ==========\n");
        for (i = 0; i < size; i ++) {
            debug_put_u32hex(((u32*)task)[i]);
            debug_puts("\n");
        }

        debug_puts("\n========== [GPU INSTRUCTION END] ==========\n");
    }
}


