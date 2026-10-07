#include "lv_anim_effect_internal.h"
#include "../../../../../src/core/lv_global.h"
#if LV_USE_ANIM_EFFECT

typedef struct {
    void * var;
    lv_anim_exec_xcb_t exec_cb;
} lv_anim_effect_del_info_t;

static void lv_anim_effect_ctx_exec_cb(void * var, int32_t val)
{
    lv_anim_effect_context_t * effect_var = (lv_anim_effect_context_t *)var;
    effect_var->exec_cb(var, val);
}

static void lv_anim_effect_direct_exec_cb(void * var, int32_t val)
{
    lv_anim_effect_context_t * effect_var = (lv_anim_effect_context_t *)var;
    effect_var->exec_cb(effect_var->obj, val);
}

static void lv_anim_effect_obj_delete_cb(lv_event_t * e)
{
    lv_anim_effect_delete(lv_event_get_current_target(e));
}

static void lv_anim_effect_delete_cb(lv_anim_t * anim)
{
    lv_free(anim->var);
    anim->var = NULL;
}

void lv_anim_effect_delete(lv_obj_t * obj)
{
    lv_ll_t var_list;
    lv_ll_init(&var_list, sizeof(lv_anim_effect_del_info_t));
    lv_ll_t * anim_ll_p = &(LV_GLOBAL_DEFAULT()->anim_state.anim_ll);
    lv_anim_t * a = lv_ll_get_head(anim_ll_p);
    lv_anim_t * a_next;
    while(a != NULL) {
        a_next = lv_ll_get_next(anim_ll_p, a);
        lv_anim_effect_context_t * var = a->var;
        if((a->exec_cb == lv_anim_effect_ctx_exec_cb || a->exec_cb == lv_anim_effect_direct_exec_cb) &&
           var != NULL && var->obj == obj) {
            lv_anim_effect_del_info_t * info = lv_ll_ins_head(&var_list);
            info->var = var;
            info->exec_cb = a->exec_cb;
        }
        a = a_next;
    }

    lv_anim_effect_del_info_t * info = lv_ll_get_head(&var_list);
    lv_anim_effect_del_info_t * next_info;
    while(info != NULL) {
        next_info = lv_ll_get_next(&var_list, info);
        lv_anim_del(info->var, info->exec_cb);
        info = next_info;
    }
    lv_ll_clear(&var_list);
}


void lv_anim_effect_add_obj_delete_cb(lv_obj_t * obj)
{
    // 没有可以判断某一监听器是否存在的接口，只能先移除再添加了
    lv_obj_remove_event_cb(obj, lv_anim_effect_obj_delete_cb);
    lv_obj_add_event_cb(obj, lv_anim_effect_obj_delete_cb, LV_EVENT_DELETE, NULL);
}

void lv_anim_effect_begin(lv_obj_t * obj)
{
    lv_anim_effect_delete(obj);
    lv_anim_effect_add_obj_delete_cb(obj);
}


void lv_anim_effect_init(lv_anim_t * anim, lv_obj_t * obj, lv_anim_exec_xcb_t exec_cb)
{
    lv_anim_set_deleted_cb(anim, lv_anim_effect_delete_cb);
    lv_anim_set_exec_cb(anim, lv_anim_effect_ctx_exec_cb);
    lv_anim_effect_context_t * var = lv_malloc(sizeof(lv_anim_effect_context_t));
    LV_ASSERT_MALLOC(var);
    var->exec_cb = exec_cb;
    var->obj = obj;
    lv_anim_set_var(anim, var);
}

void lv_anim_effect_init_direct(lv_anim_t * anim, lv_obj_t * obj, lv_anim_exec_xcb_t exec_cb)
{
    lv_anim_set_deleted_cb(anim, lv_anim_effect_delete_cb);
    lv_anim_set_exec_cb(anim, lv_anim_effect_direct_exec_cb);
    lv_anim_effect_context_t * var = lv_malloc(sizeof(lv_anim_effect_context_t));
    LV_ASSERT_MALLOC(var);
    var->exec_cb = exec_cb;
    var->obj = obj;
    lv_anim_set_var(anim, var);
}

void lv_anim_effect_start_direct(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end,
                                 lv_anim_exec_xcb_t exec_cb, lv_anim_path_cb_t path_cb, bool early_apply)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_early_apply(&anim, early_apply);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_delay(&anim, delay);
    lv_anim_set_path_cb(&anim, path_cb);
    lv_anim_set_time(&anim, time);
    lv_anim_effect_init_direct(&anim, obj, exec_cb);
    lv_anim_start(&anim);
}

void lv_anim_effect_start_ctx(lv_obj_t * obj, int32_t delay, int32_t time, int32_t start, int32_t end,
                              lv_anim_exec_xcb_t exec_cb, lv_anim_path_cb_t path_cb, bool early_apply)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_early_apply(&anim, early_apply);
    lv_anim_set_values(&anim, start, end);
    lv_anim_set_delay(&anim, delay);
    lv_anim_set_path_cb(&anim, path_cb);
    lv_anim_set_time(&anim, time);
    lv_anim_effect_init(&anim, obj, exec_cb);
    lv_anim_start(&anim);
}

void lv_anim_effect_start_opa_inout(lv_obj_t * obj, int32_t delay, int32_t time, bool in,
                                    lv_anim_path_cb_t path_cb, bool early_apply)
{
    int32_t start = in ? LV_OPA_0 : LV_OPA_100;
    int32_t end = in ? LV_OPA_100 : LV_OPA_0;
    lv_anim_effect_set_opa(obj, start);
    lv_anim_effect_start_direct(obj, delay, time, start, end, (lv_anim_exec_xcb_t)lv_anim_effect_set_opa,
                                path_cb, early_apply);
}

#endif /*LV_USE_ANIM_EFFECT*/
