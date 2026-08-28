#include "lv_anim_effect_internal.h"
#include "../../../../../src/core/lv_global.h"
#if LV_USE_ANIM_EFFECT

static void lv_anim_effect_exec_cb(void *var, int32_t val)
{
    lv_anim_effect_context_t *effect_var = (lv_anim_effect_context_t *)var;
    effect_var->exec_cb(var, val);
}

static void lv_anim_effect_obj_delete_cb(lv_event_t *e)
{
    lv_anim_effect_delete(lv_event_get_current_target(e));
}

static void lv_anim_effect_delete_cb(lv_anim_t *anim)
{
    lv_free(anim->var);
    anim->var = NULL;
}

void lv_anim_effect_delete(lv_obj_t *obj)
{
    lv_ll_t var_list;
    lv_ll_init(&var_list, sizeof(void *));
    lv_ll_t *anim_ll_p = &(LV_GLOBAL_DEFAULT()->anim_state.anim_ll);
    lv_anim_t *a = lv_ll_get_head(anim_ll_p);
    lv_anim_t *a_next;
    lv_anim_t *target_anim = NULL;
    while (a != NULL) {
        a_next = lv_ll_get_next(anim_ll_p, a);
        lv_anim_effect_context_t *var = a->var;
        if (a->exec_cb == lv_anim_effect_exec_cb && var->obj == obj) {
            void **var_p = lv_ll_ins_head(&var_list);
            *var_p = var;
        }
        a = a_next;
    }

    void **var_p = lv_ll_get_head(&var_list);
    void **next_var_p;
    while (var_p != NULL) {
        next_var_p = lv_ll_get_next(&var_list, var_p);
        lv_anim_del(*var_p, lv_anim_effect_exec_cb);
        var_p = next_var_p;
    }
    lv_ll_clear(&var_list);
}


void lv_anim_effect_add_obj_delete_cb(lv_obj_t *obj)
{
    // 没有可以判断某一监听器是否存在的接口，只能先移除再添加了
    lv_obj_remove_event_cb(obj, lv_anim_effect_obj_delete_cb);
    lv_obj_add_event_cb(obj, lv_anim_effect_obj_delete_cb, LV_EVENT_DELETE, NULL);
}


void lv_anim_effect_init(lv_anim_t *anim, lv_obj_t *obj, lv_anim_exec_xcb_t exec_cb)
{
    lv_anim_set_deleted_cb(anim, lv_anim_effect_delete_cb);
    lv_anim_set_exec_cb(anim, lv_anim_effect_exec_cb);
    lv_anim_effect_context_t *var = lv_malloc(sizeof(lv_anim_effect_context_t));
    LV_ASSERT_MALLOC(var);
    var->exec_cb = exec_cb;
    var->obj = obj;
    lv_anim_set_var(anim, var);
}

#endif /*LV_USE_ANIM_EFFECT*/
