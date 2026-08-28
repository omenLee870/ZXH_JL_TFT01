#include "asm/breakpoint.h"
#include "app_config.h"
#include "init.h"

/* --------------------------------------------------------------------------*/
/**
 * @brief 断点表
 * @note 如果需要动态修改或调用breakpoint_reg_modify则需要去掉const
 */
/* ----------------------------------------------------------------------------*/
const BP_ST break_point_table[] = {
};

static int app_breakpoint_init(void)
{
    return breakpoint_init((BP_ST *)break_point_table, ARRAY_SIZE(break_point_table));
}

early_initcall(app_breakpoint_init);

