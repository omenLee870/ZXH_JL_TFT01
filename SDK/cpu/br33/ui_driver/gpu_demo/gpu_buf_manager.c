#include "generic/list.h"
#include "system/includes.h"
#include "system/spinlock.h"
#include "gpu_buf_manager.h"

static struct list_head buf_manager_head;

/* 链表在 UI 绘制线程(gpu_buf_add)与 GPU 合成任务(gpu_buf_list_free)间并发访问, 需加锁保护 */
DEFINE_SPINLOCK(gpu_buf_lock);


void gpu_buf_init()
{
    INIT_LIST_HEAD(&buf_manager_head);
}

struct gpu_buf *gpu_buf_search(int elm_id, int task_id, int index)
{
    struct gpu_buf *p, *n;
    spin_lock(&gpu_buf_lock);
    list_for_each_entry_safe(p, n, &buf_manager_head, entry) {
        if ((p->elm_id == elm_id) && (p->task_id == task_id) && (p->index == index)) {
            spin_unlock(&gpu_buf_lock);
            return p;
        }
    }
    spin_unlock(&gpu_buf_lock);
    return NULL;
}


int gpu_buf_add(int elm_id, int task_id, int index, u8 *buf, u32 size)
{
    struct gpu_buf *gpu_buf_t = malloc(sizeof(struct gpu_buf));
    if (!gpu_buf_t) {
        return -1;
    }
    gpu_buf_t->elm_id = elm_id;
    gpu_buf_t->task_id = task_id;
    gpu_buf_t->index = index;
    gpu_buf_t->buf = buf;
    gpu_buf_t->size = size;
    spin_lock(&gpu_buf_lock);
    list_add_tail(&gpu_buf_t->entry, &buf_manager_head);
    spin_unlock(&gpu_buf_lock);
    return 0;
}

void gpu_buf_list_free()
{
    struct gpu_buf *p, *n;
    struct list_head free_head;

    INIT_LIST_HEAD(&free_head);

    /* 临界区仅做摘链操作, 内存释放放到锁外, 避免持锁期间调用 free */
    spin_lock(&gpu_buf_lock);
    list_for_each_entry_safe(p, n, &buf_manager_head, entry) {
        list_del(&p->entry);
        list_add_tail(&p->entry, &free_head);
    }
    spin_unlock(&gpu_buf_lock);

    list_for_each_entry_safe(p, n, &free_head, entry) {
        if (p->buf) {
            free(p->buf);
        }
        list_del(&p->entry);
        free(p);
    }
}
