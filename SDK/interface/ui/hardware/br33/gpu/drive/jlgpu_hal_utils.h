#ifndef __JLGPU_HAL_UTILS_H__
#define __JLGPU_HAL_UTILS_H__

#include "hwGpu.h"
#include "jlgpu_def.h"
#include<stdint.h>

typedef struct {
    uint8_t					cmd_offset;
    uint8_t					paint1_offset;
    uint8_t					paint2_offset;
    uint8_t					tex_offset;
    uint8_t					mask_offset;
    uint8_t					seg_offset;
} jlgpu_render_task_info_t;

typedef struct {
    JLGPU_CMD_TypeDef *cmd_ptr;
    JLGPU_FragPaint_Basic *paint1_ptr;
    JLGPU_FragPaint_Basic *paint2_ptr;
    JLGPU_FragImage_Basic *tex_ptr;
    JLGPU_Mask_Basic *mask_ptr;
    JLGPU_Segment_Basic *seg_ptr;
} jlgpu_render_task_ptr_t;


uint8_t compute_paint_task_len_hal(jlgpu_trans_mode_t pnt_trans_mode);
uint8_t compute_texture_task_len_hal(jlgpu_trans_mode_t tex_trans_mode);

uint8_t gpu_compute_render_task_len_hal(jlgpu_fg_mode_t fg_mode, jlgpu_trans_mode_t pnt_trans_mode, jlgpu_trans_mode_t tex_trans_mode, jlgpu_trans_mode_t seg_trans_mode, jlgpu_mask_mode_t mask_mode, jlgpu_render_task_info_t *render_task_offset);
uint8_t gpu_compute_extend_task_len_hal(jlgpu_extend_command_t command);
void gpu_get_hw_ptr_hal(uint8_t *task, jlgpu_fg_mode_t fg_mode, jlgpu_trans_mode_t pnt_trans_mode, jlgpu_trans_mode_t tex_trans_mode, jlgpu_trans_mode_t seg_trans_mode, jlgpu_mask_mode_t mask_mode, jlgpu_render_task_info_t *render_task_offset, jlgpu_render_task_ptr_t *render_task_ptr);

void gpu_dump_task_hal(void *cur_task);
void gpu_dumcp_all_tasks_hal(void *task_base);
#endif
