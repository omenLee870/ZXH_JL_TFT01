#ifndef __BREAKPOINT_H__
#define __BREAKPOINT_H__

#include "typedef.h"

#define BREAKPOINT_MAX_NUM                  2

/* --------------------------------------------------------------------------*/
/**
 * @brief 断点结构体信息
 */
/* ----------------------------------------------------------------------------*/
enum BREAKPOINT_MODE {
    BREAKPOINT_INSERT_MODE = 0,  		//插入模式
    BREAKPOINT_NORMAL_SKIP_MODE,  		//普通函数替换模式
    BREAKPOINT_INTERRUPT_SKIP_MODE,  	//中断函数替换模式
    BREAKPOINT_PART_REWRITE_MODE,  		//函数内部语句部分重写模式
    BREAKPOINT_CHILD_MODE,  			//子断点入口
};

typedef struct breakpoint_settings {
    u32 mode;                       //0:插入模式，1:跳转模式
    union {
        struct {
            void *target_addr;      //替换断点位置
            void *rewrite_fun;      		//替换处理函数
        };
        struct {
            void *insert_target_addr;   //插入断点位置
            void *insert_fun;      		//插入处理函数
        };
        struct {
            void *part_rewrite_target_addr;     //函数部分重写断点位置
            void *part_rewrite_fun;      		//函数部分重写处理函数
            void *part_rewrite_fun_return_addr; //函数部分重写返回地址
        };
        struct {
            struct breakpoint_settings *child;
            u32 child_item_num;
        };
    };
} BP_ST;

struct breakpoint_handle {
    BP_ST *bp_table;
    u8 bp_num;
};

extern const BP_ST break_point_table[];

/* ---------------------------------------------------------------------------- */
/**
 * @brief 断点函数初始化
 *
 * @param bp_table: 断点列表
 * @param bp_num: 断点数目
 *
 * @return
 */
/* ---------------------------------------------------------------------------- */
int breakpoint_init(BP_ST *bp_table, u8 bp_num);

void breakpoint_reg_modify(BP_ST *old_bp, BP_ST *new_bp);

#endif /* #ifndef __BREAKPOINT_H__ */
