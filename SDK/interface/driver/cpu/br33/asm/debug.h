#ifndef __DEBUG_H__
#define __DEBUG_H__


typedef enum etm_detect_mode {
    CPU_SFR_DETECT_MODE = 1,
    CPU_RD_BUS_DETECT_MODE,
    CPU_WR_BUS_DETECT_MODE,
    CPU_RD_WR_BUS_DETECT_MODE,
} ETM_DETECT_MODE;


#define CDBG_IDx(n, id) ((1<<(n+4)) | (id<<(n*8+8)))
#define CDBG_INV         (1<<7)
#define CDBG_PEN         (1<<3)
#define CDBG_XEN         (1<<2)
#define CDBG_WEN         (1<<1)
#define CDBG_REN         (1<<0)

void debug_init();
void exception_analyze();

/********************************** DUBUG SFR *****************************************/

u32 get_dev_id(char *name);

/* ---------------------------------------------------------------------------- */
/**
 * @brief Memory权限保护设置
 *
 * @param idx: 保护框索引, 范围: 0 ~ 3, 目前系统默认使用0和3, 用户可用1和2
 * @param begin: Memory开始地址
 * @param end: Memory结束地址
 * @param inv: 0: 保护框内, 1: 保护框外
 * @param format: "Cxwr0rw1rw2rw3rw", CPU:外设0:外设1:外设2:外设3,
 * @param ...: 外设ID号索引, 如: DBG_EQ, 见debug.h
 */
/* ---------------------------------------------------------------------------- */
void mpu_set(int idx, u32 begin, u32 end, u32 inv, const char *format, ...);


/* ---------------------------------------------------------------------------- */
/**
 * @brief 取消指定框的mpu保护
 *
 * @param idx: 保护框索引号
 */
/* ---------------------------------------------------------------------------- */
void mpu_disable_by_index(u8 idx);


/* ---------------------------------------------------------------------------- */
/**
 * @brief :取消所有保护框mpu保护
 */
/* ---------------------------------------------------------------------------- */
void mpu_diasble(void);


/* ---------------------------------------------------------------------------- */
/**
 * @brief flash PC范围设置为Flash外区域, 调用该接口后调用flash里的函数将触发异常
 */
/* ---------------------------------------------------------------------------- */
void flash_pc_limit_disable();

/* ---------------------------------------------------------------------------- */
/**
 * @brief flash PC范围限制恢复为flash代码区域, 调用该接口后可调用flash里的函数
 */
/* ---------------------------------------------------------------------------- */
void flash_pc_limit_enable();

/* ---------------------------------------------------------------------------- */
/**
 * @brief CPU内存监测点设置
 *
 * @param low_addr: 监测区域起始地址
 * @param high_addr: 监测区域结束地址
 * @param low_limit_value: 监测内存下限值
 * @param high_limit_value: 监测内存上限值
 * @param mode: 监测模式(ETM_DETECT_MODE)
 * @param limit_range_out: 0(框内触发中断) 1(框外触发中断)
 * @param trigger_exception: 0(触发普通中断) 1(触发异常中断)
 */
/* ---------------------------------------------------------------------------- */
void cpu_etm_range_value_limit_detect(void *low_addr, void *high_addr, u32 low_limit_value, u32 high_limit_value, int mode, int limit_range_out, int trigger_exception);

#endif


