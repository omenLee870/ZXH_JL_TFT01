#ifndef __UI_EFFECT_MANAGER_H__
#define __UI_EFFECT_MANAGER_H__

#include "ui_debug.h"


typedef enum {
    UI_EFFECT_NONE,
    UI_EFFECT_CUBE,
    UI_EFFECT_CUBE_3D,
    UI_EFFECT_FOOTBALL,
} ui_effect_type_t;


int ui_effect_manager_init();
int ui_effect_manager_free();

int ui_effect_manager_switch_type(ui_effect_type_t type, void *private);
int ui_effect_manager_touch();
int ui_effect_manager_click();

typedef void (*ui_effect_draw)(ui_effect_type_t type, float *M, void *private);


#define REGISTER_UI_EFFECT_HDL(_type)	REGISTER_UI_EFFECT_OPS(_type)



struct ui_effect_ops {
    int   type;     /*! 特效类型     */
    int (*init)();  /*! 特效初始化   */
    int (*enter)(); /*! 特效进入动画 */
    int (*touch)(); /*! 特效划动回调 */
    int (*click)(int x, int y); /*! 特效点击回调 */
    int (*align)(); /*! 特效对齐回调 */
    int (*leave)(); /*! 特效离开动画 */
    int (*free)();  /*! 特效释放     */
};


extern const struct ui_effect_ops ui_effect_ops_begin[];
extern const struct ui_effect_ops ui_effect_ops_end[];


#define REGISTER_UI_EFFECT_OPS(_type) \
	static const struct ui_effect_ops effect_ops_##_type sec(.ui_effect_ops) __attribute__((used)) = { \
		.type = _type,


#define ui_effect_get_ops_by_type(_type) \
		({ \
		 	printf("effect number: %lu\n", ui_effect_ops_end - ui_effect_ops_begin); \
		 	const struct ui_effect_ops *ops, *ret = NULL; \
		 	for (ops = ui_effect_ops_begin; ops < ui_effect_ops_end; ops ++) { \
				printf("ops->type: %d\n", ops->type); \
		 		if (ops->type == _type) { \
		 			ret = ops; \
		 			break; \
		 		} \
		 	} \
		 	ret; \
		 })


#endif

