#ifndef __JLGPU_CUSTOMIZE_H__
#define __JLGPU_CUSTOMIZE_H__
#include"jlgpu_hal.h"

void *jlgpu_task_get_next(void *cur_task);
void jlgpu_task_set_next(void *cur_task, void *next_task);

void jlgpu_draw_image_no_trans_to_trans(void *old_task, void *new_task, jlgpu_tex_samplemode_t tex_samplemode, uint8_t shift, uint8_t *coeff, float *matrix);
void jlgpu_draw_image_trans_to_no_trans(void *old_task, void *new_task);
void jlgpu_draw_fill_no_trans_to_trans(void *old_task, void *new_task, float *matrix);
void jlgpu_draw_fill_trans_to_no_trans(void *old_task, void *new_task);


//debug all tasks
void jlgpu_dumcp_all_tasks(void *task_base);
//debug cur task
void jlgpu_dump_task(void *cur_task);

//debug sfr win
void jlgpu_dump_sfr_win();
//debug sfr trans
void jlgpu_dump_sfr_trans();
//debug sfr colorkey
void jlgpu_dump_sfr_colorkey();
//debug sfr vtx
void jlgpu_dump_sfr_vtx();
//debug sfr uvadr
void jlgpu_dump_sfr_uvadr();

#endif
