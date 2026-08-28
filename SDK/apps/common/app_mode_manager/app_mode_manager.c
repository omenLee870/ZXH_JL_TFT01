#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_mode_manager.data.bss")
#pragma data_seg(".app_mode_manager.data")
#pragma const_seg(".app_mode_manager.text.const")
#pragma code_seg(".app_mode_manager.text")
#endif
#include "app_mode_manager.h"
#include "system/init.h"
#include "generic/errno-base.h"



#define MODE_STACK_SIZE     2



struct app_mode_mgr {
    struct list_head head;
    struct list_head head2;
    struct app_mode *curr_mode;
    u8 mode_stack[MODE_STACK_SIZE];
    u8 stack_cnt;
};

static struct app_mode_mgr g_mode_mgr;

#define for_each_app_mode(p) \
    list_for_each_entry(p, &g_mode_mgr.head, entry)

#define for_each_app_unactive_mode(p) \
    list_for_each_entry(p, &g_mode_mgr.head2, entry)

struct app_mode *app_get_current_mode()
{
    return g_mode_mgr.curr_mode;
}


u8 app_get_current_mode_name()
{
    if (g_mode_mgr.curr_mode) {
        return g_mode_mgr.curr_mode->name;
    } else {
        return 0;
    }
}

void app_set_current_mode(struct app_mode *mode)
{
    g_mode_mgr.curr_mode = mode;
}

struct app_mode *app_get_mode_by_name(u8 name)
{
    struct app_mode *p;

    for_each_app_mode(p) {
        if (p->name == name) {
            return p;
        }
    }

    for_each_app_unactive_mode(p) {
        if (p->name == name) {
            return p;
        }
    }
    return NULL;
}

bool app_have_mode(void)
{
    if (g_mode_mgr.curr_mode) {
        return true;
    }
    return false;
}

bool app_in_mode(u8 name)
{
    if (g_mode_mgr.curr_mode) {
        return g_mode_mgr.curr_mode->name == name;
    }
    return false;
}

struct app_mode *app_next_mode(struct app_mode *mode)
{
    struct list_head *next;

    next = mode->entry.next;
    if (next == &g_mode_mgr.head) {
        next = g_mode_mgr.head.next;
    }
    return list_entry(next, struct app_mode, entry);
}

struct app_mode *app_get_next_mode()
{
    struct list_head *next;
    struct app_mode *p;
    bool current_mode_unactive = FALSE;

    /*检查当前任务是否在unactive列表，如POWERON*/
    for_each_app_unactive_mode(p) {
        if (p->name == g_mode_mgr.curr_mode->name) {
            current_mode_unactive = TRUE;
            break;
        }
    }
    if (current_mode_unactive) {
        next = g_mode_mgr.head.next;
    } else {
        next = g_mode_mgr.curr_mode->entry.next;
    }

    if (next == &g_mode_mgr.head) {
        next = g_mode_mgr.head.next;
    }
    return list_entry(next, struct app_mode, entry);
}

struct app_mode *app_get_prev_mode()
{
    struct list_head *prev;

    prev = g_mode_mgr.curr_mode->entry.prev;
    if (prev == &g_mode_mgr.head) {
        prev = g_mode_mgr.head.prev;
    }
    return list_entry(prev, struct app_mode, entry);
}

int app_goto_next_mode(int param)
{
    struct app_mode *mode = app_get_next_mode();
    return app_goto_mode(mode->name, param);
}

int app_goto_prev_mode(int param)
{
    struct app_mode *mode = app_get_prev_mode();
    return app_goto_mode(mode->name, param);
}

int app_exit_mode(struct app_mode *mode)
{
    int err = 0;

    if (!mode) {
        return 0;
    }
    if (mode->ops && mode->ops->try_exit) {
        err = mode->ops->try_exit();
        if (err) {
            return err;
        }
    }
    if (mode->ops && mode->ops->exit) {
        err = mode->ops->exit();
    }
    if (err == 0 && g_mode_mgr.curr_mode == mode) {
        g_mode_mgr.curr_mode = NULL;
    }

    return err;
}

int app_exit_current_mode()
{
    return app_exit_mode(g_mode_mgr.curr_mode);
}

bool app_try_exit_curr_mode()
{
    struct app_mode *curr = g_mode_mgr.curr_mode;

    if (curr && curr->ops && curr->ops->try_exit) {
        int err = curr->ops->try_exit();
        if (err) {
            return 0;
        }
    }
    return 1;
}

bool app_try_enter_mode(struct app_mode *mode, int param)
{
    if (mode && mode->ops && mode->ops->try_enter) {
        int err = mode->ops->try_enter(param);
        if (err) {
            return 0;
        }
    }
    return 1;
}

int app_goto_mode(u8 mode_name, int param)
{
    int err = 0;
    struct app_mode *mode;
    struct app_mode *curr = g_mode_mgr.curr_mode;

    if (curr) {
        if (curr->name == mode_name) {
            return 0;
        }
        if (curr->ops && curr->ops->try_exit) {
            err = curr->ops->try_exit();
            if (err) {
                return err;
            }
        }
    }

    mode = app_get_mode_by_name(mode_name);
    if (!mode) {
        return -EINVAL;
    }
    if (mode->ops && mode->ops->try_enter) {
        err = mode->ops->try_enter(param);
        if (err) {
            return err;
        }
    }
    if (curr && curr->ops && curr->ops->exit) {
        curr->ops->exit();
    }
    if (mode->ops && mode->ops->enter) {
        err = mode->ops->enter(param);
    }
    if (err == 0) {
        g_mode_mgr.curr_mode = mode;
    }
    return err;
}

int app_push_mode(u8 mode_name)
{
    if (g_mode_mgr.stack_cnt < MODE_STACK_SIZE) {
        g_mode_mgr.mode_stack[g_mode_mgr.stack_cnt++] = mode_name;
        return 0;
    } else {
        if (g_mode_mgr.stack_cnt >= 2) {
            for (int i = 0; i < g_mode_mgr.stack_cnt - 2; i++) {
                g_mode_mgr.mode_stack[i] = g_mode_mgr.mode_stack[i + 1] ;
            }
        }
        g_mode_mgr.mode_stack[g_mode_mgr.stack_cnt - 1] = mode_name;
        return 0;
    }
}

u8 app_pop_mode()
{
    if (g_mode_mgr.stack_cnt) {
        return g_mode_mgr.mode_stack[--g_mode_mgr.stack_cnt];
    }
    return 0xff;
}
u8 app_mode_stack_clr(u8 mode_name)
{
    /* for(int i = 0;i<g_mode_mgr.stack_cnt ;i++){ */
    /* printf("%s [%d]task:%d stack_cnt:%d \n",__func__,i,g_mode_mgr.mode_stack[i],g_mode_mgr.stack_cnt); */
    /* } */
    for (int i = 0; i < g_mode_mgr.stack_cnt; i++) {
        if (g_mode_mgr.mode_stack[i] == mode_name) {
            for (int j = i; j < g_mode_mgr.stack_cnt; j++) { //仅对有效部分元素进行自加
                g_mode_mgr.mode_stack[j] = g_mode_mgr.mode_stack[j + 1];
            }
            --i;
            --g_mode_mgr.stack_cnt;
        }
    }
    return 0;
}
int app_goto_background(u8 mode_name)
{
    int err = 0;
    struct app_mode *mode;

    mode = app_get_mode_by_name(mode_name);
    if (!mode) {
        return -EINVAL;
    }
    if (mode->ops && mode->ops->goto_background) {
        err = mode->ops->goto_background();
    }
    if (err == 0 && g_mode_mgr.curr_mode == mode) {
        g_mode_mgr.curr_mode = NULL;
    }
    return err;
}

int app_goto_foreground(u8 mode_name)
{
    int err = 0;
    struct app_mode *mode;

    mode = app_get_mode_by_name(mode_name);
    if (!mode) {
        return -EINVAL;
    }
    if (mode->ops && mode->ops->goto_foreground) {
        err = mode->ops->goto_foreground();
    }
    if (err == 0) {
        g_mode_mgr.curr_mode = mode;
    }
    return err;
}

static int app_mode_mgr_init()
{
    struct app_mode *mode, *p;

    g_mode_mgr.stack_cnt = 0;
    g_mode_mgr.curr_mode = NULL;
    INIT_LIST_HEAD(&g_mode_mgr.head);
    INIT_LIST_HEAD(&g_mode_mgr.head2);

    list_for_each_app_mode(mode) {
        if (mode->index == 0xff) {
            list_add_tail(&mode->entry, &g_mode_mgr.head2);
            continue;
        }

        int insert = 0;
        for_each_app_mode(p) {
            if (p->index > mode->index) {
                __list_add(&mode->entry, p->entry.prev, &p->entry);
                insert = 1;
                break;
            }
        }
        if (!insert) {
            list_add_tail(&mode->entry, &g_mode_mgr.head);
        }
    }
    return 0;
}
early_initcall(app_mode_mgr_init);


