#ifndef __GPU_BUF_MANAGER_H__
#define __GPU_BUF_MANAGER_H__

#include "system/includes.h"

struct gpu_buf {
    struct list_head entry;
    int elm_id;
    int task_id;
    int index;
    u8 *buf;
    u32 size;
};

void gpu_buf_init();
struct gpu_buf *gpu_buf_search(int elm_id, int task_id, int index);
int gpu_buf_add(int elm_id, int task_id, int index, u8 *buf, u32 size);
void gpu_buf_list_free();

#endif
