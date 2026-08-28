#include "generic/list.h"
#include "system/includes.h"

static struct list_head buf_manager_head;

struct gpu_buf {
    struct list_head entry;
    u8 *buf;
    u32 size;
};

void gpu_buf_init()
{
    INIT_LIST_HEAD(&buf_manager_head);
}

int gpu_buf_add(u8 *buf, u32 size)
{
    struct gpu_buf *gpu_buf_t = malloc(sizeof(struct gpu_buf));
    if (!gpu_buf_t) {
        return -1;
    }
    gpu_buf_t->buf = buf;
    gpu_buf_t->size = size;
    list_add_tail(&gpu_buf_t->entry, &buf_manager_head);
    return 0;
}

void gpu_buf_list_free()
{
    struct gpu_buf *p, *n;
    list_for_each_entry_safe(p, n, &buf_manager_head, entry) {
        if (p->buf) {
            free(p->buf);
        }
        list_del(&p->entry);
        free(p);
    }
}
